/* except-win32.c                  -*-C-*-
 *
 *************************************************************************
 *
 *  Copyright (C) 2009-2015, Intel Corporation
 *  All rights reserved.
 *  
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in
 *      the documentation and/or other materials provided with the
 *      distribution.
 *    * Neither the name of Intel Corporation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *  
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 *  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 *  AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
 *  WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *  
 *  *********************************************************************
 *  
 *  PLEASE NOTE: This file is a downstream copy of a file mainitained in
 *  a repository at cilkplus.org. Changes made to this file that are not
 *  submitted through the contribution process detailed at
 *  http://www.cilkplus.org/submit-cilk-contribution will be lost the next
 *  time that a new version is released. Changes only submitted to the
 *  GNU compiler collection or posted to the git repository at
 *  https://bitbucket.org/intelcilkplusruntime/itnel-cilk-runtime.git are
 *  not tracked.
 *  
 *  We welcome your contributions to this open source project. Thank you
 *  for your assistance in helping us improve Cilk Plus.
 **************************************************************************/

// Windows shouldn't whine about uses of getenv which it considers unsafe
#define _CRT_SECURE_NO_WARNINGS 1

// Exception flags
#define EXCEPTION_UNWINDING 0x02
#define EXCEPTION_EXIT_UNWIND 0x04

#include "except-win32.h"
#include "sysdep-win.h"
#include "bug.h"
#include "local_state.h"
#include "full_frame.h"
#include "scheduler.h"
#include "jmpbuf.h"
#include "cilk_malloc.h"
#include "except.h"
#include "record-replay.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>


CILK_ABI_THROWS_VOID __cilkrts_rethrow(__cilkrts_stack_frame *sf)
{

    __cilkrts_worker *w = __cilkrts_get_tls_worker();
    struct pending_exception_info *pending_exception;

    CILK_ASSERT(NULL != w);
    CILK_ASSERT(NULL != w->l->pending_exception);

    pending_exception = w->l->pending_exception;

    // disable stealing until one of two things happens:
    // 1. the exception reaches another steal boundary and is detected by one
    //    of the Cilk exception handlers.  Each has code to reenable stealing.
    // 2. the exception is caught, copied, and the destructor is called.  The
    //    destructor is wrapped and will allow stealing after the real dtor has
    //    completed.
    CILK_ASSERT(NULL == pending_exception->saved_sf);
    pending_exception->saved_sf = __cilkrts_disallow_stealing(w, NULL);
    RaiseException(pending_exception->ExceptionCode,
                   pending_exception->ExceptionFlags,
                   pending_exception->NumberParameters,
                   pending_exception->ExceptionInformation);

    CILK_ASSERT(! "shouldn't reach this point");
}

static inline
void CallDtor (void (*dtor)(), void *pThis)
{
    // If there's no destructor, we're done

    if (NULL == dtor)
        return;

    // Invoke the destructor.  Note that we are NOT setting EBP to NULL, since
    // this seems to cause bizzare problems.  They mave have gone away now that
    // we've disallowed stealing while processing an exception, but it's not
    // worth it.  Futzing with EBP required us to go through contortions to
    // access the parameters.

    __asm
    {
        push ecx
        mov ecx, pThis
        call dtor
        pop ecx
    }
}

/* call the destructor on the given pending exception.  If there is no dtor (as
 * with primitive types and some structs/classes), CallDtor() will just return.
 */
void delete_exception_obj(__cilkrts_worker *w,
                          struct pending_exception_info *pei,
                          int delete_object)
{
    __cilkrts_stack_frame * volatile *sf;
    void *obj = (void*)pei->ExceptionInformation[info_cpp_object];
    Win32ThrowInfo *info = (Win32ThrowInfo*)pei->ExceptionInformation[info_throw_data];

    CILK_ASSERT(NULL != obj);
    CILK_ASSERT(NULL != info);

    // disable stealing while the object is being destroyed.
    CILK_ASSERT(w);
    sf = __cilkrts_disallow_stealing(w, NULL);
    CallDtor((void (*)())info->pmfnUnwind, obj);
    __cilkrts_restore_stealing(w, sf);
}

static inline
void spin_acquire (volatile long *lock) {
    while (0 != InterlockedCompareExchange(lock, 1, 0));
}

static inline
void release (volatile long *lock) {
    CILK_ASSERT(InterlockedCompareExchange(lock, 0, 1));
}

/* save a pointer to the extra throw data we generated.  The pointer will be
 * associated with the address of the exception object so that our destructor
 * wrapper can find it, later.
 */
static inline
void save_info_pointer (struct pending_exception_info *pei, void *obj) {

    pending_exceptions_t *pending = get_cilk_pending_exceptions();
    int i;

    CILK_ASSERT(pending);

    // accesses to the array need to be atomic.  They are short and infrequent,
    // so spin on the lock rather than going to the operating system.
    spin_acquire(&pending->lock);
    // iterate through the array looking for an empty entry we can use.
    for (i = 0; i < pending->max_pending_exceptions; ++i) {
        if (NULL == pending->entries[i].exception_object) {
            // found an empty entry!  Store the object pointer and pending
            // exception information pointer and return.
            pending->entries[i].exception_object = obj;
            pending->entries[i].pei = pei;
            release(&pending->lock);
            return;
        }
    }

    // array is too small and needs to be resized.
    pending->max_pending_exceptions *= 2;
    
    pending->entries = (exception_entry_t*)
        __cilkrts_realloc(pending->entries,
                          sizeof(exception_entry_t) *
                              pending->max_pending_exceptions);

    if (!pending->entries) {
        // out of memory.
        abort();
    }

    // after resizing, put this entry in the first available slot and then zero-
    // out the rest of the array.
    pending->entries[i].exception_object = obj;
    pending->entries[i].pei = pei;
    for (++i; i < pending->max_pending_exceptions; ++i) {
        pending->entries[i].exception_object = NULL;
        pending->entries[i].pei = NULL;
    }
    // don't forget to release the lock.  This is important.
    release(&pending->lock);
}

/* a pointer to the exception info was saved by save_info_pointer().  Find that
 * pointer and return it.  A table of pending exceptions that have crossed steal
 * boundaries are kept in the global context.
 */
static inline
struct pending_exception_info *find_info_pointer (void *obj) {

    pending_exceptions_t *pending = get_cilk_pending_exceptions();
    exception_entry_t *entry;
    struct pending_exception_info *pei;

    CILK_ASSERT(pending);

    spin_acquire(&pending->lock);
    // the entries in the table are a simple list.  Iterate through the list and
    // use our object pointer as a key.
    for (entry = pending->entries; entry->exception_object != obj; ++entry);
    pei = entry->pei;
    entry->pei = NULL;
    entry->exception_object = NULL;
    release(&pending->lock);
    return pei;
}


/* call the real destructor on the exception_object and then release the stack
 * on which it lived.  This function (or, actually, destructor_wrapper_thunk())
 * was put in place of the actual destructor when it was discovered that the
 * exception was going to be handled on a different stack from the one on which
 * it was thrown.
 */
static
void destructor_wrapper (void *exception_object)
{
    struct pending_exception_info *pei;
    __cilkrts_worker *w;
    cilk_fiber *fiber, *fiber_self;
    cilk_fiber_data *fdata;
    __cilkrts_stack_frame * volatile *sf;

    // get the pointer to the real throw info (with the pointer to the real
    // destructor, and call it on the exception object.
    CILK_ASSERT(exception_object);
    if (! cilkg_is_published()) abort();
    pei = find_info_pointer(exception_object);
    CILK_ASSERT(pei);
    // CallDtor() will worry about whether pmfnUnwind() is NULL.
    CallDtor(pei->real_info->pmfnUnwind, exception_object);

    // get the remaining relevant info from the exception info struct and then
    // free() it.
    fiber = pei->fiber;
    fdata = cilk_fiber_get_data(fiber);
    sf = pei->saved_sf;
    __cilkrts_free(pei);

    // there is no Cilk stack, or the stack has outstanding references.
    CILK_ASSERT(!fiber || cilk_fiber_has_references(fiber));
    
    w = __cilkrts_get_tls_worker();
    if (w) {
        // Grab stack_self before we restore stealing, so that we don't need
        // to grab the worker lock.
        CILK_ASSERT(w->l->frame_ff);
        fiber_self = w->l->frame_ff->fiber_self;

        // remove the pending_exception from the worker struct like a good
        // do-bee.  Even if we are merging exceptions, the merge should have
        // assigned pending_exception to the exception being eliminated.  This
        // way, this assertion is still useful in weeding out bugs.
        CILK_ASSERT(w->l->pending_exception);
        w->l->pending_exception = NULL;
        // restore stealing if stealing has been disallowed.  It has _not_ been
        // disallowed (by rethrow()) if this call to destructor_wrapper() was
        // made by merge_pending_exceptions().
        if (sf)
            __cilkrts_restore_stealing(w, sf);
    }

    if (fiber) {
        if (cilk_fiber_is_allocated_from_thread(fiber)) {
            cilk_fiber_remove_reference_from_thread(fiber);
        }
        else {
            if (w && sf) {
                cilk_fiber_remove_reference(fiber, &w->l->fiber_pool);
            }
            else {
                cilk_fiber_pool* global_pool = &(cilkg_get_global_state()->fiber_pool);
                cilk_fiber_remove_reference(fiber, global_pool);
            }
        }
    }
}

/* actually, this is the destructor wrapper, since it expects *this in ECX.  It
 * will turn *this into a proper argument by pushing it onto the stack and
 * calling destructor_wrapper().
 */
static
__declspec(naked)
void destructor_wrapper_thunk ()
{
    __asm {
        push ecx
        call destructor_wrapper
        pop ecx
        ret
    }
}

#define BEGIN_WITH_FRAME_LOCK(w, ff)                                     \
    do { full_frame *_locked = ff; __cilkrts_frame_lock(w, _locked); do

#define END_WITH_FRAME_LOCK(w, ff)                       \
    while (__cilkrts_frame_unlock(w, _locked), 0); } while (0)

/* populate the pending_exception_info object by duplicating the
 * EXCEPTION_RECORD and saving relevant information that will be needed during
 * cleanup.
 *
 * This function is called by the Cilk handler functions when hither-to unseen
 * (by the runtime) exceptions reach it.
 */
static inline
void wrap_exception (struct pending_exception_info *pei,
                     EXCEPTION_RECORD *record)
{
    void *rethrow_sp;
    __cilkrts_worker *w = __cilkrts_get_tls_worker();
    full_frame *ff; 

    CILK_ASSERT(NULL != pei);
    CILK_ASSERT(NULL != record);
    CILK_ASSERT(NULL != w);
    ff = w->l->frame_ff;
    
    // If this is not a C++ exception pass it to __cilkrts_report_seh_exception
    // which will report it and abort the application.
    if (record->ExceptionCode != CXX_EXCEPTION)
    {
        __cilkrts_report_seh_exception(record);
    }

    switch (record->ExceptionInformation[info_magic_number]) {
    case EH_MAGIC_NUMBER1:
    case EH_MAGIC_NUMBER2:
    case EH_MAGIC_NUMBER3:
        break;
    default:
        __cilkrts_bug("error: unknown exception structure layout");
    }

    if (is_within_stack((char*)record->ExceptionInformation[info_cpp_object])) {
        // exception is on this stack.  Save ESP here, where it is above the
        // exception object.
        __asm {
            mov DWORD PTR rethrow_sp, esp
        }
        pei->rethrow_sp = rethrow_sp;
        pei->fiber = ff->fiber_self;

#if SUPPORT_GET_CURRENT_FIBER
        // Let's check.
        CILK_ASSERT(pei->fiber == cilk_fiber_get_current_fiber());
#endif

        if (pei->fiber) {
            // there is an exception on this stack.  It will not be freed until
            // after the catch block so the stack cannot be released until then,
            // even if the catch block is executed on a different stack from
            // this one.  Our destructor wrapper will determine whether it is on
            // this stack or a different one and either unset this bit or
            // release this stack (to the global cache), as appropriate.
            cilk_fiber_add_reference(pei->fiber);
        }
        else {
            pei->w = __cilkrts_get_tls_worker(); // is this necessary?
        }
    } else {
        // the exception is not on this stack.  This is not possible when the
        // exception is coming from a spawned function.  However, it _is_ when
        // the throw happens in the continuation of a spawning function: E.g.,
        //
        // void foo () {
        //   spawn bar();
        //   throw 42;
        // }
        //
        // In this case, the exception lives on the original stack.  Thus, we
        // need to get the descriptor for that stack and preserve it rather than
        // the one on which we are executing at present.

        ff = w->l->frame_ff;
        BEGIN_WITH_FRAME_LOCK(w, ff) {
            full_frame* tmp = ff->rightmost_child;

            // If tmp is NULL, then ff is a leftmost child, 
            // and the stack we want is in ff->fiber_child.
            pei->fiber = ff->fiber_child;

            // Otherwise, this loop will execute, setting pei->fiber to
            // the stack for the sibling to the left, until we find
            // the leftmost stack.
            while (tmp) {
                pei->fiber = tmp->fiber_self;
                tmp = tmp->left_sibling;
            }
        } END_WITH_FRAME_LOCK(w, ff);

        if (pei->fiber) {
            // WARNING: This call to add_reference requires updates to
            // the reference count to be atomic because the leftmost
            // stack we found might be different from the one we are 
            // executing on.
            cilk_fiber_add_reference(pei->fiber);
        }
        pei->rethrow_sp = NULL;
    }

    pei->saved_sf = NULL;
    pei->ExceptionCode = record->ExceptionCode;
    pei->ExceptionFlags = record->ExceptionFlags;
    pei->NumberParameters = record->NumberParameters;
    pei->ExceptionInformation[info_magic_number] =
        record->ExceptionInformation[info_magic_number];
    pei->ExceptionInformation[info_throw_data] =
        record->ExceptionInformation[info_throw_data];
    pei->ExceptionInformation[info_cpp_object] =
        record->ExceptionInformation[info_cpp_object];
    pei->real_info = (Win32ThrowInfo*)record->ExceptionInformation[info_throw_data];
    // duplicate the throw info, but replace the unwind function (destructor).
    pei->fake_info.attributes = pei->real_info->attributes;
    pei->fake_info.pmfnUnwind = destructor_wrapper_thunk;
    pei->fake_info.pForwardCompat = pei->real_info->pForwardCompat;
    pei->fake_info.pCatchableTypeArray = pei->real_info->pCatchableTypeArray;
    // replace the real throw data in the pending exception with the fake one.
    pei->ExceptionInformation[info_throw_data] =
      (ULONG_PTR)&pei->fake_info;
}

/* if the exception has not been wrapped up, it needs to be done.  Otherwise,
 * some balancing needs to happen in terms of restoring stealing to a stack.
 * This function is called by the Cilk exception handlers before unwinding.
 * Since wrapping the exception needs to happen before that, and since actually
 * saving a pointer to the wrapped exception (in the pending_exception_info) has
 * to happen _after_ unwinding:
 *   return 1 if a pointer to pei needs to be stored, after unwind.
 *   return 0 otherwise (i.e., it has already been saved).
 */
static inline
void save_or_balance (__cilkrts_worker *w,
                      EXCEPTION_RECORD *exception_record)
{
    struct pending_exception_info *pei;

    if (NULL == w->l->pending_exception) {

        /* this exception has not yet been seen by the runtime, so it
           needs to be wrapped up and a pointer stored in the worker. */
        pei = (struct pending_exception_info*)
            __cilkrts_malloc(sizeof(struct pending_exception_info));
        if (!pei) abort();
        wrap_exception(pei, exception_record);
        w->l->pending_exception = pei;
        save_info_pointer(pei,
                          (void*)exception_record->
                          ExceptionInformation[info_cpp_object]);

    } else {

        /* balance disallow/restore -- saved_sf was set at the last
           rethrow() which happened on this worker. */
        CILK_ASSERT(NULL != w->l->pending_exception->saved_sf);
        __cilkrts_restore_stealing(w, w->l->pending_exception->saved_sf);
        w->l->pending_exception->saved_sf = NULL;

    }
}

typedef int (*handler_t) (struct _EXCEPTION_RECORD *exception_record,
                          struct EXCEPTION_REGISTRATION *registration,
                          struct CONTEXT *context_record,
                          void *context);

/**
 * Raw exception registration record understood by the Windows SEH
 * code.  Win32 C++ exception handling uses an extended version of this
 * structure with additional fields.
 */
struct partial_registration_t {
    struct EXCEPTION_REGISTRATION *prev;    ///< Next entry in the SEH chain
    handler_t handler;                      ///< Function to handle the exception
};

/* die with a specific exit code that means exceptions collided.
 */
static inline int exception_die (const char *err)
{
    // FIXME: EXCEPTIONS_COLLIDED should be defined in a header file with other
    // exit codes.
#define EXCEPTIONS_COLLIDED 1
    exit(EXCEPTIONS_COLLIDED);
    return 0;
}

/* get_fs0() returns the head of the exception registration chain.
 */
static inline void *get_fs0 () {

    void *fs0;

    __asm {
        push FS:0
        pop fs0
    }

    return fs0;
}

/* cilk_global_unwind() calls frame handlers to unwind their functions.  It
 * unwinds all frames below (younger than) -- not including -- the one
 * corresponding to the exception registration passed.
 */
static inline void cilk_global_unwind (struct _EXCEPTION_RECORD *er,
                                       struct EXCEPTION_REGISTRATION *reg,
                                       struct CONTEXT *context_record,
                                       void *context) {

    struct EXCEPTION_REGISTRATION *fs0, *current;
    handler_t handler;

    __try {

        // set the unwind flag so that handlers unwind their functions.
        er->ExceptionFlags |= EXCEPTION_UNWINDING;
        fs0 = (struct EXCEPTION_REGISTRATION *)get_fs0();

        // prime the iteration twice: The first handler in the list is the one
        // from this function, and we don't want to call unwind on that.  The
        // second one is set up in ExecuteHandler2() which called
        // cilk_global_unwind()'s parent (one of the Cilk handlers).  We don't
        // want to unwind that one either.  So we'll start unwinding at handler
        // #3, where the user frames begin.
        current = ((struct partial_registration_t *)fs0)->prev;
        current = ((struct partial_registration_t *)current)->prev;

        while (current != reg) {

            // the tail of the handler list is marked by a -1.  We should never
            // reach that.
            CILK_ASSERT((struct EXCEPTION_REGISTRATION *)(-1) != current);

            // call the handler now that the unwind bit is set.
            handler = ((struct partial_registration_t *)current)->handler;
            handler(er, current, context_record, context);
            current = ((struct partial_registration_t *)current)->prev;

        }

    } __except (exception_die("destructor threw an exception during unwind")) {
        CILK_ASSERT(!"shouldn't be here.");
    }

    ((struct partial_registration_t *)fs0)->prev = current;
}

// Commenting this out since it uses unsafe sprintf and is not called at all.
#if 0
char *dump_pedigree_node(char *buf, const __cilkrts_pedigree *n)
{
    if (n->parent)
        buf = dump_pedigree_node(buf, n->parent);
    return buf + sprintf(buf, " %llu", n->rank);
}
#endif

/* __cilkrts_detach_handler() replaces the default Microsoft handler in spawn
 * helper functions (the ones that detach) in Win32.  No exception record will
 * ever catch anything in such a function, so this handler only performs a
 * subset of its handling duties.  It does, however, ensure worker safety and
 * in case there has been a steal, it bundles up the exception for rethrow at
 * the sync.  If it needs to unwind (since a spawn helper may have objects with
 * destructors), it will call the system handler function that was saved by the
 * system.
 *
 * Note: this code is informed by Matt Pietrek's pseudo-code:
 * http://www.microsoft.com/msj/0197/exception/exceptiontextfigs.htm#fig9
 */
CILK_EXPORT
int __cilkrts_detach_handler (struct _EXCEPTION_RECORD *exception_record,
                              struct EXCEPTION_REGISTRATION *registration,
                              struct CONTEXT *context_record,
                              void *context) {

    __cilkrts_stack_frame *sf;
    __cilkrts_stack_frame * volatile *saved;
    __cilkrts_worker *w;

    // If this is a debug break, just let the exception propagate.  It will
    // get where it needs to go eventually
    if (0x80000003 == exception_record->ExceptionCode)
        return ExceptionContinueSearch;

#if 0
    // If the stack is being unwound, just let the exception propagate.  It
    // also will get where it needs to go eventually.  It's probably us calling
    // longjmp in __cilkrts_resume_after_longjmp_into_runtime()
    if (0x80000026 == exception_record->ExceptionCode)
        return ExceptionContinueSearch;
#endif

    w = __cilkrts_get_tls_worker();
    CILK_ASSERT(w);

    // If the stack is being unwound, just let the exception propagate.  It
    // also will get where it needs to go eventually.  It's probably us calling
    // longjmp in __cilkrts_resume_after_longjmp_into_runtime()
    if (0x80000026 == exception_record->ExceptionCode)
    {
        cilk_fiber* fiber = w->l->frame_ff->fiber_self;
#if SUPPORT_GET_CURRENT_FIBER
        CILK_ASSERT(fiber == cilk_fiber_get_current_fiber());
#endif

        //DBGPRINTF ("%d-%p: __cilkrts_detach_handler - unwind exception - sd: %p\n",
        //           w->self, GetCurrentFiber(), sd);
        CILK_ASSERT(fiber);
        if (!cilk_fiber_is_resumable(fiber))
        {
            //DBGPRINTF("%d-%p: __cilkrts_detach_handler - unwind exception - continuing\n",
            //          w->self, GetCurrentFiber(), sd);
            return ExceptionContinueSearch;
        }
    }

    /* Windows exception handling makes two passes.  The first looks for a fn who
       will handle the exception.  The fn that will handle it doesn't return but
       kicks off a global unwind up to itself.  Figure out which pass this is: */
    if ( !(exception_record->ExceptionFlags
           & (EXCEPTION_UNWINDING | EXCEPTION_EXIT_UNWIND))) {

        /* First pass: the system is probing for a function to handle the
           exception.  If there has been no steal (since the owning function is
           a spawn helper and never catches anything) simply pass the exception
           along as though no spawn had ever occurred.

           Otherwise, the exception cannot propogate across a steal, so this
           handler will wrap up the exception, call the global unwind code, and
           jump back to the sync point of the spawning function.  The spawning
           function, when it is synched, will rethrow the saved exception. */

       /*
        * If we are in replay mode, and a steal occurred during the recording
        * phase, stall till a steal actually occurs.
        */
        replay_wait_for_steal_if_parent_was_stolen(w);

        if (NULL != __cilkrts_pop_tail(w)) {

            /* nobody stole from us.  The exception is propagated normally. */
            return ExceptionContinueSearch;

        } else {
            /* Write a record to the replay log for an attempt to return to a
               stolen parent.  This must be done before the exception handler
               invokes __cilkrts_leave_frame which will bump the pedigree so
               the replay_wait_for_steal_if_parent_was_stolen() aove will match on
               replay. */
            replay_record_orphaned(w);

            /* save_or_balance() will determine whether this exception has
               already been saved (i.e., whether it has already passed through a
               steal).  If it has, it will balance the allow/disallow stealing on
               that stack.  Otherwise, it will save the exception where it can be
               found, later, by the destructor wrapper. */
            save_or_balance(w, exception_record);

            /* cilk_global_unwind() will set the unwind bit on the exception
               record, so we don't do that here. */
            saved = __cilkrts_disallow_stealing(w, NULL);
            cilk_global_unwind(exception_record,
                               registration,
                               context_record,
                               context);

            sf = w->current_stack_frame;
            CILK_ASSERT(sf);
            CILK_ASSERT(sf->except_data);
            CILK_ASSERT(sf->flags & CILK_FRAME_DETACHED);
            sf->flags |= CILK_FRAME_UNWINDING;
            CILK_ASSERT(NULL != sf->except_data);
            /* for the local unwind call the old handler to unwind. */
            ((handler_t)sf->except_data)(exception_record,
                                         registration,
                                         context_record,
                                         context);
            __cilkrts_restore_stealing(w, saved);
            __cilkrts_migrate_exception(sf); /* does not return. */
            CILK_ASSERT(! "shouldn't be here!");

        }

    } else {

        /* Second pass: There may be something to unwind in this frame.  If the
           spawned function had any objects constructed in its call, those must
           be destructed on the way through the frame.  Since we have the
           original handler function, we will simply call that to unwind the
           whole frame.  The __finally block will pop the frame and call
           __cilkrts_leave_frame(), so we won't do that, here.

           Note: this code only ever gets called if there was no steal, so
           everything is okey-dokey. */
        sf = w->current_stack_frame;
        CILK_ASSERT(sf);
        sf->flags |= CILK_FRAME_UNWINDING;
        ((handler_t)sf->except_data)(exception_record,
                                     registration,
                                     context_record,
                                     context);
        return ExceptionContinueSearch;

    }

    CILK_ASSERT(! "shouldn't be here!");
    return 0;
}

/* __cilkrts_stub_handler() replaces the default Microsoft handler in fiber
 * stub functions in Win32.  This special-case handler wraps up the exception,
 * unwinds the stack up to this point, and jumps to the parent's sync (where
 * the exception will be rethrown).
 */
int __cilkrts_stub_handler (struct _EXCEPTION_RECORD *exception_record,
                            struct EXCEPTION_REGISTRATION *registration,
                            struct CONTEXT *context_record,
                            void *context)
{
    __cilkrts_worker *w;
    __cilkrts_stack_frame *sf;
    __cilkrts_stack_frame * volatile *saved_sf;

    // If this is not a C++ exception, someone else has to handle it
    if (CXX_EXCEPTION != exception_record->ExceptionCode)
        return ExceptionContinueSearch;

    w = __cilkrts_get_tls_worker();
    CILK_ASSERT(w);

    /* save_or_balance() determines whether the exception has already been saved.
       If not, it saves it.  See the function for details. */
    save_or_balance(w, exception_record);
    saved_sf = __cilkrts_disallow_stealing(w, NULL);
    cilk_global_unwind(exception_record,
                       registration,
                       context_record,
                       context);
    /* we never unwind the stub itself.  So no call to _local_unwind() or any
       of its ilk. */

    CILK_ASSERT(w->l->pending_exception);
    CILK_ASSERT(w->l->frame_ff);
    CILK_ASSERT(NULL == w->l->frame_ff->pending_exception);

    sf = w->current_stack_frame;

    CILK_ASSERT(sf);

    // restore the correct stack pointer into the context.  If this exception
    // is the one that continues up the stack, this is the value of ESP to use.
    SP(sf) = (unsigned long)w->l->frame_ff->exception_sp;

    sf->flags |= CILK_FRAME_EXCEPTING;
    __cilkrts_restore_stealing(w, saved_sf);

    __cilkrts_c_sync(w, sf); /* does not return. */

    CILK_ASSERT(! "shouldn't be here");
}

#ifdef _DEBUG
volatile long printing_lock;

__declspec(dllexport) void __cilkrts_willtor (const char *f_string) {
  __cilkrts_worker *w;
  full_frame *ff, *tmp;

  w = __cilkrts_get_tls_worker();
  CILK_ASSERT(w);
  ff = w->l->frame_ff;

  spin_acquire(&printing_lock);
  printf("%s:\n  self:  f 0x%.8x p 0x%.8x ss 0x%.8x sc 0x%.8x\n",
         f_string, ff, ff->parent, ff->fiber_self, ff->fiber_child);
  tmp = ff->rightmost_child;
  if (tmp) {
    do {
      printf("  child: f 0x%.8x p 0x%.8x ss 0x%.8x sc 0x%.8x\n",
             tmp, tmp->parent, tmp->fiber_self, tmp->fiber_child);
    } while (tmp = tmp->left_sibling);
  }
  release(&printing_lock);
}
#endif

/* End except-win32.c */

/* Cilk_abi.c                  -*-C++-*-
 *
 *************************************************************************
 *
 * Copyright (C) 2010-2012 , Intel Corporation
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
 * WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 **************************************************************************/

/*
 * Define this macro so that compiliation of this file generates the
 * non-inlined versions of certain functions in cilk_api.h.
 */
#include "internal/abi.h"
#include "cilk/cilk_api.h"
#include "cilk/cilk_undocumented.h"
#include "cilktools/cilkscreen.h"
#include "internal/inspector-abi.h"

#include "global_state.h"
#include "os.h"
#include "os_mutex.h"
#include "bug.h"
#include "local_state.h"
#include "full_frame.h"
#include "pedigrees.h"
#include "scheduler.h"
#include "sysdep.h"
#include "except.h"
#include "cilk_malloc.h"

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#ifdef _MSC_VER
/* Some versions of icc don't support limits.h on Linux if
   gcc 4.3 or newer is installed. */
#include <limits.h>

/* Declare _ReturnAddress compiler intrinsic */
void * _ReturnAddress(void);
#pragma intrinsic(_ReturnAddress)

#include "sysdep-win.h"     // Needed for sysdep_init_module()
#endif  /* _WIN32 */

#include "metacall_impl.h"
#include "reducer_impl.h"
#include "cilk-ittnotify.h"
#include "cilk-tbb-interop.h"

#define TBB_INTEROP_DATA_DELAYED_UNTIL_BIND (void *)-1

// ABI version
#define BIND_THREAD_RTN __cilkrts_bind_thread_1

static inline
void enter_frame_internal(__cilkrts_stack_frame *sf, uint32_t version)
{
    __cilkrts_worker *w = __cilkrts_get_tls_worker();
    if (w == 0) { /* slow path */
        w = BIND_THREAD_RTN();

        sf->flags = CILK_FRAME_LAST | (version << 24);
        CILK_ASSERT((sf->flags & CILK_FRAME_FLAGS_MASK) == CILK_FRAME_LAST);
    } else {
        sf->flags = (version << 24);
        CILK_ASSERT((sf->flags & CILK_FRAME_FLAGS_MASK) == 0);
    }
    sf->call_parent = w->current_stack_frame;
    sf->worker = w;
    w->current_stack_frame = sf;
}

CILK_ABI_VOID __cilkrts_enter_frame(__cilkrts_stack_frame *sf)
{
    enter_frame_internal(sf, 0);
}

CILK_ABI_VOID __cilkrts_enter_frame_1(__cilkrts_stack_frame *sf)
{
    enter_frame_internal(sf, 1);
    sf->reserved = 0;
}

static inline
void enter_frame_fast_internal(__cilkrts_stack_frame *sf, uint32_t version)
{
    __cilkrts_worker *w = __cilkrts_get_tls_worker_fast();
    sf->flags = version << 24;
    sf->call_parent = w->current_stack_frame;
    sf->worker = w;
    w->current_stack_frame = sf;
}

CILK_ABI_VOID __cilkrts_enter_frame_fast(__cilkrts_stack_frame *sf)
{
    enter_frame_fast_internal(sf, 0);
}

CILK_ABI_VOID __cilkrts_enter_frame_fast_1(__cilkrts_stack_frame *sf)
{
    enter_frame_fast_internal(sf, 1);
    sf->reserved = 0;
}

/* Return true if undo-detach failed. */
static int __cilkrts_undo_detach(__cilkrts_stack_frame *sf)
{
    __cilkrts_worker *w = sf->worker;
    __cilkrts_stack_frame *volatile *t = w->tail;

/*    DBGPRINTF("%d - __cilkrts_undo_detach - sf %p\n", w->self, sf); */

    --t;
    w->tail = t;
    /* On x86 the __sync_fetch_and_<op> family includes a
       full memory barrier.  In theory the sequence in the
       second branch of the #if should be faster, but on
       most x86 it is not.  */
#if defined __i386__ || defined __x86_64__
    __sync_fetch_and_and(&sf->flags, ~CILK_FRAME_DETACHED);
#else
    __cilkrts_fence(); /* membar #StoreLoad */
    sf->flags &= ~CILK_FRAME_DETACHED;
#endif

    return __builtin_expect(t < w->exc, 0);
}

CILK_ABI_VOID __cilkrts_leave_frame(__cilkrts_stack_frame *sf)
{
    __cilkrts_worker *w = sf->worker;

/*    DBGPRINTF("%d-%p __cilkrts_leave_frame - sf %p, flags: %x\n", w->self, GetWorkerFiber(w), sf, sf->flags); */

#ifdef _WIN32
    /* if leave frame was called from our unwind handler, leave_frame should
       proceed no further. */
    if (sf->flags & CILK_FRAME_UNWINDING)
    {
/*        DBGPRINTF("%d - __cilkrts_leave_frame - aborting due to UNWINDING flag\n", w->self); */

        // If this is the frame of a spawn helper (indicated by the
        // CILK_FRAME_DETACHED flag) we must update the pedigree.  The pedigree
        // points to nodes allocated on the stack.  Failing to update it will
        // result in a accvio/segfault if the pedigree is walked.  This must happen
        // for all spawn helper frames, even if we're processing an exception
        if ((sf->flags & CILK_FRAME_DETACHED))
        {
	    update_pedigree_on_leave_frame(w, sf);
        }
        return;
    }
#endif

#if CILK_LIB_DEBUG
    /* ensure the caller popped itself */
    CILK_ASSERT(w->current_stack_frame != sf);
#endif

    /* The exiting function should have checked for zero flags,
       so there is no check for flags == 0 here. */

#if CILK_LIB_DEBUG
    if (__builtin_expect(sf->flags & (CILK_FRAME_EXITING|CILK_FRAME_UNSYNCHED), 0))
        __cilkrts_bug("W%u: function exiting with invalid flags %02x\n",
                      w->self, sf->flags);
#endif

    /* Must return normally if (1) the active function was called
       and not spawned, or (2) the parent has never been stolen. */
    if ((sf->flags & CILK_FRAME_DETACHED)) {
/*        DBGPRINTF("%d - __cilkrts_leave_frame - CILK_FRAME_DETACHED\n", w->self); */

#ifndef _WIN32
        if (__builtin_expect(sf->flags & CILK_FRAME_EXCEPTING, 0)) {
	    update_pedigree_on_leave_frame(w, sf);
            __cilkrts_return_exception(sf);
            /* If return_exception returns the caller is attached.
               leave_frame is called from a cleanup (destructor)
               for the frame object.  The caller will reraise the
               exception. */
	    return;
        }
#endif
        if (__builtin_expect(__cilkrts_undo_detach(sf), 0)) {
	    // The update of pedigree for leaving the frame occurs
	    // inside this call if it does not return.
            __cilkrts_c_THE_exception_check(w, sf);
        }

	update_pedigree_on_leave_frame(w, sf);
        /* This path is taken when undo-detach wins the race with stealing.
           Otherwise this strand terminates and the caller will be resumed
           via setjmp at sync. */
        if (__builtin_expect(sf->flags & CILK_FRAME_FLAGS_MASK, 0))
            __cilkrts_bug("W%u: frame won undo-detach race with flags %02x\n",
                          w->self, sf->flags);

        return;
    }

#if CILK_LIB_DEBUG
    sf->flags |= CILK_FRAME_EXITING;
#endif

    if (__builtin_expect(sf->flags & CILK_FRAME_LAST, 0))
        __cilkrts_c_return_from_initial(w); /* does return */
    else if (sf->flags & CILK_FRAME_STOLEN)
        __cilkrts_return(w); /* does return */

/*    DBGPRINTF("%d-%p __cilkrts_leave_frame - returning, StackBase: %p\n", w->self, GetWorkerFiber(w)); */
}

/* Caller must have called setjmp. */
CILK_ABI_VOID __cilkrts_sync(__cilkrts_stack_frame *sf)
{
    __cilkrts_worker *w = sf->worker;
/*    DBGPRINTF("%d-%p __cilkrts_sync - sf %p\n", w->self, GetWorkerFiber(w), sf); */
    if (__builtin_expect(!(sf->flags & CILK_FRAME_UNSYNCHED), 0))
        __cilkrts_bug("W%u: double sync %p\n", w->self, sf);
#ifndef _WIN32
    if (__builtin_expect(sf->flags & CILK_FRAME_EXCEPTING, 0)) {
        __cilkrts_c_sync_except(w, sf);
    }
#endif

    /* Save return address so we can report it to Piersol. */
#ifdef _WIN32
    w->l->sync_return_address =  _ReturnAddress();
#endif

    __cilkrts_c_sync(w, sf);
}

/*
 * __cilkrts_get_sf
 *
 * Debugging aid to provide access to the current __cilkrts_stack_frame.
 *
 * Not documented!
 */

CILK_API_VOID_PTR
__cilkrts_get_sf(void)
{
    __cilkrts_worker *w = __cilkrts_get_tls_worker();
    if (0 == w)
        return NULL;

    return w->current_stack_frame;
}

/* Call with global lock held */
static __cilkrts_worker *find_free_worker(global_state_t *g)
{
    __cilkrts_worker *w = 0;
    int i;

    // Scan the non-system workers looking for one which is free so we can
    // use it.
    for (i = g->P - 1; i < g->total_workers; ++i) {
        w = g->workers[i];
        CILK_ASSERT(WORKER_SYSTEM != w->l->type);
        if (w->l->type == WORKER_FREE) {
            w->l->type = WORKER_USER;
            w->l->team = w;
            return w;
        }
    }

    // If we ran out of workers, create a new one.  It doesn't actually belong
    // to the Cilk global state so nobody will ever try to steal from it.
    w = (__cilkrts_worker *)__cilkrts_malloc(sizeof(*w));
    __cilkrts_cilkscreen_ignore_block(w, w+1);
    make_worker(g, -1, w);
    w->l->type = WORKER_USER;
    w->l->team = w;
    return w;
}

/*
 * __cilkrts_bind_thread
 *
 * Exported function to bind a thread to the runtime.
 *
 * This function name should always have a trailing suffix for the latest ABI
 * version. This means that code built with a new compiler will not load
 * against an old copy of the runtime.
 *
 * Symbols for the function called by code compiled with old versions of the
 * compiler are created in an OS-specific manner:
 *  - On Windows the old symbols are defined in the cilk-exports.def linker
 *    definitions file as aliases of BIND_THREAD_RTN
 *  - On Linux aliased symbols are created for BIND_THREAD_RTN in this file
 *  - On MacOS the alternate entrypoints are implemented and simply call
 *    BIND_THREAD_RTN.
 */
CILK_ABI_WORKER_PTR BIND_THREAD_RTN(void)
{
    __cilkrts_worker *w;
    int start_cilkscreen = 0;
    static int unique_obj;

    // Cannot set this pointer until after __cilkrts_init_internal() call:
    global_state_t* g;

    ITT_SYNC_CREATE (&unique_obj, "Initialization");
    ITT_SYNC_PREPARE(&unique_obj);
    ITT_SYNC_ACQUIRED(&unique_obj);


    /* 1: Initialize and start the Cilk runtime */
    __cilkrts_init_internal(1);

    /*
     * 2: Choose a worker for this thread (fail if none left).  The table of
     *    user workers is protected by the global OS mutex lock.
     */
    g = cilkg_get_global_state();
    global_os_mutex_lock();
    if (__builtin_expect(g->work_done, 0))
        __cilkrts_bug("Attempt to enter Cilk while Cilk is shutting down");
    w = find_free_worker(g);
    CILK_ASSERT(w);

    __cilkrts_set_tls_worker(w);
    __cilkrts_cilkscreen_establish_worker(w);
    {
        full_frame *ff = __cilkrts_make_full_frame(w, 0);
        ff->stack_self = sysdep_make_user_stack(w);
        tbb_interop_use_saved_stack_op_info(w, ff->stack_self);
        w->l->user_thread_imported = 0;
        CILK_ASSERT(ff->join_counter == 0);
        ff->join_counter = 1;
        w->l->frame_ff = ff;
        w->reducer_map = __cilkrts_make_reducer_map(w);
        __cilkrts_set_leftmost_reducer_map(w->reducer_map, 1);
    	load_pedigree_leaf_into_user_worker(w);
    }

    // Make sure that the head and tail are reset, and saved_protected_tail
    // allows all frames to be stolen.
    //
    // Note that we must NOT check w->exc, since workers that are trying to
    // steal from it will be updating w->exc and we don't own the worker lock.
    // It's not worth taking out the lock just for an assertion.
    CILK_ASSERT(w->head == w->l->ltq);
    CILK_ASSERT(w->tail == w->l->ltq);
    CILK_ASSERT(w->protected_tail  == w->ltq_limit);

    if (0 != __cilkrts_sysdep_bind_thread(w))
        // User thread couldn't be bound (probably because of a lack of
        // resources).  Continue, but don't allow stealing from this user
        // thread.
        __cilkrts_disallow_stealing(w, NULL);

    start_cilkscreen = (0 == w->g->Q);

    if (w->self != -1) {
        // w->self != -1, means that w is a normal user worker and must be
        // accounted for by the global state since other workers can steal from
        // it.

        // w->self == -1, means that w is an overflow worker and was created on
        // demand.  I.e., it does not need to be accounted for by the global
        // state.

        __cilkrts_enter_cilk(w->g);
    }

    global_os_mutex_unlock();

    /* If there's only 1 worker, the counts will be started in
     * __cilkrts_scheduler */
    if (g->P > 1)
    {
        START_INTERVAL(w, INTERVAL_IN_SCHEDULER);
        START_INTERVAL(w, INTERVAL_WORKING);
    }

    ITT_SYNC_RELEASING(&unique_obj);

    /* Turn on Cilkscreen if this is the first worker.  This needs to be done
     * when we are NOT holding the os mutex. */
    if (start_cilkscreen)
        __cilkrts_cilkscreen_enable_instrumentation();

    return w;
}

#ifndef _MSC_VER
/*
 * Define old version-specific symbols for binding threads (since they exist in
 * all Cilk code).  These aliases prohibit newly compiled code from loading an
 * old version of the runtime.  We can handle old code with a new runtime, but
 * new code with an old runtime is verboten!
 *
 * For Windows, the aliased symbol is exported in cilk-exports.def.
 */
#ifdef _DARWIN_C_SOURCE
/*
 * Mac OS X: Unfortunately, Darwin doesn't allow aliasing, so we just make a
 * call and hope the optimizer does the right thing.
 */
CILK_ABI_WORKER_PTR __cilkrts_bind_thread (void) {
    return BIND_THREAD_RTN();
}
#else
/*
 * Linux or BSD: Use the alias attribute to make the labels for the versioned
 * functions point to the same place in the code as the original.  Using
 * the two macros is annoying but required.
 */
#define STRINGIFY(x) #x
#define ALIASED_NAME(x) __attribute__ ((alias (STRINGIFY(x))))

CILK_ABI_WORKER_PTR __cilkrts_bind_thread(void)
    ALIASED_NAME(BIND_THREAD_RTN);

#endif // defined _DARWIN_C_SOURCE
#endif // !defined _MSC_VER

CILK_API_SIZET
__cilkrts_get_stack_size(void) {
    return cilkg_get_stack_size();
}

// Method for debugging.
CILK_API_VOID __cilkrts_dump_stats(void)
{
    // While the stats aren't protected by the global OS mutex, the table
    // of workers is, so take out the global OS mutex while we're doing this
    global_os_mutex_lock();
    if (cilkg_is_published()) {
        global_state_t *g = cilkg_get_global_state();
	__cilkrts_dump_stats_to_stderr(g);
    }
    else {
	__cilkrts_bug("Attempting to report Cilk stats before the runtime has started\n");
    }    
    global_os_mutex_unlock();
}

/*
 * __cilkrts_get_stack_region_id
 *
 * Interface called by Inspector (Piersol)
 *
 * Returns a __cilkrts_region_id for the stack currently executing on a thread.
 * Returns NULL on failure.
 */

CILK_INSPECTOR_ABI(__cilkrts_region_id)
__cilkrts_get_stack_region_id(__cilkrts_thread_id thread_id)
{
    global_state_t *g = cilkg_get_global_state();
    int i;

    if (NULL == g)
        return NULL;

    for (i = 0; i < g->total_workers; i++)
    {
        if (WORKER_FREE != g->workers[i]->l->type)
        {
            if (__cilkrts_sysdep_is_worker_thread_id(g, i, thread_id))
                return (__cilkrts_region_id)g->workers[i]->l->frame_ff->stack_self;
        }
    }

    return NULL;
}

/*
 * __cilkrts_get_stack_region_properties
 *
 * Interface called by Inspector (Piersol)
 *
 * Fills in the properties for a region_id.
 *
 * Returns false on invalid region_id or improperly sized
 * __cilkrts_region_properties
 */

CILK_INSPECTOR_ABI(int)
__cilkrts_get_stack_region_properties(__cilkrts_region_id region_id,
                                      __cilkrts_region_properties *properties)
{
    if (NULL == properties)
        return 0;

    if (properties->size != sizeof(__cilkrts_region_properties))
        return 0;

    return  __cilkrts_sysdep_get_stack_region_properties((__cilkrts_stack *)region_id, properties);
}

#ifndef _WIN32
CILK_ABI_THROWS_VOID __cilkrts_rethrow(__cilkrts_stack_frame *sf)
{
    __cilkrts_gcc_rethrow(sf);
}
#endif

/*
 * __cilkrts_unwatch_stack
 *
 * Callback for TBB to tell us they don't want to watch the stack anymore
 */

static __cilk_tbb_retcode __cilkrts_unwatch_stack(void *data)
{
    __cilk_tbb_stack_op_thunk o;

    // If the __cilkrts_stack wasn't available fetch it now
    if (TBB_INTEROP_DATA_DELAYED_UNTIL_BIND == data)
    {
        __cilkrts_stack *sd;
        full_frame *ff;
        __cilkrts_worker *w = __cilkrts_get_tls_worker();
        if (NULL == w)
        {
            // Free any saved stack op information
            tbb_interop_free_stack_op_info();

            return 0;       /* Success! */
        }

        __cilkrts_worker_lock(w);
        ff = w->l->frame_ff;
        __cilkrts_frame_lock(w,ff);
        data = ff->stack_self;
        __cilkrts_frame_unlock(w,ff);
        __cilkrts_worker_unlock(w);
    }

#if CILK_LIB_DEBUG /* Debug code */
    /* Get current stack */
    __cilkrts_stack *sd;
    full_frame *ff;
    __cilkrts_worker *w = __cilkrts_get_tls_worker();
    __cilkrts_worker_lock(w);
    ff = w->l->frame_ff;
    __cilkrts_frame_lock(w,ff);
    sd = ff->stack_self;
    CILK_ASSERT (data==sd);
    __cilkrts_frame_unlock(w,ff);
    __cilkrts_worker_unlock(w);
#endif

    /* Clear the callback information */
    o.data = NULL;
    o.routine = NULL; 
    __cilkrts_set_stack_op( (struct __cilkrts_stack*)data, o );

    // Note. Do *NOT* free any saved stack information here.   If they want to
    // free the saved stack op information, they'll do it when the thread is
    // unbound

    return 0;       /* Success! */
}

/*
 * __cilkrts_watch_stack
 *
 * Called by TBB, defined by Cilk.
 *
 * Requests that Cilk invoke the stack op routine when it orphans a stack. 
 * Cilk sets *u to a thunk that TBB should call when it is no longer interested
 * in watching the stack.
 */

CILK_API_TBB_RETCODE
__cilkrts_watch_stack(__cilk_tbb_unwatch_thunk *u,
                      __cilk_tbb_stack_op_thunk o)
{
    __cilkrts_stack *sd;
    __cilkrts_worker *w;

#ifdef _MSC_VER
    // This may be called by TBB *before* the OS has given us our
    // initialization call.  Make sure the module is initialized.
    sysdep_init_module();
#endif

    // Fetch the __cilkrts_worker bound to this thread
    w = __cilkrts_get_tls_worker();
    if (NULL == w)
    {
        // Save data for later.  We'll deal with it when/if this thread binds
        // to the runtime
        tbb_interop_save_stack_op_info(o);

        u->routine = __cilkrts_unwatch_stack;
        u->data = TBB_INTEROP_DATA_DELAYED_UNTIL_BIND;

        return 0;
    }

    /* Get current stack */
    __cilkrts_worker_lock(w);
    sd = w->l->frame_ff->stack_self;
    __cilkrts_worker_unlock(w);

/*    CILK_ASSERT( !sd->stack_op_data ); */
/*    CILK_ASSERT( !sd->stack_op_routine ); */

    /* Give TBB our callback */
    u->routine = __cilkrts_unwatch_stack;
    u->data = sd;
    /* Save the callback information */
    __cilkrts_set_stack_op( sd, o );

    return 0;   /* Success! */
}


// This function must be called only within a continuation, within the stack
// frame of the continuation itself.
CILK_API_INT __cilkrts_synched(void)
{
    __cilkrts_worker *w = __cilkrts_get_tls_worker();

    // If we don't have a worker, then we're synched by definition :o)
    if (NULL == w)
        return 1;

    // Check to see if we are in a stolen continuation.  If not, then
    // we are synched.
    uint32_t flags = w->current_stack_frame->flags;
    if (0 == (flags & CILK_FRAME_UNSYNCHED))
        return 1;

    // We are in a stolen continutation, but the join counter might have been
    // decremented to one, making us synched again.  Get the full frame so
    // that we can check the join counter.  ASSUME: frame_ff is stable (can be
    // read without a lock) in a stolen continuation -- it can't be stolen
    // while it's currently executing.
    full_frame *ff = w->l->frame_ff;

    // Make sure we have a full frame
    // TBD: Don't think that we should ever not have a full frame here.
    // CILK_ASSERT(NULL != ff); ?
    if (NULL == ff)
        return 1;

    // We're synched if there are no outstanding children at this instant in
    // time.  Note that this is a known race, but it's ok since we're only
    // reading.  We can get false negatives, but not false positives. (I.e.,
    // we can read a non-one join_counter just before it goes to one, but the
    // join_counter cannot go from one to greater than one while we're
    // reading.)
    return 1 == ff->join_counter;
}




CILK_API_INT
__cilkrts_bump_loop_rank_internal(__cilkrts_worker* w)
{
    // If we don't have a worker, then the runtime is not bound to this
    // thread and there is no rank to increment
    if (NULL == w)
        return -1;

    // We're at the start of the loop body.  Advance the cilk_for loop
    // body pedigree by following the parent link and updating its
    // rank.

    // Normally, we'd just write "w->pedigree.parent->rank++"
    // But we need to cast away the "const".
    ((__cilkrts_pedigree*) w->pedigree.parent)->rank++;

    // Zero the worker's pedigree rank since this is the start of a new
    // pedigree domain.
    w->pedigree.rank = 0;

    return 0;
}

/* end cilk-abi.c */

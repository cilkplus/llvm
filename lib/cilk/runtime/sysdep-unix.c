/*
 * sysdep-unix.c
 *
 *************************************************************************
 *
 * Copyright (C) 2010-2011 , Intel Corporation
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
 **************************************************************************
 */

#ifdef __linux__
    // define _GNU_SOURCE before *any* #include.
    // Even <stdint.h> will break later #includes if this macro is not
    // already defined when it is #included.
#   define _GNU_SOURCE
#endif

#include "sysdep.h"
#include "os.h"
#include "bug.h"
#include "local_state.h"
#include "signal_node.h"
#include "full_frame.h"
#include "jmpbuf.h"
#include "cilk_malloc.h"
#include "reducer_impl.h"
#include "metacall_impl.h"


// contains notification macros for VTune.
#include "cilk-ittnotify.h"

#include <stddef.h>

#ifdef __CYGWIN__
// On Cygwin, string.h doesnt declare strcasecmp if __STRICT_ANSI__ is defined
#   undef __STRICT_ANSI__
#endif

#include <string.h>
#include <pthread.h>
#include <unistd.h>

#ifdef __APPLE__
//#   include <scheduler.h>  // Angle brackets include Apple's scheduler.h, not ours.
#endif
#ifdef __linux__
#   include <sys/resource.h>
#   include <sys/sysinfo.h>
#endif
#ifdef __FreeBSD__
#   include <sys/resource.h>
// BSD does not define MAP_ANONYMOUS, but *does* define MAP_ANON. Aren't standards great!
#   define MAP_ANONYMOUS MAP_ANON
#endif


static void internal_enforce_global_visibility();

struct global_sysdep_state
{
    pthread_t *threads;
    size_t pthread_t_size; /* for cilk_db */
};

COMMON_SYSDEP
void __cilkrts_init_worker_sysdep(struct __cilkrts_worker *w)
{
    ITT_SYNC_CREATE(w, "Scheduler");
}

COMMON_SYSDEP
void __cilkrts_destroy_worker_sysdep(struct __cilkrts_worker *w)
{
}

COMMON_SYSDEP
void __cilkrts_init_global_sysdep(global_state_t *g)
{
    internal_enforce_global_visibility();

    __cilkrts_init_tls_variables();

    CILK_ASSERT(g->total_workers >= g->P - 1);
    g->sysdep = __cilkrts_malloc(sizeof (struct global_sysdep_state));
    CILK_ASSERT(g->sysdep);
    g->sysdep->pthread_t_size = sizeof (pthread_t);
    
    // TBD: Should this value be g->total_workers, or g->P?
    //      Need to check what we are using this field for.
    g->sysdep->threads = __cilkrts_malloc(sizeof(pthread_t) * g->total_workers);
    CILK_ASSERT(g->sysdep->threads);

    return;
}

COMMON_SYSDEP
void __cilkrts_destroy_global_sysdep(global_state_t *g)
{
    if (g->sysdep->threads)
        __cilkrts_free(g->sysdep->threads);
    __cilkrts_free(g->sysdep);
}

/*************************************************************
  Creation of worker threads:
*************************************************************/

static void internal_run_scheduler_with_exceptions(__cilkrts_worker *w)
{
    /* We assume the stack grows down. */
    char var;
    __cilkrts_cilkscreen_establish_c_stack(&var - 1000000, &var);

    __cilkrts_run_scheduler_with_exceptions(w);
}

/*
 * __cilkrts_worker_stub
 *
 * Thread start function called when we start a new worker.
 *
 * This function is exported so Piersol's stack trace displays
 * reasonable information
 */
NON_COMMON void* __cilkrts_worker_stub(void *arg)
{
    /*int status;*/
    __cilkrts_worker *w = (__cilkrts_worker *)arg;

#ifdef __INTEL_COMPILER
#ifdef USE_ITTNOTIFY
    // Name the threads for Advisor.  They don't want a worker number.
    __itt_thread_set_name("Cilk Worker");
#endif // defined USE_ITTNOTIFY
#endif // defined __INTEL_COMPILER

    /* Worker startup is serialized
    status = pthread_mutex_lock(&__cilkrts_global_mutex);
    CILK_ASSERT(status == 0);*/
    CILK_ASSERT(w->l->type == WORKER_SYSTEM);
    /*status = pthread_mutex_unlock(&__cilkrts_global_mutex);
    CILK_ASSERT(status == 0);*/

    __cilkrts_set_tls_worker(w);
    internal_run_scheduler_with_exceptions(w);

    return 0;
}

// /* Return the lesser of the argument and the operating system
//    limit on the number of workers (threads) that may or ought
//    to be created. */
// int sysdep_thread_limit(int n, int physical_cpus)
// {
//     /* On Linux thread creation fails somewhere short of the
//        number of available processes. */
//     struct rlimit lim;

//     if (n > 256 + 2 * physical_cpus)
//         n = 256 + 2 * physical_cpus;

//     if (getrlimit(RLIMIT_NPROC, &lim) == 0 && lim.rlim_cur != RLIM_INFINITY)
//     {
//         /* If the limit reads 0 or absurdly small, ignore it. */
//         unsigned int maxproc = (lim.rlim_cur * 3 + 3) / 4;
//         if (maxproc > 8 + 2 * physical_cpus && maxproc < n)
//             n = maxproc;
//     }
//     return n;
// }



static void write_version_file (global_state_t *, int);

/* Create n worker threads from base..top-1
 */
static void create_threads(global_state_t *g, int base, int top)
{
    int i;

    for (i = base; i < top; i++) {
        int status;

        status = pthread_create(&g->sysdep->threads[i], NULL, __cilkrts_worker_stub, g->workers[i]);
        if (status != 0)
            __cilkrts_bug("Cilk runtime error: thread creation (%d) failed: %d\n", i, status);
    }
}

#if PARALLEL_THREAD_CREATE
static int volatile threads_created = 0;

// Create approximately half of the worker threads, and then become a worker
// ourselves.
static void * create_threads_and_work (void * arg)
{
    global_state_t *g = ((__cilkrts_worker *)arg)->g;

    create_threads(g, g->P/2, g->P-1);
    // Let the initial thread know that we're done.
    threads_created = 1;

    // Ideally this turns into a tail call that wipes out this stack frame.
    return __cilkrts_worker_stub (arg);
}
#endif
void __cilkrts_start_workers(global_state_t *g, int n)
{
    g->workers_running = 1;
    g->work_done = 0;

    if (!g->sysdep->threads)
        return;

    // Do we actually have any threads to create?
    if (n > 0)
    {
#if PARALLEL_THREAD_CREATE
            int status;
            // We create (a rounded up) half of the threads, thread one creates the rest
            int half_threads = (n+1)/2;
        
            // Create the first thread passing a different thread function, so that it creates threads itself
            status = pthread_create(&g->sysdep->threads[0], NULL, create_threads_and_work, g->workers[0]);

            if (status != 0)
                __cilkrts_bug("Cilk runtime error: thread creation (0) failed: %d\n", status);
            
            // Then the rest of the ones we have to create
            create_threads(g, 1, half_threads);

            // Now wait for the first created thread to tell us it's created all of its threads.
            // We could maybe drop this a bit lower and overlap with write_version_file.
            while (!threads_created)
                __cilkrts_yield();
#else
            // Simply create all the threads linearly here.
            create_threads(g, 0, n);
#endif
    }
    // write the version information to a file if the environment is configured
    // for it (the function makes the check).
    write_version_file(g, n);


    return;
}

void __cilkrts_stop_workers(global_state_t *g)
{
    int i;

    // Tell the workers to give up

    g->work_done = 1;

    if (g->workers_running == 0)
        return;

    if (!g->sysdep->threads)
        return;

    /* Make them all runnable. */
    if (g->P > 1) {
        CILK_ASSERT(g->workers[0]->l->signal_node);
        signal_node_msg(g->workers[0]->l->signal_node, 1);
    }

        for (i = 0; i < g->P - 1; ++i) {
            int sc_status;
            void *th_status;

            sc_status = pthread_join(g->sysdep->threads[i], &th_status);
            if (sc_status != 0)
                __cilkrts_bug("Cilk runtime error: thread join (%d) failed: %d\n", i, sc_status);
        }

    g->workers_running = 0;


    return;
}

/*
 * Restore the floating point state that is stored in a stack frame at each
 * spawn.  This should be called each time a frame is resumed.
 */
static inline void restore_fp_state (__cilkrts_stack_frame *sf) {
    __asm__ ( "ldmxcsr %0\n\t"
              "fnclex\n\t"
              "fldcw %1"
              :
              : "m" (sf->mxcsr), "m" (sf->fpcsr));
}

/* Resume user code after a spawn or sync, possibly on a different stack.

   Note: Traditional BSD longjmp would fail with a "longjmp botch"
   error rather than change the stack pointer in the wrong direction.
   Linux appears to let the program take the chance.

   This function is called to resume after a sync or steal.  In both cases
   ff->sync_sp starts out containing the original stack pointer of the loot.
   In the case of a steal, the stack pointer stored in sf points to the
   thief's new stack.  In the case of a sync, the stack pointer stored in sf
   points into original stack (i.e., it is either the same as ff->sync_sp or a
   small offset from it caused by pushes and pops between the spawn and the
   sync).  */
NORETURN __cilkrts_resume(__cilkrts_worker *w, full_frame *ff,
                          __cilkrts_stack_frame *sf)
{
    // Assert: w is the only worker that knows about ff right now, no
    // lock is needed on ff.

    const int flags = sf->flags;
    void *sp;

    w->current_stack_frame = sf;
    sf->worker = w;
    CILK_ASSERT(flags & CILK_FRAME_SUSPENDED);
    CILK_ASSERT(!sf->call_parent);
    CILK_ASSERT(w->head == w->tail);

    if (ff->simulated_stolen)
        /* We can't prevent __cilkrts_make_unrunnable_sysdep from discarding
         * the stack pointer because there is no way to tell it that we are
         * doing a simulated steal.  Thus, we must recover the stack pointer
         * here. */
        SP(sf) = ff->sync_sp;

    sp = SP(sf);

    /* Debugging: make sure stack is accessible. */
    ((volatile char *)sp)[-1];

    __cilkrts_take_stack(ff, sp);

    /* The leftmost frame has no allocated stack */
    if (ff->simulated_stolen)
        CILK_ASSERT(flags & CILK_FRAME_UNSYNCHED && ff->sync_sp == NULL);
    else if (flags & CILK_FRAME_UNSYNCHED)
        /* XXX By coincidence sync_sp could be null. */
        CILK_ASSERT(ff->stack_self != NULL && ff->sync_sp != NULL);
    else
        /* XXX This frame could be resumed unsynched on the leftmost stack */
        CILK_ASSERT((ff->sync_master == 0 || ff->sync_master == w) &&
                    ff->sync_sp == 0);
    /*if (w->l->type == WORKER_USER)
        CILK_ASSERT(ff->stack_self == NULL);*/

    // Notify the Intel tools that we're stealing code
    ITT_SYNC_ACQUIRED(sf->worker);
#ifdef ENABLE_NOTIFY_ZC_INTRINSIC
    __notify_zc_intrinsic("cilk_continue", sf);
#endif // defined ENABLE_NOTIFY_ZC_INTRINSIC

    if (ff->stack_self) {
        // Notify TBB that we are resuming.
        __cilkrts_invoke_stack_op(w, CILK_TBB_STACK_ADOPT, ff->stack_self);
    }

    sf->flags &= ~CILK_FRAME_SUSPENDED;

#ifndef __MIC__
    if (CILK_FRAME_VERSION_VALUE(sf->flags) >= 1) {
        // Restore the floating point state that was set in this frame at the
        // last spawn.
        //
        // This feature is only available in ABI 1 or later frames.
        restore_fp_state(sf);
    }
#endif

    CILK_LONGJMP(sf->ctx);
    /*NOTREACHED*/
    /* Intel's C compiler respects the preceding lint pragma */
}

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>

struct __cilkrts_stack
{
    /* If /size/ and /top/ are zero this is the system stack for thread /owner/.
       If /top/ and /size/ are both nonzero this is an allocated stack and
       /owner/ is undefined. */
    char *top;
    size_t size;
    pthread_t owner;

    /* Cilk/TBB interop callback routine/data. */
    __cilk_tbb_pfn_stack_op stack_op_routine;
    void *stack_op_data;
};

void __cilkrts_set_stack_op(__cilkrts_stack *sd,
                            __cilk_tbb_stack_op_thunk o)
{
    sd->stack_op_routine = o.routine;
    sd->stack_op_data = o.data;
}

void __cilkrts_invoke_stack_op(__cilkrts_worker *w,
                               enum __cilk_tbb_stack_op op,
                               __cilkrts_stack *sd)
{
    // If we don't have a stack we can't do much, can we?
    if (NULL == sd)
        return;

    if (0 == sd->stack_op_routine)
    {
        return;
    }

    (*sd->stack_op_routine)(op,sd->stack_op_data);
    if (op == CILK_TBB_STACK_RELEASE)
    {
        sd->stack_op_routine = 0;
        sd->stack_op_data = 0;
    }
}

/*
 * tbb_interop_save_stack_op_info
 *
 * Save TBB interop information for an unbound thread.  It will get picked
 * up when the thread is bound to the runtime.
 */
void tbb_interop_save_stack_op_info(__cilk_tbb_stack_op_thunk o)
{
    __cilk_tbb_stack_op_thunk *saved_thunk =
        __cilkrts_get_tls_tbb_interop();

    // If there is not already space allocated, allocate some.
    if (NULL == saved_thunk) {
        saved_thunk = (__cilk_tbb_stack_op_thunk*)
            __cilkrts_malloc(sizeof(__cilk_tbb_stack_op_thunk));
        __cilkrts_set_tls_tbb_interop(saved_thunk);
    }

    *saved_thunk = o;
}

/*
 * tbb_interop_save_info_from_stack
 *
 * Save TBB interop information from the __cilkrts_stack.  It will get picked
 * up when the thread is bound to the runtime next time.
 */
void tbb_interop_save_info_from_stack(__cilkrts_stack *sd)
{
    __cilk_tbb_stack_op_thunk *saved_thunk;

    // If there is no TBB interop data, just return
    if (NULL == sd || NULL == sd->stack_op_routine) return;

    saved_thunk = __cilkrts_get_tls_tbb_interop();

    // If there is not already space allocated, allocate some.
    if (NULL == saved_thunk) {
        saved_thunk = (__cilk_tbb_stack_op_thunk*)
            __cilkrts_malloc(sizeof(__cilk_tbb_stack_op_thunk));
        __cilkrts_set_tls_tbb_interop(saved_thunk);
    }

    saved_thunk->routine = sd->stack_op_routine;
    saved_thunk->data = sd->stack_op_data;
}

/*
 * tbb_interop_use_saved_stack_op_info
 *
 * If there's TBB interop information that was saved before the thread was
 * bound, apply it now
 */
void tbb_interop_use_saved_stack_op_info(__cilkrts_worker *w,
                                         __cilkrts_stack *sd)
{
    struct __cilk_tbb_stack_op_thunk *saved_thunk =
        __cilkrts_get_tls_tbb_interop();

    // If we haven't allocated a TBB interop index, we don't have any saved info
    if (NULL == saved_thunk) return;

    // Associate the saved info with the __cilkrts_stack
    __cilkrts_set_stack_op(sd, *saved_thunk);

    // Free the saved data.  We'll save it again if needed when the code
    // returns from the initial function
    tbb_interop_free_stack_op_info();
}

/*
 * tbb_interop_free_stack_op_info
 *
 * Free saved TBB interop memory.  Should only be called when the thread is
 * not bound.
 */
void tbb_interop_free_stack_op_info(void)
{
    struct __cilk_tbb_stack_op_thunk *saved_thunk =
        __cilkrts_get_tls_tbb_interop();

    // If we haven't allocated a TBB interop index, we don't have any saved info
    if (NULL == saved_thunk) return;

    // Free the memory and wipe out the TLS value
    __cilkrts_free(saved_thunk);
    __cilkrts_set_tls_tbb_interop(NULL);
}

void __cilkrts_bind_stack(full_frame *ff, char *new_sp,
                          __cilkrts_stack *parent_stack,
                          __cilkrts_worker *owner)
{
    __cilkrts_stack_frame *sf = ff->call_stack;
    __cilkrts_stack *sd = ff->stack_self;
    CILK_ASSERT(sizeof SP(sf) <= sizeof (size_t));

    SP(sf) = new_sp;

    // Need to do something with parent_stack and owner?
    return;
}

char *__cilkrts_stack_to_pointer(__cilkrts_stack *s, __cilkrts_stack_frame *sf)
{
    if (!s)
        return NULL;

    /* Preserve outgoing argument space and stack alignment on steal.
       Outgoing argument space is bounded by the difference between
       stack and frame pointers.  Some user code is known to rely on
       16 byte alignment.  Maintain 32 byte alignment for future
       compatibility. */
#define SMASK 31 /* 32 byte alignment */
    if (sf) {
        char *fp = FP(sf), *sp = SP(sf);
        int fp_align = (int)(size_t)fp & SMASK;
        ptrdiff_t space = fp - sp;
        char *top_aligned = (char *)((((size_t)s->top - SMASK) & ~(size_t)SMASK) | fp_align);
        /* Don't allocate an unreasonable amount of stack space. */
        if (space < 32)
            space = 32 + (space & SMASK);
        else if (space > 40 * 1024)
            space = 40 * 1024 + (space & SMASK);
        return top_aligned - space;
    }
    return s->top - 256;
}

#define PAGE 4096

/*
 * Return a pointer to the top of a "tiny" stack that is 64 KB (plus a buffer
 * page on each end).
 *
 * No reasonable program should need more than 64 KB, so if we hit a buffer,
 * we're doing it wrong.
 */
void *sysdep_make_tiny_stack (__cilkrts_worker *w)
{
    char *p;
    __cilkrts_stack *s;

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

    p = mmap(0, PAGE * 18, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS,
             -1, 0);
    if (MAP_FAILED == p) {
        // For whatever reason (probably ran out of memory), mmap() failed.
        // There is no stack to return, so the program loses parallelism.
        if (0 == __cilkrts_xchg(&w->g->failure_to_allocate_stack, 1)) {
            cilkos_warning("Failed to allocate memory for a new stack.\n"
                           "Continuing with some loss of parallelism.\n");
        }
        return NULL;
    }
    mprotect(p + (17 * PAGE), PAGE, PROT_NONE);
    mprotect(p, PAGE, PROT_NONE);

    return (void*)(p + (17 * PAGE));
}

/*
 * Free a "tiny" stack (created with sysdep_make_tiny_stack()).
 */
void sysdep_destroy_tiny_stack (void *p)
{
    char *s = (char*)p;
    s = s - (17 * PAGE);
    munmap((void*)s, 18 * PAGE);
}

__cilkrts_stack *__cilkrts_make_stack(__cilkrts_worker *w)
{
    __cilkrts_stack *s;
    char *p;
    size_t stack_size;

#if defined CILK_PROFILE && defined HAVE_SYNC_INTRINSICS
#define PROFILING_STACKS 1
#else
#define PROFILING_STACKS 0
#endif

    if (PROFILING_STACKS || w->g->max_stacks > 0) {
        if (w->g->max_stacks > 0 && w->g->stacks > w->g->max_stacks) {
            /* No you can't have a stack.  Not yours. */
            return NULL;
        } else {
            /* We think we are allowed to allocate a stack.  Perform an atomic
               increment on the counter and verify that there really are enough
               stacks remaining for us. */
            long hwm = __sync_add_and_fetch(&w->g->stacks, 1);
            if (w->g->max_stacks > 0 && hwm > w->g->max_stacks) {
                /* Whoops!  Another worker got to it before we did.
                   C'est la vie. */
                return NULL;
            }

#ifdef CILK_PROFILE
            /* Keeping track of the largest stack count observed by this worker
               is part of profiling.  The copies will be merged at the end of
               execution. */
            if (PROFILING_STACKS && hwm > w->l->stats.stack_hwm) {
                w->l->stats.stack_hwm = hwm;
            }
#endif
        }
    }

    stack_size = w->g->stack_size;
    CILK_ASSERT(stack_size > 0);

    p = mmap(0, stack_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS,
             -1, 0);
    if (MAP_FAILED == p) {
        // For whatever reason (probably ran out of memory), mmap() failed.
        // There is no stack to return, so the program loses parallelism.
        if (0 == __cilkrts_xchg(&w->g->failure_to_allocate_stack, 1)) {
            cilkos_warning("Failed to allocate memory for a new stack.\n"
                          "Continuing with some loss of parallelism.\n");
        }
        return NULL;
    }
    mprotect(p + stack_size - PAGE, PAGE, PROT_NONE);
    mprotect(p, PAGE, PROT_NONE);

    s = __cilkrts_malloc(sizeof (struct __cilkrts_stack));
    CILK_ASSERT(s);
    s->top = p + stack_size - PAGE;
    s->size = stack_size - (PAGE + PAGE);
    memset(&s->owner, 0, sizeof s->owner);

    s->stack_op_routine = NULL;
    s->stack_op_data = NULL;

    return s;
}

void __cilkrts_free_stack(global_state_t *g,
			  __cilkrts_stack *sd)
{
    char *s;
    size_t size;

    CILK_ASSERT(g->max_stacks <= 0);

#if defined CILK_PROFILE && defined HAVE_SYNC_INTRINSICS
    __sync_sub_and_fetch(&g->stacks, 1);
#endif

    s = sd->top;
    size = sd->size;

    CILK_ASSERT(s && size);

#if __GNUC__
    {
        char *fp = __builtin_frame_address(0);
        CILK_ASSERT(fp < s - 10000 || fp > s);
    }
#endif
    /* DEBUG: */
    ((volatile char *)s)[-1];

    s += PAGE;
    size += PAGE + PAGE;

    if (munmap(s - size, size) < 0)
        __cilkrts_bug("Cilk: stack release failed error %d", errno);

    sd->top = 0;
    sd->size = 0;
    __cilkrts_free(sd);

    return;
}

void __cilkrts_sysdep_reset_stack(__cilkrts_stack *sd)
{
    CILK_ASSERT(sd->stack_op_routine == NULL);
    CILK_ASSERT(sd->stack_op_data == NULL);
    return;
}

void __cilkrts_make_unrunnable_sysdep(__cilkrts_worker *w,
                                      full_frame *ff,
                                      __cilkrts_stack_frame *sf,
                                      int state_valid,
                                      const char *why)
{
    (void)w; /* unused */
    sf->except_data = 0;

    if (state_valid && ff->frame_size == 0)
        ff->frame_size = __cilkrts_get_frame_size(sf);

    SP(sf) = 0;
}


/*
 * __cilkrts_sysdep_is_worker_thread_id
 *
 * Returns true if the thread ID specified matches the thread ID we saved
 * for a worker.
 */

int __cilkrts_sysdep_is_worker_thread_id(global_state_t *g,
                                         int i,
                                         void *thread_id)
{
#ifdef __linux__
    pthread_t tid = *(pthread_t *)thread_id;
    if (i < 0 || i > g->total_workers)
        return 0;
    return g->sysdep->threads[i] == tid;
#else
    // Needs to be implemented
    return 0;
#endif
}

int __cilkrts_sysdep_bind_thread(__cilkrts_worker *w)
{
    if (w->self < 0) {
        // w->self < 0 means that this is an ad-hoc user worker not known to
        // the global state.  Nobody will ever try to steal from it, so it
        // does not need a scheduler_stack.
        return 0; // success
    }

    // Allocate a scheduler_stack for this user worker if one does not
    // already exist.
    if (NULL == w->l->scheduler_stack) {

        // The scheduler stack does not need to be as large as a normal
        // programm stack.  Returns null on failure (probably indicating that
        // we're out of memory).
        w->l->scheduler_stack = sysdep_make_tiny_stack(w);

        // Return success (zero) if we successfully allocated a scheduler
        // stack and failure (non-zero) if stack allocation returned NULL.
        return (NULL == w->l->scheduler_stack ? -1 : 0);
    }

    return 0; // success
}

void __cilkrts_sysdep_unbind_thread(__cilkrts_worker *w)
{
    // Needs to be implemented
}

int __cilkrts_sysdep_get_stack_region_properties(__cilkrts_stack *sd,
                                                 struct  __cilkrts_region_properties *props)
{
    return 0;
}


/*************************************************************
  Version information:
*************************************************************/

#include <dlfcn.h>
#include "internal/cilk_version.h"
#include <stdio.h>
#include <sys/utsname.h>

/* (Non-static) dummy function is used by get_runtime_path() to find the path
 * to the .so containing the Cilk runtime.
 */
void dummy_function() { }

/* return a string with the path to the Cilk runtime, or "unknown" if the path
 * cannot be determined.
 */
static const char *get_runtime_path ()
{
#ifdef __CYGWIN__
    // Cygwin doesn't support dladdr, which sucks
    return "unknown";
#else
    Dl_info info;
    if (0 == dladdr(dummy_function, &info)) return "unknown";
    return info.dli_fname;
#endif
}

/* if the environment variable, CILK_VERSION, is defined, writes the version
 * information to the specified file.
 * g is the global state that was just created, and n is the number of workers
 * that were made (or requested from RML) for it.
 */
static void write_version_file (global_state_t *g, int n)
{
    const char *env;      // environment variable.
    char buf[256];        // print buffer.
    time_t t;
    FILE *fp;
    struct utsname sys_info;
    int err;              // error code from system calls.

    // if CILK_VERSION is not set, or if the file cannot be opened, fail
    // silently.  Otherwise open the file for writing (or use stderr or stdout
    // if the user specifies).
    if (NULL == (env = getenv("CILK_VERSION"))) return;
    if (0 == strcasecmp(env, "stderr"))         fp = stderr;
    else if (0 == strcasecmp(env, "stdout"))    fp = stdout;
    else if (NULL == (fp = fopen(env, "w")))    return;

    // get a string for the current time.  E.g.,
    // Cilk runtime initialized: Thu Jun 10 13:28:00 2010
    t = time(NULL);
    strftime(buf, 256, "%a %b %d %H:%M:%S %Y", localtime(&t));
    fprintf(fp, "Cilk runtime initialized: %s\n", buf);

    // Print runtime info.  E.g.,
    // Cilk runtime information
    // ========================
    // Cilk version: 2.0.0 Build 9184
    // Built by willtor on host willtor-desktop
    // Compilation date: Thu Jun 10 13:27:42 2010
    // Compiled with ICC V99.9.9, ICC build date: 20100610

    fprintf(fp, "\nCilk runtime information\n");
    fprintf(fp, "========================\n");
    fprintf(fp, "Cilk version: %d.%d.%d Build %d\n",
            VERSION_MAJOR,
            VERSION_MINOR,
            VERSION_REV,
            VERSION_BUILD);
    fprintf(fp, "Built by "BUILD_USER" on host "BUILD_HOST"\n");
    fprintf(fp, "Compilation date: "__DATE__" "__TIME__"\n");

#ifdef __INTEL_COMPILER
    // Compiled by the Intel C/C++ compiler.
    fprintf(fp, "Compiled with ICC V%d.%d.%d, ICC build date: %d\n",
            __INTEL_COMPILER / 100,
            (__INTEL_COMPILER / 10) % 10,
            __INTEL_COMPILER % 10,
            __INTEL_COMPILER_BUILD_DATE);
#else
    // Compiled by GCC.
    fprintf(fp, "Compiled with GCC V%d.%d.%d\n",
            __GNUC__,
            __GNUC_MINOR__,
            __GNUC_PATCHLEVEL__);
#endif // defined __INTEL_COMPILER

    // Print system info.  E.g.,
    // System information
    // ==================
    // Cilk runtime path: /opt/icc/64/lib/libcilkrts.so.5
    // System OS: Linux, release 2.6.28-19-generic
    // System architecture: x86_64

    err = uname(&sys_info);
    fprintf(fp, "\nSystem information\n");
    fprintf(fp, "==================\n");
    fprintf(fp, "Cilk runtime path: %s\n", get_runtime_path());
    fprintf(fp, "System OS: %s, release %s\n",
            err < 0 ? "unknown" : sys_info.sysname,
            err < 0 ? "?" : sys_info.release);
    fprintf(fp, "System architecture: %s\n",
            err < 0 ? "unknown" : sys_info.machine);

    // Print thread info.  E.g.,
    // Thread information
    // ==================
    // System cores: 8
    // Cilk workers requested: 8
    // Thread creator: Private

    fprintf(fp, "\nThread information\n");
    fprintf(fp, "==================\n");
    fprintf(fp, "System cores: %d\n", (int)sysconf(_SC_NPROCESSORS_ONLN));
    fprintf(fp, "Cilk workers requested: %d\n", n);
#if (PARALLEL_THREAD_CREATE)
        fprintf(fp, "Thread creator: Private (parallel)\n");
#else
        fprintf(fp, "Thread creator: Private\n");
#endif

    if (fp != stderr && fp != stdout) fclose(fp);
    else fflush(fp); // flush the handle buffer if it is stdout or stderr.
}


/*
 * __cilkrts_establish_c_stack
 *
 * Tell Cilkscreen about the user stack bounds.
 *
 * Note that the Cilk V1 runtime only included the portion of the stack from
 * the entry into Cilk, down.  We don't appear to be able to find that, but
 * I think this will be sufficient.
 */

void __cilkrts_establish_c_stack(void)
{
    /* FIXME: Not implemented. */

    /* TBD: Do we need this */
    /*
    void __cilkrts_cilkscreen_establish_c_stack(char *begin, char *end);

    size_t r;
    MEMORY_BASIC_INFORMATION mbi;

    r = VirtualQuery (&mbi,
                      &mbi,
                      sizeof(mbi));

    __cilkrts_cilkscreen_establish_c_stack((char *)mbi.BaseAddress,
                                   (char *)mbi.BaseAddress + mbi.RegionSize);
    */
}

/*
 * internal_enforce_global_visibility
 *
 * Ensure global visibility of public symbols, for proper Cilk-TBB interop.
 *
 * If Cilk runtime is loaded dynamically, its symbols might remain unavailable
 * for global search with dladdr; that might prevent TBB from finding Cilk
 * in the process address space and initiating the interop protocol.
 * The workaround is for the library to open itself with RTLD_GLOBAL flag.
 */

static __attribute__((noinline))
void internal_enforce_global_visibility()
{
    void* handle = dlopen( get_runtime_path(), RTLD_GLOBAL|RTLD_LAZY );

    /* For proper reference counting, close the handle immediately. */
    if( handle) dlclose(handle);
}

/*
 * Special scheduling entrypoint for a WORKER_USER.  Ensure a new stack has been
 * created and the stack pointer has been placed on it before entering
 * worker_user_scheduler().
 *
 * Call this function the first time a WORKER_USER has returned to a stolen
 * parent and cannot continue.  Every time after that, the worker can simply
 * longjmp() like any other worker.
 */
static NOINLINE
void worker_user_scheduler()
{
    __cilkrts_worker *w = __cilkrts_get_tls_worker();

    // This must be a user worker
    CILK_ASSERT(WORKER_USER == w->l->type);

    // Run the continuation function passed to longjmp_into_runtime
    run_scheduling_stack_fcn(w);
    w->reducer_map = 0;

    cilkbug_assert_no_uncaught_exception();

    STOP_INTERVAL(w, INTERVAL_IN_SCHEDULER);
    STOP_INTERVAL(w, INTERVAL_WORKING);

    // Enter the scheduling loop on the user worker.  This function will
    // never return
    __cilkrts_run_scheduler_with_exceptions(w);

    // A WORKER_USER, at some point, will resume on the original stack and
    // leave Cilk.  Under no circumstances do we ever exit off of the bottom
    // of this stack.
    CILK_ASSERT(0);
}

/*
 * __cilkrts_sysdep_import_user_thread
 *
 * Imports a user thread the first time it returns to a stolen parent
 */

void __cilkrts_sysdep_import_user_thread(__cilkrts_worker *w)
{
    void *ctx[5]; // Jump buffer for __builtin_setjmp/longjmp.

    CILK_ASSERT(w->l->scheduler_stack);

    // It may be that this stack has been used before (i.e., the worker was
    // bound to a thread), and in principle, we could just jump back into
    // the runtime, but we'd have to keep around extra data to do that, and
    // there is no harm in starting over, here.

    // Move the stack pointer onto the scheduler stack.  The subsequent
    // call will move execution onto that stack.  We never return from
    // that call, and every time we longjmp_into_runtime() after this,
    // the w->l->env jump buffer will be populated.
    if (0 == __builtin_setjmp(ctx)) {
        ctx[2] = w->l->scheduler_stack; // replace the stack pointer.
        __builtin_longjmp(ctx, 1);
    } else {
        // We can't just pass the worker through as a parameter to
        // worker_user_scheduler because the generated code might try to
        // retrieve w using stack-relative addressing instead of bp-relative
        // addressing and would get a bogus value.
        worker_user_scheduler(); // noinline, does not return.
    }

    CILK_ASSERT(0); // Should never reach this point.
}

/*
 * Make a fake user stack descriptor to correspond to the user's stack.
 */
__cilkrts_stack *sysdep_make_user_stack (__cilkrts_worker *w)
{
    return calloc(1, sizeof(struct __cilkrts_stack));
}

/*
 * Destroy the fake user stack descriptor that corresponds to the user's stack.
 */
void sysdep_destroy_user_stack (__cilkrts_stack *sd)
{
    free(sd);
}



/*
  Local Variables: **
  c-file-style:"bsd" **
  c-basic-offset:4 **
  indent-tabs-mode:nil **
  End: **
*/

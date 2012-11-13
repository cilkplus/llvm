/* sysdep.h                  -*-C++-*-
 *
 *************************************************************************
 *
 * Copyright (C) 2009-2011 , Intel Corporation
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
 **************************************************************************/

/**
 * @file sysdep.h
 *
 * @brief Common system-dependent functions
 */

#ifndef INCLUDED_SYSDEP_DOT_H
#define INCLUDED_SYSDEP_DOT_H

#include <cilk/common.h>
#include <internal/abi.h>

#include "global_state.h"
#include "full_frame.h"
#include "os.h"
#include "os_mutex.h"

#include "cilk-tbb-interop.h"

__CILKRTS_BEGIN_EXTERN_C

// Part of inspector ABI
typedef struct __cilkrts_region_properties __cilkrts_region_properties;

/**
 * Bind the __cilkrts_stack_frame to the stack
 *
 * @param ff full_frame for the frame we're binding
 * @param new_sp Not used.
 * @param parent_stack __cilkrts_stack of this frames parent
 * @param owner __cilkrts_worker for the user worker thread that captains
 * the team that this stack is contributing to.
 */
COMMON_SYSDEP
void __cilkrts_bind_stack(full_frame *ff,
                          char *new_sp,
                          __cilkrts_stack *parent_stack,
                          __cilkrts_worker *owner);

/**
 * Return an address on the specified stack.  Mostly obsolete.
 */
COMMON_SYSDEP
char *__cilkrts_stack_to_pointer(__cilkrts_stack *sd,
                                 __cilkrts_stack_frame *sf);

/**
 * Code to initialize the system-dependent portion of the global_state_t
 *
 * @param g Pointer to the global state.
 */
COMMON_SYSDEP
void __cilkrts_init_global_sysdep(global_state_t *g);

/**
 * Code to clean up the system-dependent portion of the global_state_t
 *
 * @param g Pointer to the global state.
 */
COMMON_SYSDEP
void __cilkrts_destroy_global_sysdep(global_state_t *g);

/**
 * Passes stack range to Cilkscreen.  This functionality should be moved
 * into Cilkscreen.
 */
COMMON_SYSDEP
void __cilkrts_establish_c_stack(void);

/**
 * Allocate and initialize a __cilkrts_stack.
 *
 * @param w The worker to attribute this stack to - mostly used for stats.
 *
 * @return Pointer to the initilaized __cilkrts_stack.
 * @return NULL if we failed to allocate the stack.
 */
COMMON_SYSDEP
__cilkrts_stack *__cilkrts_make_stack(__cilkrts_worker *w);

/**
 * Release any resources associated with a __cilkrts_stack
 *
 * @param g The global state - used for stats
 * @param sd The __cilkrts_stack to be released
 */
COMMON_SYSDEP
void __cilkrts_free_stack(global_state_t *g, __cilkrts_stack *sd);

/**
 * Allocate a __cilkrts_stack with a small size for use as a scheduling stack.
 *
 * @param w The worker to attribute this stack to - mostly used for stats.
 *
 * @return Pointer to the initilaized __cilkrts_stack.
 * @return NULL if we failed to allocate the stack.
 */
COMMON_SYSDEP
void *sysdep_make_tiny_stack (__cilkrts_worker *w);

/**
 * Release any resources associated with a __cilkrts_stack created as a
 * scheduling stack.
 *
 * @param sd The __cilkrts_stack to be released
 */
COMMON_SYSDEP
void sysdep_destroy_tiny_stack (void *sd);

/**
 * Allocate and initialize a __cilkrts_stack to use to run user code.
 *
 * @param w The worker to attribute this stack to - mostly used for stats.
 *
 * @return Pointer to the initilaized __cilkrts_stack.
 * @return NULL if we failed to allocate the stack.
 */
COMMON_SYSDEP
__cilkrts_stack *sysdep_make_user_stack (__cilkrts_worker *w);

/**
 * Release any resources associated with a __cilkrts_stack created as a
 * user stack.
 *
 * @param sd The __cilkrts_stack to be released
 */
COMMON_SYSDEP
void sysdep_destroy_user_stack (__cilkrts_stack *sd);

/**
 * Reset stack-specific information so the stack can be cached and reused
 *
 * @param sd The __cilkrts_stack to be reset.
 */
COMMON_SYSDEP
void __cilkrts_sysdep_reset_stack(__cilkrts_stack *sd);

/**
 * Save system dependent information in the full_frame and
 * __cilkrts_stack_frame.  Part of promoting a
 * __cilkrts_stack_frame to a full_frame.
 *
 * @param w The worker the frame was running on.  Not used.
 * @param ff The full frame that is being created for the
 * __cilkrts_stack_frame.
 * @param sf The __cilkrts_stack_frame that's being promoted
 * to a full frame.
 * @param state_valid ?
 * @param why A description of why make_unrunnable was called.
 * Used for debugging.
 */
COMMON_SYSDEP
void __cilkrts_make_unrunnable_sysdep(__cilkrts_worker *w,
                                      full_frame *ff,
                                      __cilkrts_stack_frame *sf,
                                      int state_valid,
                                      const char *why);

/**
 * Resume execution of the full frame.
 *
 * @param w The worker to resume execution on.
 * @param ff The full_frame to resume executing.
 * @param sf The __cilkrts_stack_frame to resume executing.
 */
COMMON_SYSDEP
NORETURN __cilkrts_resume(__cilkrts_worker *w,
                          full_frame *ff,
                          __cilkrts_stack_frame *sf);

/**
 * OS-specific code to spawn worker threads.
 *
 * @param g The global state.
 * @param n Number of worker threads to start.
 */
COMMON_SYSDEP
void __cilkrts_start_workers(global_state_t *g, int n);

/**
 * OS-specific code to stop worker threads.
 *
 * @param g The global state.
 */
COMMON_SYSDEP
void __cilkrts_stop_workers(global_state_t *g);

/**
 * System dependent function called when a thread is bound to a worker.
 *
 * @param w Worker to bind to the currently executing thread.
 *
 * @return 0 on success.
 * @return non-zero on failure.
 */
COMMON_SYSDEP
int __cilkrts_sysdep_bind_thread(__cilkrts_worker *w);

/**
 * System dependent function called when a thread is unbound from a
 * worker.
 *
 * @param w Worker to unbind from the currently executing thread.
 */
COMMON_SYSDEP
void __cilkrts_sysdep_unbind_thread(__cilkrts_worker *w);

/**
 * Imports a user thread the first time it returns to a stolen parent.
 * The thread has been bound to a worker, but additional steps need to
 * be taken to start running a scheduling loop.
 *
 * @param w The worker bound to the thread.
 */
COMMON_SYSDEP
void __cilkrts_sysdep_import_user_thread(__cilkrts_worker *w);

/**
 * Fills in the __cilkrts_region_properties for a __cilkrts_stack.
 *
 * @param sd The stack that's being run on.
 * @param properties Buffer to hold information about the stack region.
 *
 * @return 1 on success.
 * @return 0 on failure.
 */
COMMON_SYSDEP
int __cilkrts_sysdep_get_stack_region_properties(__cilkrts_stack *sd,
                                                 __cilkrts_region_properties *properties);

/**
 * Returns true if the thread ID specified matches the thread ID we saved
 * for a worker.
 *
 * @param g Pointer to the global state.  Used to validate the index.
 * @param i Index for the worker.
 * @param thread_id Thread ID to be checked.
 */
COMMON_SYSDEP
int __cilkrts_sysdep_is_worker_thread_id(global_state_t *g,
                                         int i,
                                         cilkos_thread_id_t thread_id);

/**
 * Function to be run for each of the system worker threads.
 * This declaration also appears in cilk/cilk_undocumented.h -- don't change
 * one declaration without also changing the other.
 *
 * @param arg The context value passed to the thread creation routine for
 * the OS we're running on.
 *
 * @returns OS dependent.
 */
#ifdef _WIN32
/* Do not use CILK_API because __cilkrts_worker_stub must be __stdcall */
CILK_EXPORT unsigned __CILKRTS_NOTHROW __stdcall
__cilkrts_worker_stub(void *arg);
#else
/* Do not use CILK_API because __cilkrts_worker_stub have defauld visibility */
__attribute__((visibility("default")))
void* __CILKRTS_NOTHROW __cilkrts_worker_stub(void *arg);
#endif

/**
 * Initialize any OS-depenendent portions of a newly created
 * __cilkrts_worker.
 *
 * Exported for Piersol.  Without the export, Piersol doesn't display
 * useful information in the stack trace.  This declaration also appears in
 * cilk/cilk_undocumented.h -- do not modify one without modifying the other.
 *
 * @param w The worker being initialized.
 */
COMMON_SYSDEP
CILK_EXPORT
void __cilkrts_init_worker_sysdep(__cilkrts_worker *w);

/**
 * Deallocate any OS-depenendent portions of a __cilkrts_worker.
 *
 * @param w The worker being deallocaed.
 */
COMMON_SYSDEP
void __cilkrts_destroy_worker_sysdep(__cilkrts_worker *w);

/**
 * Called to do any OS-dependent setup before starting execution on a
 * frame. Mostly deals with exception handling data.
 *
 * @param w The worker the frame will run on.
 * @param ff The full_frame that is about to be resumed.
 */
COMMON_SYSDEP
void __cilkrts_setup_for_execution_sysdep(__cilkrts_worker *w,
                                          full_frame *ff);

/****************************************************************************
 * TBB interop functions
 * **************************************************************************/

/**
 * Set the TBB callback information for a stack
 *
 * @param sd The stack to set the TBB callback information for
 * @param o The TBB callback thunk.  Specifies the callback address and
 * context value.
 */
COMMON_SYSDEP
void __cilkrts_set_stack_op(__cilkrts_stack *sd,
                            __cilk_tbb_stack_op_thunk o);

/**
 * Call TBB to tell it about an "interesting" occurrance
 *
 * @param w The worker the stack is running on
 * @param op Value specifying the "interesting" occurrance
 * @param sd The stack TBB is being notified about
 */
COMMON_SYSDEP

void __cilkrts_invoke_stack_op(__cilkrts_worker *w,
                               enum __cilk_tbb_stack_op op,
                               __cilkrts_stack *sd);

/**
 * Save TBB the TBB callback address and context value in thread-local
 * storage.  We'll use it later when the thread binds to a worker.
 *
 * @param o The TBB callback thunk which is to be saved.
 */
COMMON_SYSDEP
void tbb_interop_save_stack_op_info(__cilk_tbb_stack_op_thunk o);

/**
 * Called when we bind a thread to the runtime.  If there is any TBB
 * interop information in thread-local storage, bind it to the
 * stack now.
 *
 * @param w The worker that has been bound to the thread.
 * @param sd The stack that should take over the TBB interop information.
 */
COMMON_SYSDEP
void tbb_interop_use_saved_stack_op_info(__cilkrts_worker *w,
                                         __cilkrts_stack *sd);

/**
 * Free any TBB interop information saved in thread-local storage
 */
COMMON_SYSDEP
void tbb_interop_free_stack_op_info(void);

/**
 * Migrate any TBB interop information from a __cilkrts_stack to
 * thread-local storage.  Returns immediately if no TBB interop information
 * has been associated with the stack.
 *
 * @param sd The __cilkrts_stack who's TBB interop information should be
 * saved in thread-local storage.
 */
COMMON_SYSDEP
void tbb_interop_save_info_from_stack(__cilkrts_stack *sd);


__CILKRTS_END_EXTERN_C

#endif // ! defined(INCLUDED_SYSDEP_DOT_H)

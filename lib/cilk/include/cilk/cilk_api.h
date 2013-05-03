/*
 *  @copyright
 *  Copyright (C) 2009-2011, Intel Corporation
 *  All rights reserved.
 *  
 *  @copyright
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
 *  @copyright
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
 */

/**
 * @if public_doc
 * @mainpage
 * @section intro_sec Introduction
 *
 * In addition to the Cilk Plus keywords, Intel Cilk Plus provides an API to
 * allow users to query and control the Intel Cilk Plus runtime.
 * @endif
 */

/**
 * @file cilk_api.h
 *
 * @brief Defines the documented API exposed by the Intel Cilk Plus for use
 * by applications.
 */

/**
 * @page API
 * Cilk API -- Functions callable by the user to modify the operation of the
 * Cilk scheduler.
 */

#ifndef INCLUDED_CILK_API_H
#define INCLUDED_CILK_API_H

#ifndef CILK_STUB /* Real (non-stub) definitions below */

#if ! defined(__cilk) && ! defined(USE_CILK_API)
#   ifdef _WIN32
#       error Cilk ABI is being used with non-Cilk compiler (or Cilk is disabled)
#   else
#       warning Cilk ABI is being used with non-Cilk compiler (or Cilk is disabled)
#   endif
#endif

#include <cilk/common.h>

#ifdef __cplusplus
#   include <cstddef>  /* Defines size_t */
#else
#   include <stddef.h> /* Defines size_t */
#endif

#ifdef _WIN32
#   ifndef IN_CILK_RUNTIME
/* Ensure the library is brought if any of these functions are being called. */
#       pragma comment(lib, "cilkrts")
#   endif

#   ifndef __cplusplus
#       include <wchar.h>
#   endif
#endif /* _WIN32 */

__CILKRTS_BEGIN_EXTERN_C

/** @brief Return values from __cilkrts_set_param() and __cilkrts_set_param_w() */
enum {
    __CILKRTS_SET_PARAM_SUCCESS = 0, /**< Success - parameter set */
    __CILKRTS_SET_PARAM_UNIMP   = 1, /**< Unimplemented parameter */
    __CILKRTS_SET_PARAM_XRANGE  = 2, /**< Parameter value out of range */
    __CILKRTS_SET_PARAM_INVALID = 3, /**< Invalid parameter value */
    __CILKRTS_SET_PARAM_LATE    = 4  /**< Too late to change parameter value */
};

/**
 * @brief Set user controllable parameters
 *
 * @param param - string specifying parameter to be set
 * @param value - string specifying new value
 * @returns One of: __CILKRTS_SET_PARAM_SUCCESS ( = 0),
 *    __CILKRTS_SET_PARAM_UNIMP, __CILKRTS_SET_PARAM_XRANGE,
 *    __CILKRTS_SET_PARAM_INVALID, or __CILKRTS_SET_PARAM_LATE.
 *
 * @attention The wide character function __cilkrts_set_param_w() is available
 * only on Windows.
 *
 * Allowable parameter names:
 *
 * - "nworkers" - number of cores that should run Cilk code. The value is a
 *   string of digits to be parsed by strtol. Negative numbers are not valid
 *   for "nworkers". 
 *
 *   The precedence for "nworkers" is:
 *   1) __cilkrts_set_param("nworkers")
 *   2) The CILK_NWORKERS environment variable
 *   3) The number of cores returned by the OS.
 *
 *   Setting "nworkers" to "0" sets the number of workers to the value of
 *   CILK_NWORKERS environment number or the number of cores returned by the
 *   OS.
 *
 *   "nworkers" can only be set *before* the runtime has started.  Attempting
 *   to set "nworkers" when the runtime is running will return an error code.
 *   You can use __cilkrts_end_cilk() to shut down the runtime to change the 
 *   number of workers.
 *
 * - "force reduce" - test reducer callbacks by allocating new views
 *   for every spawn within which a reducer is accessed.  This can
 *   significantly reduce performance.  The value is "1" or "true"
 *   to enable, "0" or "false" to disable.
 */
CILK_API(int) __cilkrts_set_param(const char *param, const char *value);

#ifdef _WIN32
/**
 * Set user controllable parameters using wide strings
 *
 * @copydetails __cilkrts_set_param
 */
CILK_API(int) __cilkrts_set_param_w(const wchar_t *param, const wchar_t *value);
#endif

/**
 * Shut down and deallocate all Cilk state.  The runtime will abort the
 * application if Cilk is still in use by this thread.  Otherwise the runtime
 * will wait for all other threads using Cilk to exit.
 */
CILK_API(void) __cilkrts_end_cilk(void);

/**
 * Allocate Cilk data structures, starting the runtime.
 */
CILK_API(void) __cilkrts_init(void);

/**
 * Return the number of worker threads that this instance of Cilk
 * will attempt to use.
 */
CILK_API(int) __cilkrts_get_nworkers(void);

/**
 *Return the number of worker threads allocated.
 */
CILK_API(int) __cilkrts_get_total_workers(void);

/**
 * Return a small integer indicating which Cilk worker the function is
 * currently running on.  Each thread started by the Cilk runtime library
 * (referred to as a system worker) has a unique worker number in the range
 * 1..P-1, where P is the value returned by __cilkrts_get_nworkers().
 *
 * Note that all threads started by the user or by other libraries (referred
 * to as user workers) share the worker number 0. Therefore, the worker number
 * is not unique across multiple user threads.
 */
CILK_API(int) __cilkrts_get_worker_number(void);

/**
 * Return non-zero if force reduce mode is on
 */
CILK_API(int) __cilkrts_get_force_reduce(void);

/**
 * Interact with tools
 */
CILK_API(void)
    __cilkrts_metacall(unsigned int tool, unsigned int code, void *data);

#ifdef _WIN32
typedef struct _EXCEPTION_RECORD _EXCEPTION_RECORD;

/** Callback function signature for Windows exception notification */
typedef void (*__cilkrts_pfn_seh_callback)(const _EXCEPTION_RECORD *exception);

/**
 * Debugging aid for exceptions on Windows.
 *
 * The specified function will be called when a non-C++ exception is caught
 * by the Cilk Plus runtime.  This is illegal since there's no way for the
 * Cilk Plus runtime to know how to unwind the stack across a strand boundary
 * for Structure Exceptions.
 *
 * This function allows an application to do something before the Cilk Plus
 * runtime aborts the application.
 */
CILK_API(int) __cilkrts_set_seh_callback(__cilkrts_pfn_seh_callback pfn);
#endif /* _WIN32 */

#if __CILKRTS_ABI_VERSION >= 1
/* Pedigree API is available only for compilers that use ABI version >= 1. */

/**
 * Pedigree API
 */

/* Internal implementation of __cilkrts_get_pedigree */
CILK_API(__cilkrts_pedigree)
__cilkrts_get_pedigree_internal(__cilkrts_worker *w);

/**
 * @brief Returns the current pedigree, in a linked list representation.
 *
 * This routine returns a copy of the last node in the pedigree list.
 * For example, if the current pedigree (in order) is <1, 2, 3, 4>,
 * then this method returns a node with rank == 4, and whose parent
 * field points to the node with rank of 3.  In summary, following the
 * nodes in the chain visits the terms of the pedigree in reverse.
 * 
 * The returned node is guaranteed to be valid only until the caller
 * of this routine has returned.
 */
__CILKRTS_INLINE
__cilkrts_pedigree __cilkrts_get_pedigree(void) 
{
    return __cilkrts_get_pedigree_internal(__cilkrts_get_tls_worker());    
}

/**
 * @brief DEPRECATED -- Context used by __cilkrts_get_pedigree_info.
 *
 * Callers should initialize the
 * data array to NULL, and set the size to sizeof(__cilkrts_pedigree_context_t
 * before the first call to __cilkrts_get_pedigree_info and should not examine
 * or modify it after.
 */
typedef struct
{
    __STDNS size_t size;    /**< Size of the struct in bytes */
    void *data[3];          /**< Opaque context data */
} __cilkrts_pedigree_context_t;

/**
 * @brief DEPRECATED -- Use __cilkrts_get_pedigree instead.
 *
 * This routine allows code to walk up the stack of Cilk frames to gather
 * the pedigree.
 *
 * Initialize the pedigree walk by filling the pedigree context with NULLs
 * and setting the size field to sizeof(__cilkrts_pedigree_context).
 * Other than initialization to NULL to start the walk, user coder should
 * consider the pedigree context data opaque and should not examine or
 * modify it.
 *
 * @returns  0 - Success - birthrank is valid
 * @returns >0 - End of pedigree walk
 * @returns -1 - Failure - No worker bound to thread
 * @returns -2 - Failure - Sanity check failed,
 * @returns -3 - Failure - Invalid context size
 * @returns -4 - Failure - Internal error - walked off end of chain of frames
 */
CILK_API(int)
__cilkrts_get_pedigree_info(/* In/Out */ __cilkrts_pedigree_context_t *context,
                            /* Out */    uint64_t *sf_birthrank);

/**
 * @brief DEPRECATED -- Use __cilkrts_get_pedigree().rank instead.
 *
 * Fetch the rank from the currently executing worker
 *
 * @returns  0 - Success - *rank is valid
 * @returns <0 - Failure - *rank is not changed
 */
CILK_EXPORT_AND_INLINE
int __cilkrts_get_worker_rank(uint64_t *rank) 
{
    *rank = __cilkrts_get_pedigree().rank;
    return 0;
}

/**
 * @brief Internal implementation of __cilkrts_bump_worker_rank
 */
CILK_API(int)
__cilkrts_bump_worker_rank_internal(__cilkrts_worker* w);

/**
 * @brief Increment the pedigree rank of the currently executing worker
 *
 * @returns 0 - Success - rank was incremented
 * @returns-1 - Failure
 */
CILK_EXPORT_AND_INLINE
int __cilkrts_bump_worker_rank(void)
{
    return __cilkrts_bump_worker_rank_internal(__cilkrts_get_tls_worker());
}

/**
 * @brief Internal implementation of __cilkrts_bump_worker_rank
 */
CILK_API(int)
__cilkrts_bump_loop_rank_internal(__cilkrts_worker* w);

/**
 * @brief Increment the pedigree rank for a cilk_for loop.
 *
 * A cilk_for loop is implemented using a divide and conquer recursive
 * algorithm.  This allows the work of the cilk_for loop to spread optimally
 * across the available workers.  Unfortunately, this makes the pedigree
 * for dependent on the grainsize.  Unless overridden by the cilk grainsize
 * pragma, the grainsize is based on number of workers and the number of
 * iterations in the loop.
 *
 * To fix this, the pedigree is "flattened" in a cilk_for.  A pedigree node is
 * created for the loop index, and a second node is created for the loop body.
 * The compiler generates a lambda function from the loop body that is passed
 * the low and high bounds of the loop indicies it should iterate over.  This
 * range is the "grain size". When the loop body lambda function is called,
 * the pedigree rank of the loop index node is initialized to the lower loop
 * index.
 *
 * Eventually, the compiler generated loop body lambda function should
 * increment the cilk_for rank at the end of each iteration around the
 * cilk_for loop body. However, this is not currently implemented.
 *
 * This function is provided to allow users to increment the cilk_for rank
 * themselves.  Users should call this function only at the end of a cilk_for
 * loop body.  Use of this function is not required.  If not used, the
 * pedigree sequence will change any time the loop's grainsize changes, i.e.,
 * if the program is run with a different number of workers.
 *
 * When the code generated by the compiler for the cilk_for loop body
 * "does the right thing" this function will become a noop.
 *
 * @returns  0 - Success - rank was incremented
 * @returns -1 - Failure
 */
CILK_EXPORT_AND_INLINE
int __cilkrts_bump_loop_rank(void) 
{
    return __cilkrts_bump_loop_rank_internal(__cilkrts_get_tls_worker()); 
}

#endif /* __CILKRTS_ABI_VERSION >= 1 */

__CILKRTS_END_EXTERN_C

#else /* CILK_STUB */

/* Stubs for the api functions */
#ifdef _WIN32
#define __cilkrts_set_param_w(name,value) ((value), 0)
#define __cilkrts_set_seh_callback(pfn) (0)
#endif
#define __cilkrts_set_param(name,value) ((value), 0)
#define __cilkrts_end_cilk() ((void) 0)
#define __cilkrts_init() ((void) 0)
#define __cilkrts_get_nworkers() (1)
#define __cilkrts_get_total_workers() (1)
#define __cilkrts_get_worker_number() (0)
#define __cilkrts_get_force_reduce() (0)
#define __cilkrts_metacall(tool,code,data) ((tool), (code), (data), 0)

#if __CILKRTS_ABI_VERSION >= 1
/* Pedigree stubs */
#define __cilkrts_get_pedigree_info(context, sf_birthrank) (-1)
#define __cilkrts_get_worker_rank(rank) (*(rank) = 0)
#define __cilkrts_bump_worker_rank() (-1)
#define __cilkrts_bump_loop_rank() (-1)

/**
 * A stub method for __cilkrts_get_pedigree.
 * Returns an empty __cilkrts_pedigree. 
 */ 
__CILKRTS_INLINE
__cilkrts_pedigree __cilkrts_get_pedigree_stub(void)
{
    __cilkrts_pedigree ans;
    ans.rank = 0;
    ans.parent = NULL;
    return ans;
}

/* Renamed to an actual stub method. */
#define __cilkrts_get_pedigree() __cilkrts_get_pedigree_stub()

#endif /* __CILKRTS_ABI_VERSION >= 1 */

#endif /* CILK_STUB */

#endif /* INCLUDED_CILK_API_H */

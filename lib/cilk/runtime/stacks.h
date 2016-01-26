/* stacks.h                  -*-C++-*-
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
 * @file stacks.h
 *
 * @brief Cilk uses multiple stacks to implement work-stealing.
 * __cilkrts_stack is the OS-dependent representation of a stack.
 */

#ifndef INCLUDED_STACKS_DOT_H
#define INCLUDED_STACKS_DOT_H

#include <cilk/common.h>
#include "rts-common.h"
#include "global_state.h"

__CILKRTS_BEGIN_EXTERN_C

/**
 * Release a stack back to the cache for this worker.  If the cache is full,
 * this may in turn release the stack to the global cache, or free the memory.
 *
 * @param w __cilkrts_worker that is releasing the stack.
 * @param sd __cilkrts_stack that is to be released.
 */

COMMON_PORTABLE
void __cilkrts_release_stack(__cilkrts_worker *w,
                             __cilkrts_stack *sd);

/**
 * Initialize a __cilkrts_stack_cache structure.
 *
 * @param w __cilkrts_worker that the __cilkrts_stack_cache is to be created
 * for.
 * @param c __cilkrts_stack_cache that is to be initialized.
 * @param size Maximum number of stacks to allow in the cache.
 */
COMMON_PORTABLE
void __cilkrts_init_stack_cache(__cilkrts_worker *w,
                                __cilkrts_stack_cache *c,
                                unsigned int size);

/**
 * Free any cached stacks and the free any memory allocated by the
 * __cilkrts_stack_cache.
 *
 * @param w __cilkrts_worker the __cilkrts_stack_cache was created for.
 * @param g Pointer to the global_state_t.  Used when profiling to count number
 * of stacks outstanding.
 * @param c __cilkrts_stack_cache to be deallocated
 */
COMMON_PORTABLE
void __cilkrts_destroy_stack_cache(__cilkrts_worker *w,
                                   global_state_t *g,
                                   __cilkrts_stack_cache *c);

/**
 * Free all but one local stack, returning stacks to the global pool if
 * possible.
 *
 * @param w __cilkrts_worker holding the stack cache to be trimmed.
 */
COMMON_PORTABLE
void __cilkrts_trim_stack_cache(__cilkrts_worker *w);

/**
 * Allocate a stack, pulling one out of the worker's cache if possible.  If
 * no stacks are available in the local cache, attempt to allocate one from
 * the global cache (requires locking).  If a stack still hasn't be acquired,
 * attempt to create a new one.
 *
 * @return Allocated __cilkrts_stack.
 * @return NULL if a __cilkrts_stack cannot be created, either because we
 * ran out of memory or we hit the limit on the number of stacks that may be
 * created.
 */
COMMON_PORTABLE
__cilkrts_stack *__cilkrts_get_stack(__cilkrts_worker *w);

__CILKRTS_END_EXTERN_C

#endif // ! defined(INCLUDED_STACKS_DOT_H)

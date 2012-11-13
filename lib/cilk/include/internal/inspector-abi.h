/*
 * Copyright (C) 2011 , Intel Corporation
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
 */

#ifndef CILK_INTERNAL_INSPECTOR_ABI_H
#define CILK_INTERNAL_INSPECTOR_ABI_H

#ifdef __cplusplus
#define CILK_INSPECTOR_EXTERN extern "C"
#else
#define CILK_INSPECTOR_EXTERN extern
#endif

#ifdef _WIN32
#ifdef IN_CILK_RUNTIME
#define CILK_INSPECTOR_ABI(TYPE) CILK_INSPECTOR_EXTERN __declspec(dllexport) TYPE __cdecl
#else   // ! IN_CILK_RUNTIME
#define CILK_INSPECTOR_ABI(TYPE) CILK_INSPECTOR_EXTERN __declspec(dllimport) TYPE __cdecl
#endif  // IN_CILK_RUNTIME
#else   // ! _WIN32
#define CILK_INSPECTOR_ABI(TYPE) CILK_INSPECTOR_EXTERN TYPE
#endif // _WIN32

/*
 * inspector_abi.h
 *
 * ABI for functions provided for the Piersol release of Inspector
 */

#ifdef _WIN32
typedef unsigned __cilkrts_thread_id;   // OS-specific thread ID
#else
typedef void *__cilkrts_thread_id;   // really a pointer to a pthread_t
#endif
typedef void * __cilkrts_region_id;

typedef struct __cilkrts_region_properties
{
    unsigned size;              // struct size as sanity check & upward compatibility – must be set before call
    __cilkrts_region_id parent;
    void *stack_base;
    void *stack_limit;
} __cilkrts_region_properties;

/*
 * __cilkrts_get_stack_region_id
 *
 * Returns a __cilkrts_region_id for the stack currently executing on a thread.
 * Returns NULL on failure.
 */

CILK_INSPECTOR_ABI(__cilkrts_region_id)
__cilkrts_get_stack_region_id(__cilkrts_thread_id thread_id);

/*
 * __cilkrts_get_stack_region_properties
 *
 * Fills in the properties for a region_id.
 *
 * Returns false on invalid region_id or improperly sized __cilkrts_region_properties
 */

CILK_INSPECTOR_ABI(int)
__cilkrts_get_stack_region_properties(__cilkrts_region_id region_id,
                                      __cilkrts_region_properties *properties);

#endif  // CILK_INTERNAL_INSPECTOR_ABI_H

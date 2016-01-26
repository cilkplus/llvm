/* os-windows.c                  -*-C-*-
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

#include "os.h"
#include "bug.h"
#include "cilk_malloc.h"
#include "cilktools/cilkscreen.h"
#include "windows-clean.h"
#include <internal/abi.h>
#include <stdio.h>
#include <stdarg.h>

// To declare an interlocked function for use as an intrinsic,
// include intrin.h and put the function in a #pragma intrinsic 
// statement.
#include <intrin.h>
#pragma intrinsic(_InterlockedIncrement)
#pragma intrinsic(_InterlockedExchangeAdd)

static unsigned worker_tls_index        = -1;
static unsigned pedigree_leaf_tls_index = -1;
static unsigned tbb_interop_tls_index   = -1;

#if SUPPORT_GET_CURRENT_FIBER > 0
static unsigned fiber_tls_index         = -1;
#endif

static int tls_index_allocated = 0;

/* Process group callbacks */
typedef DWORD (__stdcall * PFN_GET_ACTIVE_PROCESSOR_COUNT)(
    __in WORD GroupNumber);
typedef WORD (__stdcall * PFN_GET_ACTIVE_PROCESSOR_GROUP_COUNT)(VOID);
typedef BOOL (__stdcall * PFN_SET_THREAD_GROUP_AFFINITY)(
    __in HANDLE hThread,
    __in CONST GROUP_AFFINITY *GroupAffinity,
    __out_opt PGROUP_AFFINITY PreviousGroupAffinity);

static PFN_GET_ACTIVE_PROCESSOR_COUNT s_pfnGetActiveProcessorCount = NULL;
static PFN_GET_ACTIVE_PROCESSOR_GROUP_COUNT
    s_pfnGetActiveProcessorGroupCount = NULL;
static PFN_SET_THREAD_GROUP_AFFINITY s_pfnSetThreadGroupAffinity = NULL;


// /* Thread-local storage */
// #ifdef _WIN32
// typedef unsigned cilkos_tls_key_t;
// #else
// typedef pthread_key_t cilkos_tls_key_t;
// #endif
// cilkos_tls_key_t cilkos_allocate_tls_key();
// void cilkos_set_tls_pointer(cilkos_tls_key_t key, void* ptr);
// void* cilkos_get_tls_pointer(cilkos_tls_key_t key);

COMMON_SYSDEP void __cilkrts_init_tls_variables()
{
    if (tls_index_allocated)
        return;

    worker_tls_index        = TlsAlloc();
    pedigree_leaf_tls_index = TlsAlloc();

#if SUPPORT_GET_CURRENT_FIBER > 0
    fiber_tls_index         = TlsAlloc();
#endif
    tls_index_allocated = 1;
}


// Cleanup TLS variables when thread dies.
void __cilkrts_per_thread_tls_cleanup(void) 
{
    __cilkrts_pedigree* pedigree_tls;
    pedigree_tls = __cilkrts_get_tls_pedigree_leaf(0);
    if (pedigree_tls) {
        // Assert that we have either one or two nodes
        // left in the pedigree chain.
        // If we have more, then something is going wrong...
        CILK_ASSERT(!pedigree_tls->parent || !pedigree_tls->parent->parent);
        __cilkrts_free(pedigree_tls);
    }
}


CILK_ABI_WORKER_PTR __cilkrts_get_tls_worker(void)
{
    __cilkrts_worker *w;

    __cilkscreen_disable_checking();

#ifdef _DEBUG
    CILK_ASSERT(-1 != worker_tls_index);
#endif

    w = (__cilkrts_worker *)TlsGetValue(worker_tls_index);

    __cilkscreen_enable_checking();

    return w;
}

CILK_ABI_WORKER_PTR __cilkrts_get_tls_worker_fast(void)
{
    __cilkrts_worker *w;
    __cilkscreen_disable_checking();
    w = (__cilkrts_worker *)TlsGetValue(worker_tls_index);
    __cilkscreen_enable_checking();

    return w;
}

COMMON_SYSDEP
__cilk_tbb_stack_op_thunk *__cilkrts_get_tls_tbb_interop(void)
{
    if (-1 == tbb_interop_tls_index) {
        // No TLS initialized yet.  Initialize with a NULL value.
        tbb_interop_tls_index = TlsAlloc();
        TlsSetValue(tbb_interop_tls_index, NULL);
        return NULL;
    }

    // Otherwise, just look it up from TLS.
    return (__cilk_tbb_stack_op_thunk*) TlsGetValue(tbb_interop_tls_index);
}

// This counter should be updated atomically.
static long __cilkrts_global_pedigree_tls_counter = -1;

COMMON_SYSDEP __cilkrts_pedigree*
__cilkrts_get_tls_pedigree_leaf(int create_new)
{
#ifdef _DEBUG
    CILK_ASSERT(-1 != pedigree_leaf_tls_index);
#endif
    __cilkrts_pedigree* pedigree_tls =
        (__cilkrts_pedigree *)TlsGetValue(pedigree_leaf_tls_index);

    if (!pedigree_tls && create_new) {
        // This call creates two nodes, X and Y.
        // X == pedigree_tls[0] is the leaf node, which gets copied
        // in and out of a user worker w when w binds and unbinds.
        // Y == pedigree_tls[1] is the root node,
        // which is a constant node that represents the user worker
        // thread w.
        //
        // This call sets the TLS pointer to the new node as well.

        pedigree_tls = (__cilkrts_pedigree*)
            __cilkrts_malloc(2 * sizeof(__cilkrts_pedigree));
        __cilkrts_set_tls_pedigree_leaf(pedigree_tls);

        pedigree_tls[0].rank = 0;
        pedigree_tls[0].parent = &pedigree_tls[1];        
        
        // Create Y, whose rank begins as the global counter value.
        pedigree_tls[1].rank =
            _InterlockedIncrement(&__cilkrts_global_pedigree_tls_counter);

        pedigree_tls[1].parent = NULL;
        CILK_ASSERT(pedigree_tls[1].rank != -1);
    }
    return pedigree_tls;
}

#if SUPPORT_GET_CURRENT_FIBER > 0
COMMON_SYSDEP
cilk_fiber_sysdep* cilkos_get_tls_cilk_fiber(void)
{
    cilk_fiber_sysdep* fiber;
    __cilkscreen_disable_checking();
    fiber = (cilk_fiber_sysdep *)TlsGetValue(fiber_tls_index);
    __cilkscreen_enable_checking();

    return fiber;
}
#endif

COMMON_SYSDEP void __cilkrts_set_tls_worker(__cilkrts_worker *w)
{
    TlsSetValue(worker_tls_index, w);
}

COMMON_SYSDEP
void __cilkrts_set_tls_tbb_interop(__cilk_tbb_stack_op_thunk *t)
{
    if (-1 == tbb_interop_tls_index) {
        // No TLS initialized yet.
        // Allocate memory for the thunk, and store it into TLS.
        tbb_interop_tls_index = TlsAlloc();
    }
    TlsSetValue(tbb_interop_tls_index, t);
}

COMMON_SYSDEP void __cilkrts_set_tls_pedigree_leaf(__cilkrts_pedigree *pedigree_leaf)
{
    TlsSetValue(pedigree_leaf_tls_index, pedigree_leaf);
}

#if SUPPORT_GET_CURRENT_FIBER > 0
COMMON_SYSDEP
void cilkos_set_tls_cilk_fiber(cilk_fiber_sysdep* fiber)
{
    TlsSetValue(fiber_tls_index, fiber);
}
#endif

COMMON_SYSDEP
void* cilkos_get_current_thread_id(void)
{
    return (void*)(size_t)GetCurrentThreadId();
}

/*
 * __cilkrts_hardware_cpu_count
 *
 * Return the number of cores available on this system.
 */
COMMON_SYSDEP int __cilkrts_hardware_cpu_count(void)
{
    static int active_processors = 0;
    SYSTEM_INFO info;

    // If we've already done this, just return the value we calculated last
    // time.  It's not going to change...
    if (active_processors > 0)
        return active_processors;

    // If we've got a function to count up all the processors across all the
    // processor groups, use it.  It will return 0 if it fails.
    win_init_processor_groups();
    if (NULL != s_pfnGetActiveProcessorCount)
    {
        active_processors =
            s_pfnGetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
        if (active_processors > 0)
            return active_processors;
    }

    // Use the old function to ask Windows for the number of processors in the
    // system.  If this is an older OS, then there's no concept of processor
    // groups, and this is the total number of processors.  If this OS supports
    // >64 processors, and GetActiveProcessorCount returned an error, returning
    // the number of processors in the group this thread is executing on is the
    // best we can do
    info.dwNumberOfProcessors = 0;
    GetSystemInfo (&info);

    active_processors = info.dwNumberOfProcessors;

    return active_processors;
}

COMMON_SYSDEP unsigned long long __cilkrts_getticks(void)
{
    return __rdtsc();
}

#if defined _M_IX86  // Intel x86 32-bit
#elif defined _M_X64   // Intel x86 64-bit
#else
#   error Unsupported processor
#endif

/* Machine instructions */
COMMON_SYSDEP void __cilkrts_short_pause(void)
{
    __asm
    {
        pause
    }
}

COMMON_SYSDEP int __cilkrts_xchg(volatile int *ptr, int x)
{
    return _InterlockedExchange((void*) ptr, x);
}

COMMON_SYSDEP void __cilkrts_fence(void)
{
    __asm
    {
        mfence
    }
}

COMMON_SYSDEP void __cilkrts_idle(void)
{
    Sleep(0);
}

COMMON_SYSDEP void __cilkrts_sleep(void)
{
    Sleep(1);   // 1 millisecond
}

COMMON_SYSDEP void __cilkrts_yield(void)
{
    Sleep(0);
}

COMMON_SYSDEP __STDNS size_t cilkos_getenv(char* value, __STDNS size_t vallen,
                                           const char* varname)
{
    // GetEnvironmentVariableA is prefered over getenv or getenv_s because it
    // does not cache the value.  Because we are using a statically-linked C
    // runtime library, we have a private cache and would not see changes to
    // environment strings made by the main program.  GetEnvironmentVariableA
    // ignores this private cache and uses the underlying OS implementation of
    // environment strings.
    return GetEnvironmentVariableA(varname, value, (DWORD) vallen);
}

#define ERR_MESSAGE_LEN 1024

COMMON_SYSDEP void cilkos_error(const char *fmt, ...)
{
    char message[ERR_MESSAGE_LEN];
    va_list l;

    // Build the message text from any parameters passed
    va_start(l, fmt);
    _vsnprintf_s(message, ERR_MESSAGE_LEN, _TRUNCATE, fmt, l);
    va_end(l);

    // Windows applications do not have to have stderr connected to anything,
    // so write to *both* stderr and the debugger, and hope that someone will
    // see the message
    fprintf(stderr, "Cilk error: %s\nExiting.\n", message);

    OutputDebugStringA("Cilk error: ");
    OutputDebugStringA(message);
    OutputDebugStringA("\nExiting.\n");

    // If there's a debugger attached, give it a chance to look at the failure
    if (IsDebuggerPresent())
        DebugBreak();

    _exit(3);
}

COMMON_SYSDEP void cilkos_warning(const char *fmt, ...)
{
    char message[ERR_MESSAGE_LEN];
    va_list l;

    // Build the message text from any parameters passed
    va_start(l, fmt);
    _vsnprintf_s(message, ERR_MESSAGE_LEN, _TRUNCATE, fmt, l);
    va_end(l);

    // Windows applications do not have to have stderr connected to anything,
    // so write to *both* stderr and the debugger, and hope that someone will
    // see the message
    fprintf(stderr, "Cilk warning: %s", message);

    OutputDebugStringA("Cilk warning: ");
    OutputDebugStringA(message);
}

void win_init_processor_groups(void)
{
    HMODULE hKernel32;
    static int initialized = 0;

    // Only do this once
    if (initialized)
        return;
    initialized = 1;

    // Fetch the handle for Kernel32.  This should *always* succeed
    hKernel32 = GetModuleHandle(L"Kernel32.dll");
    if (NULL == hKernel32)
        return;

    // If this OS supports processor groups, the following functions should
    // *ALL* be available
    s_pfnGetActiveProcessorCount = (PFN_GET_ACTIVE_PROCESSOR_COUNT)
        GetProcAddress(hKernel32, "GetActiveProcessorCount");
    s_pfnGetActiveProcessorGroupCount = (PFN_GET_ACTIVE_PROCESSOR_GROUP_COUNT)
        GetProcAddress(hKernel32, "GetActiveProcessorGroupCount");
    s_pfnSetThreadGroupAffinity =
        (PFN_SET_THREAD_GROUP_AFFINITY)GetProcAddress(hKernel32,
                                                      "SetThreadGroupAffinity");

    // If we didn't get any of the functions, assume that none are available
    if ((NULL == s_pfnGetActiveProcessorCount) ||
        (NULL == s_pfnGetActiveProcessorGroupCount) ||
        (NULL == s_pfnSetThreadGroupAffinity))
    {
        s_pfnGetActiveProcessorCount = NULL;
        s_pfnGetActiveProcessorGroupCount = NULL;
        s_pfnSetThreadGroupAffinity = NULL;
    }
}

unsigned long win_get_active_processor_count(unsigned short GroupNumber)
{
    CILK_ASSERT(s_pfnGetActiveProcessorCount);
    return s_pfnGetActiveProcessorCount(GroupNumber);
}

unsigned short win_get_active_processor_group_count(void)
{
    if (NULL == s_pfnGetActiveProcessorGroupCount)
        return 0; // failure -- not implemented for this OS
    else
        return s_pfnGetActiveProcessorGroupCount();
}

int win_set_thread_group_affinity(/*HANDLE*/ void* hThread,
                                  const GROUP_AFFINITY *GroupAffinity,
                                  GROUP_AFFINITY* PreviousGroupAffinity)
{
    CILK_ASSERT(s_pfnSetThreadGroupAffinity);
    return s_pfnSetThreadGroupAffinity(hThread, GroupAffinity,
                                       PreviousGroupAffinity);
}


#define PAGE 4096
#define CILK_MIN_STACK_SIZE (4*PAGE)
#define CILK_DEFAULT_STACK_SIZE 0x100000

/*
 * Convert the user's specified stack size into a "reasonable" value
 * for this OS.
 */
size_t cilkos_validate_stack_size(size_t specified_stack_size) {
    // Convert any negative value to the default.
    if (specified_stack_size == 0) {
        CILK_ASSERT((CILK_DEFAULT_STACK_SIZE % PAGE) == 0);
        return CILK_DEFAULT_STACK_SIZE;
    }
    // Round values in between 0 and CILK_MIN_STACK_SIZE up to
    // CILK_MIN_STACK_SIZE.
    if (specified_stack_size <= CILK_MIN_STACK_SIZE) {
        return CILK_MIN_STACK_SIZE;
    }
    if ((specified_stack_size % PAGE) > 0) {
        // Round the user's stack size value up to nearest page boundary.
        return (PAGE * (1 + specified_stack_size / PAGE));
    }
    return specified_stack_size;
}

long cilkos_atomic_add(long volatile * p, long x)
{
    return _InterlockedExchangeAdd(p, x) + x;
}

/* End os-windows.c */

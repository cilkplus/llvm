/* sysdep-win-dllmain.c                  -*-C-*-
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
#include "os_mutex.h"
#include "bug.h"
#include "windows-clean.h"


#ifdef _WIN64
#include "except-win64.h"

void *handler_cookie = NULL;
extern CRITICAL_SECTION g_csTrampolineList;
#endif  // _WIN64

HMODULE g_hmodCilkrts = NULL;

// Extern declaration for the C runtime's entrypoint.  This is usually set as
// a module's entrypoint.  See the comments in cilkrts_module_startup() for an
// explanation of how and why we may initialize the runtime before getting a
// DLL_PROCESS_ATTACH notification.
extern
BOOL WINAPI
_DllMainCRTStartup(HANDLE  hDllHandle,
                   DWORD   dwReason,
                   LPVOID  lpreserved);

/*
 * DllMain
 *
 * DllMain is an optional entrypoint for a DLL.  It is called when the DLL is
 * first loaded, about to be unloaded, and when threads start and stop.
 */

BOOL WINAPI DllMain (HINSTANCE hinstDLL,  // handle to DLL module
                     DWORD dwReason,      // reason for calling function
                     LPVOID lpvReserved)  // reserved
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            CILK_ASSERT(NULL == g_hmodCilkrts);
            g_hmodCilkrts = (HMODULE)hinstDLL;

            __cilkrts_init_tls_variables();

            // Create the global OS mutex
            global_os_mutex_create();
#ifdef _WIN64
            handler_cookie = AddVectoredExceptionHandler(1, __cilkrts_vectored_handler);
            InitializeCriticalSection (&g_csTrampolineList);
#endif  // _WIN64
            break;

        case DLL_PROCESS_DETACH:
            CILK_ASSERT(NULL != g_hmodCilkrts);

#ifdef _WIN64
            RemoveVectoredExceptionHandler(handler_cookie);
#endif  // _WIN64

            // Destroy the global OS mutex
            global_os_mutex_destroy();


            g_hmodCilkrts = NULL;
            break;
        case DLL_THREAD_DETACH:
            // Cleanup variables in TLS that the thread allocated.
            __cilkrts_per_thread_tls_cleanup();
            break;
    }

    // A FALSE return will cause initalization to fail, which will unload the
    // DLL.  We never want that to happen!

    return TRUE;
}

/*
 * sysdep_init_module
 *
 * If the module has not yet been initialized, do it now.
 *
 * See the comments in cilkrts_module_startup() for an explanation of how and
 * why we may initialize the runtime before getting a DLL_PROCESS_ATTACH
 * notification.
 */
void sysdep_init_module()
{
    HMODULE hmod;

    // If the module has already been initialized, we're done.  g_hmodCilkrts
    // is set when DllMain() is called with the reason code DLL_PROCESS_ATTACH
    // which *should* have been the first call into the module.
    if (NULL != g_hmodCilkrts)
        return;

    // We'll need the module handle to call the entrypoint
    hmod = GetModuleHandle(L"cilkrts20.dll");
    CILK_ASSERT(NULL != hmod);

    // Have the C runtime initilaize itself.  This should in turn call our
    // DllMain
    _DllMainCRTStartup(hmod, DLL_PROCESS_ATTACH, 0);

#ifdef _DEBUG
    // DllMain should have initialized g_hmodCilkrts.  But we can't assume
    // that the compiler hasn't saved it in a register on us in an optimized
    // build
    CILK_ASSERT(NULL != g_hmodCilkrts);
#endif
}

/*
 * cilkrts_module_startup
 *
 * cilkrts_module_startup replaces _DllMainCRTStartup as the entrypoint for
 * the Cilk runtime DLL.  This is done via the /ENTRY linker option, set using
 * the "Entry point" settings on the "Advanced" page of the "Linker" options
 * within Visual Studio.
 *
 * We need to do this because TBB is using GetModuleHandle() to check whether
 * we're in the process and calling us to setup the stack watching code.  When
 * TBB is linked into ARBB, that's happening during ARBB's module
 * initialization, which is *BEFORE* we get our process attach call to DllMain.
 * As a result, when TBB calls __cilkrts_watch_stack(), we're hosed.  So
 * __cilkrts_watch_stack now calls sysdep_init_module() to initialize the
 * runtime if it hasn't happened yet.
 */

BOOL WINAPI cilkrts_module_startup (HMODULE hinstDLL,    // handle to DLL module
                                    DWORD dwReason,      // reason for calling function
                                    LPVOID lpvReserved)  // reserved
{
    // If this is the process attach call, and we've ALREADY initialized the
    // module, just ignore the call and return TRUE to indicate that we've
    // initialized successfully
    if ((DLL_PROCESS_ATTACH == dwReason) && (NULL != g_hmodCilkrts))
        return TRUE;

    // Pass everything else on to the C runtime
    return _DllMainCRTStartup(hinstDLL, dwReason, lpvReserved);
}

/* End sysdep-win-dllmain.c */

/* sysdep-win.c                  -*-C-*-
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

#include "sysdep-win.h"
#include "sysdep.h"
#include "os.h"
#include "bug.h"
#include "local_state.h"
#include "full_frame.h"
#include "cilk_malloc.h"
#include "metacall_impl.h"
#include "signal_node.h"

#pragma warning(push)
#pragma warning(disable: 147)   // declaration is incompatible with definition in winnt.h
#include <intrin.h>
#pragma warning(pop)
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>            // defines _beginthreadex
#include <time.h>
#include "cilk-tbb-interop.h"
#include "jmpbuf.h"
#include "metacall_impl.h"
#include "cilk-ittnotify.h"
#include "internal/cilk_version.h"
#include <cilktools/cilkscreen.h>

// Hack to allow us to disable RML
#define USE_RML 1

#if !defined(_WIN32)
#   error "sysdep-win.c contains Windows-specific code."
#endif

#ifdef _WIN64
#include "except-win64.h"
#else
#include "except-win32.h"
#endif

// MakePtr is a macro that allows you to easily add to values (including
// pointers) together without dealing with C's pointer arithmetic.  It
// essentially treats the last two parameters as DWORDs.  The first
// parameter is used to typecast the result to the appropriate pointer type.
#define MakePtr( cast, ptr, addValue ) (cast)( (DWORD_PTR)(ptr) + (DWORD_PTR)(addValue))


/**
 * System-dependent global state
 *
 * WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! WARNING!
 *
 * This structure is shared with the debugger access module cilk_db.dll
 * which reaches in to get the array of thread IDs.
 * 
 * ANY CHANGES TO THIS STRUCTURE MAY REQUIRE A MATCHING CHANGE IN CILK_DB!
 */
struct global_sysdep_state
{
    /// IDs for system worker threads - accessed by debugger via cilk_db
    unsigned *nThreadIds;

    /// Handles for system worker threads - accessed by debugger via cilk_db
    HANDLE *hThreads;

#ifndef _WIN64
    /// Exception tracking stuff for Win32.  When an exception crosses a steal, a
    /// pointer to our fake exception data is saved for later lookup.  The
    /// pending_exceptions_t struct contains all of that data.
    pending_exceptions_t pending_exceptions;
#endif
};

#ifndef _WIN64
pending_exceptions_t *get_cilk_pending_exceptions ()
{
    return &cilkg_get_global_state()->sysdep->pending_exceptions;
}
#endif


/*************************************************************
  Creation of worker threads:
*************************************************************/

static void internal_run_scheduler_with_exceptions(__cilkrts_worker *w)
{
    __cilkrts_run_scheduler_with_exceptions(w);
}

/**
 * Data expected by the SEH exception that the debugger will recognize to
 * set the name of a thread.
 */
typedef struct
{
    DWORD dwType;      ///< Must be 0x1000
    LPCSTR szName;     ///< Pointer to name (in user addr space)
    DWORD dwThreadID;  ///< Thread ID (-1=caller thread)
    DWORD dwFlags;     ///< Reserved for future use, must be zero
} THREADNAME_INFO;

#define EXCEPT_SET_THREAD_NAME 0x406D1388

/*
 * SetWorkerThreadName
 *
 * Use a Windows debugger hack to name our worker threads
 */

void SetWorkerThreadName (__cilkrts_worker *w, void *fiber)
{
    char szWorkerName[128];
    THREADNAME_INFO info;

    // If this is a user worker, don't play with it's name
    if (WORKER_SYSTEM != w->l->type)
        return;

#if defined(__INTEL_COMPILER) && defined(USE_ITTNOTIFY)
    // Name the threads for Advisor.  They don't want a worker number
    __itt_thread_set_nameA("Cilk Worker");
#endif // defined(__INTEL_COMPILER) && defined(USE_ITTNOTIFY)

    // If the debugger isn't present, there's no reason to try to talk to it
    if (! IsDebuggerPresent())
        return;

#ifdef _DEBUG
    if (NULL == fiber)
        sprintf_s (szWorkerName, 128, "Worker %d-S%p",
                   __cilkrts_get_worker_number(),
                   w->l->scheduling_fiber);
    else
        sprintf_s (szWorkerName, 128, "Worker %d-%p",
                   __cilkrts_get_worker_number(),
                   fiber);
#else
    sprintf_s (szWorkerName, 128, "Worker %d",
               __cilkrts_get_worker_number());
#endif

    // Raise the exception the debugger will recognize to set the thread name.
    // Do this in __try/__except block so we'll catch it if the debugger
    // doesn't
    info.dwType = 0x1000;
    info.szName = szWorkerName;
    info.dwThreadID = (DWORD)-1;
    info.dwFlags = 0;

    __try
    {
        RaiseException (EXCEPT_SET_THREAD_NAME,     // Exception code
                        0,                          // Flags
                        sizeof(info)/sizeof(DWORD), // Number of arguments
                        (DWORD_PTR*)&info);         // Arguments
    }
    __except (EXCEPTION_CONTINUE_EXECUTION)
    {
    }
}


/*
 * scheduler_thread_proc_for_system_worker
 *
 * Thread start function called when we start a new worker.
 */
NON_COMMON unsigned __CILKRTS_NOTHROW __stdcall
scheduler_thread_proc_for_system_worker(void *arg)
{
    __cilkrts_worker *w = (__cilkrts_worker *) arg;
    global_state_t *g = w->g;
    __cilkrts_set_tls_worker(w);

    // Newly created threads inherit the process affinity mask.  Which should
    // be good enough if we don't have multiple processor group, so we don't
    // need to play with the process affinity masks
    //
    // If we've got multiple groups, then we need to make sure that the thread
    // is running on the assigned processor group
    if (-1 != w->sysdep->processor_group)
    {
        // This system may have more than 1 processor group.  Assign the thread
        // to the correct processor group, and specify that it can run on any
        // processor in the group.
        GROUP_AFFINITY affinity;
        memset (&affinity, 0, sizeof(affinity));
        affinity.Mask = 0; // Use any processor in the group
        affinity.Group = w->sysdep->processor_group;

        if (0 == win_set_thread_group_affinity(GetCurrentThread(), &affinity,
                                               NULL))
        {
            DWORD err = GetLastError();
            DBGPRINTF ("SetThreadGroupAffinity failed with error %d. (0x%x)\n",
                       err, err);
            CILK_ASSERT (! "SetThreadGroupAffinity failed");
        }
    }

    // Save worker handle and ID
    g->sysdep->nThreadIds[w->self] = GetCurrentThreadId();

    START_INTERVAL(w, INTERVAL_IN_SCHEDULER);
    START_INTERVAL(w, INTERVAL_IN_RUNTIME);
    START_INTERVAL(w, INTERVAL_INIT_WORKER);

    // Create a cilk fiber to encapsulate the thread stack for this worker.
    START_INTERVAL(w, INTERVAL_FIBER_ALLOCATE_FROM_THREAD) {
        w->l->scheduling_fiber = cilk_fiber_allocate_from_thread();
        cilk_fiber_set_owner(w->l->scheduling_fiber, w);
    } STOP_INTERVAL(w, INTERVAL_FIBER_ALLOCATE_FROM_THREAD);
    
#if FIBER_DEBUG >= 1    
    fprintf(stderr, "w=%d: w->l->scheduling_fiber set to %p\n",
            w->self, w->l->scheduling_fiber);
#endif

    SetWorkerThreadName(w, NULL);
    STOP_INTERVAL(w, INTERVAL_INIT_WORKER);

    internal_run_scheduler_with_exceptions(w);

#if SUPPORT_GET_CURRENT_FIBER    
    // We get here only when cilk is shutting down.
    CILK_ASSERT(cilk_fiber_get_current_fiber() == w->l->scheduling_fiber);
#endif

    START_INTERVAL(w, INTERVAL_FIBER_DEALLOCATE_FROM_THREAD) {
        // Deallocate the scheduling fiber.  This operation reverses the
        // effect cilk_fiber_allocate_from_thread() and must be done in this
        // thread before it exits.
        int ref_count = cilk_fiber_deallocate_from_thread(w->l->scheduling_fiber);
        // Scheduling fibers should never have extra references to them.
        // We only get extra references into fibers because of Windows
        // exceptions.
        w->l->scheduling_fiber = NULL;
    } STOP_INTERVAL(w, INTERVAL_FIBER_DEALLOCATE_FROM_THREAD);

    STOP_INTERVAL(w, INTERVAL_IN_RUNTIME);
    STOP_INTERVAL(w, INTERVAL_IN_SCHEDULER);
    return 0;
}

/*
 * This function is exported so Piersol's stack trace displays
 * reasonable information.
 */
NON_COMMON CILK_EXPORT unsigned __CILKRTS_NOTHROW __stdcall
__cilkrts_worker_stub(void *arg)
{
    return scheduler_thread_proc_for_system_worker(arg);
}


#ifndef VER_SUITE_WH_SERVER
#define VER_SUITE_WH_SERVER 0x00008000
#endif

#ifndef SM_SERVERR2
#define SM_SERVERR2 89
#endif

static
const char *name_windows (OSVERSIONINFOEX *os)
{
    switch (os->dwMajorVersion)
    {
        case 6:
            switch (os->dwMinorVersion)
            {
                case 1: // 6.1
                    if (VER_NT_WORKSTATION == os->wProductType)
                        return "7";
                    else
                        return "Server 2008 R2";

                case 0: // 6.0
                    if (VER_NT_WORKSTATION == os->wProductType)
                        return "Vista";
                    else
                        return "Server 2008";
            }
            break;

        case 5:
            switch (os->dwMinorVersion)
            {
                case 2: // 5.2
                    if (VER_SUITE_WH_SERVER == os->wSuiteMask)
                        return "Home Server";

                    if (0 == GetSystemMetrics(SM_SERVERR2))
                        return "Server 2003";
                    else
                        return "Server 2003 R2";

                case 1: // 5.1
                    return "XP";

                case 0: // 5.0
                    return "Server 2000";
            }
            break;
    }

    return "?";
}

/*
 * write_version_file
 *
 * If the environment variable CILK_VERSION is defined, opens the specified
 * file for write fills it with version information.
 */

static void write_version_file(global_state_t *g, int nWorkers)
{
    wchar_t buf[MAX_PATH];
    wchar_t *mode;
    size_t len;
    FILE *version_file;
    wchar_t szCilkPath[MAX_PATH];
    OSVERSIONINFOEX os_info;
    SYSTEM_INFO sys_info;
    char *architecture;
    time_t t;
    struct tm *timeinfo;
    int close_file = 1;

#ifdef _DEBUG
#define BUILD_TYPE "Debug"
#else
#define BUILD_TYPE "Release"
#endif

    // If the user has set CILK_VERSION, fetch the value
    len = GetEnvironmentVariable(L"CILK_VERSION", buf, sizeof(buf)/sizeof(buf[0]));
    if (0 == len)
        return;

    if (0 == _wcsicmp (buf, L"stdout"))
    {
        close_file = 0;
        version_file = stdout;
    }
    else if (0 == _wcsicmp (buf, L"stderr"))
    {
        close_file = 0;
        version_file = stderr;
    }
    else
    {
        // Try to create the file
        if (0 != _wfopen_s (&version_file, buf, L"wt"))
            return;
    }

    // Windows shouldn't whine about uses of localtime and asctime, which it
    // considers unsafe.
#   pragma warning(push)
#   pragma warning(disable : 1786)

    // Report when the run starts
    time(&t);
    timeinfo = localtime(&t);
    fprintf (version_file, "Cilk runtime initialized: %s", asctime(timeinfo));

#   pragma warning(pop)

    // Report on the Cilk runtime information
    fprintf (version_file, "\nCilk runtime information\n========================\n");
    fprintf (version_file, "Cilk version: %d.%d.%d Build %d, build type: %s\n",
             VERSION_MAJOR,
             VERSION_MINOR,
             VERSION_REV,
             VERSION_BUILD,
             BUILD_TYPE);

#if defined(BUILD_USER) && defined(BUILD_HOST)
    fprintf (version_file, "Built by %s on host %s\n", BUILD_USER, BUILD_HOST);
#endif // defined(BUILD_USER) && defined(BUILD_HOST)
    fprintf (version_file, "Compilation date: "__DATE__" "__TIME__"\n");
#ifdef __INTEL_COMPILER
    fprintf (version_file, "Compiled with ICL V%d.%d.%d, ICL build date: %d\n",
             __INTEL_COMPILER/100, (__INTEL_COMPILER/10)%10, __INTEL_COMPILER%10,
             __INTEL_COMPILER_BUILD_DATE);
#   ifdef USE_ITTNOTIFY
    fprintf (version_file, "ITT_NOTIFY callouts %s active\n",
        (NULL != __itt_sync_prepare_ptr) ? "are" : "are not");
#   endif // USE_ITTNOTIFY
#endif // __INTEL_COMPILER


    // Report on local system information
    fprintf (version_file, "\nSystem information\n==================\n");

    // Fetch the path for the Cilk runtime
    GetModuleFileName(g_hmodCilkrts, szCilkPath, MAX_PATH);
    fprintf (version_file, "Cilk runtime path: %S\n", szCilkPath);

    // Fetch the OS version & system information
    os_info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx((LPOSVERSIONINFO)&os_info);
    GetSystemInfo (&sys_info);

    fprintf (version_file, "System OS: Windows %s, V%d.%d Build %d %s\n",
             name_windows(&os_info),
             os_info.dwMajorVersion, os_info.dwMinorVersion,
             os_info.dwBuildNumber,
             os_info.szCSDVersion);

    switch (sys_info.wProcessorArchitecture)
    {
        case PROCESSOR_ARCHITECTURE_AMD64:  architecture = "Intel64"; break;
        case PROCESSOR_ARCHITECTURE_IA64:   architecture = "Itanium"; break;
        case PROCESSOR_ARCHITECTURE_INTEL:  architecture = "x86"; break;
        default: architecture = "Unknown"; break;
    }
    fprintf (version_file, "System architecture: %s\n", architecture);

    fprintf (version_file, "\nThread information\n==================\n");
    fprintf (version_file, "System cores: %d\n", sys_info.dwNumberOfProcessors);
    fprintf (version_file, "Cilk workers requested: %d\n", nWorkers);

        fprintf (version_file, "Thread creator: Private\n");

    // Only close the file if we're not using either stdout or stderr
    if (close_file)
        fclose(version_file);
}

/*
 * set_worker_processor_groups
 *
 * Assign each worker to a processor group.  Make sure to handle wrapping
 * since it's perfectly legal to have more workers than there are cores
 */

static void set_worker_processor_groups(global_state_t *g)
{
    WORD group;
    WORD available_groups;
    int proc_group_count;
    DWORD cores_in_group;
    int nWorker;

    // Make sure the processor group functionality has been initialized
    win_init_processor_groups();

    // Get the number of active processor groups.  If this fails for some
    // reason, we're done.  The processor group was initialized to -1 in
    // __cilkrts_init_worker_sysdep.
    available_groups = win_get_active_processor_group_count();
    if (0 == available_groups)
        return;

    // Assign each worker to a processor group.
    group = 0;
    cores_in_group = win_get_active_processor_count(group);

    for (nWorker = 0; nWorker < g->P - 1; nWorker++)
    {
        // Note that this worker should be assigned to the group
        g->workers[nWorker]->sysdep->processor_group = group;

        // Decrement the number of available cores in the processor group.  If
        // we've exhausted the number of available cores, move on to the next
        // group and see how many cores there are in that processor group.
        //
        // Make sure to handle wrapping since it's perfectly legal to have
        // more workers than there are cores
        cores_in_group--;
        if (0 == cores_in_group)
        {
            group++;
            if (available_groups == group)
                group = 0;
            cores_in_group = win_get_active_processor_count(group);
        }
    }
}

void __cilkrts_start_workers(global_state_t *g, int n)
{
    int i;
    size_t stack_size;

    // Note that the system workers are running
    g->workers_running = 1;

    // We're just starting, so the work isn't done
    g->work_done = 0;

    // Try to use RML.  If that doesn't work, fall back to creating our own threads
    stack_size = g->stack_size;

    // If we're running under a sequential ptool, then skip starting the
    // workers since there's only 1 and there won't be any stealing
    if (g->under_ptool)
    {
        g->sysdep->hThreads[0] = INVALID_HANDLE_VALUE;
        return;
    }

        // Start the requested number of worker threads
        for (i = 0; i < n; ++i)
        {
            g->sysdep->hThreads[i] =
                (HANDLE)_beginthreadex((void *)NULL,                // security attrib
                                       (unsigned)stack_size,        // stack size
                                       scheduler_thread_proc_for_system_worker,       // start proc
                                       g->workers[i],               // start param
                                       0,                           // creation flags
                                       &g->sysdep->nThreadIds[i]);  // receives thread ID
            if (0 == g->sysdep->hThreads[i])
            {
                int status = errno;
                __cilkrts_bug("Cilk runtime error: thread creation (%d) failed: %d\n", i, status);
            }
        }

    write_version_file(g, n);


    return;
}

void __cilkrts_stop_workers(global_state_t *g)
{
    // Tell the workers to give up
    g->work_done = 1;

    // If we're running under a sequential ptool, then we skipped starting the
    // workers since there's only 1 and there won't be any stealing
    if (g->under_ptool)
        return;

    // Signal all the nodes to resume running.  They'll see g->work_done and
    // exit cleanly
    if (g->P > 1)
    {
        CILK_ASSERT(g->workers[0]->l->signal_node);
        signal_node_msg(g->workers[0]->l->signal_node, 1);
    }

        // Wait for all of the workers to exit, unless the sole worker is a
        // user worker.
        if (g->P > 1)
        {
            DWORD status =
                WaitForMultipleObjects(g->P - 1,             // Number of events in array
                                       g->sysdep->hThreads,  // Array of handles
                                       TRUE,                 // Wait for all
                                       INFINITE);            // Do not timeout
            CILK_ASSERT(WAIT_FAILED != status);
        }

    return;
}

void __cilkrts_init_worker_sysdep(__cilkrts_worker *w)
{
    w->sysdep = (__cilkrts_worker_sysdep_state *)__cilkrts_malloc(sizeof(*w->sysdep));
    w->l->scheduling_fiber = NULL;
#ifdef _WIN64
    w->sysdep->pRethrowExceptionRecord = NULL;
#endif

    ITT_SYNC_CREATE(w, "Scheduler");

#ifdef _WIN64
    w->sysdep->last_stackwalk_rbp = 0;
#endif

    w->sysdep->processor_group = -1;
}

void __cilkrts_destroy_worker_sysdep(__cilkrts_worker *w)
{
    if (w->sysdep)
        __cilkrts_free(w->sysdep);
}

void *GetStackBase(void)
{
    NT_TIB *pTeb = (NT_TIB *)NtCurrentTeb();
    return pTeb->StackBase;
}


#ifndef _WIN64
// exception tracking stuff for Win32.  When an exception crosses a steal, a
// pointer to our fake exception data is saved for later lookup.  The
// pending_exceptions_t struct contains all of that data.
static inline
void initialize_pending_exceptions (struct global_sysdep_state *sysdep) {

#define MPE 4
    sysdep->pending_exceptions.lock = 0;
    sysdep->pending_exceptions.max_pending_exceptions = MPE;
    sysdep->pending_exceptions.entries = (exception_entry_t*)
        __cilkrts_malloc(sizeof(exception_entry_t) * MPE);
    CILK_ASSERT(sysdep->pending_exceptions.entries);
    memset(sysdep->pending_exceptions.entries, 0x0,
       sizeof(exception_entry_t) * MPE);
#undef MPE

}
#endif

COMMON_SYSDEP
void __cilkrts_init_global_sysdep(global_state_t *g)
{
    __cilkrts_init_tls_variables();

    CILK_ASSERT(g->total_workers >= g->P - 1);

    g->sysdep = __cilkrts_malloc(sizeof (struct global_sysdep_state));
    CILK_ASSERT(g->sysdep);

    g->sysdep->hThreads = __cilkrts_malloc(sizeof(HANDLE) * g->total_workers);
    CILK_ASSERT(g->sysdep->hThreads);

    g->sysdep->nThreadIds = __cilkrts_malloc(sizeof(DWORD) * g->total_workers);
    CILK_ASSERT(g->sysdep->hThreads);

#ifndef _WIN64
    initialize_pending_exceptions(g->sysdep);
#endif

    set_worker_processor_groups(g);

    return;
}

COMMON_SYSDEP
void __cilkrts_destroy_global_sysdep(global_state_t *g)
{
    __cilkrts_free(g->sysdep->nThreadIds);
    __cilkrts_free(g->sysdep->hThreads);
    __cilkrts_free(g->sysdep);
}

// return 1 if the pointer is on the current stack, 0 otherwise.
int is_within_stack (char *ptr) {
    NT_TIB *pTeb = (NT_TIB *)NtCurrentTeb();
    if (((char *)pTeb->StackLimit > ptr) || (ptr > (char *)pTeb->StackBase))
    {
        return 0;
    }
    return 1;
}

#ifdef _DEBUG
void verify_within_stack (__cilkrts_worker *w, char *value, char *description)
{
    NT_TIB *pTeb = (NT_TIB *)NtCurrentTeb();

    if (((char *)pTeb->StackLimit > value) || (value > (char *)pTeb->StackBase))
    {
        __cilkrts_bug("%d-%p: %s (%p) not in stack range %p - %p\n",
                      w->self,
                      GetCurrentFiber(),
                      description,
                      value,
                      pTeb->StackBase, pTeb->StackLimit);
    }
}
#endif // _DEBUG

void __cilkrts_make_unrunnable_sysdep(__cilkrts_worker *w,
                                      full_frame *ff,
                                      __cilkrts_stack_frame *sf,
                                      int is_loot,
                                      const char *why)
{
    (void)w; /* unused */

    if (is_loot) {
        ff->frame_size = __cilkrts_get_frame_size(sf);
#ifndef _WIN64
        if (0 == ff->registration) {
            ff->registration = sf->ctx.Registration;
        }
        ff->trylevel = sf->ctx.TryLevel;
#endif

        DBGPRINTF ("%d-%p: __cilkrts_make_unrunnable_sysdep - saving sync stack\n"
                   "            sf: %p, EIP: %p, EBP: %p, ESP: %p, frame size: %d\n",
                   w->self, GetCurrentFiber(),
                   sf, PC(sf), FP(sf), SP(sf), ff->frame_size);

        // Null loot's sp for debugging purposes (so we'll know it's not valid)
        SP(sf) = 0;
    }
}

#ifdef _WIN64
#define USER_WORKER_FLAG 0xf000000000000000
#else
#define USER_WORKER_FLAG 0xf0000000
#endif

/*
 * __cilkrts_establish_c_stack
 *
 * Tell Cilkscreen about the user stack bounds.  For Windows we can move this
 * into Cilkscreen...
 */

void __cilkrts_establish_c_stack(void)
{
    size_t r;
    MEMORY_BASIC_INFORMATION mbi;

    // Fetch the Thread Environment Block.  This will have the current
    // stack bounds
    NT_TIB *pTeb = (NT_TIB *)NtCurrentTeb();

    // Call VirtualQuery to get the base address returned by the OS when the
    // stack was allocated. We can use any address on the stack for this
    r = VirtualQuery (&mbi,
                      &mbi,
                      sizeof(mbi));
    CILK_ASSERT(r == sizeof(mbi));

    // The stack range is the AllocationBase (the address returned by the
    // original VirtualAlloc call and the TEB's StackBase.  Remember that
    // stacks grow down, so the StackBase is the highest address on the
    // stack
    __cilkrts_cilkscreen_establish_c_stack((char *)mbi.AllocationBase,
                                           pTeb->StackBase);
}

/*
 * sysdep_destroy_tiny_stack
 *
 * Free a "tiny" stack (created with sysdep_make_tiny_stack()).
 * Not used on Windows, since we don't create stacks the same way.
 */
void sysdep_destroy_tiny_stack (void *p)
{
}


// Macros for reaching into the OS jumpbuffer.

#if !defined(_WIN64)
// Win32 
#define WIN_JMPBUF_BP(jmpbuf) ((jmpbuf)->Ebp)
#define WIN_JMPBUF_SP(jmpbuf) ((jmpbuf)->Esp)
#define UINT_FOR_SF unsigned long

#else
// Win64 
#define WIN_JMPBUF_BP(jmpbuf) ((jmpbuf)->Rbp)
#define WIN_JMPBUF_SP(jmpbuf) ((jmpbuf)->Rsp)
#define UINT_FOR_SF unsigned __int64
#endif


// Sets the stack pointer in jmpbuf to a pointer on
// the currently executing stack (i.e., the stack executing this
// instance).
//
// Returns the new sp. 
static inline
char* reset_jmpbuf_sp_for_frame(win_jmp_buffer *jmpbuf,
                                full_frame *ff)
{
    // Calculate the size of the frame we'll need
    ptrdiff_t frame_size =
        WIN_JMPBUF_BP(jmpbuf) - (UINT_FOR_SF)ff->sync_sp;

    // Set jmpbuf->Rsp to point into this fiber's stack, with enough space for
    // the spawning function's frame and adjusted to maintain the same stack
    // pointer alignment as the spawning function.  This alignment is
    // important for SSE instructions that impose 16-byte alignment
    // requirements.  We can't just align to a 16-byte boundary because the
    // compiler may know what the alignment was and have adjusted based on
    // that.
    //
    // NOTE: This should NOT be necessary for Win64, since the stack is ALWAYS
    // supposed to be 16-byte aligned.  But checking doesn't hurt...

    // First, compute the alignment of the spawning function's SP
    UINT_FOR_SF sf_sp_alignment =
        (UINT_FOR_SF) ff->sync_sp & 0x0f;
    
    // Suppress complaint about conversion from pointer to same-sized integral
    // type.  This part is not portable code, so it's not important
#pragma warning(disable: 1684)

    // Allocate space for the frame.  Leave enough extra space for two
    // alignment adjustments in opposite directions of up to 15-bytes each
    // (i.e., for a total adjustment of +/- 15 bytes).  Put Rsp in the middle
    // of the extra space (down-growning stack, so + 15 moves back up).
    WIN_JMPBUF_SP(jmpbuf) = ((UINT_FOR_SF) alloca(frame_size + 31)) + 15;
    
#pragma warning(enable: 1684)

    // Now we adjust the alignment of jmpbuf->Rsp to match the orignal SP
    WIN_JMPBUF_SP(jmpbuf) &= ~0x0fULL;         // Adjust down to nearest 16-byte alignment
    WIN_JMPBUF_SP(jmpbuf) |= sf_sp_alignment;  // Match alignment of spawning function sp
    CILK_ASSERT((WIN_JMPBUF_SP(jmpbuf) & 0x0f) == sf_sp_alignment);

    return (char*)WIN_JMPBUF_SP(jmpbuf);
}


char* sysdep_reset_jump_buffers_for_resume(cilk_fiber *fiber,
                                           full_frame *ff,
                                          __cilkrts_stack_frame *sf)
{
    win_jmp_buffer* jmpbuf = (win_jmp_buffer *)&sf->ctx;
    void* sp = reset_jmpbuf_sp_for_frame(jmpbuf, ff);

    // Adjust the saved_sp to account for the SP we're about to run.  This will
    // allow us to track fluctations in the stack
    __cilkrts_take_stack(ff, sp);
    ff->exception_sp = sp;
    
#ifdef _WIN64
    cilk_fiber_data *data = cilk_fiber_get_data(fiber);
    // Save the SP for the initial steal.  This is the highest we're allowed
    // to go on the stack when unwinding.  When we see this RSP, we must stop
    data->steal_frame_sp = (void *)sp;
    
    // Force jmpbuf->Frame to 0.  If Frame is non-zero, longjmp will call
    // any exception handlers it finds as it unwinds.  Since jmpbuf->Frame
    // is on another stack, leaving a value there will cause longjmp to
    // abort with STATUS_BAD_STACK
    jmpbuf->Frame = 0;
#endif
    return sp;
}


/* End sysdep-win.c */

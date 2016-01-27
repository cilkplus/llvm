/* sysdep-win.h                  -*-C++-*-
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

/**
 * @file sysdep-win.h
 *
 * @brief Windows implementation of OS-specific functionality.
 */

#ifndef INCLUDED_SYSDEP_WIN_DOT_H
#define INCLUDED_SYSDEP_WIN_DOT_H

#include <cilk/common.h>
#include <internal/abi.h>

#include "rts-common.h"
#include "full_frame.h"
#include "windows-clean.h"

__CILKRTS_BEGIN_EXTERN_C

/**
 * OS-dependent worker information - Windows implementation
 */
typedef struct __cilkrts_worker_sysdep_state
{
#ifdef _WIN64
    /// Exception record for "throw;" - rethrow the last exception
    EXCEPTION_RECORD *pRethrowExceptionRecord;

    unsigned long long last_stackwalk_rbp;
# endif  /* _WIN64 */

    /// Processor group the worker thread should run on.
    int   processor_group;

} __cilkrts_worker_sysdep_state;

/** HMODULE for the Cilk RunTime System DLL */
extern HMODULE g_hmodCilkrts;

/**
 * Windows-specific function to set the name of a thread.  The name will be
 * displayed by Inspector and the Visual Studio debugger. Only system worker
 * threads are named.  User workers retain whatever name the OS or user gave
 * them.
 *
 * @param w The __cilkrts_worker bound to the thread.
 * @param fiber The fiber we're running on.  Only used for debug builds.
 */
NON_COMMON
void SetWorkerThreadName (__cilkrts_worker *w, void *fiber);

/**
 * Windows-specific function to return the base of the stack we're currently
 * running on.
 *
 * @return The base address of the stack.
 */
NON_COMMON
void *GetStackBase(void);

/**
 * Called by the Windows exception handling code if we're given any exception
 * other than a C++ exception.  This function will generate a fatal error and
 * abort the application.
 *
 * @param exception SEH exception that has been caught by the Windows
 * exception handing code.
 */
NON_COMMON
NORETURN
__cilkrts_report_seh_exception(struct _EXCEPTION_RECORD *exception);


/**
 * Returns the OS fiber from a cilk_fiber.
 *
 * @param fiber The input cilk_fiber.
 *
 * @return  The OS fiber for the cilk_fiber.
 */
NON_COMMON
void *__cilkrts_get_os_fiber(cilk_fiber* fiber);

/**
 * Validate that a value is in the currently executing stack.
 *
 * @param w The worker we're currently running on. Used for debugging
 * information.
 * @param value The value that must be within the stack we're running
 * on.
 * @param description Description of the value we're verifying.  Displayed
 * as part of the bug message displayed if the value is not in the stack.
 */
NON_COMMON
void verify_within_stack (__cilkrts_worker *w,
                          char *value,
                          char *description);

/**
 * Macro to only check that values are within the stack for debug builds.
 */
#ifdef _DEBUG
#   define VERIFY_WITHIN_STACK(w, v, d) verify_within_stack(w, v, d)
#else
#   define VERIFY_WITHIN_STACK(w, v, d)
#endif

/**
 * Returns the cilk_fiber for a worker.
 *
 * @param w The worker to get the cilk_fiber for.
 *
 * @return The cilk_fiber for the worker.
 * @return NULL if we can't find the cilk_fiber.
 */
NON_COMMON
cilk_fiber *GetWorkerFiberObject(__cilkrts_worker *w);


NON_COMMON void sysdep_init_module();   // May move to sysdep.h


#if defined (_WIN32) && !defined(_WIN64)
/** Win32 jump buffer */
typedef struct __JUMP_BUFFER win_jmp_buffer;
#else
/** Win64 jump buffer */
typedef struct _JUMP_BUFFER win_jmp_buffer;
#endif

__CILKRTS_END_EXTERN_C

#endif // ! defined(INCLUDED_SYSDEP_WIN_DOT_H)

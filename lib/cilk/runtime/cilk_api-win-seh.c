/* cilk_api-win-seh.c                  -*-C++-*-
 *
 *************************************************************************
 *
 * Copyright (C) 2009-2015, Intel Corporation
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
 * *********************************************************************
 * 
 * PLEASE NOTE: This file is a downstream copy of a file mainitained in
 * a repository at cilkplus.org. Changes made to this file that are not
 * submitted through the contribution process detailed at
 * http://www.cilkplus.org/submit-cilk-contribution will be lost the next
 * time that a new version is released. Changes only submitted to the
 * GNU compiler collection or posted to the git repository at
 * https://bitbucket.org/intelcilkplusruntime/itnel-cilk-runtime.git are
 * not tracked.
 * 
 * We welcome your contributions to this open source project. Thank you
 * for your assistance in helping us improve Cilk Plus.
 **************************************************************************/

#include <stdio.h>
#include "bug.h"
#include "windows-clean.h"
#include "internal/abi.h"
#include "cilk/cilk_api.h"
#include "sysdep-win.h"

static __cilkrts_pfn_seh_callback pfn_seh_callback = NULL;

/*
 * __cilkrts_set_seh_callback
 *
 * Implementation of the exported __cilkrts_set_seh_callback function.  This
 * function is only available on Windows.
 */

CILK_API_INT __cilkrts_set_seh_callback(__cilkrts_pfn_seh_callback pfn)
{
    pfn_seh_callback = pfn;
    return 0;
}

/*
 * __cilkrts_report_seh_exception
 *
 * Called by the Windows exception handling code if we're given any exception
 * other than a C++ exception.  This function will generate a fatal error and
 * abort the application.
 */

void __cilkrts_report_seh_exception (struct _EXCEPTION_RECORD *exception)
{
    char buf[256];

    // Report the exception to the debugger log
    sprintf_s(buf, sizeof(buf), "\nFatal Cilk Runtime Error:\n"
              "Attempt to pass SEH exception 0x%08X through a spawn.\n"
              "Thrown from address 0x%p.\n",
              exception->ExceptionCode, exception->ExceptionAddress);
    OutputDebugStringA(buf);

    // If a callback was registered, report it there too
    if (pfn_seh_callback)
        pfn_seh_callback(exception);

    // Generate a fatal error.  This will kill the application
    __cilkrts_bug(buf);
}

/* sysdep-win64.c                  -*-C-*-
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
#include "except-win64.h"
#include "bug.h"
#include "local_state.h"
#include "full_frame.h"
#include "jmpbuf.h"
#include "cilk-ittnotify.h"

#include <stdio.h>
#include <malloc.h>

// This probably should be based on the hardware's pagesize, but I'll worry
// about that later
#define PAGE_SIZE 4096

// Suppress complaint about conversion from pointer to same-sized integral
// type.  This isn't portable code, so it's not important
#pragma warning(disable: 1684)

extern void __cilkrts_longjmp_and_rethrow(__cilkrts_stack_frame *sf, jmp_buf _Buf);


/**
 * Win64 implementation of  longjmp to user code for resuming a
 *   @c __cilkrts_stack_frame stalled at a sync.
 *
 * If there's an exception pending, jump to the exception jmpbuf instead of
 * the continuation so the exception will be re-raised.
 */
void sysdep_longjmp_to_sf(char* new_sp,
                          __cilkrts_stack_frame *sf,
                          full_frame *ff_for_exceptions)
{
    struct _JUMP_BUFFER *jmpbuf = (struct _JUMP_BUFFER *)&sf->ctx;
    __cilkrts_worker *w = sf->worker;
    SP(sf) = (unsigned __int64)new_sp;

    // Deal with exceptions if we need to.
    if (ff_for_exceptions) {
        ff_for_exceptions->exception_sp = new_sp;

        // Guarantee that Frame is 0 so longjmp won't attempt to unwind
        jmpbuf->Frame = 0;

        // If there's an exception restore the context and re-raise it.
        // Otherwise set the SP and jump to the saved location
        if (NULL != w->l->pending_exception)
        {
            w->l->pending_exception->sync_rsp = (ULONG64)new_sp;
            w->l->pending_exception->sync_rbp = jmpbuf->Rbp;

            // This path jumps and doesn't come back.
            __cilkrts_longjmp_and_rethrow(sf, sf->ctx);
            // UNREACHABLE
        }
    }

    // We end up here if there is no exception to rethrow.
    
    // No floating point state to restore on Win64.
    // Just longjmp to sf stored in the jump buffer of @c sf.
    longjmp(sf->ctx, 1);
}

void sysdep_save_fp_ctrl_state(__cilkrts_stack_frame *sf)
{
    // Always a noop since the FP control words are saved by the Win64
    // setjmp
}

/* End sysdep-win64.c */

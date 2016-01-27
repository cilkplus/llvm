/* sysdep-win32.c                  -*-C-*-
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
#include "bug.h"
#include "local_state.h"
#include "full_frame.h"
#include "cilk-tbb-interop.h"
#include "cilk-ittnotify.h"
#include "except-win32.h"

#include <assert.h>
#include <excpt.h>
#include <stdio.h>
#include <malloc.h>


#pragma warning (disable : 4733)

#define PAGE_SIZE 4096

// Mask for the saved MXCSR to clear out the exception bits.
// If we set them by loading an old MXCSR, an exception will
// be generated when the next SSE* instruction is executed.
// See 10.2.3.1 in the Intel 64 and IA-32 Architectures
// Software Developer's Manual, Volume 1: Basic Architecture
// for details.
#define X86_MXCSR_MASK      0xffffffc0

static inline restore_fp_state(__cilkrts_stack_frame *sf)
{
    // Restore the floating point state that was saved at the spawn.
    // This is only available with ABI 1 or later frames.
    //
    // Don't forget to mask off the MXCSR exception bits!  Setting them
    // after they have been reset will cause an exception on the next SSE*
    // instruction which would be a bad thing.
    if (CILK_FRAME_VERSION_VALUE(sf->flags) >= 1)
    {
        unsigned long saved_mxcsr = sf->mxcsr & X86_MXCSR_MASK;
        unsigned short saved_fpcsr = sf->fpcsr;

        __asm
        {
            ldmxcsr dword ptr [saved_mxcsr]
            fnclex
            fldcw   word ptr [saved_fpcsr]
        }
    }
}

// CL complains that we're modifying EBP.  We mean to modify EBP.  Live with it
#pragma warning (disable: 4731)

/**
 * Win32 implementation of longjmp to user code for resuming a @c
 *   __cilkrts_stack_frame that is stolen.  This method assumes the
 *   stack pointer in @c sf has already been set. 
 */
static NORETURN sysdep_longjmp_to_sf_stolen_frame(__cilkrts_stack_frame *sf,
                                                  char* new_sp /* Used for debugging only. */  )
{
    win_jmp_buffer* jmpbuf = (win_jmp_buffer *)&sf->ctx;
    unsigned long saved_ebp = jmpbuf->Ebp;
    unsigned long saved_ebx = jmpbuf->Ebx;
    unsigned long saved_edi = jmpbuf->Edi;
    unsigned long saved_esi = jmpbuf->Esi;
    unsigned long saved_eip = jmpbuf->Eip;
    unsigned long saved_esp = jmpbuf->Esp;

    // Make sure that the stack frame has already been saved into
    // the jump buffer.
    CILK_ASSERT((char*)saved_esp == new_sp);

    // Add 4 to Esp - _setjmp3 is expecting to execute a RET from the function.
    // Since we're going to JMP to the actual instruction, we have to allow
    // for the return address that would have been popped off the stack by the
    // RET instruction
    saved_esp +=4;

    DBGPRINTF ("%d-%p: __cilkrts_fiber_stub - jumping to stolen code\n"
               "            sf: %p, EIP: %p, EBP: %p, ESP %p\n",
               sf->worker->self,
               GetCurrentFiber(),
               sf, saved_eip, saved_ebp, saved_esp);
    
    restore_fp_state(sf);

    // Note: Every trylevel change requires the compiler to add a sync,
    // so it shouldn't be possible to throw anything from here?
    //
    // If this proves to be untrue, we'll need to push this functionality
    // into a separate function and setup the SEH registration in there
    __asm
    {
        MOV EBX, saved_ebx  // Restore preserved registers
        MOV EDI, saved_edi
        MOV ESI, saved_esi
        MOV ESP, saved_esp
        MOV EAX, saved_eip  // Move the address we're jumping to to EAX
                            // since EBP-relative accesses won't be
                            // possible after we set EBP
        MOV EBP, saved_ebp  // Set EBP to old frame - locals/params are
                            // on caller's stack
        JMP EAX             // Jump to the new address
    }
}



/**
 * Win32 implementation of longjmp to user code for resuming a
 *   @c __cilkrts_stack_frame stalled at a sync.
 */
static NORETURN sysdep_longjmp_to_sf_after_sync(char* sync_sp,
                                                __cilkrts_stack_frame *sf,
                                                full_frame *ff)
{
    struct __JUMP_BUFFER *jmpbuf = (struct __JUMP_BUFFER *)&sf->ctx;

    VERIFY_WITHIN_STACK (sf->worker, (char *)jmpbuf->Ebp, "EBP");
    VERIFY_WITHIN_STACK (sf->worker, sync_sp, "Sync ESP");
    VERIFY_WITHIN_STACK (sf->worker, (char *)ff->registration, "Registration");

    {
        // Q: why restore FS chain?  Shouldn't the fiber have restored this
        //    value when we jumped back onto this stack?
        // A: If we jump back into a spawn helper (with its own FS registration)
        //    then it will not restore the FS chain on its own because it won't
        //    return normally.  On the contrary, it is longjmp'ing out of its
        //    handler!  So, restore the FS chain manually, along with all of the
        //    registers.
        unsigned long continue_registration = ff->registration;
        CILK_ASSERT(continue_registration);
        ff->exception_sp = sync_sp;

        // TBD: The remainder of this method is almost exactly the
        // same as the code in sysdep_longjmp_to_sf, except for the
        // pesky exception-handling path.  Can we make them the same?

        unsigned long continue_ebp = jmpbuf->Ebp;
        char *continue_esp = sync_sp;
        unsigned long continue_eip = jmpbuf->Eip;       // Set by setjmp in SYNC macro
        unsigned long continue_ebx = jmpbuf->Ebx;
        unsigned long continue_edi = jmpbuf->Edi;
        unsigned long continue_esi = jmpbuf->Esi;

        // Add 4 to Esp - _setjmp3 is expecting to execute a RET from the function.
        // Since we're going to JMP to the actual instruction, we have to allow
        // for the return address that would have been popped off the stack by the
        // RET instruction
        continue_esp +=4;

        DBGPRINTF ("            sysdep_longjmp_to_sf_after_sync - jumping to continuation\n"
                   "            sf: %p, EIP: %p, EBP: %p, ESP: %p\n",
                   sf, continue_eip, continue_ebp, continue_esp);
        
        restore_fp_state(sf);

        if (sf->worker->l->pending_exception) {

            // if there is a pending exception, then rather than jumping back to
            // the function and letting it discover this, we will restore its
            // frame and call rethrow() ourselves.
            //
            // ESP _could_ be different from where it was before.  If the thrown
            // exception lives on this stack, then we want to ensure ESP is at a
            // lower address so that we don't accidentally overwrite it.  Other-
            // wise, we will just leave it alone.
            struct pending_exception_info *pei =
                sf->worker->l->pending_exception;

            if (ff->fiber_self == pei->fiber) {
                if (NULL == pei->rethrow_sp) {
                    // the rethrow_sp was not set if the exception object lives
                    // on this stack, but was thrown from another stack, e.g.,
                    //
                    // void foo () {
                    //   spawn bar();
                    //   // a steal occurs...
                    //   throw 42; // 42 is in foo()'s frame (yes, even for int)
                    // }
                    //
                    // so the continue_esp is correct and won't overwrite the
                    // exception object.  Save continue_esp, in that case.
                    pei->rethrow_sp = continue_esp;
                } else {
                    continue_esp = (char*)pei->rethrow_sp;
                }
            }

            __asm
            {
                MOV EAX, continue_registration
                MOV FS:0, EAX

                MOV EBX, continue_ebx
                MOV EDI, continue_edi
                MOV ESI, continue_esi

                MOV ESP, continue_esp
                PUSH DWORD PTR sf
                MOV EBP, continue_ebp
                CALL __cilkrts_rethrow
            }

            CILK_ASSERT(! "shouldn't be here.");
        }

        __asm
        {
            MOV EAX, continue_registration
            MOV FS:0, EAX

            MOV EBX, continue_ebx
            MOV EDI, continue_edi
            MOV ESI, continue_esi

            MOV EAX, continue_eip
            MOV ESP, continue_esp
            MOV EBP, continue_ebp
            JMP EAX
        }
    }
}



/**
 * Win32 implementation of longjmp to user code for resuming a
 *   @c __cilkrts_stack_frame stalled at a sync.
 */
void sysdep_longjmp_to_sf(char* new_sp,
                          __cilkrts_stack_frame *sf,
                          full_frame *ff_for_exceptions)
{


    if (ff_for_exceptions)  {
        sysdep_longjmp_to_sf_after_sync(new_sp, sf, ff_for_exceptions);
    }
    else {
        sysdep_longjmp_to_sf_stolen_frame(sf, new_sp);
    }
}

#pragma warning (default: 4731)

/* simpler setjmp than the built-in -- optimized for Cilk.  It expects the
 * jmpbuffer in EAX and the landing pad in EDX.
 */
__declspec(dllexport, naked)
int __cilkrts_setjmp () {
  __asm {
    mov DWORD PTR [eax], ebp
    mov DWORD PTR [4+eax], ebx
    mov DWORD PTR [8+eax], edi
    mov DWORD PTR [12+eax], esi
    mov DWORD PTR [16+eax], esp
    mov DWORD PTR [20+eax], edx
    mov edx, DWORD PTR fs: [0]
    mov DWORD PTR [24+eax], edx
    mov DWORD PTR [28+eax], 0
    push 0
    pop eax
    ret
  }
}

void sysdep_save_fp_ctrl_state(__cilkrts_stack_frame *sf)
{
    // Save the floating point control words
    // This is only available with ABI 1 or later frames.
    if (CILK_FRAME_VERSION_VALUE(sf->flags) >= 1)
    {
        unsigned long saved_mxcsr;
        unsigned short saved_fpcsr;

        __asm
        {
            stmxcsr dword ptr [saved_mxcsr]
            fstcw   word ptr [saved_fpcsr]
        }
        sf->mxcsr = saved_mxcsr;
        sf->fpcsr = saved_fpcsr;
    }
}

/* End sysdep-win32.c */

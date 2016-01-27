/* cilk_fiber-win.h                  -*-C++-*-
 *
 *************************************************************************
 *
 *  Copyright (C) 2012-2015, Intel Corporation
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

#ifndef INCLUDED_CILK_FIBER_WIN_DOT_H
#define INCLUDED_CILK_FIBER_WIN_DOT_H

#ifndef __cplusplus
#   error cilk_fiber-win.h is a C++-only header
#endif

#include "cilk_fiber.h"
#include "jmpbuf.h"

/**
 * @file cilk_fiber-win.h
 *
 * @brief Windows-specific implementation for cilk_fiber.
 */


/**
 * @brief Windows-specific fiber class derived from portable fiber class
 */
class cilk_fiber_sysdep : public cilk_fiber
{
  public:

#if SUPPORT_GET_CURRENT_FIBER    
    /**
     * @brief Gets the current fiber from TLS.
     */
    static cilk_fiber_sysdep* get_current_fiber_sysdep();
#endif

    /**
     * @brief Construct the system-dependent portion of a fiber.
     *
     * @param stack_size  The size of the stack for this fiber.
     */ 
    cilk_fiber_sysdep(std::size_t stack_size);

    /**
     * @brief Construct the system-dependent of a fiber created from a
     * thread.
     */ 
    cilk_fiber_sysdep(from_thread_t);

    /**
     * @brief Destructor
     */ 
    ~cilk_fiber_sysdep();


    /**
     * @brief OS-specific calls to convert this fiber back to thread.
     */
    void convert_fiber_back_to_thread();

    /**
     * @brief System-dependent function to suspend self and resume execution of "other".
     *
     * This fiber is suspended.
     *
     * @pre @c is_resumable() should be true.
     *
     * @param other              Fiber to resume.
     */
    void suspend_self_and_resume_other_sysdep(cilk_fiber_sysdep* other);
    
    /**
     * @brief System-dependent function called to jump to @p other
     * fiber.
     *
     * @pre @c is_resumable() should be false.
     *
     * @param other  Fiber to resume.
     */
    NORETURN jump_to_resume_other_sysdep(cilk_fiber_sysdep* other);

    /**
     *  @brief Runs the start_proc.
     *  @pre This method can only be called if is_resumable() is false.
     */
    NORETURN run();

    /**
     * @brief  Not implemented on Windows. 
     */ 
    inline char* get_stack_base_sysdep() { return NULL; }

private:
    void*                       m_win_fiber;      // Windows fiber
    bool                        m_is_user_fiber;  // true if fiber existed
    jmp_buf                     m_initial_jmpbuf; // Main-loop jump buffer

    /**
     * @brief Convert the Windows thread to a fiber.
     *
     * This method sets m_win_fiber accordingly.  The conversion of
     * the thread to a fiber is done lazily because it is an expensive
     * operation and not always needed.  The conversion must be done
     * before switching to another fiber. The behavior of this
     * function is undefined unless m_is_allocated_from_thread is true
     * and m_win_fiber is null.
     */
    void thread_to_fiber();

    /**
     * Common helper method for implementation for resume_other
     * variants.  
     */ 
    inline void resume_other_sysdep(cilk_fiber_sysdep* other);


#if FIBER_DEBUG >= 1
    // Returns true if fiber->owner is the current worker struct. 
    int fiber_owner_is_current_worker(cilk_fiber_sysdep* fiber);
#endif
};


#endif // ! defined(INCLUDED_CILK_FIBER_WIN_DOT_H)

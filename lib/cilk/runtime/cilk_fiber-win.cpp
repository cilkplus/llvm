/* cilk_fiber-win.cpp                  -*-C++-*-
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

#include "cilk_fiber-win.h"
#include "windows-clean.h"
#include "cilk_malloc.h"
#include "bug.h"
#include "os.h"

#include <cstdio>
using std::fprintf;

static void __stdcall fiber_stub(void *pvContext)
{
    // Verify that the value returned by GetFiberData matches the context.
    // Somehow the value in the fiber structure was being trashed when we
    // run under Amplifier.  This was been reported as CQ DPD200149526 and
    // been fixed.  But keep checking, just in case
    CILK_ASSERT(GetFiberData() == pvContext);

    static_cast<cilk_fiber_sysdep*>(pvContext)->run();
}

cilk_fiber_sysdep::cilk_fiber_sysdep(std::size_t stack_size)
    : cilk_fiber(stack_size)
    , m_is_user_fiber(false)
{
    initial_sp = NULL;
#ifdef _WIN64
    steal_frame_sp = 0;
#endif

    // cilk-fiber_win derived-class data members
    m_win_fiber = CreateFiber(stack_size, fiber_stub, this);
    CILK_ASSERT(m_win_fiber);

    this->set_resumable(false);
}

cilk_fiber_sysdep::cilk_fiber_sysdep(from_thread_t)
    : cilk_fiber()
    , m_win_fiber(NULL)
    , m_is_user_fiber(false)
{
    // Set as allocated from thread.
    this->set_allocated_from_thread(true);
    // Setting m_win_fiber is delayed until the first call to 
    // suspend_self_resume_other().
}

cilk_fiber_sysdep::~cilk_fiber_sysdep()
{
    if (this->is_allocated_from_thread())
    {
        CILK_ASSERT(! m_win_fiber || m_win_fiber == GetCurrentFiber());

        // If this thread was converted to a fiber by us, then undo that
        // conversion.  If the thread was already a fiber before we
        // constructed this object, or if the (lazy) conversion did not happen
        // at all, then don't do anything.
        if (! m_is_user_fiber && m_win_fiber)
            ConvertFiberToThread();
    }
    else
        DeleteFiber(m_win_fiber);

    m_win_fiber = NULL;  // Just in case
}

void cilk_fiber_sysdep::convert_fiber_back_to_thread()
{
    CILK_ASSERT(this->is_allocated_from_thread());
    if (m_win_fiber) {
        CILK_ASSERT(m_win_fiber == GetCurrentFiber());
        if (!m_is_user_fiber)
            ConvertFiberToThread();
        m_win_fiber = NULL;
    }
}


#if SUPPORT_GET_CURRENT_FIBER
cilk_fiber_sysdep* cilk_fiber_sysdep::get_current_fiber_sysdep()
{
    return cilkos_get_tls_cilk_fiber();
}
#endif

/*
 * IsPreVistaThreadAFiber
 *
 * Our own implementation of IsThreadAFiber for OS' that don't already have
 * it.
 */
static BOOL is_thread_a_fiber_pre_vista()
{
    /* This number (0x1E00) has been sighted in the wild (at least on windows
     * XP systems) as the return value from GetCurrentFiber() on non fibrous
     * threads. This is sowehow related to OS/2 where the current fiber
     * pointer is overloaded as a version field.  On non-NT systems, 0 is
     * returned.
     */
    static void* const fiber_magic_number = (void *) 0x1E00;

    void *p = GetCurrentFiber();

    // Check for value returned on NT-derived OSes
    if (fiber_magic_number == p)
        return 0;

    // Check for value returned on Win9x-derived OSes
    if (0 == p)
        return 0;

    return 1;
}

/*
 * is_thread_a_fiber
 *
 * Our implementation of IsThreadAFiber.  Unfortunately, Microsoft didn't make
 * IsThreadAFiber available until Vista.  So we have to detect whether
 * IsThreadAFiber is available and, if not, use our own implementation
 * (above).
 */
static bool is_thread_a_fiber()
{
    typedef BOOL (*pfn_is_thread_a_fiber)();

    // Pointer to actual implementation of is_thread_a_fiber.
    static pfn_is_thread_a_fiber imp_func = NULL;

    // If this is the first time in, figure out if we can get the
    // IsThreadAFiber from the OS that the user is running on.  If so, set
    // imp_func to point to IsThreadAFiber.  If not, set imp_func to point to
    // is_thread_a_fiber_pre_vista.
    if (NULL == imp_func)
    {
        // Benign race: Setting imp_func multiple times is idempotent provided
        // that the pointer assignment is atomic (which it should be).

        // Get a handle to kernel32.dll and look for IsThreadAFiber
        HMODULE hKernel32 = GetModuleHandle(L"kernel32.dll");
        if (hKernel32)
            imp_func = (pfn_is_thread_a_fiber)
                GetProcAddress(hKernel32, "IsThreadAFiber");

        // If this OS doesn't provide the IsThreadAFiber, use our implementation
        if (NULL == imp_func)
            imp_func = is_thread_a_fiber_pre_vista;
    }

    // Invoke the actual implementation.
    return imp_func();
}

void cilk_fiber_sysdep::thread_to_fiber()
{
    // If got here, then this fiber was created using the
    // cilk_fiber_sysdep(from_thread_t) constructor and the windows thread has
    // not yet been converted to a windows fiber.  Doing this conversion
    // lazily improves performance because not every thread finds itself
    // returning to a stolen parent.
    CILK_ASSERT(this->is_allocated_from_thread());
    CILK_ASSERT(! m_win_fiber);

    m_is_user_fiber = is_thread_a_fiber();
    if (m_is_user_fiber)
        m_win_fiber = GetCurrentFiber();
    else
        m_win_fiber = ConvertThreadToFiber(this);

    CILK_ASSERT(m_win_fiber);
}


#if FIBER_DEBUG >= 1
// Return "1" if fiber->owner is the current worker. 
int cilk_fiber_sysdep::fiber_owner_is_current_worker(cilk_fiber_sysdep* fiber)
{
    __cilkrts_worker *w = __cilkrts_get_tls_worker();
    if (fiber->owner != w) {
        fprintf(stderr, "ERROR: w=%p, fiber->owner=%p\n",
                w, fiber->owner);
    }
    return (fiber->owner == w);
}                                          
#endif


inline void cilk_fiber_sysdep::resume_other_sysdep(cilk_fiber_sysdep* other)
{
#if SUPPORT_GET_CURRENT_FIBER    
    cilkos_set_tls_cilk_fiber(other);
#endif

#if FIBER_DEBUG >= 1
    CILK_ASSERT(fiber_owner_is_current_worker(other));
#endif
    if (! m_win_fiber)
        thread_to_fiber();  // Lazily create a windows fiber for this thread

    CILK_ASSERT(other->m_win_fiber);

    other->set_resumable(false);
    SwitchToFiber(other->m_win_fiber);

    // Return here when another fiber resumes me.
    CILK_ASSERT(!this->is_resumable());

    // If the fiber that switched to me wants to be deallocated, do it now.
    do_post_switch_actions();
}


void cilk_fiber_sysdep::suspend_self_and_resume_other_sysdep(cilk_fiber_sysdep* other)
{
    CILK_ASSERT(this->is_resumable());
    resume_other_sysdep(other);
}

NORETURN cilk_fiber_sysdep::jump_to_resume_other_sysdep(cilk_fiber_sysdep* other)
{
    CILK_ASSERT(!this->is_resumable());
    resume_other_sysdep(other);

    // When we come back from resuming the other fiber, this fiber
    // should still not be resumable.  Then, reset to the start proc
    // at the top-level loop.
    CILK_ASSERT(!this->is_resumable());
    CILK_ASSERT(m_start_proc);
    longjmp(m_initial_jmpbuf, 1);
}

NORETURN cilk_fiber_sysdep::run()
{
    CILK_ASSERT(m_start_proc);

    // Top of the setjmp/longjmp loop.
    // We return to this point when reusing this fiber.
    setjmp(m_initial_jmpbuf);

    do_post_switch_actions();

    // Call user fiber function
    m_start_proc(this);
    __cilkrts_bug("Should not get here\n");
}

/* End cilk_fiber-win.cpp */

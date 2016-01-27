/* except-win.c                  -*-C-*-
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

#include "except-win.h"
#ifdef _WIN64
#   include "except-win64.h"
#else
#   include "except-win32.h"
#endif
#include "except.h"
#include "bug.h"
#include "local_state.h"
#include "full_frame.h"
#include "scheduler.h"
#include "sysdep.h"

void __cilkrts_save_exception_state(__cilkrts_worker *w,
                                    full_frame *ff)
{
    CILK_ASSERT(NULL == ff->pending_exception);
    ff->pending_exception = w->l->pending_exception;
    w->l->pending_exception = NULL;    

#ifdef _WIN64
    // Restore stealing on this worker
    __cilkrts_restore_stealing(w, w->ltq_limit);
#endif
}

void __cilkrts_setup_for_execution_sysdep(__cilkrts_worker *w,
                                          full_frame *ff)
{
#ifdef _WIN64
    if (w->l->pending_exception)
    {
        DBGPRINTF("%d - resuming exception, pei %p, disallow stealing\n",
                  w->self, w->l->pending_exception);
        w->l->pending_exception->saved_protected_tail =
            __cilkrts_disallow_stealing(w, 0);
        w->l->pending_exception->saved_protected_tail_worker_id = w->self;
    }
#endif
}

/*
 * __cilkrts_merge_pending_exceptions
 *
 * Merge the right exception record into the left.  The left is logically
 * earlier.
 *
 * The active exception of E is
 * E->active if it is non-NULL (in which case E->rethrow is false)
 * unresolved if E->active is NULL and E->rethrow is true
 * nil if E->active is NULL and E->rethrow is false
 *
 * The merged active exception is left active exception if it is not
 * nil, otherwise the right.
 *
 * On entry the left state is synched and can not have an unresolved
 * exception.  The merge may result in an unresolved exception.
 *
 * Due to scoping rules at most one of the caught exception lists is
 * non-NULL.
 */

struct pending_exception_info *
__cilkrts_merge_pending_exceptions (
    __cilkrts_worker *w,
    struct pending_exception_info *left,
    struct pending_exception_info *right)
{
    /* If we've only got one exception, return it */

    if (NULL == left) {
        return right;
    }

    if (NULL == right) {
        return left;
    }

#if defined _WIN32
    // call the destructor on the pending exception object.  It is actually a
    // wrapper that will do useful things like release a stack, if necessary.
    //
    // Don't forget that in Win64, we can have nested pending exceptions and we
    // need to free all of them
    CILK_ASSERT(NULL == w->l->pending_exception);
    w->l->pending_exception = right;
    while (w->l->pending_exception)
    {
        // Destructor wrapper will eventually set w->l->pending_exception to
        // NULL.
        delete_exception_obj (w, w->l->pending_exception, 1);
    }
    CILK_ASSERT(NULL == w->l->pending_exception);
#else
    CILK_ASSERT(! "can't really merge exceptions at this time.");
#endif
    return left;
}

/* End except-win.c */

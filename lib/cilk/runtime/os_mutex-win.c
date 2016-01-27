/* os_mutex-win.c                  -*-C-*-
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

#include "os_mutex.h"
#include "windows-clean.h"
#include "cilk-ittnotify.h"
#include "frame_malloc.h"
#include "bug.h"

/**
 * OS-specific implementation of os_mutex
 */
struct os_mutex
{
    CRITICAL_SECTION lock;  ///< On Windows, os_mutex is implemented with a CRITICAL_SECTION
};

// Global OS mutex initialized and destroyed by calls to DllMain
static struct os_mutex *global_os_mutex = NULL;

/*
 * __cilkrts_os_mutex_create
 *
 * Create an OS mutex.  We use an event so that it can be released
 */

struct os_mutex * __cilkrts_os_mutex_create(void)
{
    os_mutex *p = (os_mutex *)__cilkrts_frame_malloc(NULL, sizeof(*p));
    ITT_SYNC_CREATE(p, "OS Mutex");
    InitializeCriticalSectionAndSpinCount (&p->lock, 4000);
    return p;
}

/*
 * __cilkrts_os_mutex_destroy
 *
 * Free resources used by an os_mutex.  Note that we are NOT making
 * an ITT_SYNC_DESTROY call here, since this may happen after ITTNOTIFY
 * has shutdown.
 */

void __cilkrts_os_mutex_destroy(struct os_mutex *p)
{
    DeleteCriticalSection (&p->lock);
//    ITT_SYNC_DESTROY(p);              // Do not call ITT_SYNC_DESTROY - may crash
    __cilkrts_frame_free(NULL, p, sizeof(*p));
}

/*
 * __cilkrts_os_mutex_lock
 *
 * Acquire the mutex.  Waits until it's available
 */

void __cilkrts_os_mutex_lock(struct os_mutex *p)
{
    EnterCriticalSection (&p->lock);
    ITT_SYNC_ACQUIRED(p);
}

/*
 * __cilkrts_os_mutex_trylock
 *
 * Try to acquire the mutex. Returns nonzero if we got the lock.
 */
int __cilkrts_os_mutex_trylock(struct os_mutex *p)
{
    int got_lock = TryEnterCriticalSection(&p->lock);
    if (got_lock) {
        ITT_SYNC_ACQUIRED(p);
    }
    return got_lock;
}

/*
 * __cilkrts_os_mutex_unlock
 *
 * Unlock the mutex, allowing another caller to acquire it.
 */

void __cilkrts_os_mutex_unlock(struct os_mutex *p)
{
    ITT_SYNC_RELEASING(p);
    LeaveCriticalSection (&p->lock);
}

void global_os_mutex_lock()
{
    CILK_ASSERT (NULL != global_os_mutex);
    __cilkrts_os_mutex_lock(global_os_mutex);
}

void global_os_mutex_unlock()
{
    CILK_ASSERT (NULL != global_os_mutex);
    __cilkrts_os_mutex_unlock(global_os_mutex);
}

void global_os_mutex_create()
{
    CILK_ASSERT (NULL == global_os_mutex);
    global_os_mutex = __cilkrts_os_mutex_create();
}

void global_os_mutex_destroy()
{
    CILK_ASSERT (NULL != global_os_mutex);
    __cilkrts_os_mutex_destroy(global_os_mutex);
    global_os_mutex = NULL;
}

/* End os_mutex-win.c */

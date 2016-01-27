/* stacks.c                  -*-C-*-
 *
 *************************************************************************
 *
 * Copyright (C) 2009-2011 , Intel Corporation
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
 **************************************************************************/

#include "stacks.h"
#include "sysdep.h"
#include "local_state.h"
#include "frame_malloc.h"
#include "cilk-tbb-interop.h"

static void move_to_global(__cilkrts_worker *w, unsigned int until)
{
    __cilkrts_stack_cache *local = &w->l->stack_cache;
    __cilkrts_stack_cache *global = &w->g->stack_cache;

    /* If the global cache appears to be full do not take out the lock. */
    if (global->n >= global->size)
        return;

    __cilkrts_mutex_lock(w, &global->lock);
    while (global->n < global->size && local->n > until) {
        global->stacks[global->n++] = local->stacks[--local->n];
    }
    __cilkrts_mutex_unlock(w, &global->lock);
}

static void push(__cilkrts_worker *w, __cilkrts_stack *sd)
{
    __cilkrts_stack_cache *local = &w->l->stack_cache;
    const unsigned int local_size = local->size;

    /* If room in local, push sd to local stack-of-stacks */
    if (local->n < local_size) {
        local->stacks[local->n++] = sd;
        return;
    }

    if (local_size == 0) {
        __cilkrts_free_stack(w->g, sd);
        return;
    }

    /* No room in local stack-of-stacks.
     * Push half (round down) of the free stacks */
    move_to_global(w, local_size / 2);

    /* If some of the stacks didn't get moved (i.e., because the global
     * stack-of-stacks is full), then permanently destroy some stacks until we
     * are back down to half */
    while (local->n > local_size / 2)
        __cilkrts_free_stack(w->g, local->stacks[--local->n]);

    /* Push the stack onto our local stack-of-stacks */
    local->stacks[local->n++] = sd;
    return;
}

static __cilkrts_stack *pop(__cilkrts_worker *w)
{
    __cilkrts_stack_cache *local = &w->l->stack_cache;
    __cilkrts_stack_cache *global = &w->g->stack_cache;
    __cilkrts_stack *sd = 0;
    if (local->n > 0)
        return local->stacks[--local->n];
    if (global->n > 0) {
        __cilkrts_mutex_lock(w, &global->lock);
        if (global->n > 0)
            sd = global->stacks[--global->n];
        __cilkrts_mutex_unlock(w, &global->lock);
    }
    return sd;
}

#ifdef _WIN32
#   include "stacks-win.h"
#   define okay_to_release(stack) (0 == (stack)->outstanding_references)
#else
#   define okay_to_release(stack) (1)
#endif  // _WIN32

void __cilkrts_release_stack(__cilkrts_worker *w,
                             __cilkrts_stack *sd)
{
    START_INTERVAL(w, INTERVAL_FREE_STACK);
    if (sd && okay_to_release(sd)) {
        __cilkrts_invoke_stack_op(w, CILK_TBB_STACK_RELEASE,sd);
        push(w, sd);
    }
    STOP_INTERVAL(w, INTERVAL_FREE_STACK);
    return;
}

__cilkrts_stack *__cilkrts_get_stack(__cilkrts_worker *w)
{
    __cilkrts_stack *sd;

    START_INTERVAL(w, INTERVAL_ALLOC_STACK);
    sd = pop (w);
    if (sd == NULL)
        sd = __cilkrts_make_stack(w);
    else
        __cilkrts_sysdep_reset_stack(sd);
    STOP_INTERVAL(w, INTERVAL_ALLOC_STACK);
    return sd;
}

static void flush(global_state_t *g,
                  __cilkrts_stack_cache *c)
{
    /*START_INTERVAL(w, INTERVAL_FREE_STACK);*/
    while (c->n > 0)
        __cilkrts_free_stack(g, c->stacks[--c->n]);
    /*STOP_INTERVAL(w, INTERVAL_FREE_STACK);*/
}

void __cilkrts_init_stack_cache(__cilkrts_worker *w,
                                __cilkrts_stack_cache *c,
                                unsigned int size)
{
    __cilkrts_mutex_init(&c->lock);
    c->size = size;
    c->n = 0;
    c->stacks = __cilkrts_frame_malloc(w, size * sizeof(__cilkrts_stack *));
#if 0 /* Causes problems on Linux due to generated call to intel_fast_memset */
    {
      unsigned int i;
      /* Not really needed -- only indices < n are valid */
      for (i = 0; i < size; i++)
        c->stacks[i] = 0;
    }
#else
    if (size > 0)
      c->stacks[0] = 0;
#endif
}

void __cilkrts_destroy_stack_cache(__cilkrts_worker *w,
                                   global_state_t *g,
                                   __cilkrts_stack_cache *c)
{
    flush(g, c);
    __cilkrts_frame_free(w, c->stacks, c->size * sizeof(__cilkrts_stack *));
    c->stacks = 0;
    c->n = 0;
    c->size = 0;
    __cilkrts_mutex_destroy(w, &c->lock);
}

/* Free all but one local stack, returning to the global pool if possible. */

void __cilkrts_trim_stack_cache(__cilkrts_worker *w)
{
    __cilkrts_stack_cache *local = &w->l->stack_cache;

    if (local->n <= 1)
        return;

    START_INTERVAL(w, INTERVAL_FREE_STACK);

    move_to_global(w, 1);

    while (local->n > 1)
        __cilkrts_free_stack(w->g, local->stacks[--local->n]);

    STOP_INTERVAL(w, INTERVAL_FREE_STACK);
}

/* End stacks.c */

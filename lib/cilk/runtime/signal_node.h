/* signal_node.h                  -*-C++-*-
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
 * @file signal_node.h
 *
 * @brief Signal nodes allow coordinated waking and sleeping of the runtime
 * without hammering on a single location in memory.
 *
 * The workers are logically arranged in a binary tree and propagate messages
 * leaf-ward.  User workers notify the root about waking and sleeping, so only
 * that one node need share a cache line with a user worker.
 */

#ifndef INCLUDED_SIGNAL_NODE_DOT_H
#define INCLUDED_SIGNAL_NODE_DOT_H

#include "rts-common.h"
#include <cilk/common.h>

__CILKRTS_BEGIN_EXTERN_C

/** Opaque type. */
typedef struct signal_node_t signal_node_t;

/**
 * Allocate and initialize a signal_node_t
 *
 * @return The initialized signal_node_t
 */
COMMON_SYSDEP
signal_node_t *signal_node_create(void);

/**
 * Free any resources and deallocate a signal_node_t
 *
 * @param node The node to be deallocated.
 */
COMMON_SYSDEP void signal_node_destroy(signal_node_t *node);

/**
 * Test whether the node thinks the worker should go to sleep
 *
 * @param node The node to be tested.
 *
 * @return 1 If the worker should go to sleep
 * @return 0 If the worker should not go to sleep
 */
COMMON_SYSDEP
unsigned int signal_node_should_wait(signal_node_t *node);

/**
 * Specify whether the worker should go to sleep
 *
 * @param node The node to be set.
 * @param msg The value to be set.  Valid values are:
 * - 0 - the worker should go to sleep
 * - 1 - the worker should stay active
 */
COMMON_SYSDEP
void signal_node_msg(signal_node_t *node, unsigned int msg);


/**
 * Wait for the node to be set
 *
 * @param node The node to wait on
 */
COMMON_SYSDEP
void signal_node_wait(signal_node_t *node);

__CILKRTS_END_EXTERN_C

#endif // ! defined(INCLUDED_SIGNAL_NODE_DOT_H)

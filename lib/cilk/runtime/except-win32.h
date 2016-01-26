/* except-win32.h                  -*-C++-*-
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
 * @file except-win32.h
 *
 * @brief Exception handling structures and routines used for Win32 exception
 * handling.
 *
 * Most of this information is derived from the excellent MSDN article by Matt
 * Pietrek titled "A Crash Course on the Depths of Win32 Structured Exception
 * Handling" at http://www.microsoft.com/msj/0197/exception/exception.aspx
 */

#ifndef INCLUDED_EXCEPT_WIN32_DOT_H
#define INCLUDED_EXCEPT_WIN32_DOT_H

#include <cilk/common.h>

#include "except-win.h"
#include "windows-clean.h"
#include "cilk_fiber.h"

/// Forwarded declaration of a worker.
typedef struct __cilkrts_worker __cilkrts_worker;

/// Forwarded declaration of a stack frame.
typedef struct __cilkrts_stack_frame __cilkrts_stack_frame;

__CILKRTS_BEGIN_EXTERN_C

/**
 * Field indices from EXCEPTION_RECORD->ExceptionInformation[] for when an
 * exception is a C++ exception (and thereby possibly an object).
 *
 * The custom parameters of the Win32 exception include pointers to the
 * exception object and its Win32ThrowInfo structure, using which the exception
 * handler can match the thrown exception type against the types expected by
 * catch handlers.
 */
enum win32_except_info
{
    info_magic_number = 0,  ///< Magic number to identify version of code throwing exception
    info_cpp_object = 1,    ///< The object being thrown
    info_throw_data = 2,    ///< Win32ThrowInfo structure containing data about the object being thrown
};

/*
 * Unused on Win32 - Reference only
#define CT_SIMPLE_TYPE(p) ((p) & 0x01)
#define CT_REF_ONLY(p) ((p) & 0x02)
#define CT_VIRT_BASES(p) ((p) & 0x04)
 */

/**
 * Catchable type
 *
 * Information about a type a catch block can handle
 */
typedef struct
{
    /**
     * 0x01: simple type (can be copied by memmove), 0x02: can be caught by
     * reference only, 0x04: has virtual bases
     */
    DWORD properties;

    /** RTTI type descriptor for this catch block */
    TypeDescriptor* pType;

    /** How to cast the thrown object to this type */
    PMD thisDisplacement;

    /** Object size */
    int sizeOrOffset;

    /** Copy constructor address */
    void (*copyFunction)(void);
} Win32CatchableType;

/**
 * Information about the type (or types) that can catch a thrown object
 */
typedef struct
{
    /// Number of entries in the following array
    int nCatchableTypes; 

    /// Image offset to array of catchable types
    Win32CatchableType *arrayOfCatchableTypes[1];
} Win32CatchableTypeInfo;


/*
 * Unused on Win32 - Reference only
#define TI_CONST(x) (x & 0x01)
#define TI_VOLATILE(x) (x & 0x02)
 */

/**
 * Information about a thrown object.
 */
typedef struct
{
    /// 0x01: const, 0x02: volatile
    DWORD attributes;

    /// Thrown object destructor
    void (*pmfnUnwind)(void);

    /// Forward compatibility handler
    int (*pForwardCompat)(void);

    /// List of types that can catch this exception.  i.e. the actual type and
    /// all its ancestors.
    Win32CatchableTypeInfo* pCatchableTypeArray;
} Win32ThrowInfo;

/* pending_exception_info has a subset of EXCEPTION_RECORD fields (as defined in
   winnt.h).  Additionally, it maintains enough information to move an exception
   across one or more steal boundaries. */
typedef struct pending_exception_info {

    /* ESP to restore at rethrow().  If the throw occurred on a different stack
       this will be NULL. */
    void *rethrow_sp;

    /* pointer to the fiber on which the exception was thrown.  If the
       exception was thrown on a user fiber, this field is NULL. */
    cilk_fiber *fiber;

    /* saved worker.  This field is saved along with the stack since releasing
       a stack requires a worker pointer. */
    __cilkrts_worker *w;

    /* saved frame from disallowing stealing on a stack. */
    __cilkrts_stack_frame * volatile *saved_sf;

    /* duplicate of the ThrownInfo corresponding to the exception object --
       except the destructor (pmfnUnwind()) points to one of our functions that
       will do some Cilkish cleanup after destructing the object. */
    Win32ThrowInfo fake_info;

    /* "Real" Win32ThrowInfo for the thrown object.  This has the actual pointer to
       the destructor. */
    Win32ThrowInfo *real_info;

    /*** EXCEPTION_RECORD fields ***/

    /* thrower-specified code. */
    DWORD ExceptionCode;

    /* flags identifying SEH vs. C++, filter vs. unwind, etc. */
    DWORD ExceptionFlags;

    /* populated field count in ExceptionInformation. */
    DWORD NumberParameters;

    /* thrower-specified exception data (used especially in C++ exceptions --
       incorporating type, object pointer, method pointers, size, etc.). */
    ULONG_PTR ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
} pending_exception_info;

/**
 * Entry for a pending exception/object pair
 */
typedef struct
{
  void *exception_object;               ///< The thrown object
  struct pending_exception_info *pei;   ///< The exception to be re-raised
} exception_entry_t;

/**
 * exceptions need to be tracked when they cross steals in Win32.  This struct
 * keeps an array of pending exceptions the runtime system is tracking.  It isn't
 * very efficient if there are a lot of pending exceptions, but we don't have a
 * realistic use case for that, so the expected case is fast.
 */
typedef struct pending_exceptions_t pending_exceptions_t;

/**
 *@brief Array of pending exceptions the runtime is tracking.
 */
struct pending_exceptions_t
{
    /// Used to serialize access.  This is a spin-lock since accesses are
    /// relatively short and infrequent.
    volatile long lock;

    /// Set relatively small to minimize space usage in applications that don't
    /// use it.  Since accesses are atomic, the array can be resized.
    int max_pending_exceptions;

    /// Array of pending exceptions.
    exception_entry_t *entries;

};


// Methods declared only for Win-32 exceptions.

/**
 * __cilkrts_stub_handler() replaces the default Microsoft handler in fiber
 * stub functions in Win32.  This special-case handler wraps up the exception,
 * unwinds the stack up to this point, and jumps to the parent's sync (where
 * the exception will be rethrown).
 *
 * @param exception_record SEH data for the exception that has occurred.
 * @param registration SEH registration node for this exception handler
 * @param context_record Hardware context - registers and the like
 * @param context ?
 *
 * @return An entry in the EXCEPTION_DISPOSITION enumeration indicating
 * whether this exception has been dealt with or the OS should continue
 * searching for a handler.
 */
NON_COMMON
int __cilkrts_stub_handler(struct _EXCEPTION_RECORD *exception_record,
                           struct EXCEPTION_REGISTRATION *registration,
                           struct CONTEXT *context_record,
                           void *context);
/**
 * Checks whether a pointer is within the current stack bounds
 *
 * @param ptr The value to checked
 *
 * @return 0 The value is not within the stack
 * @return 1 The value is with the stack
 */
NON_COMMON
int is_within_stack (char *ptr);

typedef struct pending_exceptions_t pending_exceptions_t;

NON_COMMON
pending_exceptions_t *get_cilk_pending_exceptions ();


#if defined (_WIN32) && !defined(_WIN64)
// Win32 needs an exception stub handler.

/*
 * install_exception_stub_handler
 *
 * Called from fiber_proc_to_resume_user_code to install our exception
 * handler in place of the exception handler installed by the
 * compiler.
 *
 * A bit of background:  Win32 maintains a chain of Structured Exception
 * Handling (SEH) nodes, pointed at by the first entry in the Thread
 * Information Block (TIB).  A function that wants to catch an exception
 * creates an SEH node on the stack, and then pushes it onto the head of
 * the chain.  When the function exits, it is required to pop the SEH node
 * off the chain.  See Matt Pietrek's article "A Crash Course on the Depths
 * of Win32 Structured Exception Handling" if you want all the gory details.
 * http://www.microsoft.com/msj/0197/exception/exception.aspx 
 *
 * Given that, this function is really playing a nasty trick. Since it
 * has no exception handling of its own, the node at the head of the
 * chain of SEH chain is the node of the caller
 * (fiber_proc_to_resume_user_code).  This function overwrites the
 * address of the exception handling routine put into the node by
 * fiber_proc_to_resume_user_code.
 *
 * Since all of our longjmps will never unwind past
 * fiber_proc_to_resume_user_code, this only has to be done once, at
 * the beginning of fiber_proc_to_resume_user_code, which will be
 * called when the fiber starts executing.
 *
 * TBD: Is this last statement still true for this fiber code?
 */
static inline
void install_exception_stub_handler () {
    __asm {
        // The FS register points to the Thread Information Block (TIB) for
        // this thread, and the first entry in the TIB is the pointer to the
        // head of the chain of exception handler nodes.  Fetch the head of
        // the chain out of the TIB.
        mov eax, FS:0
            
        // EAX now has the base address of the SEH node.  The first field in
        // the SEH node is the link to the next SEH node in the chain.  Add 4
        // to EAX to get to the next field, which holds the address of the
        // function to call to handle an exception.
        add eax, 4

        // Overwrite the exception handling routine setup by
        // fiber_proc_to_resume_user_code with our own function
        lea edx, __cilkrts_stub_handler
        mov [eax], edx
    }
}
#endif



__CILKRTS_END_EXTERN_C


#endif // ! defined(INCLUDED_EXCEPT_WIN32_DOT_H)

/* except-win64.h                  -*-C++-*-
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
 * @file except-win64.h
 *
 * @brief The exception handling data for Win64 is very like the data for
 * Win32, **EXCEPT** that all addresses are RVAs instead of actual addresses.
 * The C++ exception has an extra parameter which is the image base.
 *
 * This no doubt makes loading faster since the loader doesn't have to
 * snap all of the addresses in the exception data.  However, it makes
 * our life more difficult.
 *
 * Most of this information is derived from the excellent article "Reversing
 * Microsoft Visual C++ Part I: Exception Handling" at
 * http://www.openrce.org/articles/full_view/21
 */

#ifndef INCLUDED_EXCEPT_WIN64_DOT_H
#define INCLUDED_EXCEPT_WIN64_DOT_H

#include "except.h"
#include "except-win.h"
#include "windows-clean.h"
#include <cilk/common.h>

__CILKRTS_BEGIN_EXTERN_C

/**
 * Function we install as the vectored exception handler.  Our handler will
 * be called before the OS walks the callstack for structured exception
 * handlers.
 */
LONG CALLBACK __cilkrts_vectored_handler(EXCEPTION_POINTERS *pExceptionInfo);

/**
 * Field indices from EXCEPTION_RECORD->ExceptionInformation[] for when an
 * exception is a C++ exception (and thereby possibly an object).
 *
 * The custom parameters of the Win32 exception include pointers to the
 * exception object and its Win64ThrowInfo structure, using which the exception
 * handler can match the thrown exception type against the types expected by
 * catch handlers.
 */
enum win64_except_info
{
    info_magic_number = 0,  ///< Magic number to identify version of code throwing exception
    info_cpp_object = 1,    ///< The object being thrown
    info_throw_data = 2,    ///< Win64ThrowInfo structure containing data about the object being thrown
    info_image_base = 3     ///< Image base 
};

/** Signature for a destuctor */
typedef void (*PFN_DESTRUCTOR)(ULONG_PTR pThis);

/** Unwind Map Entry */
typedef struct
{
    int toState;                ///< target state
    DWORD offUnwindFunclet;     ///< action to perform (unwind funclet address)
} UnwindMapEntry;

/**
 * Catch block descriptor
 *
 * Describes a single catch() clause of try block
 */
typedef struct
{
    /// 0x01: const, 0x02: volatile, 0x08: reference
    DWORD adjectives;

    /// RTTI descriptor of the exception type. 0=any (ellipsis)
    /// Note that this is the RVA for the TypeDescriptor
    DWORD offTypeDescriptor;

    /// RBP-based offset of the exception object in the function stack.
    /// 0 = no object (catch by type)
    int dispCatchObj;

    /// Address of the catch handler code.
    /// Returns address where to continues execution (i.e. code after the
    // try block).  Note that this is the RVA for the handler code.
    DWORD offHandlerCode;

    /// Unknown field
    DWORD unknown;

} HandlerType;

/// Macro to check for a handler for a const value
#define HT_CONST(x) (x & 0x01)

/// Macro to check for a handler for a volatile value
#define HT_VOLATILE(x) (x & 0x02)

/// Macro to check for a value by reference
#define HT_REF(x) (x & 0x08)

/**
 * Try block descriptor. Describes a try{} block with associated catches.
 */
typedef struct
{
    int tryLow;     ///< Low state this try block covers
    int tryHigh;    ///< High state this try block covers
    int catchHigh;  ///< Highest state inside catch handlers of this try
    int nCatches;   ///< Number of catch handlers
    DWORD offHandlerArray; ///< Image offset to catch handlers table - HandlerType*
} TryBlockMapEntry;

// List of expected exceptions (implemented but not enabled in MSVC by default, use /d1ESrt to enable).

/**
 * Maps IP to trylevel
 */
typedef struct
{
    DWORD   offIp;      ///< Image offset of the start of the IP range in the function
    int     iTryLevel;  ///< Try level corresponding to the range -1 indicates not in a try block
} Ip2StateMapEntry;

/**
 * Information generated by the compiler about a functions
 * exception handling data
 */
typedef struct
{
    /// Compiler version.
    /// 0x19930520: up to VC6, 0x19930521: VC7.x(2002-2003), 0x19930522: VC8 (2005)
    DWORD magicNumber;

    /// Number of entries in unwind table
    int maxState;

    /// Table of unwind destructors
    DWORD offUnwindMap;      // Image offset to UnwindMap

    /// Number of try blocks in the function
    DWORD nTryBlocks;

    /// Mapping of catch blocks to try blocks
    DWORD offTryBlockMap;       // Image offset to TryBlockMap (TryBlockMapEntry*)

    /// Number of entries in the IP to state map
    DWORD nIpMapEntries;

    /// Offsett of the first entry in the IP2StateMap
    DWORD offIptoStateMap;

    /// VC8+ only, bit 0 set if function was compiled with /EHs
    int EHFlags;
} FuncInfo;

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

    /** Offset to the RTTI type descriptor for this catch block */
    DWORD offTypeDescriptor;

    /** How to cast the thrown object to this type */
    PMD thisDisplacement;

    /** Object size */
    int sizeOrOffset;

    /** Image offset to copy constructor */
    DWORD offCopyFunction;
} Win64CatchableType;

/// Macro to check for a simple type.  That is, a type that can be copied
/// using memmove
#define CT_SIMPLE_TYPE(p) ((p) & 0x01)

/// Macro to check for a type that can only be caught by reference
#define CT_REF_ONLY(p) ((p) & 0x02)

/// Macro to check for a type that has virtual bases
#define CT_VIRT_BASES(p) ((p) & 0x04)

/**
 * Information about the type (or types) that can catch a thrown object
 */
typedef struct
{
    /// Number of entries in the following array
    int nCatchableTypes; 

    /// Image offset to array of catchable types
    DWORD offArrayOfCatchableTypes;
} Win64CatchableTypeInfo;

/**
 * Information about a thrown object.  The offset and image base for this
 * structure are contained in the SEH parameters.
 */
typedef struct
{
    /// 0x01: const, 0x02: volatile
    DWORD attributes;

    /// Image offset to destructor
    DWORD offDtor;

    /// Image offset to forward compatibility function
    DWORD pfnForwardCompat;

    /// Image offset to the list of types that can catch this exception.
    /// I.E. the actual type and all its ancestors.
    DWORD offCatchableTypeInfo;
} Win64ThrowInfo;

/// Macro to check for a const thrown value
#define TI_CONST(x) (x & 0x01)

/// Macro to check for a volatile thrown value
#define TI_VOLATILE(x) (x & 0x02)

/**
 * pending_exception_info has a subset of EXCEPTION_RECORD fields (as defined in
 * winnt.h).  Additionally, it maintains enough information to move an exception
 * across one or more steal boundaries.
 */
struct pending_exception_info
{
    /// Nested pending exception
    struct pending_exception_info *nested_exception;

    /// IP to set in __cilkrts_rethrow as the return address
    ULONG64 rethrow_rip;

    /// Original RBP and RSP for the exception.  Since we're re-raising the
    /// exception, we need to make sure the stack gets contracted appropriately
    /// when the exception is retired.  If we match the RPB, then restore the
    /// RSP in the DISPATCHER_CONTEXT.
    ULONG64 exception_rbp;
    ULONG64 exception_rsp;  ///< @copydoc exception_rbp

    /// RBP/RSP we synced to, if we did
    ULONG64 sync_rbp;
    ULONG64 sync_rsp;       ///< @copydoc sync_rbp

    /// Pointer to the stack on which the exception was thrown.  If the
    /// exception was thrown on a user stack, this field is NULL.
    cilk_fiber* exception_fiber;

    /// Saved worker.  This field is saved along with the stack since releasing
    /// a stack requires a worker pointer.
    __cilkrts_worker *w;

    /// Saved frame from disallowing stealing on a stack.
    __cilkrts_stack_frame * volatile *saved_protected_tail;

    /// The current exception record
    EXCEPTION_RECORD *pExceptRec;

    /// Copy of the Win64ThrowInfo we'll modify to contain our destructor
    Win64ThrowInfo   copy_ThrowInfo;

    /// Offset for original destructor
    DWORD       offset_dtor;

    /// True if a nested exception is coming
    int         nested_exception_found;

    /// Index of worker who saved the protected tail.  Used for debugging.
    int         saved_protected_tail_worker_id;
};

/**
 * Since addresses in the exception structures for Win64 images are specified
 * using an unsigned 32-bit offset from the image base, we need to create a
 * "trampoline" to get from the region that can be reference to our destructor
 * wrapper.
 *
 * The trampoline consists of the following instructions:
 *
 *  mov rax, CilkDestructorWrapper @n
 *  jmp rax
 *
 * Since we're building instructions, this structure requires that it be
 * packed on 1-byte boundaries.
 */

#pragma pack(push, 1)
typedef struct _ExceptionTrampoline
{
    struct _ExceptionTrampoline *pNext;     ///< Next trampoline in the chain
    char    lowerGuard[sizeof(void *)];     ///< Filled with int 3
    unsigned short movOpcode;               ///< Opcode prefix - 0xb848
    void *movValue;                         ///< 64 bit value to be loaded
    unsigned short jmpRax;                  ///< jmp rax - 0xe0ff
    char    upperGuard[sizeof(void *)];     ///< Filled with int 3
} ExceptionTrampoline;
#pragma pack(pop)

__CILKRTS_END_EXTERN_C

#endif // ! defined(INCLUDED_EXCEPT_WIN64_DOT_H)

/* except-win64.c                  -*-C-*-
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

// Windows shouldn't whine about uses of getenv which it considers unsafe
#define _CRT_SECURE_NO_WARNINGS 1

#include "except-win64.h"
#include "bug.h"
#include "local_state.h"
#include "full_frame.h"
#include "scheduler.h"
#include "malloc.h"
#include "sysdep-win.h"
#include "jmpbuf.h"
#include "cilk_malloc.h"
#include "record-replay.h"
#include <assert.h>
#include <stdio.h>
#include <setjmp.h>
#pragma warning(push)
#pragma warning(disable: 147)   // declaration is incompatible with definition in winnt.h
#include <intrin.h>
#pragma warning(pop)

// Exception flags
#if _WIN32_WINNT <= 0x0601 
//# define EXCEPTION_NONCONTINUABLE   0x0001 // Defined in WinNT.h
#   define EXCEPTION_UNWINDING        0x0002
#   define EXCEPTION_EXIT_UNWIND      0x0004
#   define EXCEPTION_STACK_INVALID    0x0008
#   define EXCEPTION_NESTED_CALL      0x0010
#   define EXCEPTION_TARGET_UNWIND    0x0020
#   define EXCEPTION_COLLIDED_UNWIND  0x0040
#endif // _WIN32_WINNT <= 0x0601
#define EXCEPTION_UNWIND_FLAGS     0x0066

ExceptionTrampoline *g_pTrampolineRoot = NULL;
CRITICAL_SECTION g_csTrampolineList;

#define MAX_TRAMPOLINE_OFFSET 0x80000000          // Really could be 4GB
#define TRAMPOLINE_SEARCH_INCR 0x0001000

//#pragma message ("Warning 1684 disabled - should be checked later")
//#pragma warning(disable: 1684)

#define MakePtr( cast, ptr, addValue ) (cast)( (DWORD_PTR)(ptr) + (DWORD_PTR)(addValue))
#define IMPLIES(a,b) (!a || b)

// Debugging stuff.  Dumps *PILES* of information as we walk the call stack.
// Should normally be disabled.

#if 0   // Set to 1 to dump Vectored Exception Handler contexts
#pragma message ("Dumping vectored exception handler contexts")
#define DUMP_VH_CONTEXT(pContext, Frame, pfnHandler, description, self, n) dump_context(pContext, Frame, pfnHandler, description, self, n)
#else
#define DUMP_VH_CONTEXT(pContext, Frame, pfnHandler, description, self, n)
#endif

#if 0   // Set to 1 to dump __cilkrts_stack_frame information
#pragma message ("Dumping __cilkrts_stack_frame information")
#define DUMP_SF(sf, desc, self, pDispatcherContext) dump_sf(sf, desc, self, pDispatcherContext)
#else
#define DUMP_SF(sf, desc, self, pDispatcherContext)
#endif

#if 0   // Set to 1 to dump GlobalUnwind contexts
#pragma message ("Dumping GlobalUnwind contexts")
#define DUMP_GU_CONTEXT(pContext, Frame, pfnHandler, description, self, n) dump_context(pContext, Frame, pfnHandler, description, self, n)
#else
#define DUMP_GU_CONTEXT(pContext, Frame, pfnHandler, description, self, n)
#endif

#if 0   // Set to 1 to dump FrameHandler contexts
#pragma message ("Dumping FrameHandler handler contexts")
#define DUMP_FH_CONTEXT(pContext, Frame, pfnHandler, description, self, n) dump_context(pContext, Frame, pfnHandler, description, self, n)
#else
#define DUMP_FH_CONTEXT(pContext, Frame, pfnHandler, description, self, n)
#endif

#if 0   // Set to 1 to dump EXCEPTION_RECORD information
#pragma message ("Dumping EXCEPTION_RECORD information")
#define DUMP_EXCEPTION(n, pExceptRec) dump_exception(n, pExceptRec)
#else
#define DUMP_EXCEPTION(n, pExceptRec)
#endif

#if 0   // Set to 1 to dump FUNC_INFO information
#define DUMP_FUNC_INFO(n, pDispatcherContext) dump_func_info(n, pDispatcherContext)
#else
#define DUMP_FUNC_INFO(n, pDispatcherContext)
#endif

#define UNWIND_HISTORY_TABLE_SIZE 12

#define UNWIND_HISTORY_TABLE_NONE 0
#define UNWIND_HISTORY_TABLE_GLOBAL 1
#define UNWIND_HISTORY_TABLE_LOCAL 2

#if _WIN32_WINNT <= 0x0601
// Use the SDK's definition for the newer windows version.
typedef struct _DISPATCHER_CONTEXT {
    ULONG64 ControlPc;
    ULONG64 ImageBase;
    PRUNTIME_FUNCTION FunctionEntry;
    ULONG64 EstablisherFrame;
    ULONG64 TargetIp;
    PCONTEXT ContextRecord;
    PEXCEPTION_ROUTINE LanguageHandler;
    PVOID HandlerData;
    PUNWIND_HISTORY_TABLE HistoryTable;
    ULONG                 ScopeIndex;
    ULONG                 Fill0;
} DISPATCHER_CONTEXT, *PDISPATCHER_CONTEXT;
#endif // _WIN32_WINNT <= 0x0601

#define UNW_FLAG_NHANDLER 0x0
#define UNW_FLAG_EHANDLER 0x1
#define UNW_FLAG_UHANDLER 0x2
#define UNW_FLAG_CHAININFO 0x4

typedef unsigned char       UBYTE;

typedef struct _UNWIND_CODE
{
    UBYTE offset;      // Offset in prolog
    UBYTE code: 4;     // Unwind operation code
    UBYTE info: 4;     // Operation info
} UNWIND_CODE, *PUNWIND_CODE;

typedef struct _UNWIND_INFO
{
    UBYTE Version         : 3;
    UBYTE Flags           : 5;
    UBYTE SizeOfProlog;
    UBYTE CountOfCodes;
    UBYTE FrameRegister  : 4;
    UBYTE FrameOffset    : 4;
    UNWIND_CODE UnwindCode[1];
    /*
    union {
        //
        // If (Flags & UNW_FLAG_EHANDLER)
        //
        OPTIONAL ULONG ExceptionHandler;
        //
        // Else if (Flags & UNW_FLAG_CHAININFO)
        //
        OPTIONAL ULONG FunctionEntry;
    };
    //
    // If (Flags & UNW_FLAG_EHANDLER)
    //
    OPTIONAL ULONG ExceptionData[];
    */
} UNWIND_INFO, *PUNWIND_INFO;

typedef EXCEPTION_DISPOSITION
    (*PFN_FRAME_HANDLER) (IN PEXCEPTION_RECORD pExceptRec,
                          IN ULONG64 EstablisherFrame,
                          IN OUT PCONTEXT pContext,
                          IN OUT PDISPATCHER_CONTEXT pDispatcherContext);
/*
 * GetFuncInfo
 *
 * Returns the pointer to the FuncInfo structure, or NULL.
 */

static inline
FuncInfo *GetFuncInfo(DISPATCHER_CONTEXT *pDispatcherContext)
{
    DWORD *data = (DWORD *)pDispatcherContext->HandlerData;
    if (0 == data[0])
        return NULL;
    return MakePtr(FuncInfo *, pDispatcherContext->ImageBase, data[0]);
}

/*
 * GetFirstTryBlockMapEntry
 *
 * Returns the address of the first TryBlockMap entry, or NULL
 */

static inline
TryBlockMapEntry *GetFirstTryBlockMapEntry(DISPATCHER_CONTEXT *pDispatcherContext)
{
    FuncInfo *pFuncInfo = GetFuncInfo(pDispatcherContext);
    if (NULL == pFuncInfo)
        return NULL;

    if (0 == pFuncInfo->offTryBlockMap)
        return NULL;

    return MakePtr (TryBlockMapEntry *, pDispatcherContext->ImageBase, pFuncInfo->offTryBlockMap);
}

/*
 * GetFirstCatchHandlerType
 *
 * Returns the address of the first HandlerType for a TryBlockMapEntry
 */

static inline
HandlerType *GetFirstCatchHandlerType(DISPATCHER_CONTEXT *pDispatcherContext,
                                      TryBlockMapEntry *pTryBlockEntry)
{
    if (0 == pTryBlockEntry->offHandlerArray)
        return NULL;

    return MakePtr (HandlerType *,
                    pDispatcherContext->ImageBase,
                    pTryBlockEntry->offHandlerArray);
}

/*
 * GetHandlerTypeDescriptor
 *
 * Returns the address of the TypeDescriptor for a catch HandlerType
 */

static inline
TypeDescriptor *GetHandlerTypeDescriptor(DISPATCHER_CONTEXT *pDispatcherContext,
                                         HandlerType *pHandler)
{
    if (0 == pHandler->offTypeDescriptor)
        return NULL;

    return MakePtr(TypeDescriptor *,
                   pDispatcherContext->ImageBase,
                   pHandler->offTypeDescriptor);
}

#if 0
/*
 * GetCatchableTypeDescriptor
 *
 * Returns the address of the TypeDescriptor for a Win64CatchableType
 */

static inline
TypeDescriptor *GetCatchableTypeDescriptor(EXCEPTION_RECORD *pExceptRec,
                                           Win64CatchableType *pCatchable)
{
    if (0 == pCatchable->offTypeDescriptor)
        return NULL;

    return MakePtr(TypeDescriptor *,
                   pExceptRec->ExceptionInformation[info_image_base],
                   pCatchable->offTypeDescriptor);
}
#endif

/*
 * GetFirstUnwindMapEntry
 *
 * Return the address of the first UnwindMap entry, or NULL
 */

static inline
UnwindMapEntry *GetFirstUnwindMapEntry(DISPATCHER_CONTEXT *pDispatcherContext)
{
    FuncInfo *pFuncInfo = GetFuncInfo(pDispatcherContext);
    if (NULL == pFuncInfo)
        return NULL;

    if (0 == pFuncInfo->offUnwindMap)
        return NULL;

    return MakePtr (UnwindMapEntry *, pDispatcherContext->ImageBase, pFuncInfo->offUnwindMap);
}

Win64CatchableTypeInfo *GetCatchableTypeInfo(EXCEPTION_RECORD *pExceptRec)
{
    Win64ThrowInfo *pThrowInfo = (Win64ThrowInfo *)pExceptRec->ExceptionInformation[info_throw_data];

    if (0 == pThrowInfo->offCatchableTypeInfo)
        return NULL;

    return MakePtr(Win64CatchableTypeInfo *,
                   pExceptRec->ExceptionInformation[info_image_base],
                   pThrowInfo->offCatchableTypeInfo);
}

Win64CatchableType *GetFirstCatchableType(EXCEPTION_RECORD *pExceptRec)
{
    Win64CatchableTypeInfo *cti = GetCatchableTypeInfo(pExceptRec);
    if (NULL == cti)
        return NULL;

    if (0 == cti->offArrayOfCatchableTypes)
        return NULL;

    return MakePtr (Win64CatchableType *,
                    pExceptRec->ExceptionInformation[info_image_base],
                    cti->offArrayOfCatchableTypes);
}

void *dump_ptr(char *output, size_t max, char *desc, DWORD offset, DWORD64 ImageBase)
{
    char buf[256];

    void *p;
    if (0 == offset)
        p = NULL;
    else
        p = MakePtr(void *, offset, ImageBase);

    sprintf_s(buf, 256, desc, p);
    strcat_s(output, max, buf);
    return p;
}

void dump_exception(int self, EXCEPTION_RECORD *pExceptionRecord)
{
    char outbuf[4096];
    char buf[256];
    Win64CatchableTypeInfo *cti;
    DWORD64 ImageBase;
    Win64ThrowInfo *pThrowInfo;
    int i;
    Win64CatchableType *ct;
    TypeDescriptor *t;

    sprintf_s(outbuf, 4096, "\n%d - ExceptionRecord @ 0x%p\n\n", self, pExceptionRecord);
    sprintf_s(buf, 256, "ExceptionCode: %08x\n", pExceptionRecord->ExceptionCode);
    strcat_s(outbuf, 4096, buf);
    sprintf_s(buf, 256, "ExceptionFlags: %08x\n", pExceptionRecord->ExceptionFlags);
    strcat_s(outbuf, 4096, buf);
    sprintf_s(buf, 256, "ExceptionRecord: %p\n", pExceptionRecord->ExceptionRecord);
    strcat_s(outbuf, 4096, buf);
    sprintf_s(buf, 256, "ExceptionAddress: %p\n", pExceptionRecord->ExceptionAddress);
    strcat_s(outbuf, 4096, buf);
    sprintf_s(buf, 256, "NumberParameters: %d\n", pExceptionRecord->NumberParameters);
    strcat_s(outbuf, 4096, buf);
    for (i = 0; i < pExceptionRecord->NumberParameters; i++)
    {
        sprintf_s(buf, 256, "  %d: %p\n", i, pExceptionRecord->ExceptionInformation[i]);
        strcat_s(outbuf, 4096, buf);
    }
    // Only attempt to dump C++ exception information
    if (CXX_EXCEPTION != pExceptionRecord->ExceptionCode)
        return;
    if (0x19930520 != pExceptionRecord->ExceptionInformation[info_magic_number])
        return;

    ImageBase = pExceptionRecord->ExceptionInformation[info_image_base];

    pThrowInfo =
        (Win64ThrowInfo *)pExceptionRecord->ExceptionInformation[info_throw_data];

    sprintf_s(buf, 256, "\nThrowInfo [%p]:\n", pThrowInfo);
    strcat_s(outbuf, 4096, buf);
    sprintf_s(buf, 256, "  attributes: 0x%x", pThrowInfo->attributes);
    strcat_s(outbuf, 4096, buf);
    if (TI_CONST(pThrowInfo->attributes))
        strcat_s(outbuf, 4096, " [const 0x01]");
    if (TI_VOLATILE(pThrowInfo->attributes))
        strcat_s(outbuf, 4096, " [volatile 0x02]");

    strcat_s(outbuf, 4096, "\n");
    dump_ptr(outbuf, 4096, "  destructor fn: 0x%p\n", pThrowInfo->offDtor, ImageBase);
    dump_ptr(outbuf, 4096, "  forward compatibility fn: 0x%p\n", pThrowInfo->pfnForwardCompat, ImageBase);
    cti = dump_ptr(outbuf, 4096, "  catchable type array: 0x%p\n", pThrowInfo->offCatchableTypeInfo, ImageBase);

    sprintf_s(buf, 256, "\nCatchableTypeInfo [%p]:\n", cti);
    strcat_s(outbuf, 4096, buf);
    sprintf_s(buf, 256, "   type count: %d\n", cti->nCatchableTypes);
    strcat_s(outbuf, 4096, buf);
    ct = dump_ptr(outbuf, 4096, "   catchable types: 0x%p\n", cti->offArrayOfCatchableTypes, ImageBase);

    for (i = 0; i < cti->nCatchableTypes; i++)
    {
        sprintf_s(buf, 256, "\n   Catchable Type %d\n", i);
        strcat_s(outbuf, 4096, buf);
        sprintf_s(buf, 256, "      properties: 0x%x", ct->properties);
        strcat_s(outbuf, 4096, buf);
        if (CT_SIMPLE_TYPE(ct->properties))
            strcat_s(outbuf, 4096, " [Simple Type 0x01]");
        if (CT_REF_ONLY(ct->properties))
            strcat_s(outbuf, 4096, " [Ref Only 0x02]");
        if (CT_VIRT_BASES(ct->properties))
            strcat_s(outbuf, 4096, " [Virtual Bases 0x04]");
        strcat_s(outbuf, 4096, "\n");
        t = (TypeDescriptor *)dump_ptr(outbuf, 4096, "      type descriptor: 0x%p\n", ct->offTypeDescriptor, ImageBase);
        if (NULL != t)
        {
            sprintf_s(buf, 256, "      type name: %s\n", t->name);
            strcat_s(outbuf, 4096, buf);
        }
        sprintf_s(buf, 256, "      size of offset: %d\n", ct->sizeOrOffset);
        strcat_s(outbuf, 4096, buf);
        dump_ptr(outbuf, 4096, "      copy constructor: 0x%p\n", ct->offCopyFunction, ImageBase);
    }
    DBGPRINTF(outbuf);
}

void dump_func_info (int self, DISPATCHER_CONTEXT *pDispatcherContext)
{
    unsigned n, m;
    UnwindMapEntry *pUnwindMap;
    Ip2StateMapEntry *pIp2StateMap;
    DWORD offProcStart;
    TryBlockMapEntry *pTryBlockMapEntry;
    FuncInfo *pFuncInfo = GetFuncInfo(pDispatcherContext);
    HandlerType *pHandler;
    TypeDescriptor *pType;
    char outbuf[4096];
    char buf[256];
    unsigned prev_offset = 0;

    sprintf_s(outbuf, 4096, "\n%d - FuncInfo @ 0x%p\n", self, pFuncInfo);
    sprintf_s(buf, 256, "magicNumber: %08x\n", pFuncInfo->magicNumber);
    strcat_s(outbuf, 4096, buf);

    sprintf_s(buf, 256, "nUnwindMapEntries: %d\n", pFuncInfo->maxState);
    strcat_s(outbuf, 4096, buf);
    pUnwindMap = GetFirstUnwindMapEntry(pDispatcherContext);
    sprintf_s(buf, 256, "UnwindMap: %p\n", pUnwindMap);
    strcat_s(outbuf, 4096, buf);

    sprintf_s(buf, 256, "nTryBlocks: %d\n", pFuncInfo->nTryBlocks);
    strcat_s(outbuf, 4096, buf);
    pTryBlockMapEntry = GetFirstTryBlockMapEntry(pDispatcherContext);
    sprintf_s(buf, 256, "TryBlockMap: %p\n", pTryBlockMapEntry);
    strcat_s(outbuf, 4096, buf);

    sprintf_s(buf, 256, "nIpMapEntries: %d\n", pFuncInfo->nIpMapEntries);
    strcat_s(outbuf, 4096, buf);
    pIp2StateMap = MakePtr (Ip2StateMapEntry *,
                      pDispatcherContext->ImageBase,
                      pFuncInfo->offIptoStateMap);
    sprintf_s(buf, 256, "IptoStateMap: %p\n", pIp2StateMap);
    strcat_s(outbuf, 4096, buf);

    sprintf_s(buf, 256, "Flags?: %p\n", pFuncInfo->EHFlags);
    strcat_s(outbuf, 4096, buf);

    sprintf_s(buf, 256, "\nUnwindMap @ pUnwindMap:\n");
    strcat_s(outbuf, 4096, buf);
    sprintf_s(buf, 256, "Entry   Funclet  toState\n");
    strcat_s(outbuf, 4096, buf);
    for (n = 0; n < pFuncInfo->maxState; n++, pUnwindMap++)
    {
        sprintf_s(buf, 256, "%3d   0x%08x %4d\n",
                  n, pUnwindMap->offUnwindFunclet, pUnwindMap->toState);
        strcat_s(outbuf, 4096, buf);
    }

    sprintf_s(buf, 256, "\nTryBlockMap:\n");
    strcat_s(outbuf, 4096, buf);
    sprintf_s(buf, 256, "Entry TryLow TryHigh CatchHigh Handlers   HandlerArray    Type(s)\n");
    strcat_s(outbuf, 4096, buf);
    for (n = 0; n < pFuncInfo->nTryBlocks; n++, pTryBlockMapEntry++)
    {
        pHandler = GetFirstCatchHandlerType(pDispatcherContext, pTryBlockMapEntry);
        sprintf_s(buf, 256, "%3d    %3d    %3d      %3d       %3d    %p",
                  n, pTryBlockMapEntry->tryLow, pTryBlockMapEntry->tryHigh,
                  pTryBlockMapEntry->catchHigh,
                  pTryBlockMapEntry->nCatches, pHandler);
        strcat_s(outbuf, 4096, buf);
        if (NULL != pHandler)
        {
            for (m = 0; m < pTryBlockMapEntry->nCatches; m++)
            {
                pType = GetHandlerTypeDescriptor(pDispatcherContext, pHandler);
                if (m == 0)
                    sprintf_s(buf, 256, "  ");
                else
                    sprintf_s(buf, 256, ", ");
                sprintf_s(buf, 256, pType->name);
                pHandler++;
            }
        }

    }

    offProcStart = pIp2StateMap->offIp;
    sprintf_s(buf, 256, "\nIptoStateMap - ProcStart offset: %p\n", offProcStart);
    strcat_s(outbuf, 4096, buf);
    sprintf_s(buf, 256, "ControlPC: %p - Image Offset: %p, Proc Offset: %p\n",
              pDispatcherContext->ControlPc,
              pDispatcherContext->ControlPc - pDispatcherContext->ImageBase,
              (pDispatcherContext->ControlPc - pDispatcherContext->ImageBase) - offProcStart);
    strcat_s(outbuf, 4096, buf);
    sprintf_s(buf, 256, "Entry ImagOffset ProcOffset     delta      state\n");
    strcat_s(outbuf, 4096, buf);
    for (n = 0; n < pFuncInfo->nIpMapEntries; n++, pIp2StateMap++)
    {
        sprintf_s(buf, 256, "%3d   0x%08x 0x%08x  0x%08x %4d\n",
                  n,
                  pIp2StateMap->offIp,
                  pIp2StateMap->offIp - offProcStart,
                  0 == prev_offset ? 0 :  pIp2StateMap->offIp - prev_offset,
                  pIp2StateMap->iTryLevel);
        strcat_s(outbuf, 4096, buf);
        prev_offset = pIp2StateMap->offIp;
    }

    DBGPRINTF(outbuf);
}

#define DUMP_FLAG(flags, f) if (flags & f) { flags &= ~f; strcat_s(buf, 256, "  " # f); }
void dump_sf(__cilkrts_stack_frame *sf, char *caller, int self,PDISPATCHER_CONTEXT pDispatcherContext)
{
    unsigned int flags;
    char buf[512];
    char *p = buf;
    size_t len;

    if (NULL == sf)
    {
        DBGPRINTF ("%d - %s - sf: NULL\n", self, caller);
        return;
    }

    sprintf_s (buf, 512, "%d - %s - sf: %p, flags: %08x - ",
               self, caller, sf, sf->flags);

    flags = sf->flags;
    DUMP_FLAG(flags, CILK_FRAME_STOLEN);
    DUMP_FLAG(flags, CILK_FRAME_UNSYNCHED)
    DUMP_FLAG(flags, CILK_FRAME_DETACHED);
    DUMP_FLAG(flags, CILK_FRAME_EXCEPTION_PROBED);
    DUMP_FLAG(flags, CILK_FRAME_EXCEPTING);
    DUMP_FLAG(flags, CILK_FRAME_LAST);
    DUMP_FLAG(flags, CILK_FRAME_EXITING);
    DUMP_FLAG(flags, CILK_FRAME_SUSPENDED);
    DUMP_FLAG(flags, CILK_FRAME_UNWINDING);

    strcat_s(buf, 512, "\n");
    p += strnlen_s(buf, 512);
    len = 512 - strnlen_s(buf, 512);

    if (NULL != pDispatcherContext)
    {
        sprintf_s(p, len, "%d - Full frame: %p, RSP: %p, RBP: %p, Frame: %p\n",
                  self, sf->worker->l->frame_ff,
                  pDispatcherContext->ContextRecord->Rsp, pDispatcherContext->ContextRecord->Rbp,
                  pDispatcherContext->EstablisherFrame);
    }

    DBGPRINTF(buf);
}

void dump_context(CONTEXT *pContext, ULONG64 EstablisherFrame, void *pfnUnwindHandler,
                  char *description, int self, int n)
{
    char outbuf[2048];
    char buf[256];
    sprintf_s (outbuf, 2048, description, self, n);

    if (0 != EstablisherFrame)
    {
        sprintf_s(buf, 256, "EstablisherFrame: %p ", EstablisherFrame);
        strcat_s (outbuf, 2048, buf);
    }
    if (NULL != pfnUnwindHandler)
    {
        sprintf_s(buf, 256, "exception_rtn: %p ", pfnUnwindHandler);
        strcat_s (outbuf, 2048, buf);
    }

    sprintf_s(buf, 256, "RBP: %p\n", pContext->Rbp);
    strcat_s (outbuf, 2048, buf);
    sprintf_s(buf, 256, "RAX: %p, RBX: %p, RCX: %p, RDX: %p\n",
              pContext->Rax,
              pContext->Rbx,
              pContext->Rcx,
              pContext->Rdx);
    strcat_s (outbuf, 2048, buf);
    sprintf_s(buf, 256, "RSI: %p, RDI: %p,  R8: %p,  R9: %p\n",
              pContext->Rsi,
              pContext->Rdi,
              pContext->R8,
              pContext->R9);
    strcat_s (outbuf, 2048, buf);
    sprintf_s(buf, 256, "R10: %p, R11: %p, R12: %p, R13: %p\n",
              pContext->R10,
              pContext->R11,
              pContext->R12,
              pContext->R13);
    strcat_s (outbuf, 2048, buf);
    sprintf_s(buf, 256, "R14: %p, R15: %p, RIP: %p, RSP: %p\n\n",
              pContext->R14,
              pContext->R15,
              pContext->Rip,
              pContext->Rsp);
    strcat_s (outbuf, 2048, buf);
    DBGPRINTF(outbuf);
}

/*
 * StealingIsDisabled
 *
 * Returns true if we've disabled stealing
 */

static int StealingIsDisabled(__cilkrts_worker *w)
{
    return w->protected_tail == w->l->ltq;
}

/*
 * __cilkrts_rethrow
 *
 * Reraises an exception so we can continue processing it
 */

CILK_ABI_THROWS_VOID __cilkrts_rethrow(__cilkrts_stack_frame *sf)
{
    __cilkrts_worker *w;
    struct pending_exception_info *pending_exception;
    ULONG64 *ret_addr_on_stack;
    _JUMP_BUFFER *jump_buffer = (_JUMP_BUFFER *)&sf->ctx;

    ret_addr_on_stack = (ULONG64 *)_AddressOfReturnAddress();

    w = sf->worker;
    CILK_ASSERT(NULL != w);

    DUMP_SF(sf, "__cilkrts_rethrow", w->self, NULL);

    // Fetch the pending exception which should have been stored earlier
    CILK_ASSERT(NULL != w->l->pending_exception);
    pending_exception = w->l->pending_exception;

    DBGPRINTF ("%d - __cilkrts_rethrow - rethrow apparently from %p, alternate RIP %p, jmp_buf RIP: %p\n"
               "     pei: %p, pExcept: %p, object; %p, sf: %p\n",
               w->self, ret_addr_on_stack[0], pending_exception->rethrow_rip, jump_buffer->Rip,
               pending_exception,
               pending_exception->pExceptRec,
               pending_exception->pExceptRec->ExceptionInformation[info_cpp_object],
               sf);



    // Overwrite the saved RIP so when we unwind the stack, RtlUnwindEx
    // will skip over the code we've already unwound

    if (sf->flags & CILK_FRAME_STOLEN)
        ret_addr_on_stack[0] = jump_buffer->Rip;    // If the frame has been stolen, assume JMPBUF is valid
    else
    {
        if (0 == pending_exception->rethrow_rip)
            ret_addr_on_stack[0] = jump_buffer->Rip;
        else
            ret_addr_on_stack[0] = pending_exception->rethrow_rip;
    }

    // Make sure that stealing is disallowed for this worker while we're
    // processing the exception.  Stealing is disabled until we've finished
    // handling the exception
    if (! StealingIsDisabled(w))
    {
        pending_exception->saved_protected_tail = __cilkrts_disallow_stealing(w, NULL);
        pending_exception->saved_protected_tail_worker_id = w->self;
    }

    RaiseException(pending_exception->pExceptRec->ExceptionCode,
                   0,   // We're starting over
                   pending_exception->pExceptRec->NumberParameters,
                   pending_exception->pExceptRec->ExceptionInformation);

    CILK_ASSERT(! "shouldn't reach this point");
}

/*
 * DestructThrownObject
 *
 * If a destructor method was specified in the Win64ThrowInfo, call it to cleanup
 * the thrown object
 */

static
void DestructThrownObject (EXCEPTION_RECORD *pExceptRec)
{
    Win64ThrowInfo *pThrowInfo;
    PFN_DESTRUCTOR pfnFunclet;

    // If this is a rethrow the Win64ThrowInfo pointer and/or object can be NULL
    if (0 == pExceptRec->ExceptionInformation[info_throw_data])
        return;
    if (0 == pExceptRec->ExceptionInformation[info_cpp_object])
        return;

    pThrowInfo = (Win64ThrowInfo *)pExceptRec->ExceptionInformation[info_throw_data];

    // There may not be a destructor
    if (0 == pThrowInfo->offDtor)
        return;

    // Get the absolute address of the destructor, and call it
    pfnFunclet = MakePtr (PFN_DESTRUCTOR,
                          pExceptRec->ExceptionInformation[info_image_base],
                          pThrowInfo->offDtor);

    pfnFunclet(pExceptRec->ExceptionInformation[info_cpp_object]);
}

/*
 * delete_exception_obj
 *
 * Call the destructor on the pending exception and then free the stack
 */

void delete_exception_obj (__cilkrts_worker *w,
                           struct pending_exception_info *pei,
                           int delete_object)
{
    CILK_ASSERT(w);

    DBGPRINTF ("%d - delete_exception_obj - deleting exception info %p, rethrow_rip: %p\n",
               w->self, pei, pei->rethrow_rip);

    // Don't attempt to restore the protected tail
    pei->saved_protected_tail = NULL;
    pei->saved_protected_tail_worker_id = -1;

    // Disable stealing while the object is being destroyed.
    DestructThrownObject (pei->pExceptRec);
}

/*
 * ModuleSize
 *
 * Given the base address of a module, return the size of the module in bytes.
 * Returns 0 if there is any error
 */

DWORD ModuleSize(ULONG64 moduleBase)
{
    IMAGE_NT_HEADERS64 *pNTHeader;
    IMAGE_DOS_HEADER *pDosHeader = (IMAGE_DOS_HEADER *)moduleBase;

    // Verify that this is a PE file
    if (IMAGE_DOS_SIGNATURE != pDosHeader->e_magic)
        return 0;

    // Make sure that it's got the right signature in the NT header.  Assume
    // that it's a 64-bit image
    pNTHeader = MakePtr (IMAGE_NT_HEADERS64 *, pDosHeader, pDosHeader->e_lfanew);

    if (pNTHeader->Signature != IMAGE_NT_SIGNATURE)
        return 0;

    if (IMAGE_NT_OPTIONAL_HDR64_MAGIC != pNTHeader->OptionalHeader.Magic)
        return 0;

    return pNTHeader->OptionalHeader.SizeOfImage;
}

/*
 * ReleaseFiberToGlobalPool
 *
 * TBD: This is icky and really shouldn't be here...
 *
 * Release the specified fiber to the global pool.
 * w should be the current worker (or NULL)
 */

static
void ReleaseFiberToGlobalPool(__cilkrts_worker* w,
                              cilk_fiber *fiber)
{
    cilk_fiber_pool* pool_to_use;
    if (w) {
        pool_to_use = &w->l->fiber_pool;
    }
    else {
        // The global state should be initialized.  Otherwise, I think we
        // have problems...
        CILK_ASSERT(cilkg_is_published());
        pool_to_use = &cilkg_get_global_state()->fiber_pool;
    }

    // There'd better be at least one outstanding reference - ours
    CILK_ASSERT(cilk_fiber_has_references(fiber));
    cilk_fiber_remove_reference(fiber, pool_to_use);
}

/*
 * DestructorWrapper
 *
 * Function that is called in place of the application's destructor so we
 * can catch the destruction of the exception object.  This tells us that
 * the saved stack is no longer needed
 */

void DestructorWrapper(void *pThis)
{
    __cilkrts_worker *w;
    void **stack = (void **)pThis;
    struct pending_exception_info *pei = (struct pending_exception_info *)stack[-1];
    cilk_fiber* exception_fiber;

    DBGPRINTF ("DestructorWrapper - PEI: %p, pExceptRect: %p, object: %p\n",
               pei, pei->pExceptRec, pei->pExceptRec->ExceptionInformation[info_cpp_object]);

    // Restore the destructor in the Win64ThrowInfo and call it to destruct the
    // exception object
    if (0 != pei->offset_dtor)
    {
        Win64ThrowInfo *pThrowInfo = (Win64ThrowInfo *)pei->pExceptRec->ExceptionInformation[info_throw_data];
        if (NULL != pThrowInfo)
        {
            CILK_ASSERT(pThrowInfo->offDtor != pei->offset_dtor);
            pThrowInfo->offDtor = pei->offset_dtor;
            DestructThrownObject(pei->pExceptRec);
        }
    }

    // Assume there's no stack to release
    exception_fiber = NULL;

    // Remove the pending exception from the worker
    w = __cilkrts_get_tls_worker();
    if (w)
    {
        // Grab full frame and stack_self while stealing is disallowed.
        full_frame *ff = w->l->frame_ff;
        cilk_fiber *fiber_self = (ff ? ff->fiber_self : NULL);
        
        DBGPRINTF ("%d - DestructorWrapper releasing pei: %p, nested_pei: %p\n",
                  w->self, pei, pei->nested_exception);

        CILK_ASSERT(w->l->pending_exception);
        CILK_ASSERT(w->l->pending_exception == pei);

        w->l->pending_exception = pei->nested_exception;

        // Re-allow stealing if we're not dealing with a nested exception.
        // pei->saved_protected_tail is set to NULL in delete_exception_obj
        // and indicates that we shouldn't attempt to restore stealing.
        if ((NULL == w->l->pending_exception) && (NULL != pei->saved_protected_tail))
        {
            // Make sure that the saved protected tail applies to this worker
            CILK_ASSERT(pei->saved_protected_tail >= w->l->ltq);
            CILK_ASSERT(pei->saved_protected_tail <= w->ltq_limit);

            DBGPRINTF ("%d - DestructorWrapper restoring stealing, protected tail: %p, ltq: %p, limit: %p\n",
                      w->self, pei->saved_protected_tail, w->l->ltq, w->ltq_limit);
            __cilkrts_restore_stealing(w, pei->saved_protected_tail);
            pei->saved_protected_tail = 0;
            pei->saved_protected_tail_worker_id = -1;
            CILK_ASSERT(! StealingIsDisabled(w));
        }
#ifdef _DEBUG
        else
        {
            if (w->l->pending_exception)
                DBGPRINTF("%d - DestructorWrapper stealing not restored due to nested exception\n",
                      w->self);
            if (pei->saved_protected_tail)
                DBGPRINTF("%d - DestructorWrapper stealing not restored due to NULL protected tail\n",
                      w->self);
        }
#endif

        // Make sure we don't try to free an exception stack that we're
        // running on!  Note that we may not have a frame to look into
        if (NULL != ff)
        {
            if (fiber_self != pei->exception_fiber)
                exception_fiber = pei->exception_fiber;
        }
    }
    else
    {
        DBGPRINTF ("DestructorWrapper - no worker, releasing pei: %p\n", pei);
    }

    // Free the pending exception information
    __cilkrts_free(pei);

    // If there's a fiber, release it to the global pool
    if (NULL != exception_fiber)
        ReleaseFiberToGlobalPool(w, exception_fiber);
}

/*
 * AllocateAndInitTrampoline
 *
 * Allocate a trampoline at the address specified.  Initialize it, and add it
 * to the chain of trampolines.
 */

ExceptionTrampoline *AllocAndInitTrampoline(ULONG64 address)
{
    int i;
    ExceptionTrampoline *pTrampoline;
    DWORD old_protection;

    // Try to allocate the trampoline at the specified address
    pTrampoline = (ExceptionTrampoline *)
        VirtualAlloc ((LPVOID)address,
                      sizeof(ExceptionTrampoline),
                      MEM_RESERVE | MEM_COMMIT,
                      PAGE_READWRITE);

    if (NULL == pTrampoline)
        return NULL;

    // Fill the guards with INT 3
    for (i = 0; i < sizeof(pTrampoline->lowerGuard); i++)
    {
        pTrampoline->lowerGuard[i] = 0xCC;
        pTrampoline->upperGuard[i] = 0xCC;
    }

    // Fill in the instructions
    pTrampoline->movOpcode = 0xb848;
    pTrampoline->movValue = DestructorWrapper;
    pTrampoline->jmpRax = 0xe0ff;

    // Add the new trampoline to the list
    EnterCriticalSection(&g_csTrampolineList);
    pTrampoline->pNext = g_pTrampolineRoot;
    g_pTrampolineRoot = pTrampoline;

    // Set the page protection to read/execute
    i = VirtualProtect (pTrampoline,
                        sizeof(ExceptionTrampoline),
                        PAGE_EXECUTE_READ,
                        &old_protection);
    LeaveCriticalSection(&g_csTrampolineList);

    CILK_ASSERT(i);

    return pTrampoline;
}

/*
 * GetTrampoline
 *
 * Search the list of trampolines for one that will work for this module.  If
 * none are suitable, try to create one
 */

void *GetTrampoline(ULONG64 imageBase)
{
    size_t vq_result;
    MEMORY_BASIC_INFORMATION mbi;
    ULONG64 p;
    ptrdiff_t offset;
    ExceptionTrampoline *pTrampoline;
    SYSTEM_INFO sysInfo;
    DWORD allocationGranularityMask;

    // See if there's an existing trampoline we can use
    EnterCriticalSection(&g_csTrampolineList);

    pTrampoline = g_pTrampolineRoot;
    while (pTrampoline)
    {
        offset = (char *)pTrampoline - (char *)imageBase;
        if ((offset > 0) && (offset < MAX_TRAMPOLINE_OFFSET))
            break;
        pTrampoline = pTrampoline->pNext;
    }
    LeaveCriticalSection(&g_csTrampolineList);

    if (pTrampoline)
        return &pTrampoline->movOpcode;

    // We need to create one.  We can only allocate memory using VirtualAlloc
    // on AllocationGranularity boundaries.  So grab the value from the OS.

    GetSystemInfo (&sysInfo);
    allocationGranularityMask = ~ (sysInfo.dwAllocationGranularity - 1);

    // Look in the module for it's size.  Round up to the next largest
    // allocation chunk.

    offset = ModuleSize(imageBase);
    if (0 == offset)
        return NULL;
    offset = allocationGranularityMask & (offset + sysInfo.dwAllocationGranularity - 1);

    while (offset < MAX_TRAMPOLINE_OFFSET)
    {
        p = imageBase + offset;

        // Get VM information about the module
        vq_result = VirtualQuery((LPCVOID)p,
                                 &mbi,
                                 sizeof(mbi));
        CILK_ASSERT(vq_result == sizeof(mbi));

        if (MEM_FREE == mbi.State)
        {
            pTrampoline = AllocAndInitTrampoline(p);
            if (pTrampoline)
                return &pTrampoline->movOpcode;
        }

        // Increment by allocation granularity
        offset += sysInfo.dwAllocationGranularity;
    }

    // We couldn't allocate one
    return NULL;
}

/*
 * HIDWORD, LODWORD
 *
 * Functions to return the high and low dword of a 64-bit value.  These
 * functions disable warning 2259 which complains about data loss
 */

static inline
DWORD LODWORD(ULONG64 value)
{
#pragma warning(push)
#pragma warning(disable: 2259)  // non-pointer conversion may lose significant bits
    return (DWORD)(value & 0xffffffff);
#pragma warning(pop)
}

static inline
DWORD HIDWORD(ULONG64 value)
{
#pragma warning(push)
#pragma warning(disable: 2259)  // non-pointer conversion may lose significant bits
    return (DWORD)(value >> 32);
#pragma warning(pop)
}

/*
 * WrapException
 *
 * Populate the pending_exception_info object by duplicating the
 * EXCEPTION_RECORD and saving relevant information that will be needed during
 * cleanup.
 *
 * This function is called by the Cilk handler functions when hither-to unseen
 * (by the runtime) exceptions reach it
 */

static
void *WrapException (__cilkrts_worker *w,
                     EXCEPTION_RECORD *pExceptRec,
                     ULONG64 rethrow_rip,
                     PCONTEXT pContext,
                     __cilkrts_stack_frame * volatile *saved_protected_tail)
{
    void *pTrampoline;
    Win64ThrowInfo *pThrowInfo;
    ULONG64 trampoline_dtor_offset;
    void **stack;
    struct pending_exception_info *pei;

    // If this exception is already wrapped, just update the rethrow_ip

    if ((w->l->pending_exception) &&
        (0 == w->l->pending_exception->nested_exception_found))
    {
        pei = w->l->pending_exception;

        DBGPRINTF ("%d - WrapException  - updating PEI: %p\n", w->self, pei);
        pei->rethrow_rip = rethrow_rip;
        pei->exception_rbp = pContext->Rbp;
        pei->exception_rsp = pContext->Rsp;

        DBGPRINTF ("%d - WrapException  - rethrow_rip set to %p\n",
                    w->self, pei->rethrow_rip);
        DBGPRINTF ("%d - WrapException  - exception_rbp, rsp updated to %p, %p\n",
                    w->self, pContext->Rbp, pContext->Rsp);

        return NULL;
    }

    // Allocate memory for the pending exception information
    pei = (struct pending_exception_info*)
        __cilkrts_malloc(sizeof(struct pending_exception_info));
    CILK_ASSERT (NULL != pei);

    DBGPRINTF ("%d - WrapException  - allocated PEI: %p\n", w->self, pei);

    // Get the trampoline we'll use to wrap the destructor
    pTrampoline = GetTrampoline (pExceptRec->ExceptionInformation[info_image_base]);
    CILK_ASSERT(NULL != pTrampoline);

    // Save the original destructor offset so our wrapper can invoke it later
    pThrowInfo = (Win64ThrowInfo *)pExceptRec->ExceptionInformation[info_throw_data];
    pei->offset_dtor = pThrowInfo->offDtor;

    // Copy the Win64ThrowInfo into the pending_exception_info so we can redirect
    // the destructor to ours.  Replace the pointer to the Win64ThrowInfo in the
    // exception parameters with our, modified, copy
    memcpy_s (&pei->copy_ThrowInfo, sizeof(Win64ThrowInfo), pThrowInfo, sizeof(Win64ThrowInfo));

    trampoline_dtor_offset = (ULONG64)pTrampoline -
                    pExceptRec->ExceptionInformation[info_image_base];
    CILK_ASSERT (HIDWORD(trampoline_dtor_offset) == 0);
    pei->copy_ThrowInfo.offDtor = LODWORD(trampoline_dtor_offset);

    // Make sure we haven't just created an infinite loop by saving the
    // offset to the trampoline
    if (pei->offset_dtor == trampoline_dtor_offset)
    {
        CILK_ASSERT((w->l->pending_exception &&
                     w->l->pending_exception->nested_exception_found));
        pei->offset_dtor = w->l->pending_exception->offset_dtor;
    }

    DBGPRINTF ("%d - WrapException  - pei->offset_dtor: %08p, pTrampoline: %p, offset, %08x\n",
               w->self, pei->offset_dtor, pTrampoline, trampoline_dtor_offset);
    CILK_ASSERT(pei->offset_dtor != trampoline_dtor_offset);

#pragma warning(push)
#pragma warning(disable: 1684)  // conversion from pointer to same-sized integral type (potential portability problem)
    pExceptRec->ExceptionInformation[info_throw_data] = (ULONG_PTR)&pei->copy_ThrowInfo;
#pragma warning(pop)

    // Save the RIP we should load in as the return information for
    // __cilkrts_rethrow
    pei->rethrow_rip = rethrow_rip;
    pei->exception_rbp = pContext->Rbp;
    pei->exception_rsp = pContext->Rsp;

    DBGPRINTF ("%d - WrapException  - rethrow_rip set to %p\n",
               w->self, pei->rethrow_rip);
    DBGPRINTF ("%d - WrapException  - exception_rbp: %p, exception_rsp %p\n",
               w->self, pei->exception_rbp, pei->exception_rsp);

    // There is an exception on this stack.  It will not be freed until
    // after the catch block, so the stack cannot be released until then,
    // even if the catch block is executed on a different stack from this
    // one.  Our destructor wrapper will determine whether it is on this
    // stack or a different one and either unset this bit or release this
    // stack (to the global cache), as appropriate.

    pei->w = w;
    pei->exception_fiber = w->l->frame_ff->fiber_self;
#if SUPPORT_GET_CURRENT_FIBER
    CILK_ASSERT(pei->exception_fiber == cilk_fiber_get_current_fiber());
#endif
    if (pei->exception_fiber) {
        cilk_fiber_add_reference(pei->exception_fiber);
    }

    // Save the exception record
    pei->pExceptRec = pExceptRec;

    // Initialize the saved_protected_tail. Exceptions will be disabled
    // until the exception is dealt with
    pei->saved_protected_tail = saved_protected_tail;
    pei->saved_protected_tail_worker_id = w->self;
    DBGPRINTF ("%d - WrapException  - stealing disallowed, saved protected tail: %p, ltq: %p, limit: %p\n",
                w->self, pei->saved_protected_tail, w->l->ltq, w->ltq_limit);

    pei->nested_exception_found = 0;
    pei->nested_exception = w->l->pending_exception;

    pei->sync_rbp = 0;
    pei->sync_rsp = 0;

    w->l->pending_exception = pei;

    // Save the pending_exception_info onto the stack.  This assumes that
    // the global unwind has already occurred, so the space on the stack
    // immediately before the exception object is no longer used.
    //
    // Why don't we simply put the pointer into the EXCEPTION_RECORD's pointer
    // to the exception object?  Because that will be used by the copy constructor,
    // and we don't want to have to wrap all of them if we don't have to.
    //
    // Since the pointer is stored immediately before the exception object, our
    // wrapper can find it when the destructor is called.

    stack = (void **)pExceptRec->ExceptionInformation[info_cpp_object];
    stack[-1] = pei;
    DBGPRINTF ("%d - WrapException  - exception wrapped for object %p pExceptRec: %p\n",
               w->self, pExceptRec->ExceptionInformation[info_cpp_object], pExceptRec);

    return pei;
}

/*
 * GlobalUnwind
 *
 * Unwind full frames on an x64 OS
 */

GlobalUnwind (__cilkrts_worker *w,
              IN PEXCEPTION_RECORD pExceptionRecordIn,
              IN ULONG64 TargetFrameIn,
              IN OUT PCONTEXT pContextIn,
              IN OUT PDISPATCHER_CONTEXT pDispatcherContextIn)
{
    CONTEXT UnwindContext;
    CONTEXT PrevUnwindContext;
	ULONG64 ControlPc;
    UNWIND_HISTORY_TABLE UnwindHistoryTable;
    ULONG64 ImageBase;
    ULONG64 EstablisherFrame;
    PEXCEPTION_ROUTINE pfnUnwindHandler;
    DISPATCHER_CONTEXT DispatcherContext;
    EXCEPTION_DISPOSITION disp;
    PRUNTIME_FUNCTION pfnRuntimeFunction;
    PVOID pvHandlerData;
    ULONG64 TargetIp;
    ULONG64 ReturnValue = 1;
    int nFrame = 1;

    TargetIp = pDispatcherContextIn->ContextRecord->Rip;

    DBGPRINTF("%d - GlobalUnwind - target frame: %p, TargetIp: %p\n",
              w->self, TargetFrameIn, TargetIp);

    // Start by saving the current context
    memcpy_s (&UnwindContext, sizeof(CONTEXT), pContextIn, sizeof(CONTEXT));
    DUMP_GU_CONTEXT (&UnwindContext, 0, NULL, "%d - GlobalUnwind - Initial context\n", w->self, 0);

    // Set the exception record flags to indicate that we're unwinding
    pExceptionRecordIn->ExceptionFlags = EXCEPTION_UNWINDING;

    // Initialize the (optional) unwind history table.
    RtlZeroMemory(&UnwindHistoryTable, sizeof(UNWIND_HISTORY_TABLE));
    UnwindHistoryTable.LowAddress = (ULONG64)-1;

    while(1)
    {
        ControlPc = UnwindContext.Rip;

        // Try to look up unwind metadata for the current function.
        pfnRuntimeFunction =
            RtlLookupFunctionEntry (ControlPc,
                                    &ImageBase,
                                    &UnwindHistoryTable);

        if (NULL == pfnRuntimeFunction)
        {
            // If we don't have a valid RUNTIME_FUNCTION, this must be a leaf
            // function.  Update the context appropriately

            UnwindContext.Rip = *(PULONG64)(UnwindContext.Rsp);
            UnwindContext.Rsp += sizeof(ULONG64);
        }
        else
        {
            // We've got a valid RUNTIME_FUNCTION.  Start by saving the current context
            memcpy_s (&PrevUnwindContext, sizeof(CONTEXT), &UnwindContext, sizeof(CONTEXT));

            // Call upon RtlVirtualUnwind to unwind the frame for us
            pfnUnwindHandler =
                RtlVirtualUnwind(UNW_FLAG_UHANDLER,
                                 ImageBase,
                                 ControlPc,
                                 pfnRuntimeFunction,
                                 &UnwindContext,
                                 &pvHandlerData,
                                 &EstablisherFrame,
    //                             &NvContext);
                                 NULL);

            // If we've unwound to the target frame, we're done
            if (TargetFrameIn == EstablisherFrame)
            {
                DBGPRINTF ("%d - Found target frame\n", w->self);
                memcpy_s (pContextIn, sizeof(CONTEXT), &PrevUnwindContext, sizeof(CONTEXT));
                return;
            }

            DUMP_GU_CONTEXT (&UnwindContext, EstablisherFrame, pfnUnwindHandler,
                             "%d - GlobalUnwind - Frame %d\n", w->self, nFrame);
            nFrame++;

            // If we've got an unwind handler, call it
            if (NULL != pfnUnwindHandler)
            {
                PrevUnwindContext.Rax = (ULONG64)ReturnValue;

                DispatcherContext.ControlPc        = ControlPc;
                DispatcherContext.ImageBase        = ImageBase;
                DispatcherContext.FunctionEntry    = pfnRuntimeFunction;
                DispatcherContext.EstablisherFrame = EstablisherFrame;
                DispatcherContext.TargetIp = TargetIp;
                DispatcherContext.ContextRecord    = &UnwindContext;
                DispatcherContext.LanguageHandler  = pfnUnwindHandler;
                DispatcherContext.HandlerData      = pvHandlerData;
                DispatcherContext.HistoryTable     = &UnwindHistoryTable;
                DispatcherContext.ScopeIndex       = 0;

                DBGPRINTF ("%d - Calling unwind handler %p\n", w->self, pfnUnwindHandler);

                disp = pfnUnwindHandler(pExceptionRecordIn,
                                        (PVOID)EstablisherFrame,
                                        &PrevUnwindContext,
                                        &DispatcherContext);

                CILK_ASSERT (ExceptionContinueSearch == disp);
            }
/*
            Swap                = ActiveContext;
            ActiveContext       = ActiveUnwindContext;
            ActiveUnwindContext = Swap;
            */
        }
    }
}

extern
EXCEPTION_DISPOSITION __CxxFrameHandler3 (IN PEXCEPTION_RECORD pExceptRec,
                     IN ULONG64 EstablisherFrame,
                     IN OUT PCONTEXT pContext,
                     IN OUT PDISPATCHER_CONTEXT pDispatcherContext);

/*
 * CallCxxFrameHandler
 *
 * Pass an exception to the C++ Frame Handler who's address is stored in the
 * __cilkrts_stack_frame.except_data field
 */

EXCEPTION_DISPOSITION
CallCxxFrameHandler (__cilkrts_stack_frame *sf,
                     IN PEXCEPTION_RECORD pExceptRec,
                     IN ULONG64 EstablisherFrame,
                     IN OUT PCONTEXT pContext,
                     IN OUT PDISPATCHER_CONTEXT pDispatcherContext)
{
    PFN_FRAME_HANDLER pfnFrameHandler;

    // If we've got __cilkrts_stack_frame, and it's got an exception handler,
    // use the exception handler the compiler passed us.  If not, default to
    // our instance.  We really should use a function from the excepting 
    // function's module, but there's no way to find it.  Our copy of 
    // __CxxFrameHandler3 should be close enough
    if ((NULL == sf) || (NULL == sf->except_data))
        pfnFrameHandler = __CxxFrameHandler3;
    else
        pfnFrameHandler = (PFN_FRAME_HANDLER)sf->except_data;

    DBGPRINTF("Calling C++ handler, ControlPC: %p\n", pDispatcherContext->ControlPc);

    return pfnFrameHandler (pExceptRec,
                            EstablisherFrame,
                            pContext,
                            pDispatcherContext);
}

/*
 * CilkEhFirstPass
 *
 * Called by __cilkrts_frame_handler when we're making the first pass over
 * the frame to determine if it will handle the exception.
 */

EXCEPTION_DISPOSITION
CilkEhFirstPass (IN __cilkrts_worker *w,
                 IN PEXCEPTION_RECORD pExceptRec,
                 IN ULONG64 EstablisherFrame,
                 IN OUT PCONTEXT pContext,
                 IN OUT PDISPATCHER_CONTEXT pDispatcherContext)
{
    __cilkrts_stack_frame *sf = NULL;
    __cilkrts_stack_frame *parent_sf;
    __cilkrts_stack_frame *volatile *volatile saved_protected_tail;
    EXCEPTION_DISPOSITION disp;
    cilk_fiber* fiber;
    char *steal_frame_sp;

    DUMP_EXCEPTION(w->self, pExceptRec);
    DUMP_FUNC_INFO(w->self, pDispatcherContext);

    // If this is a debug break, just let the exception propagate.  It will
    // get where it needs to go eventually
    if (0x80000003 == pExceptRec->ExceptionCode)
        return ExceptionContinueSearch;

    // Only C++ exceptions are handled and passed on
    if (CXX_EXCEPTION != pExceptRec->ExceptionCode)
        __cilkrts_bug("error: tried to pass SEH exception %08x through a spawn",
                      pExceptRec->ExceptionCode);

    // Disallow stealing while we're processing this exception.  Note that
    // before leaving this function, we *must* either:
    //   - pass the saved protected tail into the wrapped exception
    //   - restore stealing
    // Failure to do one of these two will result in a worker that can
    // no longer be stolen from, causing a loss of parallelism
    saved_protected_tail =  __cilkrts_disallow_stealing(w, 0);

    // Grab the current stack frame.  We need to iterate over the Cilk stack
    // frames to find one which hasn't been handled yet
    sf = w->current_stack_frame;
    while (sf)
    {
        // If this is the first time we've probed this frame, drop out
        if (0 == (sf->flags & CILK_FRAME_EXCEPTION_PROBED))
            break;

        // If this is the last frame, drop out.  We'll pass it on to the
        // C++ exception handler
        if (sf->flags & CILK_FRAME_LAST)
        {
            DBGPRINTF ("%d - CilkEhFirstPass - repeat of last frame\n", w->self);
            break;
        }

        DBGPRINTF ("%d - CilkEhFirstPass - skipping sp %p - already probed\n", w->self, sf);
        sf = sf->call_parent;
    }

    DUMP_SF(sf, "CilkEhFirstPass", w->self, pDispatcherContext);
    CILK_ASSERT (NULL != sf);

    DUMP_FH_CONTEXT (pContext, EstablisherFrame, NULL, "%d - CilkEhFirstPass - Initial Context\n", w->self, 0);
    DUMP_FH_CONTEXT (pDispatcherContext->ContextRecord, pDispatcherContext->EstablisherFrame, NULL,
                     "%d - CilkEhFirstPass - Dispatcher Context\n", w->self, 0);

    // Do not update anything for this corner case:
    // 1) exception is thrown from a function call in a recursive spawning function without spawning anything
    // 2) only one worker is executing that spawning function 
    if (sf < sf->call_parent &&
        pDispatcherContext->ContextRecord->Rbp <= (ULONG64)sf) {
        __cilkrts_restore_stealing(w, saved_protected_tail);
        return CallCxxFrameHandler (sf,
                                    pExceptRec,
                                    EstablisherFrame,
                                    pContext,
                                    pDispatcherContext);
    }
 
    // If we're unsynched, sync before we go any further
    if (sf->flags & CILK_FRAME_UNSYNCHED)
    {
        // If the SP in __cilkrts_stack_frame->ctx has been zeroed, restore
        // the value.  This can happen if the continuation has been stolen.
        DBGPRINTF ("%d - CilkEhFirstPass - SP(SF): %p\n", w->self, SP(sf));
        if (0 == SP(sf))
        {
            SP(sf) = (ULONG64)sf->worker->l->frame_ff->exception_sp;
            DBGPRINTF ("%d - CilkEhFirstPass - SP(SF) set to exception_sp: %p\n", w->self, SP(sf));
        }

        // Unwind anything on the stack below the frame we've been
        // called for.
        //
        // Note that GlobalUnwind() will set the unwind bit in the
        // exception record
        GlobalUnwind(w,
                     pExceptRec,
                     EstablisherFrame,
                     pContext,
                     pDispatcherContext);

        // If we haven't already done it, wrap up the exception for later.
        // Note that we're passing NULL for the DISPATCHER_CONTEXT.  This
        // will cause WrapException to set rethrow_ip to 0, telling
        // __cilkrts_rethrow that it should use the IP from the jmpbuf
        DBGPRINTF ("%d - CilkEhFirstPass - Unsynched - Wrap exception and sync\n", w->self);
        WrapException (w, pExceptRec, 0, pDispatcherContext->ContextRecord, saved_protected_tail);
        // TODO: Do we need to destroy local objects?

        __cilkrts_sync(sf); // Should not return

        CILK_ASSERT(! "CilkEhFirstPass - after synch - shouldn't reach this point");
    }

    // Note that we've examined this frame.  we do this after checking for
    // unsynched frames because we *know* they're going to come back and we're
    // going to want to re-examine them
    sf->flags |= CILK_FRAME_EXCEPTION_PROBED;

    // If this is the last frame, just give it to the C++ handler
    if (sf->flags & CILK_FRAME_LAST)
    {
        __cilkrts_restore_stealing(w, saved_protected_tail);
        return CallCxxFrameHandler (sf,
                                    pExceptRec,
                                    EstablisherFrame,
                                    pContext,
                                    pDispatcherContext);
    }

    // If this a spawn helper frame, the CILK_FRAME_DETACHED flag will be set
    if (CILK_FRAME_DETACHED & sf->flags)
    {
#ifdef _DEBUG
        _JUMP_BUFFER *jump_buffer = (_JUMP_BUFFER *)&sf->ctx;
#endif
        ULONG64 rethrow_rip;

       /*
        * If we are in replay mode, and a steal occurred during the recording
        * phase, stall till a steal actually occurs.
        */
        replay_wait_for_steal_if_parent_was_stolen(w);

        // See if our parent has been stolen

        parent_sf = __cilkrts_pop_tail(w);

        // If our parent hadn't been stolen, simply remove the flag and
        // let the normal probing continue.  The unwind funclet for the
        // spawn helper will update the pedigree.
        if (NULL != parent_sf)
        {
            // Set FRAME_UNWINDING which will prevent any Cilk unwind funclets
            // from doing much of anything except updating the pedigree
            sf->flags |= CILK_FRAME_UNWINDING;

            __cilkrts_restore_stealing(w, saved_protected_tail);
            DBGPRINTF ("%d - CilkEhFirstPass - Detached frame's parent %p not stolen - Pass detached first-pass frame to C++ handler\n", w->self, parent_sf);
            return CallCxxFrameHandler (sf,
                                        pExceptRec,
                                        EstablisherFrame,
                                        pContext,
                                        pDispatcherContext);
        }

        DBGPRINTF ("%d - CilkEhFirstPass - Detached frame's parent stolen - Context RIP: %p, JumpBuf RIP: %p, stolen: %d, call parent: %p\n",
                   w->self, pDispatcherContext->ContextRecord->Rip, jump_buffer->Rip,
                   (sf->flags & CILK_FRAME_STOLEN) == CILK_FRAME_STOLEN,
                   sf->call_parent);

        // Write a record to the replay log for an attempt to return to a
        // stolen parent.  This must be done before the exception handler
        // invokes __cilkrts_leave_frame which will bump the pedigree so
        // the replay_wait_for_steal_if_parent_was_stolen() above will match on
        // replay.
        replay_record_orphaned(w);

        // Unwind anything on the stack below the frame we've been
        // called for.
        //
        // Note that GlobalUnwind() will set the unwind bit in the
        // exception record

        GlobalUnwind(w,
                     pExceptRec,
                     EstablisherFrame,
                     pContext,
                     pDispatcherContext);

        // Our parent has been stolen.  We unwind everything on this stack
        // and migrate the exception to the parent.
        //
        // If we haven't already done it, wrap up the exception for later
        DBGPRINTF ("%d - CilkEhFirstPass - detached w/stolen parent - Wrap exception and migrate, sf: %p\n", w->self, sf);
        DBGPRINTF ("%d - CilkEhFirstPass - Context RIP: %p, JumpBuf RIP: %p\n",
                   w->self, pDispatcherContext->ContextRecord->Rip, jump_buffer->Rip);

        // If this frame was stolen, take the IP from the saved context in the
        // __cilkrts_stack_frame.  Otherwise, use the IP from this context record
//        if (CILK_FRAME_STOLEN == (sf->flags & CILK_FRAME_STOLEN))
#if 0
        if (WORKER_USER == w->l->type)
            rethrow_rip = 0;
        else
            rethrow_rip = pDispatcherContext->ContextRecord->Rip;
#else
            rethrow_rip = pDispatcherContext->ContextRecord->Rip;
#endif
        WrapException (w, pExceptRec, rethrow_rip, pDispatcherContext->ContextRecord, saved_protected_tail);

        // Set FRAME_UNWINDING which will prevent any Cilk unwind funclets
        // from doing much of anything

        sf->flags |= CILK_FRAME_UNWINDING;

        // Destroy any local objects.  The unwind funclet for the spawn helper
        // will call __cilkrts_leave_frame, but the CILK_FRAME_UNWINDING flag
        // will cause it to exit without doing anything

        disp = CallCxxFrameHandler (sf,
                                    pExceptRec,
                                    EstablisherFrame,
                                    pContext,
                                    pDispatcherContext);

        __cilkrts_migrate_exception(sf); /* does not return. */

        CILK_ASSERT(! "CilkEhFirstPass - after migrate - shouldn't reach this point");
    }

    // If this frame was stolen, execute a return

    if (CILK_FRAME_STOLEN & sf->flags)
    {
        __cilkrts_stack_frame *worker_current_stack_frame = w->current_stack_frame;

        // Call the C++ exception handler.  If this frame has a handler for the
        // exception, it will unwind and continue from there
        DBGPRINTF ("%d - CilkEhFirstPass - stolen frame - let C++ try to handle it\n", w->self);
        disp = CallCxxFrameHandler (sf,
                                    pExceptRec,
                                    EstablisherFrame,
                                    pContext,
                                    pDispatcherContext);

        // Call __cilkrts_return to find our parent frame
        DBGPRINTF ("%d - CilkEhFirstPass - stolen frame - return from %p\n", w->self, sf);
        __cilkrts_return(w);
        DBGPRINTF ("%d - CilkEhFirstPass - stolen frame - new current frame is %p, flags: %p\n",
                   w->self, w->current_stack_frame, w->current_stack_frame->flags);

        // Add the stack frame set by __cilkrts_return to the linked list of
        // frames
        sf->call_parent = w->current_stack_frame;

        // Restore the current_stack_frame
        w->current_stack_frame = worker_current_stack_frame;

        // Clear the STOLEN flag, since we've "unstolen" the frame and the
        // unwind funclet shouldn't do much of anything.  Setting UNWINDING
        // will guarantee that __cilkrts_leave_frame exits early
        sf->flags &= ~CILK_FRAME_STOLEN;
        sf->flags |= CILK_FRAME_UNWINDING;

        DBGPRINTF ("%d - CilkEhFirstPass - frame \"unstolen\", parent %p, continue unwinding\n",
                   w->self, sf->call_parent);

        __cilkrts_restore_stealing(w, saved_protected_tail);
        return disp;
    }


    fiber = w->l->frame_ff->fiber_self;
#if SUPPORT_GET_CURRENT_FIBER
    CILK_ASSERT(fiber == cilk_fiber_get_current_fiber());
#endif
    
    // Fetch the initial SP for the frame that stole the work from the stack
    // object.
    //    sd = GetWorkerStackObject(w);
    if (NULL == fiber)
        steal_frame_sp = NULL;
    else {
        cilk_fiber_data* fdata = cilk_fiber_get_data(fiber);
        steal_frame_sp = fdata->steal_frame_sp;
    }

    // If this is the last frame on this stack we'll need to sync.  We do this
    // this here because:
    // - Any frame we steal must have an exception handler, since the compiler
    //   should have set one up for the spawn
    // - On Win32 we can insert an exception handler into the exception handler
    //   chain.  Win64 uses address-based exception handling, so we need to
    //   watch for attempts to unwind off the stack
    if (steal_frame_sp == (char *)pDispatcherContext->ContextRecord->Rsp)
    {
        // Call the C++ exception handler.  If this frame has a handler for the
        // exception, it will unwind and continue from there
        DBGPRINTF ("%d - CilkEhFirstPass - last stack frame - let C++ try to handle it\n",
                   w->self);
        disp = CallCxxFrameHandler (sf,
                                    pExceptRec,
                                    EstablisherFrame,
                                    pContext,
                                    pDispatcherContext);

        // Peek ahead a __cilkrts_stack_frame.  It had better be unsynched,
        // since it should be the frame we stole work from
        CILK_ASSERT (NULL != sf->call_parent);
        sf = sf->call_parent;
        CILK_ASSERT (sf->flags & CILK_FRAME_UNSYNCHED);

        // If the SP in __cilkrts_stack_frame->ctx has been zeroed, restore
        // the value.  This can happen if the continuation has been stolen.
        //
        // Not sure if this is necessary...
        DBGPRINTF ("%d - CilkEhFirstPass - SP(SF): %p\n", w->self, SP(sf));
        if (0 == SP(sf))
        {
            SP(sf) = (ULONG64)sf->worker->l->frame_ff->exception_sp;
            DBGPRINTF ("%d - CilkEhFirstPass - SP(SF) set to exception_sp: %p\n", w->self, SP(sf));
        }

        // Unwind anything on the stack below the frame we've been
        // called for.
        //
        // Note that GlobalUnwind() will set the unwind bit in the
        // exception record
        GlobalUnwind(w,
                     pExceptRec,
                     EstablisherFrame,
                     pContext,
                     pDispatcherContext);

        // If we haven't already done it, wrap up the exception for later.
        // Note that we're passing NULL for the DISPATCHER_CONTEXT.  This
        // will cause WrapException to set rethrow_ip to 0, telling
        // __cilkrts_rethrow that it should use the IP from the jmpbuf
        DBGPRINTF ("%d - CilkEhFirstPass - last stack frame - Wrap exception and sync\n", w->self);
        WrapException (w, pExceptRec, 0, pDispatcherContext->ContextRecord, saved_protected_tail);

        __cilkrts_sync(sf); // Should not return

        CILK_ASSERT(! "CilkEhFirstPass - after synch - shouldn't reach this point");
    }

    // If we get here, there's nothing special to do for this frame.  Pass the
    // exception to the C++ exception handler to deal with.
    DBGPRINTF ("%d - CilkEhFirstPass - Pass first-pass frame to C++ handler\n", w->self);
    __cilkrts_restore_stealing(w, saved_protected_tail);
    return CallCxxFrameHandler (sf,
                                pExceptRec,
                                EstablisherFrame,
                                pContext,
                                pDispatcherContext);
}

/*
 * CilkEhSecondPass
 *
 * Called by __cilkrts_frame_handler when we're making the second pass over
 * the frame to unwind.
 */

EXCEPTION_DISPOSITION
CilkEhSecondPass (IN __cilkrts_worker *w,
                  IN PEXCEPTION_RECORD pExceptRec,
                  IN ULONG64 EstablisherFrame,
                  IN OUT PCONTEXT pContext,
                  IN OUT PDISPATCHER_CONTEXT pDispatcherContext)
{
    __cilkrts_stack_frame *sf = NULL;
    EXCEPTION_DISPOSITION disp;

    // Grab the current stack frame.  It should always be correct now.
    sf = w->current_stack_frame;
    DUMP_SF(sf, "CilkEhSecondPass", w->self, pDispatcherContext);

    DUMP_FH_CONTEXT (pContext, EstablisherFrame, NULL, "%d - CilkEhSecondPass - Initial Context\n", w->self, 0);
    DUMP_FH_CONTEXT (pDispatcherContext->ContextRecord, pDispatcherContext->EstablisherFrame, NULL,
                     "%d - CilkEhSecondPass - Dispatcher Context\n", w->self, 0);

    // Clear the EXCEPTION_PROBED flag so we don't confuse ourselves if we come
    // back to this frame
    sf->flags &= ~CILK_FRAME_EXCEPTION_PROBED;

    // Do not update anything for this corner case:
    // 1) exception is thrown from a function call in a recursive spawning function without spawning anything
    // 2) only one worker is executing that spawning function 
    if (sf < sf->call_parent &&
        pDispatcherContext->ContextRecord->Rbp <= (ULONG64)sf) {
    }
    // If we've messed up the destination RSP by re-raising the exception,
    // restore the original value so we don't expand the stack
    else if ((w->l->pending_exception) && (NULL == w->l->pending_exception->nested_exception))
    {
        if (pDispatcherContext->ContextRecord->Rbp == w->l->pending_exception->exception_rbp)
        {
            DBGPRINTF ("%d - exception_rbp %p matches.  Setting ContextRecord->Rsp to %p instead of %p\n", w->self,
                       w->l->pending_exception->exception_rbp,
                       w->l->pending_exception->exception_rsp,
                       pDispatcherContext->ContextRecord->Rsp);
            pDispatcherContext->ContextRecord->Rsp = w->l->pending_exception->exception_rsp;
            w->l->pending_exception->exception_rbp = 0;
            DBGPRINTF ("%d - pExceptRec %p, exception object: %p\n",
                       w->self, w->l->pending_exception->pExceptRec,
                       w->l->pending_exception->pExceptRec->ExceptionInformation[info_cpp_object]);
        }
        else
        {
            DBGPRINTF ("%d - exception_rbp %p != pContext->Rbp %p.  NOT Setting ContextRecord->Rsp to %p, Rsp remains %p\n", w->self,
                       w->l->pending_exception->exception_rbp,
                       pDispatcherContext->ContextRecord->Rbp,
                       w->l->pending_exception->exception_rsp,
                       pDispatcherContext->ContextRecord->Rsp);
        }

        if (pDispatcherContext->ContextRecord->Rbp == w->l->pending_exception->sync_rbp)
        {
            DBGPRINTF ("%d - sync_rbp %p matches.  Setting ContextRecord->Rsp to %p instead of %p\n", w->self,
                       w->l->pending_exception->sync_rbp,
                       w->l->pending_exception->sync_rsp,
                       pDispatcherContext->ContextRecord->Rsp);
            pDispatcherContext->ContextRecord->Rsp = w->l->pending_exception->sync_rsp;
            w->l->pending_exception->sync_rbp = 0;
            DBGPRINTF ("%d - pExceptRec %p, exception object: %p\n",
                       w->self, w->l->pending_exception->pExceptRec,
                       w->l->pending_exception->pExceptRec->ExceptionInformation[info_cpp_object]);
        }
        else
        {
            DBGPRINTF ("%d - sync_rbp %p != pContext->Rbp %p.  NOT Setting ContextRecord->Rsp to %p, Rsp remains %p\n", w->self,
                       w->l->pending_exception->sync_rbp,
                       pDispatcherContext->ContextRecord->Rbp,
                       w->l->pending_exception->sync_rsp,
                       pDispatcherContext->ContextRecord->Rsp);
        }
    }

    // Call the C++ handler to run any unwind funclets
    DBGPRINTF ("%d - CilkEhSecondPass - Pass second-pass frame to C++ handler\n", w->self);
    disp = CallCxxFrameHandler (sf,
                                pExceptRec,
                                EstablisherFrame,
                                pContext,
                                pDispatcherContext);

    DUMP_FH_CONTEXT (pContext, EstablisherFrame, NULL, "%d - CilkEhSecondPass - Exiting Context\n", w->self, 0);
    DUMP_FH_CONTEXT (pDispatcherContext->ContextRecord, pDispatcherContext->EstablisherFrame, NULL,
                     "%d - CilkEhSecondPass - Exiting Dispatcher Context", w->self, 0);

    return disp;
}

/*
 * __cilkrts_frame_handler
 *
 * Cilk's implementation of C++'s low-level frame exception handler.  The
 * compiler uses this instead of __CxxFrameHandler in Cilk functions and Cilk
 * spawn helper functions.
 *
 * This handler will bundle up the exception for rethrow if necessary.
 *
 * pExceptRec - Exception record being processed
 * EstablisherFrame - Frame which specified this routine as it's exception handler
 * pContext - Registers when exception was raised
 * pDispatcherContext - Additional information about this frame
 */

CILK_EXPORT
EXCEPTION_DISPOSITION
__cilkrts_frame_handler (IN PEXCEPTION_RECORD pExceptRec,
                         IN ULONG64 EstablisherFrame,
                         IN OUT PCONTEXT pContext,
                         IN OUT PDISPATCHER_CONTEXT pDispatcherContext)
{
    // Fetch the worker struct. If there isn't one yet, just pass the exception
    // to the C++ frame handler
    __cilkrts_worker *w = __cilkrts_get_tls_worker();
    if (NULL == w)
    {
        return CallCxxFrameHandler (NULL,
                                    pExceptRec,
                                    EstablisherFrame,
                                    pContext,
                                    pDispatcherContext);
    }

    if (pExceptRec->ExceptionFlags & EXCEPTION_UNWIND_FLAGS)
        return CilkEhSecondPass(w, pExceptRec, EstablisherFrame, pContext, pDispatcherContext);
    else
        return CilkEhFirstPass(w, pExceptRec, EstablisherFrame, pContext, pDispatcherContext);
}

/*
 * CheckForNewException
 *
 * Called on the first frame of a stack walk.  Tests whether the function that
 * raised the exception was in the Cilk runtime.  If it was, then we know that
 * we're dealing with a nested exception
 */

void CheckForNewException(ULONG64 Rip,
                          __cilkrts_worker *w,
                          EXCEPTION_RECORD *pExceptionRec)
{
    struct pending_exception_info *pei = w->l->pending_exception;

    // Min/Max address for the Cilk RunTime System DLL
    static ULONG64 s_ulCilkrtsMin = 0;
    static ULONG64 s_ulCilkrtsMax = 0;

    // We can't have a nested exception if we haven't aready wrapped the exception
    if (NULL == pei)
        return;

    // It's not a nested exception if this is a rethrow
    if (0 == pExceptionRec->ExceptionInformation[info_cpp_object])
        return;

    if (0 == s_ulCilkrtsMin)
    {
        DWORD dwSize;

        CILK_ASSERT (NULL != g_hmodCilkrts);
        dwSize = ModuleSize((ULONG64)g_hmodCilkrts);
        if (0 == dwSize)
            return;

        s_ulCilkrtsMin = (ULONG64)g_hmodCilkrts;
        s_ulCilkrtsMax = s_ulCilkrtsMin + dwSize;
    }

    // If the address is not inside the Cilk RunTime System DLL, then this
    // is a new exception thrown in the catch block
    w->l->pending_exception->nested_exception_found = ((Rip < s_ulCilkrtsMin) || (Rip > s_ulCilkrtsMax));
    if (0 == w->l->pending_exception->nested_exception_found)
        return;

    // If this is an exception we already know about, it's not a nested exception
    while (pei)
    {
        if (pExceptionRec->ExceptionInformation[info_cpp_object] ==
            pei->pExceptRec->ExceptionInformation[info_cpp_object])
        {
            DBGPRINTF ("%d - CheckForNewException - Found nested rethrown object %p - not a nested exception\n",
                       w->self, pExceptionRec->ExceptionInformation[info_cpp_object]);
            w->l->pending_exception->nested_exception_found = 0;
            return;
        }
        pei = pei->nested_exception;
    }

#ifdef _DEBUG
    pei = w->l->pending_exception;
    DBGPRINTF ("%d - CheckForNewException - Found nested exception for exception record %p, object at %p\n",
               w->self, pExceptionRec, pExceptionRec->ExceptionInformation[info_cpp_object]);
    while (pei)
    {
        DBGPRINTF ("%d -                                            nested exception record %p, object at %p\n",
                   w->self, pei->pExceptRec, pei->pExceptRec->ExceptionInformation[info_cpp_object]);
        pei = pei->nested_exception;
    }
#endif
}

/*
 * unhandled_exception
 *
 * Called when an exception hasn't been handled and we've run out of stack.
 * We've already run destructors up to this point, so we can't give up and
 * allow Microsoft to try to find an exception handler.
 *
 * Tell the user that they're SOL and terminate the application.
 *
 * Needless to say, this function does not return.
 */
void unhandled_exception(EXCEPTION_RECORD *pExceptionRecord)
{
    fprintf(stderr,
            "\nUnhandled Microsoft C++ exception thrown from location 0x%p.\n"
            "Application terminated.\n",
            pExceptionRecord->ExceptionAddress);
    TerminateProcess(GetCurrentProcess(), ERROR_UNHANDLED_EXCEPTION);
}

/*
 * reset_exception_probed
 *
 * Clear the CILK_FRAME_EXCEPTION_PROBED flag in the earliest stack frame
 * which has it set.  This will be called from __cilkrts_vectored_handler
 * when we detect that we're dealing with a nested exception.  We have to
 * reset the flag because the exception handler for the function will be
 * called twice; once for catch block, and once for the rest of the function.
 * If we didn't reset the flag, we'd get out of sync as we walk the list
 * of __cilkrts_stack_frames.
 */

void reset_exception_probed(__cilkrts_worker *w)
{
    __cilkrts_stack_frame *sf = w->current_stack_frame;
    __cilkrts_stack_frame *last_sf = sf;

    // Spin through the frames looking for the last one which has the
    // EXCEPTION_PROBED flag set.
    while(sf)
    {
        if (0 == (sf->flags & CILK_FRAME_EXCEPTION_PROBED))
            break;
        last_sf = sf;
        sf = sf->call_parent;
    }

    if (last_sf)
    {
        // Clear the EXCEPTION_PROBED flag
        last_sf->flags &= ~CILK_FRAME_EXCEPTION_PROBED;

        // We've consumed the RBP and shouldn't need it again
        w->sysdep->last_stackwalk_rbp = 0;

        DBGPRINTF("%d - reset_exception_probed - Cleared EXCEPTION_PROBED in sf %p\n",
                  w->self, last_sf);
    }
}

/*
 * __cilkrts_vectored_handler
 *
 * Vectored exception handlers are given a chance to handle the exception
 * before the OS does the standard structured exception handler traversal.
 *
 * The Cilk vectored exception hander allows us to walk the list of frames and
 * accept that Cilk frames may have a frame pointer that is not in the stack.
 */

LONG CALLBACK __cilkrts_vectored_handler(EXCEPTION_POINTERS *pExceptionInfo)
{
    __cilkrts_worker *w;
    CONTEXT HandlerContext;
    CONTEXT UnwindContext;
    ULONG64 ControlPc;
    UNWIND_HISTORY_TABLE UnwindHistoryTable;
    ULONG64 ImageBase;
    ULONG64 EstablisherFrame;
    PEXCEPTION_ROUTINE pfnUnwindHandler;
    DISPATCHER_CONTEXT DispatcherContext;
    EXCEPTION_DISPOSITION disp;
    PRUNTIME_FUNCTION pfnRuntimeFunction;
    PVOID pvHandlerData;
    NT_TIB *pTeb;
    ULONG64 stackBase, stackLimit;
    int nFrame = 1;
    ULONG64 saved_last_stackwalk_rbp;

    // If this is not a C++ exception, we're not interested
    if (CXX_EXCEPTION != pExceptionInfo->ExceptionRecord->ExceptionCode)
        return EXCEPTION_CONTINUE_SEARCH;

    // If this is not a Cilk thread, we're not interested
    w = __cilkrts_get_tls_worker();
    if (NULL == w)
        return EXCEPTION_CONTINUE_SEARCH;

    // Get the current stack limits
    pTeb = (NT_TIB *)NtCurrentTeb();
    stackBase = (ULONG64)pTeb->StackBase;
    stackLimit = (ULONG64)pTeb->StackLimit;

    DBGPRINTF ("\n\n*************************************************************************\n"
               "%d - __cilkrts_vectored_handler entry\n"
               "stackLimit: %p, stackBase: %p, pei on entry: %p\n",
               w->self, stackLimit, stackBase, w->l->pending_exception);

    if (w->l->pending_exception)
        CILK_ASSERT(StealingIsDisabled(w));
    else
    {
        CILK_ASSERT(! StealingIsDisabled(w));

        // Clean out the last stackwalk RBP.  If there's no pending exception,
        // there's no chance of a nested exception.  Anything in there is a
        // leftover from the last except.
        w->sysdep->last_stackwalk_rbp = 0;
    }

    // Start by saving the current context
    memcpy_s (&UnwindContext, sizeof(CONTEXT), pExceptionInfo->ContextRecord, sizeof(CONTEXT));
    DUMP_VH_CONTEXT (&UnwindContext, 0, NULL, "%d - __cilkrts_vectored_handler - Initial context\n",
                     w->self, 0);

    // Initialize the (optional) unwind history table.
    RtlZeroMemory(&UnwindHistoryTable, sizeof(UNWIND_HISTORY_TABLE));
    UnwindHistoryTable.LowAddress = (ULONG64)-1;

    while(1)
    {
        ControlPc = UnwindContext.Rip;

        // Try to look up unwind metadata for the current function.
        pfnRuntimeFunction =
            RtlLookupFunctionEntry (ControlPc,
                                    &ImageBase,
                                    &UnwindHistoryTable);

        if (NULL == pfnRuntimeFunction)
        {
            DBGPRINTF("%d - __cilkrts_vectored_handler - No runtime function for frame %d, RIP: %p, RSP: %p, assuming leaf function\n",
                      w->self, nFrame, UnwindContext.Rip, UnwindContext.Rsp);
            // If we don't have a valid RUNTIME_FUNCTION, this must be a leaf
            // function.  Update the context appropriately

            UnwindContext.Rip = *(PULONG64)(UnwindContext.Rsp);
            UnwindContext.Rsp += sizeof(ULONG64);

            // Unlike most code, it's valid for the RBP for a Cilk function to
            // be in another stack.  It is *NEVER* valid for the RSP to ever be
            // in another stack
            if (UnwindContext.Rsp < stackLimit)
            {
                DBGPRINTF("Invalid Unwind context RSP - terminating application\n");
                // Does not return
                unhandled_exception (pExceptionInfo->ExceptionRecord);
            }
            if (UnwindContext.Rsp > stackBase)
            {
                DBGPRINTF("Invalid Unwind context RSP - terminating application\n");
                // Does not return
                unhandled_exception (pExceptionInfo->ExceptionRecord);
            }
        }
        else
        {
            // Unlike most code, it's valid for the RBP for a Cilk function to
            // be in another stack.  It is *NEVER* valid for the RSP to ever be
            // in another stack
            if (UnwindContext.Rsp < stackLimit)
            {
                DBGPRINTF("Invalid Unwind context RSP - terminating application\n");
                // Does not return
                unhandled_exception (pExceptionInfo->ExceptionRecord);
            }
            if (UnwindContext.Rsp > stackBase)
            {
                DBGPRINTF("Invalid Unwind context RSP - terminating application\n");
                // Does not return
                unhandled_exception (pExceptionInfo->ExceptionRecord);
            }

            // Call upon RtlVirtualUnwind to unwind the frame for us
            // and give us the unwind handler
            pfnUnwindHandler =
                RtlVirtualUnwind(UNW_FLAG_UHANDLER,
                                 ImageBase,
                                 ControlPc,
                                 pfnRuntimeFunction,
                                 &UnwindContext,
                                 &pvHandlerData,
                                 &EstablisherFrame,
                                 NULL);

            // If this is the first frame, see if it's in the CilkRTS dll.
            // If it is, then this exception was re-raised by us, so we're
            // still processing the same exception.  If it's not, and there's
            // an existing pending exception, we need to note that this is a
            // new exception

            if (1 == nFrame)
            {
                CheckForNewException(UnwindContext.Rip, w, pExceptionInfo->ExceptionRecord);
            }

            DUMP_VH_CONTEXT (&UnwindContext, EstablisherFrame, pfnUnwindHandler,
                             "%d - __cilkrts_vectored_handler - frame %d\n", w->self, nFrame);

            // Validate frame alignment
            if (EstablisherFrame & 7)
            {
                DBGPRINTF("Invalid frame alignment - terminating application\n");
                // Does not return
                unhandled_exception (pExceptionInfo->ExceptionRecord);
            }

            // Check if this is a nested exception
            if ((UnwindContext.Rbp == w->sysdep->last_stackwalk_rbp) &&
                (UnwindContext.Rbp > stackLimit) &&
                (UnwindContext.Rbp < stackBase))
            {
                DBGPRINTF("%d - __cilkrts_vectored_handler - Found matching RBP in frame %d - nested exception\n",
                          w->self, nFrame);

                // If this is a nested exception, we've already set the
                // EXCEPTION_PROBED flag.  Reset it so we don't get out
                // of sync
                reset_exception_probed (w);
            }

            // If we've got an unwind handler, call it
            if (NULL != pfnUnwindHandler)
            {
                memcpy_s (&HandlerContext, sizeof(CONTEXT), pExceptionInfo->ContextRecord, sizeof(CONTEXT));

                saved_last_stackwalk_rbp = w->sysdep->last_stackwalk_rbp;
                w->sysdep->last_stackwalk_rbp = UnwindContext.Rbp;

                DispatcherContext.ControlPc        = ControlPc;
                DispatcherContext.ImageBase        = ImageBase;
                DispatcherContext.FunctionEntry    = pfnRuntimeFunction;
                DispatcherContext.EstablisherFrame = EstablisherFrame;
                DispatcherContext.TargetIp = 0;    // Don't have one yet?
                DispatcherContext.ContextRecord    = &UnwindContext;
                DispatcherContext.LanguageHandler  = pfnUnwindHandler;
                DispatcherContext.HandlerData      = pvHandlerData;
                DispatcherContext.HistoryTable     = &UnwindHistoryTable;
                DispatcherContext.ScopeIndex       = 0;

                DBGPRINTF ("%d - __cilkrts_vectored_handler - Frame %d - Calling exception handler %p\n",
                           w->self, nFrame, pfnUnwindHandler);

                disp = pfnUnwindHandler(pExceptionInfo->ExceptionRecord,
                                        (PVOID)EstablisherFrame,
                                        &HandlerContext,
                                        &DispatcherContext);

                DBGPRINTF ("%d - __cilkrts_vectored_handler - Frame %d - exception handler %p returned %d\n",
                           w->self, nFrame, pfnUnwindHandler, disp);
                CILK_ASSERT (ExceptionContinueSearch == disp);

                w->sysdep->last_stackwalk_rbp = saved_last_stackwalk_rbp;
            }
            nFrame++;
        }
    }
}

/* End except-win64.c */

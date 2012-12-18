//===----- CGCilkPlusRuntime.cpp - Interface to the Cilk Plus Runtime -----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This provides Cilk Plus code generation. The purpose of the runtime is to
// encapsulate everything for Cilk spawn/sync/for. This includes making calls
// to the cilkrts library and generating spawn helper functions.
//
//===----------------------------------------------------------------------===//
#include "CGCilkPlusRuntime.h"
#include "CodeGenFunction.h"
#include "clang/AST/Stmt.h"
#include "llvm/Analysis/RegionInfo.h"
#include "llvm/Attributes.h"
#include "llvm/InlineAsm.h"
#include "llvm/Intrinsics.h"
#include "llvm/Function.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/CodeExtractor.h"
#include "llvm/TypeBuilder.h"
#include "llvm/ValueSymbolTable.h"

namespace {

typedef void *__CILK_JUMP_BUFFER[5];

struct __cilkrts_pedigree {};
struct __cilkrts_stack_frame {};
struct __cilkrts_worker {};

enum {
  __CILKRTS_ABI_VERSION = 1
};

enum {
  CILK_FRAME_STOLEN           =    0x01,
  CILK_FRAME_UNSYNCHED        =    0x02,
  CILK_FRAME_DETACHED         =    0x04,
  CILK_FRAME_EXCEPTION_PROBED =    0x08,
  CILK_FRAME_EXCEPTING        =    0x10,
  CILK_FRAME_LAST             =    0x80,
  CILK_FRAME_EXITING          =  0x0100,
  CILK_FRAME_SUSPENDED        =  0x8000,
  CILK_FRAME_UNWINDING        = 0x10000
};

#define CILK_FRAME_VERSION (__CILKRTS_ABI_VERSION << 24)
#define CILK_FRAME_VERSION_MASK  0xFF000000
#define CILK_FRAME_FLAGS_MASK    0x00FFFFFF
#define CILK_FRAME_VERSION_VALUE(_flags) (((_flags) & CILK_FRAME_VERSION_MASK) >> 24)
#define CILK_FRAME_MBZ  (~ (CILK_FRAME_STOLEN           | \
                            CILK_FRAME_UNSYNCHED        | \
                            CILK_FRAME_DETACHED         | \
                            CILK_FRAME_EXCEPTION_PROBED | \
                            CILK_FRAME_EXCEPTING        | \
                            CILK_FRAME_LAST             | \
                            CILK_FRAME_EXITING          | \
                            CILK_FRAME_SUSPENDED        | \
                            CILK_FRAME_UNWINDING        | \
                            CILK_FRAME_VERSION_MASK))

typedef uint32_t cilk32_t;
typedef uint64_t cilk64_t;
typedef void (*__cilk_abi_f32_t)(void *data, cilk32_t low, cilk32_t high);
typedef void (*__cilk_abi_f64_t)(void *data, cilk64_t low, cilk64_t high);

typedef void (__cilkrts_enter_frame)(__cilkrts_stack_frame *sf);
typedef void (__cilkrts_enter_frame_1)(__cilkrts_stack_frame *sf);
typedef void (__cilkrts_enter_frame_fast)(__cilkrts_stack_frame *sf);
typedef void (__cilkrts_enter_frame_fast_1)(__cilkrts_stack_frame *sf);
typedef void (__cilkrts_leave_frame)(__cilkrts_stack_frame *sf);
typedef void (__cilkrts_sync)(__cilkrts_stack_frame *sf);
typedef void (__cilkrts_return_exception)(__cilkrts_stack_frame *sf);
typedef void (__cilkrts_rethrow)(__cilkrts_stack_frame *sf);
typedef __cilkrts_worker *(__cilkrts_get_tls_worker)();
typedef __cilkrts_worker *(__cilkrts_get_tls_worker_fast)();
typedef __cilkrts_worker *(__cilkrts_bind_thread_1)();
typedef void (__cilkrts_cilk_for_32)(__cilk_abi_f32_t body, void *data,
                                     cilk32_t count, int grain);
typedef void (__cilkrts_cilk_for_64)(__cilk_abi_f64_t body, void *data,
                                     cilk64_t count, int grain);

#define CILKRTS_FUNC(name, CGF) \
   CGF.CGM.CreateRuntimeFunction(llvm::TypeBuilder<__cilkrts_##name, \
                                 false>::get(CGF.getLLVMContext()), \
                                 "__cilkrts_"#name)
} // namespace

typedef std::map<llvm::LLVMContext*, llvm::StructType*> TypeBuilderCache;

namespace llvm {

// Specializations of llvm::TypeBuilder for:
//   __cilkrts_pedigree,
//   __cilkrts_worker,
//   __cilkrts_stack_frame

template <bool X>
class TypeBuilder<__cilkrts_pedigree, X> {
public:
  static StructType *get(LLVMContext &C) {
    static TypeBuilderCache cache;
    TypeBuilderCache::iterator I = cache.find(&C);
    if (I != cache.end())
      return I->second;
    StructType *Ty = StructType::create(C, "__cilkrts_pedigree");
    cache[&C] = Ty;
    Ty->setBody(
      TypeBuilder<uint64_t,            X>::get(C), // rank
      TypeBuilder<__cilkrts_pedigree*, X>::get(C), // next
      NULL);
    return Ty;
  }
  enum {
    rank,
    next
  };
};

template <bool X>
class TypeBuilder<__cilkrts_worker, X> {
public:
  static StructType *get(LLVMContext &C) {
    static TypeBuilderCache cache;
    TypeBuilderCache::iterator I = cache.find(&C);
    if (I != cache.end())
      return I->second;
    StructType *Ty = StructType::create(C, "__cilkrts_worker");
    cache[&C] = Ty;
    Ty->setBody(
      TypeBuilder<__cilkrts_stack_frame**, X>::get(C), // tail
      TypeBuilder<__cilkrts_stack_frame**, X>::get(C), // head
      TypeBuilder<__cilkrts_stack_frame**, X>::get(C), // exc
      TypeBuilder<__cilkrts_stack_frame**, X>::get(C), // protected_tail
      TypeBuilder<__cilkrts_stack_frame**, X>::get(C), // ltq_limit
      TypeBuilder<int32_t,                 X>::get(C), // self
      TypeBuilder<void*,                   X>::get(C), // g
      TypeBuilder<void*,                   X>::get(C), // l
      TypeBuilder<void*,                   X>::get(C), // reducer_map
      TypeBuilder<__cilkrts_stack_frame*,  X>::get(C), // current_stack_frame
      TypeBuilder<__cilkrts_stack_frame**, X>::get(C), // saved_protected_tail
      TypeBuilder<void*,                   X>::get(C), // sysdep
      TypeBuilder<__cilkrts_pedigree,      X>::get(C), // pedigree
      NULL);
    return Ty;
  }
  enum {
    tail,
    head,
    exc,
    protected_tail,
    ltq_limit,
    self,
    g,
    l,
    reducer_map,
    current_stack_frame,
    saved_protected_tail,
    sysdep,
    pedigree
  };
};

template <bool X>
class TypeBuilder<__cilkrts_stack_frame, X> {
public:
  static StructType *get(LLVMContext &C) {
    static TypeBuilderCache cache;
    TypeBuilderCache::iterator I = cache.find(&C);
    if (I != cache.end())
      return I->second;
    StructType *Ty = StructType::create(C, "__cilkrts_stack_frame");
    cache[&C] = Ty;
    Ty->setBody(
      TypeBuilder<uint32_t,               X>::get(C), // flags
      TypeBuilder<int32_t,                X>::get(C), // size
      TypeBuilder<__cilkrts_stack_frame*, X>::get(C), // call_parent
      TypeBuilder<__cilkrts_worker*,      X>::get(C), // worker
      TypeBuilder<void*,                  X>::get(C), // except_data
      TypeBuilder<__CILK_JUMP_BUFFER,     X>::get(C), // ctx
      TypeBuilder<uint32_t,               X>::get(C), // mxcsr
      TypeBuilder<uint16_t,               X>::get(C), // fpcsr
      TypeBuilder<uint16_t,               X>::get(C), // reserved
      TypeBuilder<__cilkrts_pedigree,     X>::get(C), // parent_pedigree
      NULL);
    return Ty;
  }
  enum {
    flags,
    size,
    call_parent,
    worker,
    except_data,
    ctx,
    mxcsr,
    fpcsr,
    reserved,
    parent_pedigree
  };
};

} // namespace llvm

using namespace clang;
using namespace CodeGen;
using namespace llvm;

// The fake runtime can be used to turn Cilk spawn/sync/for statements in the
// AST into serial code that does not require the cilkrts library. This class
// is intended to be used mainly for testing everything in Cilk before codegen.
class CGCilkPlusFakeRuntime : public CGCilkPlusRuntime {
public:
  CGCilkPlusFakeRuntime(CodeGenModule &CGM)
  : CGCilkPlusRuntime(CGM)
  {}

  ~CGCilkPlusFakeRuntime() {}

  void EmitCilkSpawn(CodeGenFunction &CGF, const CilkSpawnStmt &S)
  {
    CGF.EmitStmt(S.getSubStmt());
  }

  void EmitCilkSpawn(CodeGenFunction &CGF, const CilkSpawnCapturedStmt &S)
  {
    CGF.EmitStmt(S.getSubStmt());
  }

  void EmitCilkSync(CodeGenFunction &CGF, const CilkSyncStmt &S)
  {
    // Nothing to do.
  }

  //virtual void EmitCilkFor(CodeGenFunction &CGF, const CilkForStmt &S);
};

// Helper typedefs for cilk struct TypeBuilders.
typedef llvm::TypeBuilder<__cilkrts_stack_frame, false> StackFrameBuilder;
typedef llvm::TypeBuilder<__cilkrts_worker, false> WorkerBuilder;
typedef llvm::TypeBuilder<__cilkrts_pedigree, false> PedigreeBuilder;


static Value *GEP(CGBuilderTy &B, Value *Base, int field)
{
  return B.CreateConstInBoundsGEP2_32(Base, 0, field);
}

static
void StoreField(CGBuilderTy &B, Value *Val, Value *Dst, int field)
{
  B.CreateStore(Val, GEP(B, Dst, field));
}

static Value *LoadField(CGBuilderTy &B, Value *Src, int field)
{
  return B.CreateLoad(GEP(B, Src, field));
}

/// Emit inline assembly code to save the floating point
/// state, for x86 Only
static void EmitSaveFloatingPointState(CGBuilderTy &B, Value *SF)
{
  typedef void (AsmPrototype)(uint32_t*, uint16_t*);
  llvm::FunctionType *FTy =
    TypeBuilder<AsmPrototype, false>::get(B.getContext());

  Value *Asm = InlineAsm::get(FTy,
                              "stmxcsr $0\n\t" "fnstcw $1",
                              "*m,*m,~{dirflag},~{fpsr},~{flags}",
                              /*sideeffects*/ true);

  Value *mxcsrField = GEP(B, SF, StackFrameBuilder::mxcsr);
  Value *fpcsrField = GEP(B, SF, StackFrameBuilder::fpcsr);

  B.CreateCall2(Asm, mxcsrField, fpcsrField);
}

/// Try to find a function with the given name, creating it if it
/// doesn't already exists. If the function needed to be created
/// then return false, signifying that the caller needs to add
/// the function body
static bool GetOrCreateFunction(const char *FnName, CodeGenFunction& CGF,
                                Function *&Fn)
{
  llvm::Module &Module = CGF.CGM.getModule();
  LLVMContext &Ctx = CGF.getLLVMContext();

  Fn = Module.getFunction(FnName);

  // if the function already exists then let the
  // caller know that it is complete
  if (Fn)
    return true;

  // Otherwise we have to create it
  typedef void (cilk_function)(__cilkrts_stack_frame*);
  llvm::FunctionType *FTy = TypeBuilder<cilk_function, false>::get(Ctx);
  Fn = Function::Create(FTy, GlobalValue::InternalLinkage, FnName, &Module);

  // and let the caller know that the function is incomplete
  // and the body still needs to be added
  return false;
}

/// Return a call to the CILK_SETJMP function
static CallInst *EmitCilkSetJmp(CGBuilderTy &B, Value *SF,
                                CodeGenFunction &CGF)
{
  LLVMContext &Ctx = CGF.getLLVMContext();

  // We always want to save the floating point state too
  EmitSaveFloatingPointState(B, SF);

  llvm::Type *Int32Ty = llvm::Type::getInt32Ty(Ctx);
  llvm::Type *Int8PtrTy = llvm::Type::getInt8PtrTy(Ctx);

  // Get the buffer to store program state
  // Buffer is a void**.
  Value *Buf = GEP(B, SF, StackFrameBuilder::ctx);

  // Store the frame pointer in the 0th slot
  Value *FrameAddr =
    B.CreateCall(CGF.CGM.getIntrinsic(Intrinsic::frameaddress),
                 ConstantInt::get(Int32Ty, 0));

  Value *FrameSaveSlot = GEP(B, Buf, 0);
  B.CreateStore(FrameAddr, FrameSaveSlot);

  // Store stack pointer in the 2nd slot
  Value *StackAddr = B.CreateCall(CGF.CGM.getIntrinsic(Intrinsic::stacksave));

  Value *StackSaveSlot = GEP(B, Buf, 2);
  B.CreateStore(StackAddr, StackSaveSlot);

  // Call LLVM's EH setjmp, which is lightweight.
  Value *F = CGF.CGM.getIntrinsic(Intrinsic::eh_sjlj_setjmp);
  Buf = B.CreateBitCast(Buf, Int8PtrTy);

  return B.CreateCall(F, Buf);
}

/// Get or create a LLVM function for __cilkrts_pop_frame.
/// It is equivalent to the following C code
///
/// __cilkrts_pop_frame(__cilkrts_stack_frame *sf) {
///   sf->worker->current_stack_frame = sf->call_parent;
///   sf->call_parent = 0;
/// }
static Function *GetCilkPopFrameFn(CodeGenFunction &CGF)
{
  Function *Fn = 0;

  if (GetOrCreateFunction("__cilkrts_pop_frame", CGF, Fn))
    return Fn;

  // If we get here we need to add the function body
  LLVMContext &Ctx = CGF.getLLVMContext();

  Value *SF = Fn->arg_begin();

  BasicBlock *Entry = BasicBlock::Create(Ctx, "entry", Fn);
  CGBuilderTy B(Entry);

  // sf->worker->current_stack_frame = sf.call_parent;
  StoreField(B,
    LoadField(B, SF, StackFrameBuilder::call_parent),
    LoadField(B, SF, StackFrameBuilder::worker),
    WorkerBuilder::current_stack_frame);

  // sf->call_parent = 0;
  StoreField(B,
    Constant::getNullValue(TypeBuilder<__cilkrts_stack_frame*, false>::get(Ctx)),
    SF, StackFrameBuilder::call_parent);

  B.CreateRetVoid();

  return Fn;
}

/// Get or create a LLVM function for __cilkrts_detach.
/// It is equivalent to the following C code
///
/// void __cilkrts_detach(struct __cilkrts_stack_frame *sf) {
///   struct __cilkrts_worker *w = sf->worker;
///   struct __cilkrts_stack_frame *volatile *tail = w->tail;
///
///   sf->spawn_helper_pedigree = w->pedigree;
///   sf->call_parent->parent_pedigree = w->pedigree;
///
///   w->pedigree.rank = 0;
///   w->pedigree.next = &sf->spawn_helper_pedigree;
///
///   *tail++ = sf->call_parent;
///   w->tail = tail;
///
///   sf->flags |= CILK_FRAME_DETACHED;
/// }
static Function *GetCilkDetachFn(CodeGenFunction &CGF)
{
  Function *Fn = 0;

  if (GetOrCreateFunction("__cilkrts_detach", CGF, Fn))
    return Fn;

  // If we get here we need to add the function body
  LLVMContext &Ctx = CGF.getLLVMContext();

  Value *SF = Fn->arg_begin();

  BasicBlock *Entry = BasicBlock::Create(Ctx, "entry", Fn);
  CGBuilderTy B(Entry);

  // struct __cilkrts_worker *w = sf->worker;
  Value *W = LoadField(B, SF, StackFrameBuilder::worker);

  // __cilkrts_stack_frame *volatile *tail = w->tail;
  Value *Tail = LoadField(B, W, WorkerBuilder::tail);

  // sf->spawn_helper_pedigree = w->pedigree;
  StoreField(B,
             LoadField(B, W, WorkerBuilder::pedigree),
             SF, StackFrameBuilder::parent_pedigree);

  // sf->call_parent->parent_pedigree = w->pedigree;
  StoreField(B,
             LoadField(B, W, WorkerBuilder::pedigree),
             LoadField(B, SF, StackFrameBuilder::call_parent),
             StackFrameBuilder::parent_pedigree);

  // w->pedigree.rank = 0;
  {
    StructType *STy = PedigreeBuilder::get(Ctx);
    llvm::Type *Ty = STy->getElementType(PedigreeBuilder::rank);
    StoreField(B,
               ConstantInt::get(Ty, 0),
               GEP(B, W, WorkerBuilder::pedigree),
               PedigreeBuilder::rank);
  }

  // w->pedigree.next = &sf->spawn_helper_pedigree;
  StoreField(B,
             GEP(B, SF, StackFrameBuilder::parent_pedigree),
             GEP(B, W, WorkerBuilder::pedigree),
             PedigreeBuilder::next);

  // *tail++ = sf->call_parent;
  B.CreateStore(LoadField(B, SF, StackFrameBuilder::call_parent), Tail);
  Tail = B.CreateConstGEP1_32(Tail, 1);

  // w->tail = tail;
  StoreField(B, Tail, W, WorkerBuilder::tail);

  // sf->flags |= CILK_FRAME_DETACHED;
  {
    Value *F = LoadField(B, SF, StackFrameBuilder::flags);
    F = B.CreateOr(F, ConstantInt::get(F->getType(), CILK_FRAME_DETACHED));
    StoreField(B, F, SF, StackFrameBuilder::flags);
  }

  B.CreateRetVoid();

  return Fn;
}

/// Get or create a LLVM function for __cilkrts_detach.
/// Calls to this function is always inlined, as it saves
/// the current stack/frame pointer values
/// It is equivalent to the following C code
///
/// void __cilk_sync(struct __cilkrts_stack_frame *sf) {
///   if (sf->flags & CILK_FRAME_UNSYNCHED) {
///     sf->parent_pedigree = sf->worker->pedigree;
///     SAVE_FLOAT_STATE(*sf);
///     if (!CILK_SETJMP(sf->ctx))
///       __cilkrts_sync(sf);
///     else if (sf->flags & CILK_FRAME_EXCEPTING)
///       __cilkrts_rethrow(sf);
///   }
///   ++sf->worker->pedigree.rank;
/// }
static Function *GetCilkSyncFn(CodeGenFunction &CGF)
{
  Function *Fn = 0;

  if (GetOrCreateFunction("__cilk_sync", CGF, Fn))
    return Fn;

  // If we get here we need to add the function body
  LLVMContext &Ctx = CGF.getLLVMContext();

  Value *SF = Fn->arg_begin();

  BasicBlock *Entry = BasicBlock::Create(Ctx, "cilk.sync.test", Fn),
             *SaveState = BasicBlock::Create(Ctx, "cilk.sync.savestate", Fn),
             *SyncCall = BasicBlock::Create(Ctx, "cilk.sync.runtimecall", Fn),
             *Excepting = BasicBlock::Create(Ctx, "cilk.sync.excepting", Fn),
             *Rethrow = BasicBlock::Create(Ctx, "cilk.sync.rethrow", Fn),
             *Exit = BasicBlock::Create(Ctx, "cilk.sync.end", Fn);

  // Entry
  {
    CGBuilderTy B(Entry);

    // if (sf->flags & CILK_FRAME_UNSYNCHED)
    Value *Flags = LoadField(B, SF, StackFrameBuilder::flags);
    Flags = B.CreateAnd(Flags,
                        ConstantInt::get(Flags->getType(),
                                         CILK_FRAME_UNSYNCHED));
    Value *Zero = ConstantInt::get(Flags->getType(), 0);
    Value *Unsynced = B.CreateICmpEQ(Flags, Zero);
    B.CreateCondBr(Unsynced, Exit, SaveState);
  }

  // SaveState
  {
    CGBuilderTy B(SaveState);

    // sf.parent_pedigree = sf.worker->pedigree;
    StoreField(B,
      LoadField(B, LoadField(B, SF, StackFrameBuilder::worker),
                WorkerBuilder::pedigree),
      SF, StackFrameBuilder::parent_pedigree);

    // if (!CILK_SETJMP(sf.ctx))
    Value *C = EmitCilkSetJmp(B, SF, CGF);
    C = B.CreateICmpEQ(C, ConstantInt::get(C->getType(), 0));
    B.CreateCondBr(C, SyncCall, Excepting);
  }

  // SyncCall
  {
    CGBuilderTy B(SyncCall);

    // __cilkrts_sync(&sf);
    B.CreateCall(CILKRTS_FUNC(sync, CGF), SF);
    B.CreateBr(Exit);
  }

  // Excepting
  {
    CGBuilderTy B(Excepting);
    Value *Flags = LoadField(B, SF, StackFrameBuilder::flags);
    Flags = B.CreateAnd(Flags,
                        ConstantInt::get(Flags->getType(),
                                         CILK_FRAME_EXCEPTING));
    Value *Zero = ConstantInt::get(Flags->getType(), 0);
    Value *C = B.CreateICmpEQ(Flags, Zero);
    B.CreateCondBr(C, Exit, Rethrow);
  }

  // Rethrow
  {
    CGBuilderTy B(Rethrow);
    B.CreateCall(CILKRTS_FUNC(rethrow, CGF), SF);
    B.CreateBr(Exit);
  }

  // Exit
  {
    CGBuilderTy B(Exit);

    // ++sf.worker->pedigree.rank;
    Value *Rank = LoadField(B, SF, StackFrameBuilder::worker);
    Rank = GEP(B, Rank, WorkerBuilder::pedigree);
    Rank = GEP(B, Rank, PedigreeBuilder::rank);
    B.CreateStore(B.CreateAdd(B.CreateLoad(Rank),
                  ConstantInt::get(Rank->getType()->getPointerElementType(),
                                   1)),
                  Rank);
    B.CreateRetVoid();
  }

  Fn->addFnAttr(Attributes::AlwaysInline);

  return Fn;
}

/// Get or create a LLVM function for __cilk_parent_prologue.
/// It is equivalent to the following C code
///
///  void __cilk_parent_prologue(__cilkrts_stack_frame *sf) {
///    __cilkrts_enter_frame_1(sf);
///  }
static Function *GetCilkParentPrologue(CodeGenFunction &CGF)
{
  Function *Fn = 0;

  if (GetOrCreateFunction("__cilk_parent_prologue", CGF, Fn))
    return Fn;

  // If we get here we need to add the function body
  LLVMContext &Ctx = CGF.getLLVMContext();

  Value *SF = Fn->arg_begin();

  BasicBlock *Entry = BasicBlock::Create(Ctx, "entry", Fn);
  CGBuilderTy B(Entry);

  // __cilkrts_enter_frame_1(sf)
  B.CreateCall(CILKRTS_FUNC(enter_frame_1, CGF), SF);

  B.CreateRetVoid();

  return Fn;
}

/// Get or create a LLVM function for __cilk_parent_epilogue.
/// It is equivalent to the following C code
///
///  void __cilk_parent_epilogue(__cilkrts_stack_frame *sf) {
///    __cilkrts_pop_frame(sf);
///    if (sf->flags != CILK_FRAME_VERSION)
///      __cilkrts_leave_frame(sf);
///  }
static Function *GetCilkParentEpilogue(CodeGenFunction &CGF)
{
  Function *Fn = 0;

  if (GetOrCreateFunction("__cilk_parent_epilogue", CGF, Fn))
    return Fn;

  // If we get here we need to add the function body
  LLVMContext &Ctx = CGF.getLLVMContext();

  Value *SF = Fn->arg_begin();

  BasicBlock *Entry = BasicBlock::Create(Ctx, "entry", Fn),
             *B1 = BasicBlock::Create(Ctx, "", Fn),
             *Exit  = BasicBlock::Create(Ctx, "exit", Fn);

  // Entry
  {
    CGBuilderTy B(Entry);

    // __cilkrts_pop_frame(sf)
    B.CreateCall(GetCilkPopFrameFn(CGF), SF);

    // if (sf->flags != CILK_FRAME_VERSION)
    Value *Flags = LoadField(B, SF, StackFrameBuilder::flags);
    Value *Cond = B.CreateICmpNE(Flags,
      ConstantInt::get(Flags->getType(), CILK_FRAME_VERSION));
    B.CreateCondBr(Cond, B1, Exit);
  }

  // B1
  {
    CGBuilderTy B(B1);

    // __cilkrts_leave_frame(sf);
    B.CreateCall(CILKRTS_FUNC(leave_frame, CGF), SF);
    B.CreateBr(Exit);
  }

  // Exit
  {
    CGBuilderTy B(Exit);
    B.CreateRetVoid();
  }

  return Fn;
}

/// Get or create a LLVM function for __cilk_helper_prologue.
/// It is equivalent to the following C code
///
///  void __cilk_helper_prologue(__cilkrts_stack_frame *sf) {
///    __cilkrts_enter_frame_fast_1(sf);
///    __cilkrts_detach(sf);
///  }
static llvm::Function *GetCilkHelperPrologue(CodeGenFunction &CGF)
{
  Function *Fn = 0;

  if (GetOrCreateFunction("__cilk_helper_prologue", CGF, Fn))
    return Fn;

  // If we get here we need to add the function body
  LLVMContext &Ctx = CGF.getLLVMContext();

  Value *SF = Fn->arg_begin();

  BasicBlock *Entry = BasicBlock::Create(Ctx, "entry", Fn);
  CGBuilderTy B(Entry);

  // __cilkrts_enter_frame_fast_1(sf);
  B.CreateCall(CILKRTS_FUNC(enter_frame_fast_1, CGF), SF);

  // __cilkrts_detach(sf);
  B.CreateCall(GetCilkDetachFn(CGF), SF);

  B.CreateRetVoid();

  return Fn;
}

/// Get or create a LLVM function for __cilk_helper_epilogue.
/// It is equivalent to the following C code
///
///  void __cilk_helper_epilogue(__cilkrts_stack_frame *sf) {
///    __cilkrts_pop_frame(sf);
///    __cilkrts_leave_frame(sf);
///  }
static llvm::Function *GetCilkHelperEpilogue(CodeGenFunction &CGF)
{
  Function *Fn = 0;

  if (GetOrCreateFunction("__cilk_helper_epilogue", CGF, Fn))
    return Fn;

  // If we get here we need to add the function body
  LLVMContext &Ctx = CGF.getLLVMContext();

  Value *SF = Fn->arg_begin();

  BasicBlock *Entry = BasicBlock::Create(Ctx, "entry", Fn);
  CGBuilderTy B(Entry);

  // __cilkrts_pop_frame(sf);
  B.CreateCall(GetCilkPopFrameFn(CGF), SF);

  // __cilkrts_leave_frame(sf);
  B.CreateCall(CILKRTS_FUNC(leave_frame, CGF), SF);

  B.CreateRetVoid();

  return Fn;
}

/// Get or initialize a __cilkrts_stack_frame for the spawning function.
/// If the stack frame needed to be allocated then inserts a calls to
/// initialize and de-initialize it
static llvm::Value *GetParentStackFrame(CodeGenFunction &CGF)
{
  const char *Name = "__cilkrts_sf";
  llvm::Value *V = CGF.CurFn->getValueSymbolTable().lookup(Name);

  // if the variable already exists then return the reference
  if (V)
    return V;

  // otherwise we have to allocate it
  llvm::LLVMContext &Ctx = CGF.getLLVMContext();

  llvm::Type *SFTy = StackFrameBuilder::get(Ctx);
  llvm::AllocaInst *SF = CGF.CreateTempAlloca(SFTy);
  SF->setName(Name);

  // we also need to initialize it by adding the prologue
  // to the top of the spawning function
  {
    CGBuilderTy Builder(CGF.AllocaInsertPt);
    Builder.CreateCall(GetCilkParentPrologue(CGF), SF);
  }

  // and destruct it by adding the epilogue function right
  // before the spawning function returns
  {
    CGBuilderTy Builder(CGF.ReturnBlock.getBlock());

    // By adding a _Cilk_sync call in the Unified Return
    // Block we should automatically get an implicit sync at the
    // end of all spawning functions.
    //
    // This will not handle other types of implicit syncs, such as
    // syncing at the end of a try statement, before a throw
    // statement, syncing before abnormal exit due to an exception, or
    // when dealing with _Cilk_for. It only handles the most basic
    // case of syncing before you exit a spawing function normally
    //
    // Destructors are called before the implicit sync, as expected
    //
    // It handles the case where a _Cilk_spawn in the last statement
    // in a function and the implicit sync is the continuation
    //
    // The return value is calculated before the implicit sync
    Builder.CreateCall(GetCilkSyncFn(CGF), SF);

    Builder.CreateCall(GetCilkParentEpilogue(CGF), SF);
  }

  return SF;
}

namespace clang {
namespace CodeGen {

CGCilkPlusRuntime::CGCilkPlusRuntime(CodeGenModule &CGM)
  : CGM(CGM)
{
}

CGCilkPlusRuntime::~CGCilkPlusRuntime()
{
}

// Note: this does codegen for the old CilkSpawnStmt
// It cannot handle exceptions, and will be removed as
// soon as captured statements can handle all use cases
void
CGCilkPlusRuntime::EmitCilkSpawn(CodeGenFunction &CGF,
                                 const CilkSpawnStmt &S)
{
  // Get or initialize the __cilkrts_stack_frame
  Value *SF = GetParentStackFrame(CGF);

  BasicBlock *Entry = CGF.createBasicBlock("cilk.spawn.savestate"),
             *Body = CGF.createBasicBlock("cilk.spawn.helpercall"),
             *Exit  = CGF.createBasicBlock("cilk.spawn.continuation");

  CGF.EmitBlock(Entry);

  {
    CGBuilderTy B(Entry);

    // Need to save state before spawning
    Value *C = EmitCilkSetJmp(B, SF, CGF);
    C = B.CreateICmpEQ(C, ConstantInt::get(C->getType(), 0));
    B.CreateCondBr(C, Body, Exit);
  }

  CGF.EmitBlock(Body);
  {
    CGF.SetEmittingCilkSpawn(true);
    CGF.EmitStmt(S.getSubStmt());
    CGF.SetEmittingCilkSpawn(false);
  }
  CGF.EmitBlock(Exit);

  // Extract the spawn statement into a spawn helper function
  RegionInfo RI;
  DominatorTree DT;
  Region R(Body, Exit, &RI, &DT);
  Function *H = CodeExtractor(DT, *R.getNode(), false).extractCodeRegion();

  if (!H)
    CGF.ErrorUnsupported(&S,
        "unsupported spawning call; cannot extract into a function");

  H->addFnAttr(Attributes::NoInline);
  H->setName("__cilk_spawn_helper");

  // Add a __cilkrts_stack_frame to the helper
  BasicBlock &BB = H->getEntryBlock();
  CGBuilderTy Builder(&BB, BB.begin());

  llvm::Type *SFTy = StackFrameBuilder::get(CGF.getLLVMContext());
  Value *HelperSF = Builder.CreateAlloca(SFTy);

  // Find the location to insert the init/detach call for the stack frame
  Function *SpawnPointMarkerFn
    = CGF.CGM.getModule().getFunction("__cilk_spawn_point");

  if (!SpawnPointMarkerFn)
    CGF.ErrorUnsupported(&S,
        "unsupported spawning call; cannot find the spawning point");

  assert(SpawnPointMarkerFn->hasOneUse() &&
         "Multiple uses of spawn pointer marked function");

  CallInst *Call = cast<CallInst>(SpawnPointMarkerFn->use_back());

  // Pass the stack frame
  Instruction *New
    = CallInst::Create(GetCilkHelperPrologue(CGF),
                       ArrayRef<Value*>(HelperSF), "", Call);

  // Replace the call to the marker function with a call to the
  // stack frame init/detach function
  Call->replaceAllUsesWith(New);
  Call->eraseFromParent();
  SpawnPointMarkerFn->eraseFromParent();

  // Add a call to the epilogue right before the function returns
  // We have to do this for all BasicBlocks that have a return statement
  for (Function::iterator I = H->begin(), E = H->end(); I != E; ++I) {
    if (!isa<ReturnInst>(I->getTerminator()))
      continue;

    CGBuilderTy B(&I->getInstList().back());
    B.CreateCall(GetCilkHelperEpilogue(CGF), HelperSF);
  }
}

void
CGCilkPlusRuntime::EmitCilkSpawn(CodeGenFunction &CGF,
                                 const CilkSpawnCapturedStmt &S)
{
  // Get or initialize the __cilkrts_stack_frame
  Value *SF = GetParentStackFrame(CGF);

  const FunctionDecl *HelperDecl = S.getFunctionDecl();
  assert((HelperDecl->getNumParams() == 1) && "only one argument expected");
  assert(HelperDecl->hasBody() && "missing function body");

  const RecordDecl *RD = S.getRecordDecl();
  QualType RecordTy = CGF.getContext().getRecordType(RD);

  // Initialize the captured struct
  AggValueSlot Slot = CGF.CreateAggTemp(RecordTy, "agg.captured");
  LValue SlotLV = CGF.MakeAddrLValue(Slot.getAddr(), RecordTy,
                                     Slot.getAlignment());

  RecordDecl::field_iterator CurField = RD->field_begin();
  for (CapturedStmt::capture_const_iterator I = S.capture_begin(),
                                            E = S.capture_end();
                                            I != E; ++I, ++CurField) {
    LValue LV = CGF.EmitLValueForFieldInitialization(SlotLV, *CurField);
    ArrayRef<VarDecl *> ArrayIndexes;
    CGF.EmitInitializerForField(*CurField, LV, I->getCopyExpr(), ArrayIndexes);
  }

  // The first argument is the address of captured struct
  llvm::SmallVector<llvm::Value *, 1> Args;
  Args.push_back(SlotLV.getAddress());

  CGM.getCaptureDeclMap().insert(
      std::pair<const FunctionDecl*,
                const CilkSpawnCapturedStmt*>(HelperDecl, &S));

  // Emit the helper function
  CGM.EmitTopLevelDecl(const_cast<FunctionDecl*>(HelperDecl));
  llvm::Function *H 
    = dyn_cast<llvm::Function>(CGM.GetAddrOfFunction(GlobalDecl(HelperDecl)));

  if (!H)
    CGF.ErrorUnsupported(&S,
        "unsupported spawning call; cannot extract into a function");

  H->setLinkage(llvm::Function::InternalLinkage);

  BasicBlock *Entry = CGF.createBasicBlock("cilk.spawn.savestate"),
             *Body = CGF.createBasicBlock("cilk.spawn.helpercall"),
             *Exit  = CGF.createBasicBlock("cilk.spawn.continuation");

  CGF.EmitBlock(Entry);

  {
    CGBuilderTy B(Entry);

    // Need to save state before spawning
    Value *C = EmitCilkSetJmp(B, SF, CGF);
    C = B.CreateICmpEQ(C, ConstantInt::get(C->getType(), 0));
    B.CreateCondBr(C, Body, Exit);
  }

  CGF.EmitBlock(Body);
  {
    // Emit call to the helper function
    CGF.EmitCallOrInvoke(H, Args);
  }
  CGF.EmitBlock(Exit);

  // The helper function *cannot* be inlined
  H->addFnAttr(Attributes::NoInline);

  // Add a __cilkrts_stack_frame to the helper
  BasicBlock &BB = H->getEntryBlock();
  CGBuilderTy Builder(&BB, BB.begin());

  llvm::Type *SFTy = StackFrameBuilder::get(CGF.getLLVMContext());
  Value *HelperSF = Builder.CreateAlloca(SFTy);

  // Find the location to insert the init/detach call for the stack frame
  Function *SpawnPointMarkerFn
    = CGF.CGM.getModule().getFunction("__cilk_spawn_point");

  if (!SpawnPointMarkerFn)
    CGF.ErrorUnsupported(&S,
        "unsupported spawning call; cannot find the spawning point");

  CallInst *SpawnPointCall = 0;
  for (llvm::Value::use_iterator UI = SpawnPointMarkerFn->use_begin(),
                                 UE = SpawnPointMarkerFn->use_end();
                                 UI != UE; ++UI) {
    assert(isa<CallInst>(*UI) && "should be a call instruction");
    CallInst *Call = cast<CallInst>(*UI);
    if (Call->getParent()->getParent() == H) {
      assert(!SpawnPointCall && "should not call multiple times");
      SpawnPointCall = Call;
    }
  }
  assert(SpawnPointCall && "should call __cilk_spawn_point");

  // Replace the call to the marker function with a call to the
  // stack frame init/detach function
  CallInst::Create(GetCilkHelperPrologue(CGF),
                   ArrayRef<Value*>(HelperSF), "", SpawnPointCall);

  assert(SpawnPointCall->use_empty() && "should not have any use");
  SpawnPointCall->eraseFromParent();

  // Cleanup if there is no call to the __cilk_spawn_point
  if (SpawnPointMarkerFn->use_empty())
      SpawnPointMarkerFn->eraseFromParent();

  // Add a call to the epilogue right before the function returns
  // We have to do this for all BasicBlocks that have a return statement
  for (Function::iterator I = H->begin(), E = H->end(); I != E; ++I) {
    if (!isa<ReturnInst>(I->getTerminator()))
      continue;

    CGBuilderTy B(&I->getInstList().back());
    B.CreateCall(GetCilkHelperEpilogue(CGF), HelperSF);
  }
}

/// Emits a call to the __cilk_sync function
void
CGCilkPlusRuntime::EmitCilkSync(CodeGenFunction &CGF,
                                const CilkSyncStmt &S)
{
  // Get the __cilkrts_stackframe, creating it
  // if it doesn't already exist
  Value *SF = GetParentStackFrame(CGF);

  CGF.Builder.CreateCall(GetCilkSyncFn(CGF), SF);
}

CGCilkPlusRuntime *CreateCilkPlusRuntime(CodeGenModule &CGM)
{
  return new CGCilkPlusRuntime(CGM);
}

CGCilkPlusRuntime *CreateCilkPlusFakeRuntime(CodeGenModule &CGM)
{
  return new CGCilkPlusFakeRuntime(CGM);
}

} // namespace CodeGen
} // namespace clang

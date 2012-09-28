//===----- CGCilkPlusRuntime.cpp - Interface to the Cilk Plus Runtime -----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This provides Cilk Plus code generation
//
//===----------------------------------------------------------------------===//
#include "CGCilkPlusRuntime.h"
#include "CodeGenFunction.h"
#include "clang/AST/Stmt.h"
#include "llvm/TypeBuilder.h"
#include "llvm/InlineAsm.h"
#include "llvm/Function.h"
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
#define CILK_FRAME_MBZ  (~ (CILK_FRAME_STOLEN | \
                            CILK_FRAME_UNSYNCHED | \
                            CILK_FRAME_DETACHED | \
                            CILK_FRAME_EXCEPTION_PROBED | \
                            CILK_FRAME_EXCEPTING | \
                            CILK_FRAME_LAST | \
                            CILK_FRAME_EXITING | \
                            CILK_FRAME_SUSPENDED | \
                            CILK_FRAME_UNWINDING | \
                            CILK_FRAME_VERSION_MASK))

typedef uint32_t cilk32_t;
typedef uint64_t cilk64_t;
typedef void (*__cilk_abi_f32_t)(void *data, cilk32_t low, cilk32_t high);
typedef void (*__cilk_abi_f64_t)(void *data, cilk64_t low, cilk64_t high);

typedef void (__cilkrts_enter_frame)(__cilkrts_stack_frame* sf);
typedef void (__cilkrts_enter_frame_1)(__cilkrts_stack_frame* sf);
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

#define CILKRTS_FUNC(name, CGM) \
   CGM.CreateRuntimeFunction(llvm::TypeBuilder<__cilkrts_##name, \
                               false>::get(CGM.getLLVMContext()), \
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

class CGCilkPlusFakeRuntime : public CGCilkPlusRuntime {
public:
  CGCilkPlusFakeRuntime(CodeGenModule &CGM)
  : CGCilkPlusRuntime(CGM)
  {}

  ~CGCilkPlusFakeRuntime() {}

  LValue EmitCilkSpawn(CodeGenFunction &CGF, const CilkSpawnExpr &E)
  {
    return CGF.EmitLValue(cast<CilkSpawnExpr>(E).getSubExpr());
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

// The name of the __cilkrts_stack_frame object in a function.
static std::string StackFrameName()
{
  return "__cilkrts_sf";
}

// The name of the __cilkrts_worker* in a function.
static std::string WorkerName()
{
  return "__cilkrts_w";
}

static llvm::Value *GEP(CGBuilderTy &B, llvm::Value *Base, int field)
{
  return B.CreateConstInBoundsGEP2_32(Base, 0, field);
}

static
void StoreField(CGBuilderTy &B, llvm::Value *Val, llvm::Value *Dst, int field)
{
  B.CreateStore(Val, GEP(B, Dst, field));
}

static llvm::Value *LoadField(CGBuilderTy &B, llvm::Value *Src, int field)
{
  return B.CreateLoad(GEP(B, Src, field));
}

// Only for x86
static void EmitSaveFloatState(CGBuilderTy &Builder, llvm::Value *SF)
{
  typedef void (AsmPrototype)(uint32_t*, uint16_t*);
  llvm::FunctionType *FTy =
    llvm::TypeBuilder<AsmPrototype, false>::get(Builder.getContext());

  llvm::Value *Asm = llvm::InlineAsm::get(FTy,
                                          "stmxcsr $0\n\tfnstcw $1",
                                          "*m,*m,~{dirflag},~{fpsr},~{flags}",
                                          /*sideeffects*/ true);

  llvm::Value *mxcsrSlot = GEP(Builder, SF, StackFrameBuilder::mxcsr);
  llvm::Value *fpcsrSlot = GEP(Builder, SF, StackFrameBuilder::fpcsr);
  
  Builder.CreateCall2(Asm, mxcsrSlot, fpcsrSlot);
}

static llvm::Value *EmitCilkSetJmp(CGBuilderTy &B, llvm::Value *SF)
{
  return llvm::ConstantInt::get(llvm::Type::getInt32Ty(B.getContext()), 0);
}

/// Generate a function that implements the cilk epilogue. The function
/// implements the following C code:
///
/// void __cilk_epilogue(__cilkrts_stack_frame *sf)
/// {
///   sf->worker->current_stack_frame = sf->call_parent;
///   sf->call_parent = 0;
///   if (sf->flags != CILK_FRAME_VERSION)
///     __cilkrts_leave_frame(sf);
/// }
static void EmitCilkEpilogue(CodeGenFunction &CGF, llvm::Value *SF)
{
  using namespace llvm;

  LLVMContext &Ctx = CGF.getLLVMContext();
  llvm::Module &Module = CGF.CGM.getModule();

  // Get or create the cilk prologue function in the current module.
  const std::string EpilogueName = "__cilk_epilogue";
  Function *Epilogue = Module.getFunction(EpilogueName);
  if (!Epilogue) {
    typedef void (__cilk_epilogue)(__cilkrts_stack_frame*);
    llvm::FunctionType *FTy = TypeBuilder<__cilk_epilogue, false>::get(Ctx);
    Epilogue = Function::Create(FTy,
                                GlobalValue::InternalLinkage,
                                EpilogueName,
                                &Module);

    BasicBlock *Entry = BasicBlock::Create(Ctx, "entry", Epilogue),
               *B1 = BasicBlock::Create(Ctx, "", Epilogue),
               *Exit = BasicBlock::Create(Ctx, "exit", Epilogue);

    Value *SF = Epilogue->arg_begin();
    { // Entry
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
      // if (sf->flags != CILK_FRAME_VERSION)
      Value *Flags = LoadField(B, SF, StackFrameBuilder::flags);
      Value *Cond = B.CreateICmpNE(Flags,
        ConstantInt::get(Flags->getType(), CILK_FRAME_VERSION));
      B.CreateCondBr(Cond, B1, Exit);
    }
    { // B1
      CGBuilderTy B(B1);
      // __cilkrts_leave_frame(sf);
      B.CreateCall(CILKRTS_FUNC(leave_frame, CGF.CGM), SF);
      B.CreateBr(Exit);
    }
    { // Exit
      CGBuilderTy B(Exit);
      B.CreateRetVoid();
    }
  }

  CGBuilderTy B(CGF.ReturnBlock.getBlock());
  B.CreateCall(Epilogue, SF);
}

/// Generate a function that implements the cilk prologue. The function
/// implements the following C code:
///
/// void __cilk_prologue(__cilkrts_stack_frame *sf, __cilkrts_worker** w)
/// {
///   *w = __cilkrts_get_tls_worker();
///   if (!(*w)) {
///     *w = __cilkrts_bind_thread_1();
///     sf->flags = CILK_FRAME_LAST;
///   } else {
///     sf->flags = 0;
///   }
///   sf->call_parent = (*w)->current_stack_frame;
///   sf->worker = *w;
///   sf->flags |= CILK_FRAME_VERSION;
///   sf->reserved = 0;
///   (*w)->current_stack_frame = sf;
/// }
static llvm::Function *
EmitCilkPrologueFunction(const std::string &Name, CodeGenModule &CGM)
{
  using namespace llvm;
  llvm::Module &Module = CGM.getModule();
  LLVMContext &Ctx = Module.getContext();

  typedef void (__cilk_prologue)(__cilkrts_stack_frame*, __cilkrts_worker**);
  llvm::FunctionType *FTy = TypeBuilder<__cilk_prologue, false>::get(Ctx);
  Function *F = Function::Create(FTy,
                                 GlobalValue::InternalLinkage,
                                 Name,
                                 &Module);

  BasicBlock *Entry = BasicBlock::Create(Ctx, "entry", F),
             *B1 = BasicBlock::Create(Ctx, "", F),
             *B2 = BasicBlock::Create(Ctx, "", F),
             *Exit = BasicBlock::Create(Ctx, "exit", F);

  Value *SF = F->arg_begin();
  Value *W = ++F->arg_begin();
  Value *Flags = 0;
  llvm::Type *FlagsTy = 0;
  { // Entry
    CGBuilderTy B(Entry);
    Flags = GEP(B, SF, StackFrameBuilder::flags);
    FlagsTy = Flags->getType()->getPointerElementType();
    // __cilkrts_worker *w = __cilkrts_get_tls_worker();
    B.CreateStore(B.CreateCall(CILKRTS_FUNC(get_tls_worker, CGM)), W);
    // if (!w) {
    B.CreateCondBr(B.CreateIsNull(B.CreateLoad(W)), B1, B2);
  }
  { // B1
    CGBuilderTy B(B1);
    // w = __cilkrts_bind_thread_1();
    B.CreateStore(B.CreateCall(CILKRTS_FUNC(bind_thread_1, CGM)), W);
    // sf.flags = CILK_FRAME_LAST;
    B.CreateStore(ConstantInt::get(FlagsTy, CILK_FRAME_UNSYNCHED),
                  Flags);
    B.CreateBr(Exit);
  }
  { // B2
    CGBuilderTy B(B2);
    // sf.flags = 0;
    B.CreateStore(ConstantInt::get(FlagsTy, 0), Flags);
    B.CreateBr(Exit);
  }
  { // Exit
    CGBuilderTy B(Exit);
    W = B.CreateLoad(W);
    // sf.call_parent = w->current_stack_frame;
    StoreField(B,
      LoadField(B, W, WorkerBuilder::current_stack_frame),
      SF, StackFrameBuilder::call_parent);
    // sf.worker = w;
    StoreField(B, W, SF, StackFrameBuilder::worker);
    // sf.flags |= CILK_FRAME_VERSION;
    B.CreateStore(B.CreateOr(B.CreateLoad(Flags),
                             ConstantInt::get(FlagsTy, CILK_FRAME_VERSION)),
                  Flags);
    // sf.reserved = 0;
    Value *V = GEP(B, SF, StackFrameBuilder::reserved);
    B.CreateStore(ConstantInt::get(V->getType()->getPointerElementType(), 0), V);
    // w->current_stack_frame = &sf;
    StoreField(B, SF, W, WorkerBuilder::current_stack_frame);
    B.CreateRetVoid();
  }
  return F;
}

static void EmitCilkPrologue(CodeGenFunction &CGF)
{
  llvm::LLVMContext &Ctx = CGF.getLLVMContext();
  llvm::Module &Module = CGF.CGM.getModule();

  // Allocate the cilk stack frame and worker.
  llvm::Type *SFTy = StackFrameBuilder::get(Ctx);
  llvm::AllocaInst *SF = CGF.CreateTempAlloca(SFTy);
  llvm::Type *WTy = WorkerBuilder::get(Ctx)->getPointerTo();
  llvm::AllocaInst *W = CGF.CreateTempAlloca(WTy);
  SF->setName(StackFrameName());
  W->setName(WorkerName());

  // Get or create the cilk prologue function in the current module.
  const std::string PrologueName = "__cilk_prologue";
  llvm::Function *Prologue = Module.getFunction(PrologueName);
  if (!Prologue) {
    Prologue = EmitCilkPrologueFunction(PrologueName, CGF.CGM);
  }

  // Call the prologue to initialize the stack frame and worker.
  CGBuilderTy Builder(CGF.AllocaInsertPt);
  Builder.CreateCall2(Prologue, SF, W);

  EmitCilkEpilogue(CGF, SF);
}

/// Generate a function that implements a sync. The function
/// implements the following C code:
///
/// void __cilk_sync(__cilkrts_stack_frame *sf)
/// {
///   if (sf->flags & CILK_FRAME_UNSYNCHED, 0) {
///     sf->parent_pedigree = sf->worker->pedigree;
///     SAVE_FLOAT_STATE(*sf);
///     if (!__builtin_setjmp(sf->ctx))
///       __cilkrts_sync(sf);
///   }
///   ++sf->worker->pedigree.rank;
/// }
static llvm::Function *
EmitCilkSyncFunction(const std::string &Name, CodeGenModule &CGM)
{
  using namespace llvm;
  llvm::Module &Module = CGM.getModule();
  LLVMContext &Ctx = Module.getContext();

  typedef void (__cilk_sync)(__cilkrts_stack_frame*);
  llvm::FunctionType *FTy = TypeBuilder<__cilk_sync, false>::get(Ctx);
  Function *F = Function::Create(FTy,
                                 GlobalValue::InternalLinkage,
                                 Name,
                                 &Module);

  BasicBlock *Entry = BasicBlock::Create(Ctx, "entry", F),
             *B1 = BasicBlock::Create(Ctx, "", F),
             *B2 = BasicBlock::Create(Ctx, "", F),
             *Exit = BasicBlock::Create(Ctx, "exit", F);

  Value *SF = F->arg_begin();
  { // Entry
    CGBuilderTy B(Entry);
    // if (sf.flags & CILK_FRAME_UNSYNCHED, 0)
    Value *Flags = LoadField(B, SF, StackFrameBuilder::flags);
    Flags = B.CreateAnd(Flags,
                        ConstantInt::get(Flags->getType(),
                                         CILK_FRAME_UNSYNCHED));
    Value *Zero = ConstantInt::get(Flags->getType(), 0);
    Value *Unsynced = B.CreateICmpEQ(Flags, Zero);
    B.CreateCondBr(Unsynced, Exit, B1);
  }
  { // B1
    CGBuilderTy B(B1);
    // sf.parent_pedigree = sf.worker->pedigree;
    StoreField(B,
      LoadField(B, LoadField(B, SF, StackFrameBuilder::worker),
                WorkerBuilder::pedigree),
      SF, StackFrameBuilder::parent_pedigree);
    // SAVE_FLOAT_STATE(sf);
    EmitSaveFloatState(B, SF);
    // if (!setjmp(sf.ctx))
    Value *C = EmitCilkSetJmp(B, SF);
    C = B.CreateNot(C);
    C = B.CreateICmpEQ(C, ConstantInt::get(C->getType(), 0));
    B.CreateCondBr(C, B2, Exit);
  }
  { // B2
    CGBuilderTy B(B2);
    // __cilkrts_sync(&sf);
    B.CreateCall(CILKRTS_FUNC(sync, CGM), SF);
    B.CreateBr(Exit);
  }
  { // Exit
    CGBuilderTy B(Exit);
    // ++sf.worker->pedigree.rank;
    Value *Rank = LoadField(B, SF, StackFrameBuilder::worker);
    Rank = GEP(B, Rank, WorkerBuilder::pedigree);
    Rank = GEP(B, Rank, PedigreeBuilder::rank);
    B.CreateStore(B.CreateAdd(B.CreateLoad(Rank),
                    ConstantInt::get(Rank->getType()->getPointerElementType(), 1)),
                  Rank);
    B.CreateRetVoid();
  }
  return F;
}

static void EmitCilkSync(CodeGenFunction &CGF, llvm::Value *SF)
{
  llvm::Module &Module = CGF.CGM.getModule();
  const std::string SyncName = "__cilk_sync";
  llvm::Function *Sync = Module.getFunction(SyncName);
  if (!Sync) {
    Sync = EmitCilkSyncFunction(SyncName, CGF.CGM);
  }
  CGF.Builder.CreateCall(Sync, SF);
}

/// Emit the prologue in the cilk spawn helper function. The prologue
/// implements the following C code:
///
/// void __cilk_helper_prologue(__cilkrts_stack_frame *sf,
///                             __cilkrts_worker *w)
/// {
///   sf->call_parent = w->current_stack_frame;
///   sf->worker = w;
///   sf->flags = CILK_FRAME_VERSION;
///   sf->reserved = 0;
///   w->current_stack_frame = sf;
///   __cilkrts_stack_frame *volatile *tail = w->tail;
///   sf->spawn_helper_pedigree = w->pedigree;
///   sf->call_parent->parent_pedigree = w->pedigree;
///   w->pedigree.rank = 0;
///   w->pedigree.next = &sf->spawn_helper_pedigree;
///   *tail++ = sf->call_parent;
///   w->tail = tail;
///   sf->flags |= CILK_FRAME_DETACHED;
/// }
static void EmitSpawnHelperPrologue(CGBuilderTy &B,
                                    llvm::Value *SF,
                                    llvm::Value *W)
{
  using namespace llvm;
  // sf->call_parent = w->current_stack_frame;
  StoreField(B, LoadField(B, W, WorkerBuilder::current_stack_frame),
             SF, StackFrameBuilder::call_parent);
  // sf->worker = w;
  StoreField(B, W, SF, StackFrameBuilder::worker);
  // sf->flags = CILK_FRAME_VERSION;
#if 0
  {
  StoreField(B,
             CILK_FRAME_VERSION,
             SF, StackFrameBuilder::flags);
  }
  // sf->reserved = 0;
  {
    Value *R = GEP(B, SF, StackFrameBuilder::reserved);
    B.CreateStore(ConstantInt::get(R->getType()->getPointerElementType(), 0), R);
  }
#endif
  // w->current_stack_frame = sf;
  StoreField(B, SF, W, WorkerBuilder::current_stack_frame);
  // __cilkrts_stack_frame *volatile *tail = w->tail;
  llvm::Value *Tail = LoadField(B, W, WorkerBuilder::tail);
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
//  StoreField(B,
//             Zero,
//             LoadField(B, W, WorkerBuilder::pedigree),
//             PedigreeBuilder::rank);
  // w->pedigree.next = &sf->spawn_helper_pedigree;
  StoreField(B,
             GEP(B, SF, StackFrameBuilder::parent_pedigree),
             GEP(B, W, WorkerBuilder::pedigree),
             PedigreeBuilder::next);
  // *tail++ = sf->call_parent;
  B.CreateStore(LoadField(B, SF, StackFrameBuilder::call_parent), Tail);
  B.CreateStore(B.CreateAdd(Tail, ConstantInt::get(Tail->getType(), 1)), Tail);
  // w->tail = tail;
  StoreField(B, Tail, W, WorkerBuilder::tail);
  // sf->flags |= CILK_FRAME_DETACHED;
  {
    Value *F = LoadField(B, SF, StackFrameBuilder::flags);
    F = B.CreateOr(F, ConstantInt::get(F->getType(), CILK_FRAME_DETACHED));
    StoreField(B, F, SF, StackFrameBuilder::flags);
  }
}

/// Emit the epilogue in the cilk spawn helper function. The epilogue
/// implements the following C code:
///
/// void __cilk_helper_epilogue(__cilkrts_stack_frame *sf)
/// {
///   sf->worker->current_stack_frame = sf->call_parent;
///   sf->call_parent = 0;
///   __cilkrts_leave_frame(sf);
/// }
static void EmitSpawnHelperEpilogue(CGBuilderTy &B,
                                    CodeGenModule &CGM,
                                    llvm::Value *SF)
{
  // sf->worker->current_stack_frame = sf->call_parent;
  StoreField(B,
             LoadField(B, SF, StackFrameBuilder::call_parent),
             LoadField(B, SF, StackFrameBuilder::worker),
             WorkerBuilder::current_stack_frame);
  // sf->call_parent = 0;
  StoreField(B, 0, SF, StackFrameBuilder::call_parent);
  // __cilkrts_leave_frame(sf);
  B.CreateCall(CILKRTS_FUNC(leave_frame, CGM), SF);
}

static void EmitSpawnHelper(CodeGenFunction &CFG)
{
  // After emitting SpawnLambda there should be a single
  // use of the corresponding llvm function.
}

static llvm::Value *GetNamedValue(CodeGenFunction &CGF,
                                  const std::string &Name)
{
  llvm::Value *V = CGF.CurFn->getValueSymbolTable().lookup(Name);
  if (!V) {
    EmitCilkPrologue(CGF);
    V = CGF.CurFn->getValueSymbolTable().lookup(Name);
  }
  return V;
}

static llvm::Value *GetStackFrame(CodeGenFunction &CGF)
{
  return GetNamedValue(CGF, StackFrameName());
}

static llvm::Value *GetWorker(CodeGenFunction &CGF)
{
  return GetNamedValue(CGF, WorkerName());
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

LValue
CGCilkPlusRuntime::EmitCilkSpawn(CodeGenFunction &CGF,
                                 const CilkSpawnExpr &E)
{
  return CGF.EmitLValue(E.getSubExpr());
}

void
CGCilkPlusRuntime::EmitCilkSync(CodeGenFunction &CGF,
                                const CilkSyncStmt &S)
{
  ::EmitCilkSync(CGF, GetStackFrame(CGF));
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

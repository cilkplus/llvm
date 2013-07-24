//===- SCCP.cpp - Sparse Conditional Constant Propagation -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements sparse conditional constant propagation and merging:
//
// Specifically, this:
//   * Assumes values are constant unless proven otherwise
//   * Assumes BasicBlocks are dead unless proven otherwise
//   * Proves values to be constant, and replaces them with constants
//   * Proves conditional branches to be unconditional
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "cilkabi"

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"
#include "llvm/Transforms/Scalar.h"
#include <sstream>

using namespace llvm;

namespace {

struct CilkABI : public ModulePass {
  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    //AU.addRequired<TargetLibraryInfo>();
  }

  static char ID;
  CilkABI() : ModulePass(ID) {
    initializeCilkABIPass(*PassRegistry::getPassRegistry());
  }

  bool runOnModule(Module &M);
};

enum ISAClass {
  IC_XMM,
  IC_YMM1,
  IC_YMM2,
  IC_ZMM
};

enum ParamKind {
  PK_Vector,
  PK_LinearConst,
  PK_Linear,
  PK_Uniform,
  PK_Mask
};

struct ParamInfo {
  ParamKind Kind;
  Value *Step;

  ParamInfo(ParamKind Kind)
  : Kind(Kind), Step(0)
  {}

  ParamInfo(ParamKind Kind, Value *Step)
  : Kind(Kind), Step(Step)
  {}
};

} // end anonymous namespace

static
ISAClass getISAClass(StringRef Processor) {
  return StringSwitch<ISAClass>(Processor)
    // SSE2
    .Case("pentium_4", IC_XMM)
    .Case("pentium_4_sse3", IC_XMM)
    .Case("core_2_duo_ssse3", IC_XMM)
    .Case("core_2_duo_sse4_1", IC_XMM)
    .Case("core_i7_sse4_2", IC_XMM)
    // AVX
    .Case("core_2nd_gen_avx", IC_YMM1)
    .Case("core_3rd_gen_avx", IC_YMM1)
    // AVX2
    .Case("core_4th_gen_avx", IC_YMM2)
    // MIC
    .Case("mic", IC_ZMM);
}

static
char encodeISAClass(ISAClass ISA) {
  switch (ISA) {
  case IC_XMM: return 'x';
  case IC_YMM1: return 'y';
  case IC_YMM2: return 'Y';
  case IC_ZMM: return 'z';
  }
  assert(false && "unknown isa");
  return 0;
}

static
Type *getVectorType(Type *Ty, ISAClass ISA) {
  if (Ty->isAggregateType())
    Ty = Type::getInt32Ty(Ty->getContext());

  unsigned NumElements = 1;
  if (Ty->isIntegerTy(8))
    switch (ISA) {
    case IC_XMM:
    case IC_YMM1:
    case IC_YMM2: NumElements = 16; break;
    case IC_ZMM: assert(false && "incomplete");
    }
  else if (Ty->isIntegerTy(16))
    switch (ISA) {
    case IC_XMM:
    case IC_YMM1: NumElements = 8; break;
    case IC_YMM2: NumElements = 16; break;
    case IC_ZMM: assert(false && "incomplete");
    }
  else if (Ty->isIntegerTy(32))
    switch (ISA) {
    case IC_XMM:
    case IC_YMM1: NumElements = 4; break;
    case IC_YMM2: NumElements = 8; break;
    case IC_ZMM: assert(false && "incomplete");
    }
  else if (Ty->isIntegerTy(64))
    switch (ISA) {
    case IC_XMM:
    case IC_YMM1: NumElements = 2; break;
    case IC_YMM2: NumElements = 4; break;
    case IC_ZMM: assert(false && "incomplete");
    }
  else if (Ty->isFloatTy())
    switch (ISA) {
    case IC_XMM: NumElements =  4; break;
    case IC_YMM1:
    case IC_YMM2: NumElements = 8; break;
    case IC_ZMM: assert(false && "incomplete");
    }
  else if (Ty->isDoubleTy())
    switch (ISA) {
    case IC_XMM: NumElements = 2; break;
    case IC_YMM1:
    case IC_YMM2: NumElements = 4; break;
    case IC_ZMM: assert(false && "incomplete");
    }
  else if (Ty->isPointerTy())
    switch (ISA) {
    case IC_XMM: NumElements = 2; break; // TODO 32bit vs. 64bit
    case IC_YMM1: assert(false && "incomplete");
    case IC_YMM2: assert(false && "incomplete");
    case IC_ZMM: assert(false && "incomplete");
    }
    
  assert(NumElements > 1 && "invalid NumElements");
  return VectorType::get(Ty, NumElements);
}

// Return a constant vector <0, 1, ..., N - 1>
static
Constant *getIndicesVector(Type *Ty, unsigned N) {
  SmallVector<Constant*, 4> Indices;
  for (unsigned i = 0; i < N; ++i)
    Indices.push_back(ConstantInt::get(Ty, i));
  return ConstantVector::get(Indices);
}

// Return a value representing:
//   <Arg, Arg, ..., Arg> + <Step, Step, ..., Step> * <0, 1, ..., VLen - 1>
static
Value *buildLinearArg(IRBuilder<> &B, unsigned VLen, Value *Arg, Value *Step) {
  Type *Ty = Arg->getType();
  return        
    B.CreateAdd(
      B.CreateVectorSplat(VLen, Arg),
      B.CreateMul(
        getIndicesVector(Ty, VLen),
        B.CreateVectorSplat(VLen, B.CreateIntCast(Step, Ty, true))));
}

static
FunctionType *encodeParameters(Function *Func,
                               MDNode *ArgName,
                               MDNode *ArgStep,
                               bool Mask,
                               ISAClass ISA,
                               Type *&CharacteristicTy,
                               SmallVectorImpl<ParamInfo> &Info,
                               std::ostringstream &MangledParams) {
  assert(Func && "Func is null");
  unsigned ArgSize = Func->arg_size();

  LLVMContext &Context = Func->getContext();

  if (ArgName->getNumOperands() != 1 + ArgSize ||
      ArgStep->getNumOperands() != 1 + ArgSize)
    return 0;

  
  if (!CharacteristicTy && !Func->getReturnType()->isVoidTy())
    CharacteristicTy = getVectorType(Func->getReturnType(), ISA);

  SmallVector<Type*, 4> Tys;

  Function::const_arg_iterator Arg = Func->arg_begin();
  for (unsigned i = 1, ie = 1 + ArgSize; i < ie; ++i, ++Arg) {
    MDString *Name = dyn_cast<MDString>(ArgName->getOperand(i));
    if (!Name)
      return 0;

    Value *Step = ArgStep->getOperand(i);
    if (Step->getType() != Type::getInt32Ty(Context))
      return 0;
    if (dyn_cast<UndefValue>(Step)) {
      Type *ArgVTy = getVectorType(Arg->getType(), ISA);
      if (!CharacteristicTy)
        CharacteristicTy = ArgVTy;
      MangledParams << "v";
      unsigned NumArgs = CharacteristicTy->getVectorNumElements() /
                         ArgVTy->getVectorNumElements();
      NumArgs = NumArgs < 1 ? 1 : NumArgs;
      for (unsigned j = 0; j < NumArgs; ++j)
        Tys.push_back(ArgVTy);
      Info.push_back(ParamInfo(PK_Vector));
    } else if (ConstantInt *C = dyn_cast<ConstantInt>(Step)) {
      if (C->isZero()) {
        MangledParams << "u";
        Tys.push_back(Arg->getType());
        Info.push_back(ParamInfo(PK_Uniform));
      } else {
        MangledParams << "l";
        Tys.push_back(Arg->getType());
        Info.push_back(ParamInfo(PK_LinearConst, C));
      }
    } else {
      MangledParams << "s";
      Tys.push_back(Arg->getType());
      assert(false && "Variable step linear arguments incomplete");
    }
  }

  if (Mask) {
    Type *Ty = getVectorType(CharacteristicTy->getVectorElementType(), ISA);
    unsigned N = CharacteristicTy->getVectorNumElements(),
             M = Ty->getVectorNumElements();
    Type *MaskIntTy = Type::getIntNTy(Context, Ty->getScalarSizeInBits());
    Type *MaskVTy = VectorType::get(MaskIntTy, M);
    for (unsigned i = 0; i < N / M; ++i)
      Tys.push_back(MaskVTy);
  }
  
  Type *RetTy = Func->getReturnType();
  RetTy = RetTy->isVoidTy() ? RetTy : CharacteristicTy;
  return FunctionType::get(RetTy, Tys, false);
}

static
void setVectorVariantAttributes(Function *Func, Function *NewFunc,
                                const std::string &Processor) {
  AttrBuilder NewFuncAttrs(Func->getAttributes(), AttributeSet::FunctionIndex);

  std::string CPU = StringSwitch<std::string>(Processor)
    .Case("pentium_4",         "pentium4")
    .Case("pentium_4_sse3",    "yonah")
    .Case("core_2_duo_ssse3",  "core2")
    .Case("core_2_duo_sse4_1", "penryn")
    .Case("core_i7_sse4_2",    "corei7")
    .Case("core_2nd_gen_avx",  "corei7-avx")
    .Case("core_3rd_gen_avx",  "core-avx-i")
    .Case("core_4th_gen_avx",  "core-avx2")
    .Case("mic",               "")
    .Default("");

  if (!CPU.empty())
    NewFuncAttrs.addAttribute("cpu", CPU);

  if (NewFuncAttrs.hasAttributes())
    NewFunc->setAttributes(AttributeSet::get(NewFunc->getContext(),
                                             AttributeSet::FunctionIndex,
                                             NewFuncAttrs));
}

static
void createVectorVariantWrapper(Function *ScalarFunc,
                                Function *VectorFunc,
                                unsigned VLen,
                                const SmallVectorImpl<ParamInfo> &Info) {
  assert(ScalarFunc->arg_size() == Info.size() &&
         "Wrong number of parameter infos");
  assert((VLen & (VLen - 1)) == 0 &&
         "VLen must be a power-of-2");

  LLVMContext &Context = ScalarFunc->getContext();
  BasicBlock *Entry = BasicBlock::Create(Context, "entry", VectorFunc),
             *LoopCond = BasicBlock::Create(Context, "loop.cond", VectorFunc),
             *LoopBody = BasicBlock::Create(Context, "loop.body", VectorFunc),
             *LoopEnd = BasicBlock::Create(Context, "loop.end", VectorFunc);

  Value *VectorRet = 0;
  SmallVector<Value*, 4> VectorArgs;
  // The loop counter.
  Type *IndexTy = Type::getInt32Ty(Context);
  Value *Index = 0;

  IRBuilder<> Builder(Entry);
  {
    if (!VectorFunc->getReturnType()->isVoidTy())
      VectorRet = Builder.CreateAlloca(VectorFunc->getReturnType());

    Function::arg_iterator VI = VectorFunc->arg_begin();
    for (SmallVectorImpl<ParamInfo>::const_iterator I = Info.begin(),
         IE = Info.end(); I != IE; ++I) {
      Value *Arg = VI++;
      switch (I->Kind) {
      case PK_Vector: {
        assert(Arg->getType()->isVectorTy() && "Not a vector");
        unsigned NumElements = Arg->getType()->getVectorNumElements();
        if (NumElements == VLen)
          VectorArgs.push_back(Arg);
        else if (NumElements > VLen) {
          Value *Mask = getIndicesVector(Builder.getInt32Ty(), VLen);
          VectorArgs.push_back(Builder.CreateShuffleVector(Arg, Arg, Mask));
        } else {
          VI--;
          unsigned NumArgs = VLen / NumElements;
          // Copy NumArgs into temporary array for shuffling (Advancing VI by
          // NumArgs).
          SmallVector<Value*, 4> ShuffleArgs;
          for (unsigned j = 0; j < NumArgs; ++j) {
            assert(VI->getType()->getVectorNumElements() == NumElements &&
                   "different vector sizes");
            ShuffleArgs.push_back(VI++);
          }
          // Join all the vector arguments collected in ShuffleArgs into a
          // single vector of size VLen.
          for (unsigned Size = NumElements; Size < VLen; Size *= 2) {
            Value *Mask = getIndicesVector(Builder.getInt32Ty(), Size * 2);
            for (unsigned k = 0, ke = VLen / Size - 1; k < ke; ++k) {
              ShuffleArgs[k] = Builder.CreateShuffleVector(ShuffleArgs[k*2],
                                                           ShuffleArgs[k*2+1],
                                                           Mask);
        
            }
          }
          assert(ShuffleArgs[0]->getType()->getVectorNumElements() == VLen &&
                 "final shuffled vector has incorrect num elements");
          VectorArgs.push_back(ShuffleArgs[0]);
        }
      } break;
      case PK_LinearConst:
        VectorArgs.push_back(buildLinearArg(Builder, VLen, Arg, I->Step));
        break;
      case PK_Linear:
        assert(false && "non-const linear feature incomplete");
        break;
      case PK_Uniform:
        VectorArgs.push_back(Builder.CreateVectorSplat(VLen, Arg));
        break;
      case PK_Mask:
        break;
      }
    }

    Index = Builder.CreateAlloca(IndexTy, 0, "index");
    Builder.CreateStore(ConstantInt::get(IndexTy, 0), Index);
    Builder.CreateBr(LoopCond);
  }

  // Copy the names from the scalar args to the vector args.
  {
    Function::arg_iterator SI = ScalarFunc->arg_begin(),
                           SE = ScalarFunc->arg_end();
    SmallVectorImpl<Value*>::iterator VI = VectorArgs.begin();
    for ( ; SI != SE; ++SI, ++VI)
      (*VI)->setName(SI->getName());
  }

  Builder.SetInsertPoint(LoopCond);
  {
    Value *Cond = Builder.CreateICmpULT(Builder.CreateLoad(Index),
                                        ConstantInt::get(IndexTy, VLen));
    Builder.CreateCondBr(Cond, LoopBody, LoopEnd);
  }

  Builder.SetInsertPoint(LoopBody);
  {
    Value *VecIndex = Builder.CreateLoad(Index);

    // Build the argument list for the scalar function by extracting element
    // 'VecIndex' from the vector arguments.
    SmallVector<Value*, 4> ScalarArgs;
    for (SmallVectorImpl<Value*>::iterator VI = VectorArgs.begin(),
         VE = VectorArgs.end(); VI != VE; ++VI) {
      assert((*VI)->getType()->isVectorTy() && "Not a vector");
      ScalarArgs.push_back(Builder.CreateExtractElement(*VI, VecIndex));
    }

    // Call the scalar function with the extracted scalar arguments.
    Value *ScalarRet = Builder.CreateCall(ScalarFunc, ScalarArgs);

    // If the function returns a value insert the scalar return value into the
    // vector return value.
    if (VectorRet) {
      Value *V = Builder.CreateLoad(VectorRet);
      V = Builder.CreateInsertElement(V, ScalarRet, VecIndex);
      Builder.CreateStore(V, VectorRet);
    }

    // Index = Index + 1
    VecIndex = Builder.CreateAdd(VecIndex, ConstantInt::get(IndexTy, 1));
    Builder.CreateStore(VecIndex , Index);
    Builder.CreateBr(LoopCond);
  }

  // TODO: handle masks

  Builder.SetInsertPoint(LoopEnd);
  {
    if (VectorRet)
      Builder.CreateRet(Builder.CreateLoad(VectorRet));
    else
      Builder.CreateRetVoid();
  }
}

static bool createVectorVariant(Module &M, MDNode *Root) {
  if (Root->getNumOperands() == 0)
    return false;
  Function *Func = dyn_cast<Function>(Root->getOperand(0));
  if (!Func)
    return false;

  bool Elemental = false;
   
  MDNode *ArgName = 0,
         *ArgStep = 0,
         *VecTypeHint = 0,
         *Processor = 0,
         *Mask = 0;

  for (unsigned i = 1, ie = Root->getNumOperands(); i < ie; ++i) {
    MDNode *Node = dyn_cast<MDNode>(Root->getOperand(i));
    if (!Node || Node->getNumOperands() < 1)
      return false;
    MDString *Name = dyn_cast<MDString>(Node->getOperand(0));
    if (!Name)
      return false;

    if (Name->getString() == "elemental") {
      Elemental = true;
    } else if (Name->getString() == "arg_name") {
      ArgName = Node;
    } else if (Name->getString() == "arg_step") {
      ArgStep = Node;
    } else if (Name->getString() == "vec_type_hint") {
      VecTypeHint = Node;
    } else if (Name->getString() == "processor") {
      Processor = Node;
    } else if (Name->getString() == "mask") {
      Mask = Node;
    } else {
      DEBUG(dbgs() << "Unknown metadata " << Name->getString() << "\n");
      return false;
    }
  }

  if (!Elemental || !ArgName || !ArgStep)
    return false;

  // The default processor is pentium_4.
  std::string ProcessorName = "pentium_4";
  if (Processor) {
    if (Processor->getNumOperands() != 2)
      return false;
    MDString *Name = dyn_cast<MDString>(Processor->getOperand(1));
    if (!Name)
      return false;
    ProcessorName = Name->getString().str();
  }
  ISAClass ISA = getISAClass(ProcessorName);

  bool IsMasked = true;
  if (Mask) {
    if (Mask->getNumOperands() != 2)
      return false;
    ConstantInt *C = dyn_cast<ConstantInt>(Mask->getOperand(1));
    if (!C)
      return false;
    IsMasked = C->isOne();
  }

  Type *CharacteristicTy = 0;
  if (VecTypeHint) {
    if (VecTypeHint->getNumOperands() != 3)
      return false;
    // TODO
  }

  SmallVector<ParamInfo, 4> Info;
  std::ostringstream MangledParams;
  FunctionType *NewFuncTy = encodeParameters(Func,
                                             ArgName, ArgStep,
                                             IsMasked,
                                             ISA,
                                             CharacteristicTy,
                                             Info,
                                             MangledParams);
  if (!NewFuncTy)
    return false;

  unsigned VLen = CharacteristicTy->getVectorNumElements();

  // Generate the mangled name.
  std::ostringstream MangledName;
  MangledName << "_ZGV" // Magic prefix
              << encodeISAClass(ISA)
              << (IsMasked ? 'M' : 'N')
              << VLen
              << MangledParams.str()
              << "_" << Func->getName().str();

  DEBUG(dbgs() << "Creating elemental function " << MangledName.str() << "\n");

  // Declare the vector variant function in to the module.
  Function *NewFunc =
    dyn_cast<Function>(M.getOrInsertFunction(MangledName.str(), NewFuncTy));
  if (!NewFunc || !NewFunc->empty())
    return false;

  setVectorVariantAttributes(Func, NewFunc, ProcessorName);

  // Define the vector variant if the scalar function is defined in this module.
  if (!Func->empty())
    createVectorVariantWrapper(Func, NewFunc, VLen, Info);

  return true;
}

bool CilkABI::runOnModule(Module &M) {
  NamedMDNode *Root = M.getNamedMetadata("cilk.functions");
  if (!Root)
    return false;

  // Generate a vector variant for each function in cilk.functions.
  bool ModuleChanged = false;
  for (unsigned i = 0, ie = Root->getNumOperands(); i < ie; ++i) {
    MDNode *Node = Root->getOperand(i);
    ModuleChanged |= createVectorVariant(M, Node);
  }
  return ModuleChanged;
}

char CilkABI::ID = 0;

INITIALIZE_PASS_BEGIN(CilkABI, "cilkabi", "Cilk ABI", false, false)
INITIALIZE_PASS_END(CilkABI, "cilkabi", "Cilk ABI", false, false)

// createIPSCCPPass - This is the public interface to this file.
ModulePass *llvm::createCilkABIPass() {
  return new CilkABI();
}

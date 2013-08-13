//===----- CGElementalFunction.cpp - CodeGen for Elemental Functions ------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
/// \brief This files implements code generation for Cilk Plus elemental
/// functions.
///
//===----------------------------------------------------------------------===//

#include "CodeGenModule.h"
#include "clang/Basic/TargetInfo.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/Debug.h"

using namespace clang;
using namespace CodeGen;

struct CilkElementalGroup {
  typedef SmallVector<CilkProcessorAttr::CilkProcessor, 1> ProcessorVector;
  typedef SmallVector<QualType, 1> VecLengthForVector;
  typedef SmallVector<unsigned, 1> VecLengthVector;
  typedef SmallVector<CilkLinearAttr*, 8> LinearVector;
  typedef SmallVector<CilkUniformAttr*, 8> UniformVector;
  typedef SmallVector<llvm::MDNode*, 2> MaskVector;
  ProcessorVector Processor;
  VecLengthVector VecLength;
  VecLengthForVector VecLengthFor;
  LinearVector Linear;
  UniformVector Uniform;
  MaskVector Mask;
};

static llvm::MDNode *MakeVecLengthMetadata(CodeGenModule &CGM, StringRef Name,
                                           QualType T, uint64_t VL) {
  llvm::LLVMContext &Context = CGM.getLLVMContext();
  llvm::Value *attrMDArgs[] = {
    llvm::MDString::get(Context, Name),
    llvm::UndefValue::get(CGM.getTypes().ConvertType(T)),
    llvm::ConstantInt::get(CGM.Int32Ty, VL)
  };
  return llvm::MDNode::get(Context, attrMDArgs);
}

static bool CheckElementalArguments(CodeGenModule &CGM, const FunctionDecl *FD,
                                    llvm::Function *Fn, bool &HasThis) {
  // Check the return type.
  QualType RetTy = FD->getResultType();
  if (RetTy->isAggregateType()) {
    CGM.Error(FD->getLocation(), "the return type for this elemental "
                                 "function is not supported yet");
    return false;
  }

  // Check each parameter type.
  for (unsigned I = 0, E = FD->param_size(); I < E; ++I) {
    const ParmVarDecl *VD = FD->getParamDecl(I);
    QualType Ty = VD->getType();
    assert(!Ty->isIncompleteType() && "incomplete type");
    if (Ty->isAggregateType()) {
      CGM.Error(VD->getLocation(), "the parameter type for this elemental "
                                   "function is not supported yet");
      return false;
    }
  }

  HasThis = isa<CXXConstructorDecl>(FD) || isa<CXXDestructorDecl>(FD) ||
    (isa<CXXMethodDecl>(FD) && cast<CXXMethodDecl>(FD)->isInstance());

  // At this point, no passing struct arguments by value.
  unsigned NumArgs = FD->param_size();
  unsigned NumLLVMArgs = Fn->arg_size();

  // There is a single implicit 'this' parameter.
  if (HasThis && (NumArgs + 1 == NumLLVMArgs))
    return true;

  return NumArgs == NumLLVMArgs;
}

void CodeGenModule::EmitCilkElementalMetadata(const CGFunctionInfo &FnInfo,
                                              const FunctionDecl *FD,
                                              llvm::Function *Fn) {
  if (!FD->hasAttr<CilkElementalAttr>())
    return;

  if (FD->isVariadic())
    return;

  // Do not emit any vector variant if there is an unsupported feature.
  bool HasImplicitThis = false;
  if (!CheckElementalArguments(*this, FD, Fn, HasImplicitThis))
    return;

  llvm::LLVMContext &Context = getLLVMContext();
  ASTContext &C = getContext();

  // Common metadata nodes.
  llvm::NamedMDNode *CilkElementalMetadata =
    getModule().getOrInsertNamedMetadata("cilk.functions");
  llvm::Value *ElementalMDArgs[] = {
    llvm::MDString::get(Context, "elemental")
  };
  llvm::MDNode *ElementalNode = llvm::MDNode::get(Context, ElementalMDArgs);
  llvm::Value *MaskMDArgs[] = {
    llvm::MDString::get(Context, "mask"),
    llvm::ConstantInt::get(llvm::IntegerType::getInt1Ty(Context), 1)
  };
  llvm::MDNode *MaskNode = llvm::MDNode::get(Context, MaskMDArgs);
  MaskMDArgs[1] = llvm::ConstantInt::get(llvm::IntegerType::getInt1Ty(Context), 0);
  llvm::MDNode *NoMaskNode = llvm::MDNode::get(Context, MaskMDArgs);
  llvm::Value *UndefStep = llvm::UndefValue::get(IntTy);
  SmallVector<llvm::Value*, 8> ParameterNameArgs;
  ParameterNameArgs.push_back(llvm::MDString::get(Context, "arg_name"));
  llvm::MDNode *ParameterNameNode = 0;

  // Vector variant metadata.
  llvm::Value *VariantMDArgs[] = {
    llvm::MDString::get(Context, "variant"),
    llvm::UndefValue::get(llvm::Type::getVoidTy(Context))
  };
  llvm::MDNode *VariantNode = llvm::MDNode::get(Context, VariantMDArgs);

  // Group elemental function attributes by vector() declaration.
  typedef llvm::SmallDenseMap<unsigned, CilkElementalGroup, 4> GroupMap;
  GroupMap Groups;
  for (Decl::attr_iterator AI = FD->attr_begin(), AE = FD->attr_end();
       AI != AE;
       ++AI) {
    if (CilkElementalAttr *A = dyn_cast<CilkElementalAttr>(*AI)) {
      unsigned key = A->getGroup().getRawEncoding();
      Groups.FindAndConstruct(key);
    } else if (CilkProcessorAttr *A = dyn_cast<CilkProcessorAttr>(*AI)) {
      unsigned key = A->getGroup().getRawEncoding();
      Groups[key].Processor.push_back(A->getProcessor());
    } else if (CilkVecLengthForAttr *A = dyn_cast<CilkVecLengthForAttr>(*AI)) {
      unsigned key = A->getGroup().getRawEncoding();
      Groups[key].VecLengthFor.push_back(A->getTypeHint());
    } else if (CilkVecLengthAttr *A = dyn_cast<CilkVecLengthAttr>(*AI)) {
      unsigned key = A->getGroup().getRawEncoding();
      const Expr *LengthExpr = A->getLength();
      assert(isa<IntegerLiteral>(LengthExpr) && "integeral literal expected");
      unsigned VLen = cast<IntegerLiteral>(LengthExpr)->getValue().getZExtValue();
      Groups[key].VecLength.push_back(VLen);
    } else if (CilkMaskAttr *A = dyn_cast<CilkMaskAttr>(*AI)) {
      unsigned key = A->getGroup().getRawEncoding();
      Groups[key].Mask.push_back(A->getMask() ? MaskNode : NoMaskNode);
    } else if (CilkLinearAttr *A = dyn_cast<CilkLinearAttr>(*AI)) {
      unsigned key = A->getGroup().getRawEncoding();
      Groups[key].Linear.push_back(A);
    } else if (CilkUniformAttr *A = dyn_cast<CilkUniformAttr>(*AI)) {
      unsigned key = A->getGroup().getRawEncoding();
      Groups[key].Uniform.push_back(A);
    }
  }

  for (GroupMap::iterator GI = Groups.begin(), GE = Groups.end();
       GI != GE;
       ++GI) {
    CilkElementalGroup &G = GI->second;

    // Parameter information.
    const ParmVarDecl *FirstNonStepParm = 0;
    SmallVector<llvm::Value*, 8> StepArgs;
    StepArgs.push_back(llvm::MDString::get(Context, "arg_step"));

    // Push implicit 'this' argument if necessary. This assumes that 
    // 'uniform(this)' or 'linear(this)' is not a valid clause.
    if (HasImplicitThis) {
      ParameterNameArgs.push_back(llvm::MDString::get(Context, "this"));
      StepArgs.push_back(UndefStep);
    }

    for (unsigned I = 0; I != FD->getNumParams(); ++I) {
      const ParmVarDecl *Parm = FD->getParamDecl(I);
      StringRef ParmName = Parm->getName();
      if (!ParameterNameNode)
        ParameterNameArgs.push_back(llvm::MDString::get(Context, ParmName));
      // Look for a linear attribute for this parameter.
      CilkElementalGroup::LinearVector::iterator SI = G.Linear.begin(),
                                                 SE = G.Linear.end();
      for (; SI != SE; ++SI)
        if ((*SI)->getParameter()->getName() == ParmName)
          break;
      if (SI == SE) {
        CilkElementalGroup::UniformVector::iterator UI = G.Uniform.begin(),
                                                    UE = G.Uniform.end();
        for (; UI != UE; ++UI)
          if ((*UI)->getParameter()->getName() == ParmName)
            break;
        if (UI != UE)
          // This is uniform, give this paramater a step of 0 as placeholder.
          StepArgs.push_back(llvm::ConstantInt::get(IntTy, 0));
        else {
          // Not linear/uniform, give this parameter an undefined step as
          // placeholder.
          StepArgs.push_back(UndefStep);
          if (!FirstNonStepParm)
            FirstNonStepParm = Parm;
        }
      } else if (IdentifierInfo *S = (*SI)->getStepParameter()) {
        StepArgs.push_back(llvm::MDString::get(Context, S->getName()));
      } else {
        StepArgs.push_back(llvm::ConstantInt::get(IntTy,
                                                  (*SI)->getStepValue()));
      }
    }
    llvm::MDNode *StepNode = llvm::MDNode::get(Context, StepArgs);
    if (!ParameterNameNode)
      ParameterNameNode = llvm::MDNode::get(Context, ParameterNameArgs);

    // If there is no vectorlengthfor() in this group, determine the
    // characteristic type. This can depend on the linear/uniform attributes,
    // so it can differ between groups.
    //
    // The rules for computing the characteristic type are:
    //
    // a) For a non‐void function, the characteristic data type is the
    //    return type.
    //
    // b) If the function has any non‐uniform, non‐linear parameters, the
    //    the characteristic data type is the type of the first such parameter.
    //
    // c) If the characteristic data type determined by a) or b) above is
    //    struct, union, or class type which is pass‐by‐value (except fo
    //    the type that maps to the built‐in complex data type)
    //    the characteristic data type is int.
    //
    // d) If none of the above three cases is applicable, 
    //    the characteristic data type is int.
    //
    // e) For Intel Xeon Phi native and offload compilation, if the resulting
    //    characteristic data type is 8-bit or 16‐bit integer data type
    //    the characteristic data type is int.
    // 
    // These rules missed the reference types and we use their pointer types.
    //
    if (G.VecLengthFor.empty()) {
      QualType FnRetTy = FD->getResultType();
      QualType CharacteristicType;
      if (!FnRetTy->isVoidType())
        CharacteristicType = FnRetTy;
      else if (FirstNonStepParm)
        CharacteristicType = FirstNonStepParm->getType().getCanonicalType();
      else
        CharacteristicType = C.IntTy;

      if (CharacteristicType->isReferenceType()) {
        QualType BaseTy = CharacteristicType.getNonReferenceType();
        CharacteristicType = C.getPointerType(BaseTy);
      } else if (CharacteristicType->isAggregateType())
        CharacteristicType = C.IntTy;
      // FIXME: handle Xeon Phi targets.
      G.VecLengthFor.push_back(CharacteristicType);
    }

    // If no mask variants are specified, generate both.
    if (G.Mask.empty()) {
      G.Mask.push_back(MaskNode);
      G.Mask.push_back(NoMaskNode);
    }

    // If no target processor is specified, use the current target.
    if (G.Processor.empty())
      G.Processor.push_back(CilkProcessorAttr::Unspecified);

    // If no vector length is specified, push a dummy value to iterate over.
    if (G.VecLength.empty())
      G.VecLength.push_back(0);

    for (CilkElementalGroup::VecLengthForVector::iterator
          TI = G.VecLengthFor.begin(),
          TE = G.VecLengthFor.end();
          TI != TE;
          ++TI) {

      for (CilkElementalGroup::ProcessorVector::iterator
            PI = G.Processor.begin(),
            PE = G.Processor.end();
          PI != PE;
          ++PI) {

        // Processor node.
        uint64_t VectorRegisterBytes = 0;
        llvm::MDNode *ProcessorNode = 0;
        if (*PI == CilkProcessorAttr::Unspecified) {
          // Since no vector(processor(...)) attribute is present, inspect the
          // current target features to determine the appropriate vector size.
          // This is currently X86 specific.
          if (Target.hasFeature("avx2"))
            VectorRegisterBytes = 64;
          else if (Target.hasFeature("avx"))
            VectorRegisterBytes = 32;
          else if (Target.hasFeature("sse2"))
            VectorRegisterBytes = 16;
          else if (Target.hasFeature("sse") &&
                  (*TI)->isFloatingType() &&
                  C.getTypeSizeInChars(*TI).getQuantity() == 4)
            VectorRegisterBytes = 16;
          else if (Target.hasFeature("mmx") && (*TI)->isIntegerType())
            VectorRegisterBytes = 8;
        } else {
          llvm::Value *attrMDArgs[] = {
            llvm::MDString::get(Context, "processor"),
            llvm::MDString::get(Context,
                                CilkProcessorAttr::getProcessorString(*PI))
          };
          ProcessorNode = llvm::MDNode::get(Context, attrMDArgs);
          VectorRegisterBytes = CilkProcessorAttr::getProcessorVectorBytes(*PI);
        }

        for (CilkElementalGroup::VecLengthVector::iterator
              LI = G.VecLength.begin(),
              LE = G.VecLength.end();
             LI != LE;
             ++LI) {

          uint64_t VL = *LI ? *LI :
            (CharUnits::fromQuantity(VectorRegisterBytes)
             / C.getTypeSizeInChars(*TI));

          llvm::MDNode *VecTypeNode
            = MakeVecLengthMetadata(*this, "vec_length", *TI, VL);

          for (CilkElementalGroup::MaskVector::iterator
                MI = G.Mask.begin(),
                ME = G.Mask.end();
               MI != ME;
               ++MI) {

            SmallVector <llvm::Value*, 7> kernelMDArgs;
            kernelMDArgs.push_back(Fn);
            kernelMDArgs.push_back(ElementalNode);
            kernelMDArgs.push_back(ParameterNameNode);
            kernelMDArgs.push_back(StepNode);
            kernelMDArgs.push_back(VecTypeNode);
            kernelMDArgs.push_back(*MI);
            if (ProcessorNode)
              kernelMDArgs.push_back(ProcessorNode);
            kernelMDArgs.push_back(VariantNode);
            llvm::MDNode *KernelMD = llvm::MDNode::get(Context, kernelMDArgs);
            CilkElementalMetadata->addOperand(KernelMD);
            ElementalVariantToEmit.push_back(
                ElementalVariantInfo(&FnInfo, FD, Fn, KernelMD));
          }
        }
      }
    }
  }
}

// Vector variants CodeGen for elemental functions.
namespace {

enum ISAClass {
  IC_XMM,
  IC_YMM1,
  IC_YMM2,
  IC_ZMM,
  IC_Unknown
};

enum ParamKind {
  PK_Vector,
  PK_LinearConst,
  PK_Linear,
  PK_Uniform
};

struct ParamInfo {
  ParamKind Kind;
  llvm::Value *Step;

  ParamInfo(ParamKind Kind)
  : Kind(Kind), Step(0)
  {}

  ParamInfo(ParamKind Kind, llvm::Value *Step)
  : Kind(Kind), Step(Step)
  {}
};

} // end anonymous namespace

static ISAClass getISAClass(StringRef Processor) {
  return llvm::StringSwitch<ISAClass>(Processor)
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
    .Case("mic", IC_ZMM)
    .Default(IC_Unknown);
}

static char encodeISAClass(ISAClass ISA) {
  switch (ISA) {
  case IC_XMM: return 'x';
  case IC_YMM1: return 'y';
  case IC_YMM2: return 'Y';
  case IC_ZMM: return 'z';
  case IC_Unknown: llvm_unreachable("ISA unknwon");
  }
  llvm_unreachable("unknown isa");
  return 0;
}

// Return a constant vector <0, 1, ..., N - 1>
static llvm::Constant *getIndicesVector(llvm::Type *Ty, unsigned N) {
  SmallVector<llvm::Constant*, 4> Indices;
  for (unsigned i = 0; i < N; ++i)
    Indices.push_back(llvm::ConstantInt::get(Ty, i));
  return llvm::ConstantVector::get(Indices);
}

// Return a value representing:
//   <Arg, Arg, ..., Arg> + <Step, Step, ..., Step> * <0, 1, ..., VLen - 1>
static llvm::Value *buildLinearArg(llvm::IRBuilder<> &B, unsigned VLen,
                                   llvm::Value *Arg, llvm::Value *Step) {
  llvm::Type *Ty = Arg->getType();
  llvm::Value *Base = B.CreateVectorSplat(VLen, Arg);
  llvm::Value *Offset = B.CreateMul(getIndicesVector(Step->getType(), VLen),
                                    B.CreateVectorSplat(VLen, Step));
  if (Ty->isPointerTy())
    return B.CreateGEP(Base, Offset);
  assert(Ty->isIntegerTy() && "expected an integer type");
  return B.CreateAdd(Base, B.CreateIntCast(Offset, Base->getType(), false));
}

static llvm::Value *buildMask(llvm::IRBuilder<> &B, unsigned VL,
                              llvm::Value *Mask) {
  llvm::Type *Ty = Mask->getType()->getVectorElementType();
  if (Ty->isFloatTy())
    Mask = B.CreateBitCast(Mask, llvm::VectorType::get(B.getInt32Ty(), VL));
  else if (Ty->isDoubleTy())
    Mask = B.CreateBitCast(Mask, llvm::VectorType::get(B.getInt64Ty(), VL));
  else
    assert((Ty->isIntegerTy()|| Ty->isPointerTy()) && "unexpected type");

  return B.CreateICmpNE(Mask, llvm::Constant::getNullValue(Mask->getType()));
}

static llvm::FunctionType *encodeParameters(llvm::Function *Func,
                                            llvm::MDNode *ArgName,
                                            llvm::MDNode *ArgStep,
                                            bool Mask,
                                            ISAClass ISA,
                                            llvm::Type *VectorDataTy,
                                            SmallVectorImpl<ParamInfo> &Info,
                                     llvm::raw_svector_ostream &MangledParams) {
  assert(Func && "Func is null");
  unsigned ArgSize = Func->arg_size();

  if (ArgName->getNumOperands() != 1 + ArgSize ||
      ArgStep->getNumOperands() != 1 + ArgSize)
    return 0;

  SmallVector<llvm::Type*, 4> Tys;
  llvm::Function::const_arg_iterator Arg = Func->arg_begin();
  for (unsigned i = 1, ie = 1 + ArgSize; i < ie; ++i, ++Arg) {
    llvm::MDString *Name = dyn_cast<llvm::MDString>(ArgName->getOperand(i));
    if (!Name)
      return 0;

    llvm::Value *Step = ArgStep->getOperand(i);
    if (dyn_cast<llvm::UndefValue>(Step)) {
      MangledParams << "v";
      unsigned VL = VectorDataTy->getVectorNumElements();
      Tys.push_back(llvm::VectorType::get(Arg->getType(), VL));
      Info.push_back(ParamInfo(PK_Vector));
    } else if (llvm::ConstantInt *C = dyn_cast<llvm::ConstantInt>(Step)) {
      if (C->isZero()) {
        MangledParams << "u";
        Tys.push_back(Arg->getType());
        Info.push_back(ParamInfo(PK_Uniform));
      } else {
        MangledParams << "l";
        if (!C->isOne())
          MangledParams << C->getZExtValue();
        Tys.push_back(Arg->getType());
        Info.push_back(ParamInfo(PK_LinearConst, C));
      }
    } else if (llvm::MDString *StepName = dyn_cast<llvm::MDString>(Step)) {
      // Search Func parameters for StepName to determine the index.
      unsigned Number = 0;
      for (llvm::Function::arg_iterator I = Func->arg_begin(),
                                        IE = Func->arg_end();
           I != IE; ++I, ++Number) {
        if (I->getName().equals(StepName->getString()))
          break;
      }
      if (Number >= Func->arg_size())
        return 0; // Metadata is broken.
      MangledParams << "s" << Number;
      Tys.push_back(Arg->getType());

      llvm::LLVMContext &Context = Func->getContext();
      Step = llvm::ConstantInt::get(llvm::Type::getInt32Ty(Context), Number);
      Info.push_back(ParamInfo(PK_Linear, Step));
    } else {
      // Unknown Step type.
      return 0;
    }
  }

  if (Mask)
    Tys.push_back(VectorDataTy);

  llvm::Type *RetTy = Func->getReturnType();
  RetTy = RetTy->isVoidTy() ? RetTy : VectorDataTy;
  return llvm::FunctionType::get(RetTy, Tys, false);
}

static void setVectorVariantAttributes(llvm::Function *Func,
                                       llvm::Function *NewFunc,
                                       const std::string &Processor) {
  llvm::AttrBuilder NewFuncAttrs(Func->getAttributes(),
                                 llvm::AttributeSet::FunctionIndex);

  std::string CPU = llvm::StringSwitch<std::string>(Processor)
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
    NewFunc->setAttributes(
        llvm::AttributeSet::get(NewFunc->getContext(),
                                llvm::AttributeSet::FunctionIndex,
                                NewFuncAttrs));
}

static void createVectorVariantWrapper(llvm::Function *ScalarFunc,
                                       llvm::Function *VectorFunc,
                                       unsigned VLen,
                                       const SmallVectorImpl<ParamInfo> &Info) {
  assert(ScalarFunc->arg_size() == Info.size() &&
         "Wrong number of parameter infos");
  assert((VLen & (VLen - 1)) == 0 && "VLen must be a power-of-2");

  bool IsMasked = VectorFunc->arg_size() == ScalarFunc->arg_size() + 1;
  llvm::LLVMContext &Context = ScalarFunc->getContext();
  llvm::BasicBlock *Entry
    = llvm::BasicBlock::Create(Context, "entry", VectorFunc);
  llvm::BasicBlock *LoopCond
    = llvm::BasicBlock::Create(Context, "loop.cond", VectorFunc);
  llvm::BasicBlock *LoopBody
    = llvm::BasicBlock::Create(Context, "loop.body", VectorFunc);
  llvm::BasicBlock *MaskOn
    = IsMasked ? llvm::BasicBlock::Create(Context, "mask_on", VectorFunc) : 0;
  llvm::BasicBlock *MaskOff
    = IsMasked ? llvm::BasicBlock::Create(Context, "mask_off", VectorFunc) : 0;
  llvm::BasicBlock *LoopStep
    = llvm::BasicBlock::Create(Context, "loop.step", VectorFunc);
  llvm::BasicBlock *LoopEnd
    = llvm::BasicBlock::Create(Context, "loop.end", VectorFunc);

  llvm::Value *VectorRet = 0;
  SmallVector<llvm::Value*, 4> VectorArgs;

  // The loop counter.
  llvm::Type *IndexTy = llvm::Type::getInt32Ty(Context);
  llvm::Value *Index = 0;
  llvm::Value *Mask = 0;

  // Copy the names from the scalar args to the vector args.
  {
    llvm::Function::arg_iterator SI = ScalarFunc->arg_begin(),
                                 SE = ScalarFunc->arg_end(),
                                 VI = VectorFunc->arg_begin();
    for ( ; SI != SE; ++SI, ++VI)
      VI->setName(SI->getName());
    if (IsMasked)
      VI->setName("mask");
  }

  llvm::IRBuilder<> Builder(Entry);
  {
    if (!VectorFunc->getReturnType()->isVoidTy())
      VectorRet = Builder.CreateAlloca(VectorFunc->getReturnType());

    Index = Builder.CreateAlloca(IndexTy, 0, "index");
    Builder.CreateStore(llvm::ConstantInt::get(IndexTy, 0), Index);

    llvm::Function::arg_iterator VI = VectorFunc->arg_begin();
    for (SmallVectorImpl<ParamInfo>::const_iterator I = Info.begin(),
         IE = Info.end(); I != IE; ++I, ++VI) {
      llvm::Value *Arg = VI;
      switch (I->Kind) {
      case PK_Vector:
        assert(Arg->getType()->isVectorTy() && "Not a vector");
        assert(VLen == Arg->getType()->getVectorNumElements() &&
               "Wrong number of elements");
        break;
      case PK_LinearConst:
        Arg = buildLinearArg(Builder, VLen, Arg, I->Step);
        Arg->setName(VI->getName() + ".linear");
        break;
      case PK_Linear: {
        unsigned Number = cast<llvm::ConstantInt>(I->Step)->getZExtValue();
        llvm::Function::arg_iterator ArgI = VectorFunc->arg_begin();
        std::advance(ArgI, Number);
        llvm::Value *Step = ArgI;
        Arg = buildLinearArg(Builder, VLen, Arg, Step);
        Arg->setName(VI->getName() + ".linear");
      } break;
      case PK_Uniform:
        Arg = Builder.CreateVectorSplat(VLen, Arg);
        Arg->setName(VI->getName() + ".uniform");
        break;
      }
      VectorArgs.push_back(Arg);
    }

    if (IsMasked)
      Mask = buildMask(Builder, VLen, VI);

    Builder.CreateBr(LoopCond);
  }

  Builder.SetInsertPoint(LoopCond);
  {
    llvm::Value *Cond = Builder.CreateICmpULT(
        Builder.CreateLoad(Index), llvm::ConstantInt::get(IndexTy, VLen));
    Builder.CreateCondBr(Cond, LoopBody, LoopEnd);
  }

  llvm::Value *VecIndex = 0;

  Builder.SetInsertPoint(LoopBody);
  {
    VecIndex = Builder.CreateLoad(Index);
    if (IsMasked) {
      llvm::Value *ScalarMask = Builder.CreateExtractElement(Mask, VecIndex);
      Builder.CreateCondBr(ScalarMask, MaskOn, MaskOff);
    }
  }

  Builder.SetInsertPoint(IsMasked ? MaskOn : LoopBody);
  {
    // Build the argument list for the scalar function by extracting element
    // 'VecIndex' from the vector arguments.
    SmallVector<llvm::Value*, 4> ScalarArgs;
    for (SmallVectorImpl<llvm::Value*>::iterator VI = VectorArgs.begin(),
         VE = VectorArgs.end(); VI != VE; ++VI) {
      assert((*VI)->getType()->isVectorTy() && "Not a vector");
      ScalarArgs.push_back(Builder.CreateExtractElement(*VI, VecIndex));
    }

    // Call the scalar function with the extracted scalar arguments.
    llvm::Value *ScalarRet = Builder.CreateCall(ScalarFunc, ScalarArgs);

    // If the function returns a value insert the scalar return value into the
    // vector return value.
    if (VectorRet) {
      llvm::Value *V = Builder.CreateLoad(VectorRet);
      V = Builder.CreateInsertElement(V, ScalarRet, VecIndex);
      Builder.CreateStore(V, VectorRet);
    }

    Builder.CreateBr(LoopStep);
  }

  if (IsMasked) {
    Builder.SetInsertPoint(MaskOff);
    if (VectorRet) {
      llvm::Value *V = Builder.CreateLoad(VectorRet);
      llvm::Value *Zero
        = llvm::Constant::getNullValue(ScalarFunc->getReturnType());
      V = Builder.CreateInsertElement(V, Zero, VecIndex);
      Builder.CreateStore(V, VectorRet);
    }
    Builder.CreateBr(LoopStep);
  }

  Builder.SetInsertPoint(LoopStep);
  {
    // Index = Index + 1
    VecIndex = Builder.CreateAdd(VecIndex, llvm::ConstantInt::get(IndexTy, 1));
    Builder.CreateStore(VecIndex, Index);
    Builder.CreateBr(LoopCond);
  }

  Builder.SetInsertPoint(LoopEnd);
  {
    if (VectorRet)
      Builder.CreateRet(Builder.CreateLoad(VectorRet));
    else
      Builder.CreateRetVoid();
  }
}

static bool createVectorVariant(llvm::MDNode *Root,
                                const FunctionDecl *FD,
                                llvm::Function *F) {
  llvm::Module &M = *F->getParent();

  if (Root->getNumOperands() == 0)
    return false;
  llvm::Function *Func = dyn_cast<llvm::Function>(Root->getOperand(0));
  if (Func != F)
    return false;

  bool Elemental = false;

  unsigned VariantIndex = 0;
  llvm::MDNode *ArgName = 0,
               *ArgStep = 0,
               *VecLength = 0,
               *Processor = 0,
               *Mask = 0,
               *Variant = 0;

  for (unsigned i = 1, ie = Root->getNumOperands(); i < ie; ++i) {
    llvm::MDNode *Node = dyn_cast<llvm::MDNode>(Root->getOperand(i));
    if (!Node || Node->getNumOperands() < 1)
      return false;
    llvm::MDString *Name = dyn_cast<llvm::MDString>(Node->getOperand(0));
    if (!Name)
      return false;

    if (Name->getString() == "elemental") {
      Elemental = true;
    } else if (Name->getString() == "arg_name") {
      ArgName = Node;
    } else if (Name->getString() == "arg_step") {
      ArgStep = Node;
    } else if (Name->getString() == "vec_length") {
      VecLength = Node;
    } else if (Name->getString() == "processor") {
      Processor = Node;
    } else if (Name->getString() == "mask") {
      Mask = Node;
    } else if (Name->getString() == "variant") {
      VariantIndex = i;
      Variant = Node;
    } else {
      DEBUG(llvm::dbgs() << "Unknown metadata " << Name->getString() << "\n");
      return false;
    }
  }

  if (!Elemental || !ArgName || !ArgStep || !VecLength || !Variant) {
    DEBUG(llvm::dbgs() << "Missing necessary metadata node" << "\n");
    return false;
  }

  if (llvm::Value *V = Variant->getOperand(1))
    if (!V->getType()->isVoidTy())
      return false;

  // The default processor is pentium_4.
  std::string ProcessorName = "pentium_4";
  if (Processor) {
    if (Processor->getNumOperands() != 2)
      return false;
    llvm::MDString *Name = dyn_cast<llvm::MDString>(Processor->getOperand(1));
    if (!Name)
      return false;
    ProcessorName = Name->getString().str();
  }
  ISAClass ISA = getISAClass(ProcessorName);
  if (ISA == IC_Unknown)
    return false;

  bool IsMasked = true;
  if (Mask) {
    if (Mask->getNumOperands() != 2)
      return false;
    llvm::ConstantInt *C = dyn_cast<llvm::ConstantInt>(Mask->getOperand(1));
    if (!C)
      return false;
    IsMasked = C->isOne();
  }

  llvm::Type *VectorDataTy = 0;
  uint64_t VLen = 0;
  {
    if (VecLength->getNumOperands() != 3)
      return false;
    llvm::Type *Ty = VecLength->getOperand(1)->getType();
    if (!llvm::VectorType::isValidElementType(Ty))
      return false;

    llvm::Value *VL = VecLength->getOperand(2);
    assert(isa<llvm::ConstantInt>(VL) && "vector length constant expected");
    VLen = cast<llvm::ConstantInt>(VL)->getZExtValue();
    VectorDataTy = llvm::VectorType::get(Ty, VLen);
  }

  SmallVector<ParamInfo, 4> Info;
  SmallString<16> ParamStr;
  llvm::raw_svector_ostream MangledParams(ParamStr);
  llvm::FunctionType *NewFuncTy = encodeParameters(Func, ArgName, ArgStep,
                                                   IsMasked, ISA, VectorDataTy,
                                                   Info, MangledParams);
  if (!NewFuncTy)
    return false;

  // Generate the mangled name.
  SmallString<32> NameStr;
  llvm::raw_svector_ostream MangledName(NameStr);
  MangledName << "_ZGV" // Magic prefix
              << encodeISAClass(ISA)
              << (IsMasked ? 'M' : 'N')
              << VLen
              << MangledParams.str()
              << "_"
              << Func->getName();

  DEBUG(llvm::dbgs() << "Creating elemental function "
                     << MangledName.str() << "\n");

  // Declare the vector variant function in to the module.
  llvm::Function *NewFunc =
    dyn_cast<llvm::Function>(M.getOrInsertFunction(MangledName.str(), NewFuncTy));
  if (!NewFunc || !NewFunc->empty())
    return false;

  setVectorVariantAttributes(Func, NewFunc, ProcessorName);

  // Define the vector variant if the scalar function is not a declaration.
  if (FD->hasBody())
    createVectorVariantWrapper(Func, NewFunc, VLen, Info);

  // Update the vector variant metadata.
  {
    assert(VariantIndex && "invalid variant index");
    llvm::LLVMContext &Context = Func->getContext();
    llvm::Value *VariantMDArgs[] = {
      llvm::MDString::get(Context, "variant"),
      NewFunc
    };
    llvm::MDNode *VariantNode = llvm::MDNode::get(Context, VariantMDArgs);
    Root->replaceOperandWith(VariantIndex, VariantNode);
  }

  return true;
}

void CodeGenModule::EmitCilkElementalVariants() {
  for (SmallVectorImpl<ElementalVariantInfo>::iterator
      I = ElementalVariantToEmit.begin(),
      E = ElementalVariantToEmit.end(); I != E; ++I)
    createVectorVariant(I->KernelMD, I->FD, I->Fn);
}

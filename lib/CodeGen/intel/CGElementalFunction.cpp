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

#if INTEL_SPECIFIC_CILKPLUS

#include "CodeGenModule.h"
#include "clang/Basic/TargetInfo.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "elemental-codegen"

using namespace clang;
using namespace CodeGen;
typedef CodeGenModule::CilkElementalGroup CilkElementalGroup;

static llvm::MDNode *MakeVecLengthMetadata(CodeGenModule &CGM, StringRef Name,
                                           QualType T, uint64_t VL) {
  llvm::LLVMContext &Context = CGM.getLLVMContext();
  llvm::Metadata *attrMDArgs[] = {
      llvm::MDString::get(Context, Name),
      llvm::ValueAsMetadata::get(
          llvm::UndefValue::get(CGM.getTypes().ConvertType(T))),
      llvm::ValueAsMetadata::get(llvm::ConstantInt::get(CGM.Int32Ty, VL))};
  return llvm::MDNode::get(Context, attrMDArgs);
}

static bool CheckElementalArguments(CodeGenModule &CGM, const FunctionDecl *FD,
                                    llvm::Function *Fn, bool &HasThis) {
  // Check the return type.
  QualType RetTy = FD->getReturnType();
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

  HasThis = isa<CXXMethodDecl>(FD) && cast<CXXMethodDecl>(FD)->isInstance();

  // At this point, no passing struct arguments by value.
  unsigned NumArgs = FD->param_size();
  unsigned NumLLVMArgs = Fn->arg_size();

  // There is a single implicit 'this' parameter.
  if (HasThis && (NumArgs + 1 == NumLLVMArgs))
    return true;

  return NumArgs == NumLLVMArgs;
}

/// \brief Generates a properstep argument for each function parameter.
/// Returns true if this is a non-linear and non-uniform variable.
/// Otherwise returns false.
static bool handleParameter(CodeGenModule &CGM, CilkElementalGroup &G,
                            StringRef ParmName,
                            SmallVectorImpl<llvm::Metadata *> &StepArgs,
                            SmallVectorImpl<llvm::Metadata *> &AligArgs) {
  // Update the alignment args.
  unsigned Alignment;
  if (G.getAlignedAttr(ParmName, &Alignment)) {
    AligArgs.push_back(llvm::ValueAsMetadata::get(
        llvm::ConstantInt::get(CGM.IntTy, Alignment)));
  } else {
    AligArgs.push_back(
        llvm::ValueAsMetadata::get(llvm::UndefValue::get(CGM.IntTy)));
  }
  // Update the step args.
  std::pair<int, std::string> LinStep;
  if (G.getUniformAttr(ParmName)) {
    // If this is uniform, then use step 0 as placeholder.
    StepArgs.push_back(
        llvm::ValueAsMetadata::get(llvm::ConstantInt::get(CGM.IntTy, 0)));
    return false;
  } else if (G.getLinearAttr(ParmName, &LinStep)) {
    if (LinStep.first != 0) {
      StepArgs.push_back(llvm::ValueAsMetadata::get(
          llvm::ConstantInt::get(CGM.IntTy, LinStep.first)));
    } else {
      StepArgs.push_back(
          llvm::MDString::get(CGM.getLLVMContext(), LinStep.second));
    }
    return false;
  }
  // If this is non-linear and non-uniform, use undefined step as placeholder.
  StepArgs.push_back(
      llvm::ValueAsMetadata::get(llvm::UndefValue::get(CGM.IntTy)));
  return true;
}

// Vector variants CodeGen for elemental functions.
namespace {

enum ISAClass { IC_XMM, IC_YMM1, IC_YMM2, IC_ZMM, IC_Unknown };

enum ParamKind { PK_Vector, PK_LinearConst, PK_Linear, PK_Uniform };

struct ParamInfo {
  ParamKind Kind;
  llvm::Value *Step;

  ParamInfo(ParamKind Kind) : Kind(Kind), Step(0) {}

  ParamInfo(ParamKind Kind, llvm::Value *Step) : Kind(Kind), Step(Step) {}
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
  case IC_XMM:
    return 'x';
  case IC_YMM1:
    return 'y';
  case IC_YMM2:
    return 'Y';
  case IC_ZMM:
    return 'z';
  case IC_Unknown:
    break;
  }
  llvm_unreachable("Unknown ISA");
  return 0;
}

static llvm::FunctionType *
encodeParameters(llvm::Function *Func, llvm::MDNode *ArgName,
                 llvm::MDNode *ArgStep, llvm::MDNode *ArgAlig, bool Mask,
                 llvm::Type *VectorDataTy, SmallVectorImpl<ParamInfo> &Info,
                 llvm::raw_svector_ostream &MangledParams) {
  assert(Func && "Func is null");
  unsigned ArgSize = Func->arg_size();

  assert((ArgName->getNumOperands() == 1 + ArgSize) && "invalid metadata");
  assert((ArgStep->getNumOperands() == 1 + ArgSize) && "invalid metadata");
  assert((ArgAlig->getNumOperands() == 1 + ArgSize) && "invalid metadata");

  SmallVector<llvm::Type *, 4> Tys;
  llvm::Function::const_arg_iterator Arg = Func->arg_begin();
  for (unsigned i = 1, ie = 1 + ArgSize; i < ie; ++i, ++Arg) {
    llvm::Metadata *Step = ArgStep->getOperand(i);
    if (llvm::ValueAsMetadata *VAM = dyn_cast<llvm::ValueAsMetadata>(Step)) {
      llvm::Value *V = VAM->getValue();
      if (isa<llvm::UndefValue>(V)) {
        MangledParams << "v";
        unsigned VL = VectorDataTy->getVectorNumElements();
        Tys.push_back(llvm::VectorType::get(Arg->getType(), VL));
        Info.push_back(ParamInfo(PK_Vector));
      } else if (llvm::ConstantInt *C = dyn_cast<llvm::ConstantInt>(V)) {
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
      } else
        llvm_unreachable("invalid step metadata");
    } else if (llvm::MDString *StepName = dyn_cast<llvm::MDString>(Step)) {
      // Search parameter names for StepName to determine the index.
      unsigned Idx = 0, NumParams = ArgName->getNumOperands() - 1;
      for (; Idx < NumParams; ++Idx) {
        // The first operand is the argument name kind metadata.
        llvm::Metadata *V = ArgName->getOperand(Idx + 1);
        assert(isa<llvm::MDString>(V) && "invalid metadata");
        llvm::MDString *MS = cast<llvm::MDString>(V);
        if (MS->getString().equals(StepName->getString()))
          break;
      }
      assert((Idx < NumParams) && "step parameter not found");

      MangledParams << "s" << Idx;
      Tys.push_back(Arg->getType());
      llvm::LLVMContext &Context = Func->getContext();
      Info.push_back(ParamInfo(
          PK_Linear,
          llvm::ConstantInt::get(llvm::Type::getInt32Ty(Context), Idx)));
    } else
      llvm_unreachable("invalid step metadata");

    llvm::Metadata *Align = ArgAlig->getOperand(i);
    if (llvm::ValueAsMetadata *VAM = dyn_cast<llvm::ValueAsMetadata>(Align)) {
      llvm::Value *V = VAM->getValue();
      if (llvm::ConstantInt *C = dyn_cast<llvm::ConstantInt>(V)) {
        MangledParams << "a";
        if (!C->isZero())
          MangledParams << C->getZExtValue();
      }
    } else if (llvm::MDString *AligName = dyn_cast<llvm::MDString>(Align)) {
      // Search parameter names for AligName to determine the index.
      unsigned Idx = 0, NumParams = ArgName->getNumOperands() - 1;
      for (; Idx < NumParams; ++Idx) {
        // The first operand is the argument name kind metadata.
        llvm::Metadata *V = ArgName->getOperand(Idx + 1);
        assert(isa<llvm::MDString>(V) && "invalid metadata");
        llvm::MDString *MS = cast<llvm::MDString>(V);
        if (MS->getString().equals(AligName->getString()))
          break;
      }
      assert((Idx < NumParams) && "aligned parameter not found");

      MangledParams << "A" << Idx;
    } else
      llvm_unreachable("invalid step metadata");
  }

  if (Mask)
    Tys.push_back(VectorDataTy);

  llvm::Type *RetTy = Func->getReturnType();
  RetTy = RetTy->isVoidTy() ? RetTy : VectorDataTy;
  return llvm::FunctionType::get(RetTy, Tys, false);
}

/// Extracts info from metadata of this elemental function for its mangled name.
/// Returns true if all extracted parts are valid.
/// Otherwise returns false.
static bool getMetaInfo(llvm::Function *Func, llvm::MDNode *MetadataRoot,
                       llvm::MDNode **ArgName, llvm::MDNode **ArgAlig,
                       llvm::MDNode **ArgStep,
                       llvm::Type **VectorDataTy, uint64_t *VLen,
                       llvm::MDNode **Mask,
                       llvm::MDNode **Variant, unsigned *VariantIndex,
                       std::string& ProcessorName) {

  if (MetadataRoot->getNumOperands() == 0)
    return false;

  llvm::ValueAsMetadata *FuncVAM =
      dyn_cast<llvm::ValueAsMetadata>(MetadataRoot->getOperand(0));
  if (!FuncVAM)
    return false;
  llvm::Function *F = dyn_cast<llvm::Function>(FuncVAM->getValue());
  if (Func != F)
    return false;

  *VectorDataTy = 0;
  *VLen = 0;
  ProcessorName = "pentium_4"; // The default processor is pentium_4.
  bool Elemental = false;

  for (unsigned i = 1, ie = MetadataRoot->getNumOperands(); i < ie; ++i) {
    llvm::MDNode *Node = dyn_cast<llvm::MDNode>(MetadataRoot->getOperand(i));
    if (!Node || Node->getNumOperands() < 1)
      return false;
    llvm::MDString *Name = dyn_cast<llvm::MDString>(Node->getOperand(0));
    if (!Name)
      return false;
    if (Name->getString() == "elemental")
      Elemental = true;
    else if (Name->getString() == "arg_name")
      *ArgName = Node;
    else if (Name->getString() == "arg_step")
      *ArgStep = Node;
    else if (Name->getString() == "arg_alig") {
      if (ArgAlig)
        *ArgAlig = Node;
    } else if (Name->getString() == "vec_length") {
      if (Node->getNumOperands() != 3)
        return false;
      llvm::ValueAsMetadata *TypeVAM =
          dyn_cast<llvm::ValueAsMetadata>(Node->getOperand(1));
      if (!TypeVAM)
        return false;
      llvm::Type *Ty = TypeVAM->getType();
      if (!llvm::VectorType::isValidElementType(Ty))
        return false;
      llvm::ValueAsMetadata *ValVAM =
          dyn_cast<llvm::ValueAsMetadata>(Node->getOperand(2));
      if (!ValVAM)
        return false;
      llvm::Value *VL = ValVAM->getValue();
      assert(isa<llvm::ConstantInt>(VL) && "vector length constant expected");
      *VLen = cast<llvm::ConstantInt>(VL)->getZExtValue();
      *VectorDataTy = llvm::VectorType::get(Ty, *VLen);
    } else if (Name->getString() == "processor") {
      if (Node->getNumOperands() != 2)
        return false;
      llvm::MDString *PName = dyn_cast<llvm::MDString>(Node->getOperand(1));
      if (!PName)
        return false;
      ProcessorName = PName->getString().str();
    } else if (Name->getString() == "mask") {
      if (Mask)
        *Mask = Node;
    } else if (Name->getString() == "variant") {
      if (Variant) {
        *VariantIndex = i;
        *Variant = Node;
      }
    } else {
      DEBUG(llvm::dbgs() << "Unknown metadata " << Name->getString() << "\n");
      return false;
    }
  }
  return Elemental;
}

/// Saves the generated mangled name in the give stream.
/// Returns true if the generation is OK.
/// Otherwise returns false.
static bool writeMangledName(std::string ProcessorName, bool IsMasked,
                             llvm::raw_svector_ostream &MangledName,
                             llvm::MDNode *ArgName, llvm::MDNode *ArgStep,
                             llvm::MDNode *ArgAlig, llvm::Type *VectorDataTy,
                             uint64_t VLen, llvm::Function *Func) {
  ISAClass ISA = getISAClass(ProcessorName);
  if (ISA == IC_Unknown)
    return false;
  SmallVector<ParamInfo, 4> Info;
  SmallString<16> ParamStr;
  llvm::raw_svector_ostream MangledParams(ParamStr);
  llvm::FunctionType *NewFuncTy =
      encodeParameters(Func, ArgName, ArgStep, ArgAlig, IsMasked, VectorDataTy,
                       Info, MangledParams);
  if (!NewFuncTy)
    return false;
  MangledName << "_ZGV" // Magic prefix
              << encodeISAClass(ISA) << (IsMasked ? 'M' : 'N') << VLen
              << MangledParams.str() << "_";
  DEBUG(llvm::dbgs() << "Dealing with elemental function " << MangledName.str()
                     << "\n");
  return true;
}


/// \brief Generates mangled name for a given function.
/// Returns true if a mangled name was successfully generated.
/// Otherwise returns false.
static bool createMangledName(llvm::Function *Func, llvm::MDNode *MD,
                              bool IsMasked,
                              llvm::raw_svector_ostream &MangledName) {
  llvm::MDNode *ArgName = 0, *ArgStep = 0, *ArgAlig = 0;
  llvm::Type *VectorDataTy = 0;
  uint64_t VLen = 0;
  std::string ProcessorName;

  if (!getMetaInfo(Func, MD, &ArgName, &ArgAlig, &ArgStep,
                  &VectorDataTy, &VLen,
                  nullptr, nullptr, nullptr, ProcessorName))
    return false;

  if (!ArgName || !ArgStep || !ArgAlig || !VectorDataTy) {
    DEBUG(llvm::dbgs() << "Missing necessary metadata node" << "\n");
    return false;
  }

  return writeMangledName(ProcessorName, IsMasked, MangledName, ArgName,
                          ArgStep, ArgAlig, VectorDataTy, VLen, Func);
}

void CodeGenModule::EmitCilkElementalAttribute(llvm::Function *Func,
                                               llvm::MDNode *MD,
                                               bool IsMasked) {
  SmallString<32> NameStr;
  llvm::raw_svector_ostream MangledName(NameStr);

  if (createMangledName(Func, MD, IsMasked, MangledName)) {
    llvm::AttrBuilder FuncAttrs(Func->getAttributes(),
                                llvm::AttributeSet::FunctionIndex);

    // Store new mangled name in ElementalAttributes map
    auto PairToInsert = std::make_pair(MangledName.str(), Func);
    auto InsertionResult = ElementalAttributes.insert(PairToInsert);
    auto AttributeIterator = InsertionResult.first;
    bool WasInserted = InsertionResult.second;
    if (!WasInserted || AttributeIterator->first() == "")
      return;

    FuncAttrs.addAttribute(AttributeIterator->first());

    if (FuncAttrs.hasAttributes())
      Func->setAttributes(llvm::AttributeSet::get(
          Func->getContext(), llvm::AttributeSet::FunctionIndex, FuncAttrs));
  }
}

// The following is common part for 'cilk vector functions' and
// 'omp declare simd' functions metadata generation.
void CodeGenModule::EmitVectorVariantsMetadata(const CGFunctionInfo &FnInfo,
                                               const FunctionDecl *FD,
                                               llvm::Function *Fn,
                                               GroupMap &Groups) {

  // Do not emit any vector variant if there is an unsupported feature.
  bool HasImplicitThis = false;
  if (!CheckElementalArguments(*this, FD, Fn, HasImplicitThis))
    return;

  llvm::LLVMContext &Context = getLLVMContext();
  ASTContext &C = getContext();

  // Common metadata nodes.
  llvm::NamedMDNode *CilkElementalMetadata =
      getModule().getOrInsertNamedMetadata("cilk.functions");
  llvm::Metadata *ElementalMDArgs[] = {
      llvm::MDString::get(Context, "elemental")};
  llvm::MDNode *ElementalNode = llvm::MDNode::get(Context, ElementalMDArgs);
  llvm::Metadata *MaskMDArgs[] = {
      llvm::MDString::get(Context, "mask"),
      llvm::ValueAsMetadata::get(
          llvm::ConstantInt::get(llvm::IntegerType::getInt1Ty(Context), 1))};
  llvm::MDNode *MaskNode = llvm::MDNode::get(Context, MaskMDArgs);
  MaskMDArgs[1] = llvm::ValueAsMetadata::get(
      llvm::ConstantInt::get(llvm::IntegerType::getInt1Ty(Context), 0));
  llvm::MDNode *NoMaskNode = llvm::MDNode::get(Context, MaskMDArgs);
  SmallVector<llvm::Metadata *, 8> ParameterNameArgs;
  ParameterNameArgs.push_back(llvm::MDString::get(Context, "arg_name"));
  llvm::MDNode *ParameterNameNode = 0;

  for (auto &GI : Groups) {
    CilkElementalGroup &G = GI.second;

    // Parameter information.
    QualType FirstNonStepParmType;
    SmallVector<llvm::Metadata *, 8> AligArgs;
    SmallVector<llvm::Metadata *, 8> StepArgs;
    AligArgs.push_back(llvm::MDString::get(Context, "arg_alig"));
    StepArgs.push_back(llvm::MDString::get(Context, "arg_step"));

    // Handle implicit 'this' parameter if necessary.
    if (HasImplicitThis) {
      ParameterNameArgs.push_back(llvm::MDString::get(Context, "this"));
      bool IsNonStepParm =
          handleParameter(*this, G, "this", StepArgs, AligArgs);
      // This implicit 'this' is considered as the first parameter. This
      // aligns with the icc 13.1 implementation.
      if (IsNonStepParm)
        FirstNonStepParmType = cast<CXXMethodDecl>(FD)->getThisType(C);
    }

    // Handle explicit paramenters.
    for (unsigned I = 0; I != FD->getNumParams(); ++I) {
      const ParmVarDecl *Parm = FD->getParamDecl(I);
      StringRef ParmName = Parm->getName();
      if (!ParameterNameNode)
        ParameterNameArgs.push_back(llvm::MDString::get(Context, ParmName));
      bool IsNonStepParm =
          handleParameter(*this, G, ParmName, StepArgs, AligArgs);
      if (IsNonStepParm && FirstNonStepParmType.isNull())
        FirstNonStepParmType = Parm->getType();
    }

    llvm::MDNode *StepNode = llvm::MDNode::get(Context, StepArgs);
    llvm::MDNode *AligNode = llvm::MDNode::get(Context, AligArgs);
    if (!ParameterNameNode)
      ParameterNameNode = llvm::MDNode::get(Context, ParameterNameArgs);

    // If there is no vectorlengthfor() in this group, determine the
    // characteristic type. This can depend on the linear/uniform attributes,
    // so it can differ between groups.
    //
    // The rules for computing the characteristic type are:
    //
    // a) For a non-void function, the characteristic data type is the
    //    return type.
    //
    // b) If the function has any non-uniform, non-linear parameters, the
    //    the characteristic data type is the type of the first such parameter.
    //
    // c) If the characteristic data type determined by a) or b) above is
    //    struct, union, or class type which is pass-by-value (except fo
    //    the type that maps to the built-in complex data type)
    //    the characteristic data type is int.
    //
    // d) If none of the above three cases is applicable,
    //    the characteristic data type is int.
    //
    // e) For Intel Xeon Phi native and offload compilation, if the resulting
    //    characteristic data type is 8-bit or 16-bit integer data type
    //    the characteristic data type is int.
    //
    // These rules missed the reference types and we use their pointer types.
    //
    if (G.VecLengthFor.empty()) {
      QualType FnRetTy = FD->getReturnType();
      QualType CharacteristicType;
      if (!FnRetTy->isVoidType())
        CharacteristicType = FnRetTy;
      else if (!FirstNonStepParmType.isNull())
        CharacteristicType = FirstNonStepParmType.getCanonicalType();
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

    //    // If no mask variants are specified, generate both.
    //    if (G.Mask.empty()) {
    //      G.Mask.push_back(1);
    //      G.Mask.push_back(0);
    //    }

    // If no target processor is specified, use the current target.
    if (G.Processor.empty())
      G.Processor.push_back(CilkProcessorAttr::Unspecified);

    // If no vector length is specified, push a dummy value to iterate over.
    if (G.VecLength.empty())
      G.VecLength.push_back(0);

    for (auto &TI : G.VecLengthFor) {
      for (auto &PI : G.Processor) {
        // Processor node.
        uint64_t VectorRegisterBytes = 0;
        llvm::MDNode *ProcessorNode = 0;
        if (PI == CilkProcessorAttr::Unspecified) {
          // Since no vector(processor(...)) attribute is present, inspect the
          // current target features to determine the appropriate vector size.
          // This is currently X86 specific.
          if (Target.hasFeature("avx2"))
            VectorRegisterBytes = 64;
          else if (Target.hasFeature("avx"))
            VectorRegisterBytes = 32;
          else if (Target.hasFeature("sse2"))
            VectorRegisterBytes = 16;
          else if (Target.hasFeature("sse") && TI->isFloatingType() &&
                   C.getTypeSizeInChars(TI).getQuantity() == 4)
            VectorRegisterBytes = 16;
          else if (Target.hasFeature("mmx") && (TI)->isIntegerType())
            VectorRegisterBytes = 8;
        } else {
          llvm::Metadata *attrMDArgs[] = {
              llvm::MDString::get(Context, "processor"),
              llvm::MDString::get(Context,
                                  CilkProcessorAttr::getProcessorString(PI))};
          ProcessorNode = llvm::MDNode::get(Context, attrMDArgs);
          VectorRegisterBytes = CilkProcessorAttr::getProcessorVectorBytes(PI);
        }

        for (auto &LI : G.VecLength){
          uint64_t VL =
              LI ? LI : (CharUnits::fromQuantity(VectorRegisterBytes) /
                           C.getTypeSizeInChars(TI));

          llvm::MDNode *VecTypeNode =
              MakeVecLengthMetadata(*this, "vec_length", TI, VL);

          {
            SmallVector<llvm::Metadata *, 7> kernelMDArgs;
            kernelMDArgs.push_back(llvm::ValueAsMetadata::get(Fn));
            kernelMDArgs.push_back(ElementalNode);
            kernelMDArgs.push_back(ParameterNameNode);
            kernelMDArgs.push_back(StepNode);
            kernelMDArgs.push_back(AligNode);
            kernelMDArgs.push_back(VecTypeNode);
            if (!G.Mask.empty())
              kernelMDArgs.push_back((G.Mask.back() == 0) ? NoMaskNode
                                                          : MaskNode);
            if (ProcessorNode)
              kernelMDArgs.push_back(ProcessorNode);
            llvm::MDNode *KernelMD = llvm::MDNode::get(Context, kernelMDArgs);
            CilkElementalMetadata->addOperand(KernelMD);
            // Add attributes to function.
            if (G.Mask.empty()) {
              EmitCilkElementalAttribute(Fn, KernelMD, /*IsMasked=*/true);
              EmitCilkElementalAttribute(Fn, KernelMD, /*IsMasked=*/false);
            } else {
              bool IsMasked = G.Mask.back() == 1;
              EmitCilkElementalAttribute(Fn, KernelMD, IsMasked);
            }
          }
          //          for (CilkElementalGroup::MaskVector::iterator
          //                MI = G.Mask.begin(),
          //                ME = G.Mask.end();
          //               MI != ME;
          //               ++MI) {
          //
          //            SmallVector <llvm::Value*, 7> kernelMDArgs;
          //            kernelMDArgs.push_back(Fn);
          //            kernelMDArgs.push_back(ElementalNode);
          //            kernelMDArgs.push_back(ParameterNameNode);
          //            kernelMDArgs.push_back(StepNode);
          //            kernelMDArgs.push_back(AligNode);
          //            kernelMDArgs.push_back(VecTypeNode);
          //            kernelMDArgs.push_back((*MI==0)?(NoMaskNode):(MaskNode));
          //            if (ProcessorNode)
          //              kernelMDArgs.push_back(ProcessorNode);
          //            kernelMDArgs.push_back(VariantNode);
          //            llvm::MDNode *KernelMD = llvm::MDNode::get(Context,
          //            kernelMDArgs);
          //            CilkElementalMetadata->addOperand(KernelMD);
          //            ElementalVariantToEmit.push_back(
          //                ElementalVariantInfo(&FnInfo, FD, Fn, KernelMD));
          //          }
        }
      }
    }
  }
}

void CodeGenModule::EmitCilkElementalMetadata(const CGFunctionInfo &FnInfo,
                                              const FunctionDecl *FD,
                                              llvm::Function *Fn) {
  if (!FD->hasAttr<CilkElementalAttr>())
    return;

  if (FD->isVariadic())
    return;

  if (!Fn)
    return;

  // Group elemental function attributes by vector() declaration.
  GroupMap Groups;
  for (Decl::attr_iterator AI = FD->attr_begin(), AE = FD->attr_end(); AI != AE;
       ++AI) {
    if (CilkElementalAttr *A = dyn_cast<CilkElementalAttr>(*AI)) {
      unsigned key = A->getGroup().getRawEncoding();
      Groups.FindAndConstruct(key);
    } else if (CilkProcessorAttr *A = dyn_cast<CilkProcessorAttr>(*AI)) {
      unsigned key = A->getGroup().getRawEncoding();
      Groups[key].Processor.push_back(A->getProcessor());
    } else if (CilkVecLengthAttr *A = dyn_cast<CilkVecLengthAttr>(*AI)) {
      unsigned key = A->getGroup().getRawEncoding();
      const Expr *LengthExpr = A->getLength();
      assert(isa<IntegerLiteral>(LengthExpr) && "integeral literal expected");
      unsigned VLen =
          cast<IntegerLiteral>(LengthExpr)->getValue().getZExtValue();
      Groups[key].VecLength.push_back(VLen);
    } else if (CilkMaskAttr *A = dyn_cast<CilkMaskAttr>(*AI)) {
      unsigned key = A->getGroup().getRawEncoding();
      Groups[key].Mask.push_back(A->getMask() ? 1 : 0);
    } else if (CilkLinearAttr *A = dyn_cast<CilkLinearAttr>(*AI)) {
      unsigned key = A->getGroup().getRawEncoding();
      std::string Name = A->getParameter()->getName();
      std::string IdName = "";
      int Step = 0;
      if (IdentifierInfo *S = A->getStepParameter()) {
        IdName = S->getName();
      } else if (IntegerLiteral *E =
                     dyn_cast<IntegerLiteral>(A->getStepExpr())) {
        Step = E->getValue().getSExtValue();
      }
      Groups[key].setLinear(Name, IdName, Step);
    } else if (CilkAlignedAttr *A = dyn_cast<CilkAlignedAttr>(*AI)) {
      unsigned key = A->getGroup().getRawEncoding();
      std::string Name = A->getParameter()->getName();
      int Align = -1;
      if (IntegerLiteral *E = dyn_cast<IntegerLiteral>(A->getAlignedExpr()))
        Align = E->getValue().getSExtValue();
      if (Align >= 0)
        Groups[key].setAligned(Name, Align);
    } else if (CilkUniformAttr *A = dyn_cast<CilkUniformAttr>(*AI)) {
      unsigned key = A->getGroup().getRawEncoding();
      Groups[key].setUniform(A->getParameter()->getName());
    }
  }

  EmitVectorVariantsMetadata(FnInfo, FD, Fn, Groups);
}

// Return a constant vector <0, 1, ..., N - 1>
static llvm::Constant *getIndicesVector(llvm::Type *Ty, unsigned N) {
  SmallVector<llvm::Constant *, 4> Indices;
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
    assert((Ty->isIntegerTy() || Ty->isPointerTy()) && "unexpected type");

  return B.CreateICmpNE(Mask, llvm::Constant::getNullValue(Mask->getType()));
}

static void setVectorVariantAttributes(llvm::Function *Func,
                                       llvm::Function *NewFunc,
                                       const std::string &Processor) {
  llvm::AttrBuilder NewFuncAttrs(Func->getAttributes(),
                                 llvm::AttributeSet::FunctionIndex);

  std::string CPU = llvm::StringSwitch<std::string>(Processor)
                        .Case("pentium_4", "pentium4")
                        .Case("pentium_4_sse3", "yonah")
                        .Case("core_2_duo_ssse3", "core2")
                        .Case("core_2_duo_sse4_1", "penryn")
                        .Case("core_i7_sse4_2", "corei7")
                        .Case("core_2nd_gen_avx", "corei7-avx")
                        .Case("core_3rd_gen_avx", "core-avx-i")
                        .Case("core_4th_gen_avx", "core-avx2")
                        .Case("mic", "")
                        .Default("");

  if (!CPU.empty())
    NewFuncAttrs.addAttribute("cpu", CPU);

  if (NewFuncAttrs.hasAttributes())
    NewFunc->setAttributes(llvm::AttributeSet::get(
        NewFunc->getContext(), llvm::AttributeSet::FunctionIndex,
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
  llvm::BasicBlock *Entry =
      llvm::BasicBlock::Create(Context, "entry", VectorFunc);
  llvm::BasicBlock *LoopCond =
      llvm::BasicBlock::Create(Context, "loop.cond", VectorFunc);
  llvm::BasicBlock *LoopBody =
      llvm::BasicBlock::Create(Context, "loop.body", VectorFunc);
  llvm::BasicBlock *MaskOn =
      IsMasked ? llvm::BasicBlock::Create(Context, "mask_on", VectorFunc) : 0;
  llvm::BasicBlock *MaskOff =
      IsMasked ? llvm::BasicBlock::Create(Context, "mask_off", VectorFunc) : 0;
  llvm::BasicBlock *LoopStep =
      llvm::BasicBlock::Create(Context, "loop.step", VectorFunc);
  llvm::BasicBlock *LoopEnd =
      llvm::BasicBlock::Create(Context, "loop.end", VectorFunc);

  llvm::Value *VectorRet = 0;
  SmallVector<llvm::Value *, 4> VectorArgs;

  // The loop counter.
  llvm::Type *IndexTy = llvm::Type::getInt32Ty(Context);
  llvm::Value *Index = 0;
  llvm::Value *Mask = 0;

  // Copy the names from the scalar args to the vector args.
  {
    llvm::Function::arg_iterator SI = ScalarFunc->arg_begin(),
                                 SE = ScalarFunc->arg_end(),
                                 VI = VectorFunc->arg_begin();
    for (; SI != SE; ++SI, ++VI)
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

    auto VI = VectorFunc->arg_begin();
    for (auto &I : Info) {
      auto &A = *VI;
      llvm::Value *Arg = &A;
      switch (I.Kind) {
      case PK_Vector:
        assert(Arg->getType()->isVectorTy() && "Not a vector");
        assert(VLen == Arg->getType()->getVectorNumElements() &&
               "Wrong number of elements");
        break;
      case PK_LinearConst:
        Arg = buildLinearArg(Builder, VLen, Arg, I.Step);
        Arg->setName(VI->getName() + ".linear");
        break;
      case PK_Linear: {
        unsigned Number = cast<llvm::ConstantInt>(I.Step)->getZExtValue();
        auto ArgI = VectorFunc->arg_begin();
        std::advance(ArgI, Number);
        auto &B = *ArgI;
        llvm::Value *Step = &B;
        Arg = buildLinearArg(Builder, VLen, Arg, Step);
        Arg->setName(VI->getName() + ".linear");
      } break;
      case PK_Uniform:
        Arg = Builder.CreateVectorSplat(VLen, Arg);
        Arg->setName(VI->getName() + ".uniform");
        break;
      }
      VectorArgs.push_back(Arg);
      VI++;
    }

    if (IsMasked){
      auto &C = *VI;
      Mask = buildMask(Builder, VLen, &C);
    }

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
    SmallVector<llvm::Value *, 4> ScalarArgs;
    for (auto &VI : VectorArgs) {
      assert((VI)->getType()->isVectorTy() && "Not a vector");
      ScalarArgs.push_back(Builder.CreateExtractElement(VI, VecIndex));
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
      llvm::Value *Zero =
          llvm::Constant::getNullValue(ScalarFunc->getReturnType());
      V = Builder.CreateInsertElement(V, Zero, VecIndex);
      Builder.CreateStore(V, VectorRet);
    }
    Builder.CreateBr(LoopStep);
  }

  Builder.SetInsertPoint(LoopStep);
  {
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

static bool createVectorVariant(llvm::MDNode *Root, const FunctionDecl *FD,
                                llvm::Function *Func) {
  std::string ProcessorName;
  unsigned VariantIndex = 0;
  llvm::MDNode *ArgName = 0, *ArgStep = 0, *ArgAlig = 0, *Mask = 0,
               *Variant = 0;
  llvm::Type *VectorDataTy = 0;
  uint64_t VLen = 0;

  if (!getMetaInfo(Func, Root, &ArgName, nullptr, &ArgStep, &VectorDataTy,
                   &VLen, &Mask, &Variant, &VariantIndex, ProcessorName))
    return false;

  if (!ArgName || !ArgStep || !VectorDataTy || !Variant) {
    DEBUG(llvm::dbgs() << "Missing necessary metadata node"
                       << "\n");
    return false;
  }

  if (auto *V =
          (dyn_cast<llvm::ValueAsMetadata>(Variant->getOperand(1)))->getValue())
    if (!V->getType()->isVoidTy())
      return false;

  bool IsMasked = true;
  if (Mask) {
    if (Mask->getNumOperands() != 2)
      return false;
    auto *V =
        (dyn_cast<llvm::ValueAsMetadata>(Mask->getOperand(1)))->getValue();
    llvm::ConstantInt *C = dyn_cast<llvm::ConstantInt>(V);
    if (!C)
      return false;
    IsMasked = C->isOne();
  }

  SmallVector<ParamInfo, 4> Info;
  SmallString<16> ParamStr;
  llvm::raw_svector_ostream MangledParams(ParamStr);
  llvm::FunctionType *NewFuncTy =
      encodeParameters(Func, ArgName, ArgStep, ArgAlig, IsMasked, VectorDataTy,
                       Info, MangledParams);
  if (!NewFuncTy)
    return false;

  // Generate the mangled name.
  SmallString<32> NameStr;
  llvm::raw_svector_ostream MangledName(NameStr);
  if (!writeMangledName(ProcessorName, IsMasked, MangledName, ArgName, ArgStep,
                        ArgAlig, VectorDataTy, VLen, Func))
    return false;

  // Declare the vector variant function in to the module.
  llvm::Module &M = *Func->getParent();
  llvm::Function *NewFunc = dyn_cast<llvm::Function>(
      M.getOrInsertFunction(MangledName.str(), NewFuncTy));
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
    llvm::Metadata *VariantMDArgs[] = {llvm::MDString::get(Context, "variant"),
                                       llvm::ValueAsMetadata::get(NewFunc)};
    llvm::MDNode *VariantNode = llvm::MDNode::get(Context, VariantMDArgs);
    Root->replaceOperandWith(VariantIndex, VariantNode);
  }
  return true;
}

void CodeGenModule::EmitCilkElementalVariants() {
  for (auto &I : ElementalVariantToEmit) {
    createVectorVariant(I.KernelMD, I.FD, I.Fn);
  }
}

#endif // INTEL_SPECIFIC_CILKPLUS

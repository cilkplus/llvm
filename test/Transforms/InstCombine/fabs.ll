; RUN: opt < %s -instcombine -S | FileCheck %s

; Make sure all library calls are eliminated when the input is known positive.

declare float @fabsf(float)
declare double @fabs(double)
declare fp128 @fabsl(fp128)

define float @square_fabs_call_f32(float %x) {
  %mul = fmul float %x, %x
  %fabsf = tail call float @fabsf(float %mul)
  ret float %fabsf

; CHECK-LABEL: square_fabs_call_f32(
; CHECK-NEXT: %mul = fmul float %x, %x
; CHECK-NEXT: ret float %mul
}

define double @square_fabs_call_f64(double %x) {
  %mul = fmul double %x, %x
  %fabs = tail call double @fabs(double %mul)
  ret double %fabs

; CHECK-LABEL: square_fabs_call_f64(
; CHECK-NEXT: %mul = fmul double %x, %x
; CHECK-NEXT: ret double %mul
}

define fp128 @square_fabs_call_f128(fp128 %x) {
  %mul = fmul fp128 %x, %x
  %fabsl = tail call fp128 @fabsl(fp128 %mul)
  ret fp128 %fabsl

; CHECK-LABEL: square_fabs_call_f128(
; CHECK-NEXT: %mul = fmul fp128 %x, %x
; CHECK-NEXT: ret fp128 %mul
}

; Make sure all intrinsic calls are eliminated when the input is known positive.

declare float @llvm.fabs.f32(float)
declare double @llvm.fabs.f64(double)
declare fp128 @llvm.fabs.f128(fp128)
declare <4 x float> @llvm.fabs.v4f32(<4 x float>)

define float @square_fabs_intrinsic_f32(float %x) {
  %mul = fmul float %x, %x
  %fabsf = tail call float @llvm.fabs.f32(float %mul)
  ret float %fabsf

; CHECK-LABEL: square_fabs_intrinsic_f32(
; CHECK-NEXT: %mul = fmul float %x, %x
; CHECK-NEXT: ret float %mul
}

define double @square_fabs_intrinsic_f64(double %x) {
  %mul = fmul double %x, %x
  %fabs = tail call double @llvm.fabs.f64(double %mul)
  ret double %fabs

; CHECK-LABEL: square_fabs_intrinsic_f64(
; CHECK-NEXT: %mul = fmul double %x, %x
; CHECK-NEXT: ret double %mul
}

define fp128 @square_fabs_intrinsic_f128(fp128 %x) {
  %mul = fmul fp128 %x, %x
  %fabsl = tail call fp128 @llvm.fabs.f128(fp128 %mul)
  ret fp128 %fabsl

; CHECK-LABEL: square_fabs_intrinsic_f128(
; CHECK-NEXT: %mul = fmul fp128 %x, %x
; CHECK-NEXT: ret fp128 %mul
}

; Shrinking a library call to a smaller type should not be inhibited by nor inhibit the square optimization.

define float @square_fabs_shrink_call1(float %x) {
  %ext = fpext float %x to double
  %sq = fmul double %ext, %ext
  %fabs = call double @fabs(double %sq)
  %trunc = fptrunc double %fabs to float
  ret float %trunc

; CHECK-LABEL: square_fabs_shrink_call1(
; CHECK-NEXT: %trunc = fmul float %x, %x
; CHECK-NEXT: ret float %trunc
}

define float @square_fabs_shrink_call2(float %x) {
  %sq = fmul float %x, %x
  %ext = fpext float %sq to double
  %fabs = call double @fabs(double %ext)
  %trunc = fptrunc double %fabs to float
  ret float %trunc

; CHECK-LABEL: square_fabs_shrink_call2(
; CHECK-NEXT: %sq = fmul float %x, %x
; CHECK-NEXT: ret float %sq
}

; A scalar fabs op makes the sign bit zero, so masking off all of the other bits means we can return zero.

define i32 @fabs_value_tracking_f32(float %x) {
  %call = call float @llvm.fabs.f32(float %x)
  %bc = bitcast float %call to i32
  %and = and i32 %bc, 2147483648
  ret i32 %and

; CHECK-LABEL: fabs_value_tracking_f32(
; CHECK:       ret i32 0
}

; TODO: A vector fabs op makes the sign bits zero, so masking off all of the other bits means we can return zero.

define <4 x i32> @fabs_value_tracking_v4f32(<4 x float> %x) {
  %call = call <4 x float> @llvm.fabs.v4f32(<4 x float> %x)
  %bc = bitcast <4 x float> %call to <4 x i32>
  %and = and <4 x i32> %bc, <i32 2147483648, i32 2147483648, i32 2147483648, i32 2147483648>
  ret <4 x i32> %and

; CHECK-LABEL: fabs_value_tracking_v4f32(
; CHECK:       ret <4 x i32> %and
}


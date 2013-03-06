; RUN: opt -cilk-sf-late-init -S < %s | FileCheck %s

%__cilkrts_stack_frame = type { i32, i32, %__cilkrts_stack_frame*, %__cilkrts_worker*, i8*, [5 x i8*], i32, i16, i16, %__cilkrts_pedigree }
%__cilkrts_worker = type { %__cilkrts_stack_frame**, %__cilkrts_stack_frame**, %__cilkrts_stack_frame**, %__cilkrts_stack_frame**, %__cilkrts_stack_frame**, i32, i8*, i8*, i8*, %__cilkrts_stack_frame*, %__cilkrts_stack_frame**, i8*, %__cilkrts_pedigree }
%__cilkrts_pedigree = type { i64, %__cilkrts_pedigree* }

declare void @__cilk_parent_prologue(%__cilkrts_stack_frame*)
declare void @__cilk_parent_epilogue(%__cilkrts_stack_frame*)
declare void @__cilk_sync(%__cilkrts_stack_frame*)
declare void @__cilk_excepting_sync(%__cilkrts_stack_frame*)
declare void @__cilk_spawn_helper(%struct.capture*)
declare void @_Z21__cilk_spawn_helpermangled(%struct.capture*)
declare i32 @__gxx_personality_v0(...)

%struct.capture = type { i32 }

;
; conditional spawn -> late init
define void @test_basic(i1 %cond) {
  %agg.captured = alloca %struct.capture
  %sf = alloca %__cilkrts_stack_frame
  call void @__cilk_parent_prologue(%__cilkrts_stack_frame* %sf)
  br i1 %cond, label %spawn, label %done

spawn:
  call void @__cilk_spawn_helper(%struct.capture* %agg.captured)
  br label %done

done:
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  call void @__cilk_parent_epilogue(%__cilkrts_stack_frame* %sf)
  ret void

  ; CHECK: alloca %__cilkrts_stack_frame
  ; CHECK-NEXT: getelementptr inbounds %__cilkrts_stack_frame* {{.*}} 0
  ; CHECK-NEXT: store i32 16777216,

  ; CHECK: spawn:
  ; CHECK-NEXT: call void @__cilk_conditional_parent_prologue
  ; CHECK-NEXT: call void @__cilk_spawn_helper
  ; CHECK: call void @__cilk_conditional_sync
  ; CHECK: call void @__cilk_conditional_parent_epilogue
}

;
; conditional spawn (mangled) -> late init
define void @test_mangled(i1 %cond) {
  %agg.captured = alloca %struct.capture
  %sf = alloca %__cilkrts_stack_frame
  call void @__cilk_parent_prologue(%__cilkrts_stack_frame* %sf)
  br i1 %cond, label %spawn, label %done

spawn:
  call void @_Z21__cilk_spawn_helpermangled(%struct.capture* %agg.captured)
  br label %done

done:
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  call void @__cilk_parent_epilogue(%__cilkrts_stack_frame* %sf)
  ret void

  ; CHECK: alloca %__cilkrts_stack_frame
  ; CHECK-NEXT: getelementptr inbounds %__cilkrts_stack_frame* {{.*}} 0
  ; CHECK-NEXT: store i32 16777216,

  ; CHECK: spawn:
  ; CHECK-NEXT: call void @__cilk_conditional_parent_prologue
  ; CHECK-NEXT: call void @_Z21__cilk_spawn_helpermangled
}

;
; unconditional spawn -> regular init
define void @test_unconditional1() {
  %agg.captured = alloca %struct.capture
  %sf = alloca %__cilkrts_stack_frame
  call void @__cilk_parent_prologue(%__cilkrts_stack_frame* %sf)
  call void @__cilk_spawn_helper(%struct.capture* %agg.captured)
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  call void @__cilk_parent_epilogue(%__cilkrts_stack_frame* %sf)
  ret void

  ; CHECK: alloca %__cilkrts_stack_frame
  ; CHECK-NEXT: call void @__cilk_parent_prologue
  ; CHECK-NEXT: call void @__cilk_spawn_helper
  ; CHECK-NEXT: call void @__cilk_sync
  ; CHECK-NEXT: call void @__cilk_parent_epilogue
}

define void @test_unconditional2(i1 %cond) {
  %agg.captured = alloca %struct.capture
  %sf = alloca %__cilkrts_stack_frame
  call void @__cilk_parent_prologue(%__cilkrts_stack_frame* %sf)
  br i1 %cond, label %spawn, label %after

spawn:
  call void @__cilk_spawn_helper(%struct.capture* %agg.captured)
  br label %after

after:
  call void @__cilk_spawn_helper(%struct.capture* %agg.captured)
  br label %done

done:
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  call void @__cilk_parent_epilogue(%__cilkrts_stack_frame* %sf)
  ret void

  ; CHECK: alloca %__cilkrts_stack_frame
  ; CHECK-NEXT: call void @__cilk_parent_prologue

  ; CHECK: spawn:
  ; CHECK-NOT: call void @__cilk_parent_prologue
  ; CHECK: call void @__cilk_spawn_helper

  ; CHECK: after:
  ; CHECK-NOT: call void @__cilk_parent_prologue
  ; CHECK: call void @__cilk_spawn_helper
}

;
; multiple branches -> multiple inits
define void @test_multiple_branches(i1 %cond1, i1 %cond2) {
  %agg.captured = alloca %struct.capture
  %sf = alloca %__cilkrts_stack_frame
  call void @__cilk_parent_prologue(%__cilkrts_stack_frame* %sf)
  br i1 %cond1, label %spawn1, label %after

spawn1:
  call void @__cilk_spawn_helper(%struct.capture* %agg.captured)
  br label %after

after:
  br i1 %cond2, label %spawn2, label %done

spawn2:
  call void @__cilk_spawn_helper(%struct.capture* %agg.captured)
  br label %done

done:
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  call void @__cilk_parent_epilogue(%__cilkrts_stack_frame* %sf)
  ret void

  ; CHECK: alloca %__cilkrts_stack_frame
  ; CHECK-NEXT: getelementptr inbounds %__cilkrts_stack_frame* {{.*}} 0
  ; CHECK-NEXT: store i32 16777216,

  ; CHECK: spawn1:
  ; CHECK-NEXT: call void @__cilk_conditional_parent_prologue
  ; CHECK-NEXT: call void @__cilk_spawn_helper

  ; CHECK: spawn2:
  ; CHECK-NEXT: call void @__cilk_conditional_parent_prologue
  ; CHECK-NEXT: call void @__cilk_spawn_helper

  ; CHECK: call void @__cilk_conditional_sync
  ; CHECK: call void @__cilk_conditional_parent_epilogue
}

;
; spawn dominates a spawn -> only init once
define void @test_dominated(i1 %cond1, i1 %cond2) {
  %agg.captured = alloca %struct.capture
  %sf = alloca %__cilkrts_stack_frame
  call void @__cilk_parent_prologue(%__cilkrts_stack_frame* %sf)
  br i1 %cond1, label %spawn1, label %done

spawn1:
  call void @__cilk_spawn_helper(%struct.capture* %agg.captured)
  br label %spawn2

spawn2:
  call void @__cilk_spawn_helper(%struct.capture* %agg.captured)
  call void @__cilk_spawn_helper(%struct.capture* %agg.captured)
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  br label %done

done:
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  call void @__cilk_parent_epilogue(%__cilkrts_stack_frame* %sf)
  ret void

  ; CHECK: alloca %__cilkrts_stack_frame
  ; CHECK-NEXT: getelementptr inbounds %__cilkrts_stack_frame* {{.*}} 0
  ; CHECK-NEXT: store i32 16777216,

  ; CHECK: spawn1:
  ; CHECK-NEXT: call void @__cilk_conditional_parent_prologue
  ; CHECK-NEXT: call void @__cilk_spawn_helper

  ; CHECK: spawn2:
  ; CHECK-NOT: call void @__cilk_conditional_parent_prologue
  ; CHECK-NEXT: call void @__cilk_spawn_helper
  ; CHECK-NOT: call void @__cilk_conditional_parent_prologue
  ; CHECK-NEXT: call void @__cilk_spawn_helper
  ; CHECK-NEXT: call void @__cilk_sync

  ; CHECK: call void @__cilk_conditional_sync
  ; CHECK: call void @__cilk_conditional_parent_epilogue
}

define void @test_multiple_inits(i1 %cond) {
  %agg.captured = alloca %struct.capture
  %sf = alloca %__cilkrts_stack_frame
  call void @__cilk_parent_prologue(%__cilkrts_stack_frame* %sf)
  br i1 %cond, label %spawn, label %done

spawn:
  call void @__cilk_spawn_helper(%struct.capture* %agg.captured)
  br label %done

done:
  call void @__cilk_parent_prologue(%__cilkrts_stack_frame* %sf)
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  call void @__cilk_parent_epilogue(%__cilkrts_stack_frame* %sf)
  ret void

  ; CHECK: alloca %__cilkrts_stack_frame
  ; CHECK-NEXT: call void @__cilk_parent_prologue

  ; CHECK: spawn:
  ; CHECK-NEXT: call void @__cilk_spawn_helper

  ; CHECK: done:
  ; CHECK-NEXT: call void @__cilk_parent_prologue
}

define void @test_multiple_epilogues(i1 %cond) {
  %agg.captured = alloca %struct.capture
  %sf = alloca %__cilkrts_stack_frame
  call void @__cilk_parent_prologue(%__cilkrts_stack_frame* %sf)
  br i1 %cond, label %spawn, label %done

spawn:
  invoke void @__cilk_spawn_helper(%struct.capture* %agg.captured) to label %done unwind label %err

done:
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  call void @__cilk_parent_epilogue(%__cilkrts_stack_frame* %sf)
  ret void

err:
  %e = landingpad { i8*, i32 } personality i8* bitcast (i32 (...)* @__gxx_personality_v0 to i8*)
    cleanup
  call void @__cilk_excepting_sync(%__cilkrts_stack_frame* %sf)
  call void @__cilk_parent_epilogue(%__cilkrts_stack_frame* %sf)
  resume { i8*, i32 } %e

  ; CHECK: done:
  ; CHECK-NEXT: call void @__cilk_conditional_sync
  ; CHECK-NEXT: call void @__cilk_conditional_parent_epilogue

  ; CHECK: err:
  ; CHECK: call void @__cilk_excepting_sync
  ; CHECK-NEXT: call void @__cilk_conditional_parent_epilogue
}

!cilk.spawn = !{!0, !1}
!cilk.sync = !{!2, !3}

!0 = metadata !{void (%struct.capture*)* @__cilk_spawn_helper}
!1 = metadata !{void (%struct.capture*)* @_Z21__cilk_spawn_helpermangled}
!2 = metadata !{void (%__cilkrts_stack_frame*)* @__cilk_sync}
!3 = metadata !{void (%__cilkrts_stack_frame*)* @__cilk_excepting_sync}

; CHECK: cilk.sync = !{!2, !3, !
; CHECK: metadata !{void (%__cilkrts_stack_frame*)* @__cilk_conditional_sync}

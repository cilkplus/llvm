; RUN: opt -elide-cilk-sync -S < %s | FileCheck %s

@condptr = external global i1
@cond2ptr = external global i1
@switch_condptr = external global i8

%__cilkrts_stack_frame = type { i32, i32, %__cilkrts_stack_frame*, %__cilkrts_worker*, i8*, [5 x i8*], i32, i16, i16, %__cilkrts_pedigree }
%__cilkrts_worker = type { %__cilkrts_stack_frame**, %__cilkrts_stack_frame**, %__cilkrts_stack_frame**, %__cilkrts_stack_frame**, %__cilkrts_stack_frame**, i32, i8*, i8*, i8*, %__cilkrts_stack_frame*, %__cilkrts_stack_frame**, i8*, %__cilkrts_pedigree }
%__cilkrts_pedigree = type { i64, %__cilkrts_pedigree* }

%struct.capture = type { i32 }

declare void @__cilk_parent_prologue(%__cilkrts_stack_frame*)
declare void @__cilk_sync(%__cilkrts_stack_frame*)
declare void @__cilk_spawn_helper(%struct.capture*)
declare void @_Z21__cilk_spawn_helpermangled(%struct.capture*)
;
; The following tests are for basic cases where one sync dominates another.

define void @test_elide_sync() {
  %sf = alloca %__cilkrts_stack_frame
  %agg.captured = alloca %struct.capture
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  call void @__cilk_spawn_helper(%struct.capture* %agg.captured)
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  ret void
  ; CHECK: define void @test_elide_sync
  ; CHECK-NOT: call void @__cilk_sync
  ; CHECK: call void @__cilk_spawn_helper
  ; CHECK: call void @__cilk_sync
  ; CHECK: ret void
}

define void @test_elide_second_sync() {
  %sf = alloca %__cilkrts_stack_frame
  %agg.captured = alloca %struct.capture
  call void @__cilk_spawn_helper(%struct.capture* %agg.captured)
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  ret void
  ; CHECK: define void @test_elide_second_sync
  ; CHECK: call void @__cilk_sync
  ; CHECK-NOT: call void @__cilk_sync
  ; CHECK: ret void
}

define void @test_second_sync_in_if() {
  %sf = alloca %__cilkrts_stack_frame
  %agg.captured = alloca %struct.capture
  call void @__cilk_spawn_helper(%struct.capture* %agg.captured)
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  %cond = load i1* @condptr
  br i1 %cond, label %if, label %done

if:
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  br label %done

done:
  ret void
  ; CHECK: define void @test_second_sync_in_if
  ; CHECK: call void @__cilk_sync
  ; CHECK-NOT: call void @__cilk_sync
  ; CHECK: ret void
}

define void @test_first_sync_in_if() {
  %sf = alloca %__cilkrts_stack_frame
  %agg.captured = alloca %struct.capture
  call void @__cilk_spawn_helper(%struct.capture* %agg.captured)
  %cond = load i1* @condptr
  br i1 %cond, label %if, label %done

if:
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  br label %done

done:
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  ret void
  ; CHECK: define void @test_first_sync_in_if
  ; CHECK: call void @__cilk_sync
  ; CHECK: call void @__cilk_sync
  ; CHECK: ret void
}

define void @test_both_syncs_in_if() {
  %sf = alloca %__cilkrts_stack_frame
  %agg.captured = alloca %struct.capture
  call void @__cilk_spawn_helper(%struct.capture* %agg.captured)
  %cond = load i1* @condptr
  br i1 %cond, label %if, label %done

if:
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  br label %done

done:
  ret void
  ; CHECK: define void @test_both_syncs_in_if
  ; CHECK: call void @__cilk_sync
  ; CHECK-NOT: call void @__cilk_sync
  ; CHECK: ret void
}

define void @test_sync_in_loop() {
  %sf = alloca %__cilkrts_stack_frame
  %agg.captured = alloca %struct.capture
  call void @__cilk_spawn_helper(%struct.capture* %agg.captured)
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  %cond = load i1* @condptr
  br i1 %cond, label %loop, label %done

loop:
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  %cond2 = load i1* @cond2ptr
  br i1 %cond2, label %loop, label %done

done:
  ret void
  ; CHECK: define void @test_sync_in_loop
  ; CHECK: call void @__cilk_sync
  ; CHECK-NOT: call void @__cilk_sync
  ; CHECK: ret void
}

define void @test_nested_control_flow() {
  %sf = alloca %__cilkrts_stack_frame
  %agg.captured = alloca %struct.capture
  call void @__cilk_spawn_helper(%struct.capture* %agg.captured)
  %cond = load i1* @condptr
  br i1 %cond, label %if, label %done

if:
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  %cond2 = load i1* @cond2ptr
  br i1 %cond2, label %loop, label %done

loop:
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  br label %if

done:
  ret void
  ; CHECK: define void @test_nested_control_flow
  ; CHECK: call void @__cilk_sync
  ; CHECK-NOT: call void @__cilk_sync
  ; CHECK: ret void
}


;
; The following tests are for cases where a spawn exists between two syncs.

define void @test_spawn_between() {
  %sf = alloca %__cilkrts_stack_frame
  %agg.captured = alloca %struct.capture
  call void @__cilk_spawn_helper(%struct.capture* %agg.captured)
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  call void @__cilk_spawn_helper(%struct.capture* %agg.captured)
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  ret void
  ; CHECK: define void @test_spawn_between
  ; CHECK: call void @__cilk_sync
  ; CHECK: call void @__cilk_spawn_helper
  ; CHECK: call void @__cilk_sync
  ; CHECK: ret void
}

define void @test_spawn_maybe_between() {
  %sf = alloca %__cilkrts_stack_frame
  %agg.captured = alloca %struct.capture
  call void @__cilk_spawn_helper(%struct.capture* %agg.captured)
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  %cond = load i1* @condptr
  br i1 %cond, label %if, label %done

if:
  call void @__cilk_spawn_helper(%struct.capture* %agg.captured)
  br label %done

done:
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  ret void
  ; CHECK: define void @test_spawn_maybe_between
  ; CHECK: call void @__cilk_sync
  ; CHECK: call void @__cilk_spawn_helper
  ; CHECK: call void @__cilk_sync
  ; CHECK: ret void
}

define void @test_sync_spawn_loop() {
  %sf = alloca %__cilkrts_stack_frame
  %agg.captured = alloca %struct.capture
  call void @__cilk_spawn_helper(%struct.capture* %agg.captured)
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  %cond = load i1* @condptr
  br i1 %cond, label %loop, label %done

loop:
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  call void @__cilk_spawn_helper(%struct.capture* %agg.captured)
  %cond2 = load i1* @cond2ptr
  br i1 %cond2, label %loop, label %done

done:
  ret void
  ; CHECK: define void @test_sync_spawn_loop
  ; CHECK: call void @__cilk_sync
  ; CHECK: loop:
  ; CHECK: call void @__cilk_sync
  ; CHECK: ret void
}

;
; The following tests are for cases where all paths to a sync go through syncs,
; but there is no single dominator.

define void @test_sync_in_both_branches() {
  %sf = alloca %__cilkrts_stack_frame
  %agg.captured = alloca %struct.capture
  call void @__cilk_spawn_helper(%struct.capture* %agg.captured)
  %cond = load i1* @condptr
  br i1 %cond, label %if, label %else

if:
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  br label %done

else:
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  br label %done

done:
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  ret void
  ; CHECK: define void @test_sync_in_both_branches
  ; CHECK: call void @__cilk_sync
  ; CHECK: call void @__cilk_sync
  ; CHECK: done:
  ; CHECK-NOT: call void @__cilk_sync
  ; CHECK: ret void
}

define void @test_sync_in_all_branches() {
  %sf = alloca %__cilkrts_stack_frame
  %agg.captured = alloca %struct.capture
  call void @__cilk_spawn_helper(%struct.capture* %agg.captured)
  %switch_cond = load i8* @switch_condptr
  switch i8 %switch_cond, label %default [ i8 0, label %a
                                           i8 1, label %b
                                           i8 2, label %c ]

a:
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  br label %done

b:
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  br label %done

c:
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  br label %done

default:
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  br label %done

done:
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  ret void
  ; CHECK: define void @test_sync_in_all_branches
  ; CHECK: call void @__cilk_sync
  ; CHECK: call void @__cilk_sync
  ; CHECK: call void @__cilk_sync
  ; CHECK: call void @__cilk_sync
  ; CHECK: done:
  ; CHECK-NOT: call void @__cilk_sync
  ; CHECK: ret void
}

define void @test_sync_in_some_branches() {
  %sf = alloca %__cilkrts_stack_frame
  %agg.captured = alloca %struct.capture
  call void @__cilk_spawn_helper(%struct.capture* %agg.captured)
  %switch_cond = load i8* @switch_condptr
  switch i8 %switch_cond, label %default [ i8 0, label %a
                                           i8 1, label %b
                                           i8 2, label %c ]

a:
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  br label %done

b:
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  br label %done

c:
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  br label %done

default:
  br label %done

done:
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  ret void
  ; CHECK: define void @test_sync_in_some_branches
  ; CHECK: call void @__cilk_sync
  ; CHECK: call void @__cilk_sync
  ; CHECK: call void @__cilk_sync
  ; CHECK: done:
  ; CHECK: call void @__cilk_sync
  ; CHECK: ret void
}

declare i32 @__gxx_personality_v0(...)

define void @test_invoke() {
  br label %skip

skip:
  %sf = alloca %__cilkrts_stack_frame
  %agg.captured = alloca %struct.capture
  invoke void @_Z21__cilk_spawn_helpermangled(%struct.capture* %agg.captured) to label %normal unwind label %except

normal:
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  br label %done

except:
  %1 = landingpad { i8*, i32 } personality i32 (...)* @__gxx_personality_v0 catch i8* null
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  br label %done

done:
  call void @__cilk_sync(%__cilkrts_stack_frame* %sf)
  ret void

  ; CHECK: define void @test_invoke
  ; CHECK: call void @__cilk_sync
  ; CHECK: call void @__cilk_sync
  ; CHECK: done:
  ; CHECK-NOT: call void @__cilk_sync
  ; CHECK: ret void
}

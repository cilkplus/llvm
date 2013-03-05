; RUN: opt -elide-cilk-sync -S < %s | FileCheck %s

@condptr = external global i1
@cond2ptr = external global i1
@switch_condptr = external global i8

%__cilkrts_stack_frame = type { i32, i32, %__cilkrts_stack_frame*, %__cilkrts_worker*, i8*, [5 x i8*], i32, i16, i16, %__cilkrts_pedigree }
%__cilkrts_worker = type { %__cilkrts_stack_frame**, %__cilkrts_stack_frame**, %__cilkrts_stack_frame**, %__cilkrts_stack_frame**, %__cilkrts_stack_frame**, i32, i8*, i8*, i8*, %__cilkrts_stack_frame*, %__cilkrts_stack_frame**, i8*, %__cilkrts_pedigree }
%__cilkrts_pedigree = type { i64, %__cilkrts_pedigree* }

%struct.capture = type { i32 }
%struct.capture.0 = type { i32*, i32* }

declare void @__cilk_parent_prologue(%__cilkrts_stack_frame*)
declare void @__cilk_parent_epilogue(%__cilkrts_stack_frame*)
declare void @__cilk_sync(%__cilkrts_stack_frame*)
declare void @__cilk_excepting_sync(%__cilkrts_stack_frame*, i8*)
declare void @__cilk_spawn_helper(%struct.capture*)
declare void @_Z21__cilk_spawn_helpermangled(%struct.capture*)
declare void @_Z21__cilk_spawn_helperV0PZ3fibiE7capture(%struct.capture.0*)
declare i8* @__cxa_begin_catch(i8*)
declare void @__cxa_end_catch()
declare i32 @llvm.eh.sjlj.setjmp(i8*)
declare i8* @llvm.frameaddress(i32)
declare i8* @llvm.stacksave()

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

define i32 @_Z3fibi(i32 %n) {
entry:
  %__cilkrts_sf = alloca %__cilkrts_stack_frame
  call void @__cilk_parent_prologue(%__cilkrts_stack_frame* %__cilkrts_sf)
  %n.addr = alloca i32, align 4
  %x = alloca i32, align 4
  %y = alloca i32, align 4
  %agg.captured = alloca %struct.capture.0, align 8
  %exn.slot = alloca i8*
  %ehselector.slot = alloca i32
  %cleanup.dest.slot = alloca i32
  store i32 %n, i32* %n.addr, align 4
  call void @__cilk_sync(%__cilkrts_stack_frame* %__cilkrts_sf)
  br label %cilk.spawn.savestate

cilk.spawn.savestate:                             ; preds = %entry
  %0 = getelementptr inbounds %__cilkrts_stack_frame* %__cilkrts_sf, i32 0, i32 6
  %1 = getelementptr inbounds %__cilkrts_stack_frame* %__cilkrts_sf, i32 0, i32 7
  call void asm sideeffect "stmxcsr $0\0A\09fnstcw $1", "*m,*m,~{dirflag},~{fpsr},~{flags}"(i32* %0, i16* %1)
  %2 = getelementptr inbounds %__cilkrts_stack_frame* %__cilkrts_sf, i32 0, i32 5
  %3 = call i8* @llvm.frameaddress(i32 0)
  %4 = getelementptr inbounds [5 x i8*]* %2, i32 0, i32 0
  store i8* %3, i8** %4
  %5 = call i8* @llvm.stacksave()
  %6 = getelementptr inbounds [5 x i8*]* %2, i32 0, i32 2
  store i8* %5, i8** %6
  %7 = bitcast [5 x i8*]* %2 to i8*
  %8 = call i32 @llvm.eh.sjlj.setjmp(i8* %7)
  %9 = icmp eq i32 %8, 0
  br i1 %9, label %cilk.spawn.helpercall, label %cilk.spawn.continuation

cilk.spawn.helpercall:                            ; preds = %cilk.spawn.savestate
  %10 = getelementptr inbounds %struct.capture.0* %agg.captured, i32 0, i32 0
  store i32* %x, i32** %10, align 8
  %11 = getelementptr inbounds %struct.capture.0* %agg.captured, i32 0, i32 1
  store i32* %n.addr, i32** %11, align 8
  invoke void @_Z21__cilk_spawn_helperV0PZ3fibiE7capture(%struct.capture.0* %agg.captured)
          to label %invoke.cont unwind label %lpad

invoke.cont:                                      ; preds = %cilk.spawn.helpercall
  br label %cilk.spawn.continuation

cilk.spawn.continuation:                          ; preds = %invoke.cont, %cilk.spawn.savestate
  %12 = load i32* %n.addr, align 4
  %sub = sub nsw i32 %12, 2
  %call = invoke i32 @_Z3fibi(i32 %sub)
          to label %invoke.cont1 unwind label %lpad

invoke.cont1:                                     ; preds = %cilk.spawn.continuation
  store i32 %call, i32* %y, align 4
  call void @__cilk_sync(%__cilkrts_stack_frame* %__cilkrts_sf)
  br label %try.cont

lpad:                                             ; preds = %cilk.spawn.continuation, %cilk.spawn.helpercall
  %13 = landingpad { i8*, i32 } personality i8* bitcast (i32 (...)* @__gxx_personality_v0 to i8*)
          catch i8* null
  %14 = extractvalue { i8*, i32 } %13, 0
  store i8* %14, i8** %exn.slot
  %15 = extractvalue { i8*, i32 } %13, 1
  store i32 %15, i32* %ehselector.slot
  call void @__cilk_sync(%__cilkrts_stack_frame* %__cilkrts_sf)
  br label %catch

catch:                                            ; preds = %lpad
  %exn = load i8** %exn.slot
  call void @__cilk_excepting_sync(%__cilkrts_stack_frame* %__cilkrts_sf, i8* %exn)
  %exn2 = load i8** %exn.slot
  %16 = call i8* @__cxa_begin_catch(i8* %exn2) nounwind
  invoke void @__cxa_end_catch()
          to label %invoke.cont4 unwind label %lpad3

invoke.cont4:                                     ; preds = %catch
  br label %try.cont

try.cont:                                         ; preds = %invoke.cont4, %invoke.cont1
  call void @__cilk_sync(%__cilkrts_stack_frame* %__cilkrts_sf)
  %17 = load i32* %x, align 4
  %18 = load i32* %y, align 4
  %add = add nsw i32 %17, %18
  store i32 1, i32* %cleanup.dest.slot
  call void @__cilk_parent_epilogue(%__cilkrts_stack_frame* %__cilkrts_sf)
  ret i32 %add

lpad3:                                            ; preds = %catch
  %19 = landingpad { i8*, i32 } personality i8* bitcast (i32 (...)* @__gxx_personality_v0 to i8*)
          cleanup
  %20 = extractvalue { i8*, i32 } %19, 0
  store i8* %20, i8** %exn.slot
  %21 = extractvalue { i8*, i32 } %19, 1
  store i32 %21, i32* %ehselector.slot
  call void @__cilk_parent_epilogue(%__cilkrts_stack_frame* %__cilkrts_sf)
  br label %eh.resume

eh.resume:                                        ; preds = %lpad3
  %exn5 = load i8** %exn.slot
  %sel = load i32* %ehselector.slot
  %lpad.val = insertvalue { i8*, i32 } undef, i8* %exn5, 0
  %lpad.val6 = insertvalue { i8*, i32 } %lpad.val, i32 %sel, 1
  resume { i8*, i32 } %lpad.val6

  ; CHECK: define i32 @_Z3fibi(i32 %n)
  ;
  ; CHECK-NOT: call void @__cilk_sync
  ; CHECK: br label %cilk.spawn.savestate
  ;
  ; CHECK: invoke.cont1:
  ; CHECK: call void @__cilk_sync
  ; CHECK: br label %try.cont
  ;
  ; CHECK: lpad:
  ; CHECK: call void @__cilk_sync
  ; CHECK-NEXT: br label %catch
  ;
  ; CHECK: try.cont:
  ; CHECK-NOT: call void @__cilk_sync
  ; CHECK: ret i32
  ;
  ; CHECK: resume { i8*, i32 }
}

!cilk.spawn = !{!0, !1, !2}
!cilk.sync = !{!3}

!0 = metadata !{void (%struct.capture*)* @__cilk_spawn_helper}
!1 = metadata !{void (%struct.capture*)* @_Z21__cilk_spawn_helpermangled}
!2 = metadata !{void (%struct.capture.0*)* @_Z21__cilk_spawn_helperV0PZ3fibiE7capture}
!3 = metadata !{void (%__cilkrts_stack_frame*)* @__cilk_sync}

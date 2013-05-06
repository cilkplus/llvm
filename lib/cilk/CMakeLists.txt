###############################################################################
# Build for Cilk Plus runtime library
###############################################################################

# Install cilk header files
set(output_dir ${LLVM_BINARY_DIR}/include)

set(cilk_header_files
    cilk/cilk.h
    cilk/cilk_api.h
    cilk/cilk_api_linux.h
    cilk/cilk_stub.h
    cilk/cilk_undocumented.h
    cilk/common.h
    cilk/holder.h
    cilk/hyperobject_base.h
    cilk/metaprogramming.h
    cilk/reducer.h
    cilk/reducer_file.h
    cilk/reducer_list.h
    cilk/reducer_max.h
    cilk/reducer_min.h
    cilk/reducer_min_max.h
    cilk/reducer_opadd.h
    cilk/reducer_opand.h
    cilk/reducer_opmul.h
    cilk/reducer_opor.h
    cilk/reducer_opxor.h
    cilk/reducer_ostream.h
    cilk/reducer_string.h
   )

set(out_files)
foreach( f ${cilk_header_files} )
  set( src ${CMAKE_CURRENT_SOURCE_DIR}/include/${f} )
  set( dst ${output_dir}/${f} )
  add_custom_command(OUTPUT ${dst}
    DEPENDS ${src}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${src} ${dst}
    COMMENT "Copying cilk's ${f}...")
  list(APPEND out_files ${dst})
endforeach( f )
add_custom_target(cilk-headers ALL DEPENDS ${out_files})
set_target_properties(cilk-headers PROPERTIES FOLDER "Misc")

install(DIRECTORY ""
  FILE_PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
  DESTINATION lib${LLVM_LIBDIR_SUFFIX})

# Sources files for the libraries
set(cilk_runtime_sources
    runtime/cilk-abi.c
    runtime/cilk-abi-vla.c
    runtime/cilk-abi-vla-internal.c
    runtime/cilk_api.c
    runtime/cilk_malloc.c
    runtime/c_reducers.c
    runtime/frame_malloc.c
    runtime/full_frame.c
    runtime/jmpbuf.c
    runtime/local_state.c
    runtime/metacall_impl.c
    runtime/os_mutex-unix.c
    runtime/os-unix.c
    runtime/scheduler.c
    runtime/signal_node.c
    runtime/spin_mutex.c
    runtime/stats.c
    runtime/symbol_test.c
    runtime/sysdep-unix.c
    runtime/worker_mutex.c
    runtime/bug.cpp
    runtime/cilk-abi-cilk-for.cpp
    runtime/cilk_fiber.cpp
    runtime/cilk_fiber-unix.cpp
    runtime/except-gcc.cpp
    runtime/global_state.cpp
    runtime/reducer_impl.cpp
    runtime/record-replay.cpp
    runtime/pedigrees.c
   )

 set(COMMON_FLAGS
     -fPIC
     -fcilkplus
     -Wall
     -Wno-typedef-redefinition
     -Wno-c11-extensions
     -I${CMAKE_CURRENT_SOURCE_DIR}/include
     -I${CMAKE_CURRENT_SOURCE_DIR}/runtime

     # Defintions
     -DIN_CILK_RUNTIME=1
     # The runtime ABI version is 1 by default
     -D__CILKRTS_ABI_VERSION=1
     -DBUILD_USER=
     -DBUILD_HOST=
     )

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  list(APPEND COMMON_FLAGS -D_DARWIN_C_SOURCE)
endif()

string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)

# Set CILKRTS_DEBUG=1 to debug the runtime itself.
set(CILKRTS_DEBUG 0 CACHE BOOL "Enable debugging the cilk runtime")

# Ignore the CMAKE_BUILD_TYPE setting, because you usually don't want to debug
# the runtime. In debug mode, the runtime will print a lot of information,
# leading to huge log files when running tests. See CILKRTS_DEBUG for overriding
# this behaviour.
if(${BUILD_TYPE} MATCHES "DEBUG")
  if(NOT CILKRTS_DEBUG)
    message(STATUS "Warning: ignoring CMAKE_BUILD_TYPE=Debug; set CILKRTS_DEBUG=1 to enable debugging the runtime")
  endif()
endif()

if(CILKRTS_DEBUG)
  list(APPEND COMMON_FLAGS -D_DEBUG)
else()
  list(APPEND COMMON_FLAGS -DNDEBUG)
endif()

string(REPLACE " " ";" CFLAGS ${CMAKE_C_FLAGS_${BUILD_TYPE}})
list(APPEND CFLAGS "-std=c99")
string(REPLACE " " ";" CXXFLAGS ${CMAKE_CXX_FLAGS_${BUILD_TYPE}})

# Build objects using the newly built clang.
set(out_objs)
foreach( f ${cilk_runtime_sources} )
  if (f MATCHES ".cpp")
    string(REPLACE ".cpp" ".o" dst ${f})
    set(FLAGS ${CXXFLAGS} ${COMMON_FLAGS})
  else()
    string(REPLACE ".c" ".o" dst ${f})
    set(FLAGS ${CFLAGS} ${COMMON_FLAGS})
  endif()

  clang_compile("${CMAKE_CURRENT_BINARY_DIR}/${dst}" ${f}
    CFLAGS ${FLAGS})
  list(APPEND out_objs ${dst})
endforeach( f )

# Generate libraries
macro(add_cilk_runtime name)
  parse_arguments(LIB "ARCH;OBJS;SOURCES;CFLAGS;DEFS;LINKFLAGS" "" ${ARGN})
  add_library(${name} SHARED ${LIB_OBJS})
  set_target_compile_flags(${name} ${LIB_CFLAGS})
  set_target_link_flags(${name} ${LIB_LINKFLAGS})
  set_property(TARGET ${name} APPEND PROPERTY
    COMPILE_DEFINITIONS ${LIB_DEFS})
  set_target_properties(${name} PROPERTIES
    OSX_ARCHITECTURES "${LIB_ARCH}"
    LIBRARY_OUTPUT_DIRECTORY ${LLVM_BINARY_DIR}/lib)
  install(TARGETS ${name}
    LIBRARY DESTINATION lib${LLVM_LIBDIR_SUFFIX})
endmacro()

add_cilk_runtime(cilkrts
  ARCH "x86_64"
  OBJS ${out_objs})

add_dependencies(cilkrts clang)
target_link_libraries(cilkrts dl pthread)
set_target_properties(cilkrts PROPERTIES SOVERSION "5")

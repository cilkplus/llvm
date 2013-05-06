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

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/runtime)

# Generate definitions
set(CILK_COMMON_DEFINITIONS
    # The runtime ABI version is 1 by default
    __CILKRTS_ABI_VERSION=1
    IN_CILK_RUNTIME=1
    # No user variable in CMAKE
    BUILD_USER="anonymous"
    BUILD_HOST="${CMAKE_HOST_SYSTEM_NAME}"
   )

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set(CILK_COMMON_DEFINITIONS ${CILK_COMMON_DEFINITIONS} "_DARWIN_C_SOURCE")
endif()

# If we are building clang with ccache, we need to remove ARG1 to build
# compiler-rt with clang.
if(${CMAKE_C_COMPILER} MATCHES "ccache")
  set(CMAKE_C_COMPILER_ARG1 "")
  set(CMAKE_CXX_COMPILER_ARG1 "")
endif()

# Set clang/clang++ as the compiler
set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/bin/clang")
set(CMAKE_CXX_COMPILER "${CMAKE_BINARY_DIR}/bin/clang++")

# Select C/C++ flags
set(BASE_C_FLAGS "-fcilkplus -std=c99 -Wall -Wno-c11-extensions -Wno-typedef-redefinition")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${BASE_C_FLAGS}")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${BASE_C_FLAGS}")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} ${BASE_C_FLAGS}")
set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} ${BASE_C_FLAGS}")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${BASE_C_FLAGS}")

set(BASE_CXX_FLAGS "-fcilkplus -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${BASE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${BASE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${BASE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} ${BASE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${BASE_CXX_FLAGS}")

# Generate libraries
macro(add_cilk_runtime name)
  parse_arguments(LIB "ARCH;SOURCES;CFLAGS;DEFS;LINKFLAGS" "" ${ARGN})
  add_library(${name} SHARED ${LIB_SOURCES})
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
  SOURCES ${cilk_runtime_sources})

add_dependencies(cilkrts clang)
target_link_libraries(cilkrts dl pthread)
set_target_properties(cilkrts PROPERTIES
               COMPILE_DEFINITIONS "${CILK_COMMON_DEFINITIONS}"
               SOVERSION "5")

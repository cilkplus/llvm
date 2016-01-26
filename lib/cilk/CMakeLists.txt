#  Copyright (C) 2014-2015, Intel Corporation
#  All rights reserved.
#  
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#  
#    * Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in
#      the documentation and/or other materials provided with the
#      distribution.
#    * Neither the name of Intel Corporation nor the names of its
#      contributors may be used to endorse or promote products derived
#      from this software without specific prior written permission.
#  
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
#  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
#  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
#  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
#  AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
#  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
#  WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#  POSSIBILITY OF SUCH DAMAGE.
#  
#  *********************************************************************
#  
#  PLEASE NOTE: This file is a downstream copy of a file mainitained in
#  a repository at cilkplus.org. Changes made to this file that are not
#  submitted through the contribution process detailed at
#  http://www.cilkplus.org/submit-cilk-contribution will be lost the next
#  time that a new version is released. Changes only submitted to the
#  GNU compiler collection or posted to the git repository at
#  https://bitbucket.org/intelcilkplusruntime/itnel-cilk-runtime.git are
#  not tracked.
#  
#  We welcome your contributions to this open source project. Thank you
#  for your assistance in helping us improve Cilk Plus.

###############################################################################
# README
###############################################################################
# This CMake configuration provides users with a basic build tool for Mac or
# Windows, using the following build environments:
# Mac    : Cilk Plus/LLVM compiler, available at http://cilkplus.github.io
# Windows: Intel(R) C++ Compiler (14.0 or later), configured with
#          Visual Studio (2012 or later), on Windows 7/8 with SDK installed
#
# How to build:
#   1. Set compiler variables properly (path, include/library path, etc.)
#   2. Create a build directory 
#   3. Invoke cmake in the build directory
#   4. Invoke the make tool indicated in step 3 ("make" or "nmake")
#
# Mac example:
#   > mkdir ./build && cd ./build
#   Release build:
#   > cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
#           -DCMAKE_INSTALL_PREFIX=./install ..
#   Debug build:
#   > cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
#           -DCMAKE_INSTALL_PREFIX=./install -DCILK_BUILD_TYPE=Debug ..
#   > make && make install
#
# Windows example:
#   > mkdir .\build && cd .\build
#   64-bit build:
#   > cmake -G "NMake Makefiles" -DCMAKE_C_COMPILER=icl \
#           -DCMAKE_CXX_COMPILER=icl -DCMAKE_INSTALL_PREFIX=.\install ..
#   32-bit build:
#   > cmake -G "NMake Makefiles" -DCILK_ARCH=32 -DCMAKE_C_COMPILER=icl \
#           -DCMAKE_CXX_COMPILER=icl -DCMAKE_INSTALL_PREFIX=.\install ..
#   > nmake && nmake install
#
# Known problems/limitation:
#   1. No 32-bit build for Mac

cmake_minimum_required(VERSION 3.0.0)

project(CilkRuntime C CXX)

# Default options
if(NOT DEFINED CILK_BUILD_TYPE)
    set(CILK_BUILD_TYPE Release)
endif()
set(CMAKE_BUILD_TYPE ${CILK_BUILD_TYPE})

if(NOT DEFINED CILK_ARCH)
    set(CILK_ARCH "64")
endif()

# Enables MASM on Windows
if(${WIN32})
    enable_language(ASM_MASM)
endif()


###############################################################################
# Source file definitions
###############################################################################

# List of common source files
set(cilkrts_srcs
    runtime/bug.cpp
    runtime/cilk-abi.c
    runtime/cilk-abi-cilk-for.cpp
    runtime/cilk-abi-vla-internal.c
    runtime/cilk_api.c
    runtime/cilk_fiber.cpp
    runtime/cilk_malloc.c
    runtime/c_reducers.c
    runtime/frame_malloc.c
    runtime/full_frame.c
    runtime/global_state.cpp
    runtime/jmpbuf.c
    runtime/local_state.c
    runtime/metacall_impl.c
    runtime/pedigrees.c
    runtime/record-replay.cpp
    runtime/reducer_impl.cpp
    runtime/scheduler.c
    runtime/signal_node.c
    runtime/spin_mutex.c
    runtime/stats.c
    runtime/worker_mutex.c
)

# Architecture-dependent source files (fixed as x86 in this file)
set(cilkrts_sysdep runtime/config/x86)
list(APPEND cilkrts_srcs ${cilkrts_sysdep}/cilk-abi-vla.c)

# OS-dependent source files
if(${WIN32})
    list(APPEND cilkrts_srcs
         runtime/cilk_api-win-seh.c
         runtime/cilk_fiber-win.cpp
         runtime/except-win.c
         runtime/os_mutex-win.c
         runtime/os-win.c
         runtime/sysdep-win.c
         runtime/sysdep-win-dllmain.c
    )
    if("${CILK_ARCH}" STREQUAL "32")
        list(APPEND cilkrts_srcs
             runtime/except-win32.c
             runtime/safe_seh.asm
             runtime/sysdep-win32.c
        )
    else()
        # "64" by default
        list(APPEND cilkrts_srcs
             runtime/except-win64.c
             runtime/win64-asm.asm
             runtime/sysdep-win64.c
        )
    endif()
else()
    list(APPEND cilkrts_srcs
         runtime/cilk_fiber-unix.cpp
         runtime/except-gcc.cpp
         runtime/os_mutex-unix.c 
         runtime/os-unix.c
         runtime/sysdep-unix.c
         ${cilkrts_sysdep}/os-unix-sysdep.c
         runtime/sslib/ignore_handler_s.c
         runtime/sslib/safe_str_constraint.c
         runtime/sslib/snprintf_support.c
         runtime/sslib/strcpy_s.c
         runtime/sslib/strncpy_s.c
         runtime/sslib/strnlen_s.c
    )
endif()
 

###############################################################################
# Flags and include path.
###############################################################################

# Include paths
include_directories(
    ${CilkRuntime_SOURCE_DIR}/include
    ${CilkRuntime_SOURCE_DIR}/runtime
    ${CilkRuntime_SOURCE_DIR}/runtime/sslib
    ${CilkRuntime_SOURCE_DIR}/${cilkrts_sysdep}
)

# Preprocessor defs and common flags
if(${WIN32})
    set(cilk_flags
        /Q_multisrc-
        /DUNICODE
        /DIN_CILK_RUNTIME=1
        /EHsc
        /GS
    )
    set(cilk_c_flags
        /Qstd=c99
    )
    set(cilk_cxx_flags
        /GR-
    )
    set(cilk_link_flags
        /DYNAMICBASE
        /NXCOMPAT
        /DEBUG
        /PDB:cilkrts20.pdb
        /DEF:${CilkRuntime_SOURCE_DIR}/runtime/cilk-exports.def
        /ENTRY:cilkrts_module_startup
    )
    set_property(SOURCE ${cilkrts_sysdep}/cilk-abi-vla.c
                 APPEND PROPERTY COMPILE_FLAGS
                 "/w /GS-"
    )
    if("${CILK_ARCH}" STREQUAL "32") 
        set_property(SOURCE runtime/safe_seh.asm
                     APPEND PROPERTY COMPILE_FLAGS
                     "/D_MSC_VER=${MSVC_VERSION} /safeseh"
        )
        list(APPEND cilk_link_flags /SAFESEH)
    endif()
    if("${CILK_BUILD_TYPE}" STREQUAL "Release")
        list(APPEND cilk_flags /Qipo)
    endif()
    if("${CILK_BUILD_TYPE}" STREQUAL "Debug")
        list(APPEND cilk_flags /DSUPPORT_GET_CURRENT_FIBER=1)
    endif()
else()
    # Common *nix flags
    set(cilk_flags
        -DIN_CILK_RUNTIME=1
        -fPIC
        -Wall
        -DHAVE_ALLOCA_H
        -DHAVE_ATTRIBUTE_VISIBILITY
        -DHAVE_PTHREAD_AFFINITY_NP
        -DDONT_USE_CPU_ALLOC_SIZE
        -D__need_wchar_t
        -D__need_size_t
    )
    # OS-dependent flags
    if(${APPLE})
        list(APPEND cilk_flags
             -D_DARWIN_C_SOURCE=1
             -Wno-uninitialized
             -Wno-unused-variable
        )
    else()
        list(APPEND cilk_flags
             -D_GNU_SOURCE
             -D_FORTIFY_SOURCE=2
        )
    endif()
    # Common C flags
    set(cilk_c_flags
        -std=c99
    )
    # -fcilkplus is required when using GCC/LLVM
    if(NOT "${CMAKE_C_COMPILER_ID}" STREQUAL "Intel")
        set_property(SOURCE runtime/cilk-abi.c
                            runtime/cilk-abi-cilk-for.cpp
                            runtime/cilk_api.c
                            runtime/global_state.cpp
                            runtime/reducer_impl.cpp
                            runtime/scheduler.c
                     APPEND PROPERTY COMPILE_FLAGS -fcilkplus 
        )
    endif()
    # Link flags
    if(${APPLE})
        list(APPEND cilk_link_flags
             "-Wl,-exported_symbols_list,${CilkRuntime_SOURCE_DIR}/runtime/mac-symbols.txt"
        )
    else()
        list(APPEND cilk_link_flags
             "-Wl,--version-script,${CilkRuntime_SOURCE_DIR}/runtime/gnu-symbols.ver"
             # security flags
             -Wl,-z,noexecstack
             -Wl,-z,now
             -ldl
             -Wl,-z,relro
        )
    endif()
endif()

# Override the default flags for Windows build.
if(${WIN32})
    set(CMAKE_C_FLAGS_RELEASE "/MT /O2 /Ob2 /DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "/MT /O2 /Ob2 /DNDEBUG")
    set(CMAKE_C_FLAGS_DEBUG "/D_DEBUG /MTd /Zi /Od /Ob0 /RTC1")
    set(CMAKE_CXX_FLAGS_DEBUG "/D_DEBUG /MTd /Zi /Od /Ob0 /RTC1")
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} /INCREMENTAL:NO")
endif()
# Sets global flags.
foreach(f ${cilk_flags})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${f}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${f}")
endforeach()
foreach(f ${cilk_c_flags})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${f}")
endforeach()
foreach(f ${cilk_cxx_flags})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${f}")
endforeach()
foreach(f ${cilk_link_flags})
    set(final_link_flags "${final_link_flags} ${f}")
endforeach()


################################################################################
# Target definitions
################################################################################

# Shared library
add_library(cilkrts SHARED ${cilkrts_srcs})
set(cilk_targets cilkrts)
if(${WIN32})
    set_target_properties(cilkrts PROPERTIES
                          OUTPUT_NAME cilkrts20 # should be compatible with icl
                          LINK_FLAGS ${final_link_flags}
    )
else()
    set_target_properties(cilkrts PROPERTIES
                          OSX_ARCHITECTURES x86_64
                          MACOSX_RPATH true
                          LINKER_LANGUAGE CXX
                          LINK_FLAGS ${final_link_flags}
                          SOVERSION 5 # should be compatible with clang
    )
    target_link_libraries(cilkrts pthread)
    # Static library
    add_library(cilkrts_static STATIC ${cilkrts_srcs})
    set_target_properties(cilkrts_static PROPERTIES
                          OSX_ARCHITECTURES x86_64
                          LINKER_LANGUAGE CXX
                          OUTPUT_NAME cilkrts
    )
    list(APPEND cilk_targets cilkrts_static)
endif()

# Install headers and libraries
install(DIRECTORY ${CilkRuntime_SOURCE_DIR}/include/cilk
        DESTINATION include
        FILES_MATCHING PATTERN "*.h"
)

#file(COPY ${CilkRuntime_SOURCE_DIR}/include/cilk 
#     DESTINATION ../../../../lib/clang/${CLANG_VERSION}/include/
#     FILES_MATCHING PATTERN "*.h")

if(${WIN32})
    install(TARGETS ${cilk_targets}
            RUNTIME DESTINATION bin
            ARCHIVE DESTINATION lib
    )
    install(FILES ${CilkRuntime_BINARY_DIR}/cilkrts20.pdb
            DESTINATION bin
    )
else()
    install(TARGETS ${cilk_targets}
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib 
    )
endif()

# Print the current environment
message("--------------")
message("  C Compiler = ${CMAKE_C_COMPILER}")
message("CXX Compiler = ${CMAKE_CXX_COMPILER}")
message("     C Flags = ${CMAKE_C_FLAGS}")
message("   CXX Flags = ${CMAKE_CXX_FLAGS}")
message("  LINK Flags = ${final_link_flags}")
message(" Install Dir = ${CMAKE_INSTALL_PREFIX}")
message("  Build Type = ${CILK_BUILD_TYPE}")
message("  Build Arch = ${CILK_ARCH}")
message("     Targets = ${cilk_targets}")
message("--------------")
#add_dependencies (compiler-rt cilkrts)

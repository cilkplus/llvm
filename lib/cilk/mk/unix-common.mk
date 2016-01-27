#########################################################################
#
# Copyright (C) 2009-2011 , Intel Corporation
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 
#   * Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#   * Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in
#     the documentation and/or other materials provided with the
#     distribution.
#   * Neither the name of Intel Corporation nor the names of its
#     contributors may be used to endorse or promote products derived
#     from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
# OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
# AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
# WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
###########################################################################
# Common environment for unix systems
#

# Default target is linux64 (release mode)
ifeq ($(findstring 64,$(shell uname -m)),)
  # If running on a 32-bit OS, default target is linux32
    ifeq ($(shell uname),Darwin)
        OUT ?= mac32
    else
        OUT ?= linux32
    endif
else
    ifeq ($(shell uname),Darwin)
        OUT ?= mac64
    else
        OUT ?= linux64
    endif
endif

# Strip the 'r' suffix off, if present.  Default is release mode.
ifeq ($(OUT),linux64r)
    override OUT = linux64
else ifeq ($(OUT),linux32r)
    override OUT = linux32
else ifeq ($(OUT),mac64r)
    override OUT = mac64
else ifeq ($(OUT),mac32r)
    override OUT = mac32
else ifeq ($(OUT),micr)
    override OUT = mic
else ifeq ($(OUT),miclinuxr)
    override OUT = miclinux
else ifeq ($(OUT),mic2linuxr)
    override OUT = mic2linux
endif

ifeq ($(OUT),linux64)
    # Linux 64 release mode
    SIZE = 64
    TARGET_OS=linux
    TARGET_ARCHITECTURE=intel64
else ifeq ($(OUT),linux32)
    # Linux 32 release mode
    SIZE = 32
    TARGET_OS=linux
    TARGET_ARCHITECTURE=ia32
else ifeq ($(OUT),mac64)
    # Mac OS 64 release mode
    SIZE = 64
    TARGET_OS=macos
    TARGET_ARCHITECTURE=intel64
else ifeq ($(OUT),mac32)
    # Mac OS 32 release mode
    SIZE = 32
    TARGET_OS=macos
    TARGET_ARCHITECTURE=ia32
else ifeq ($(OUT),mic)
    # MIC release mode
    SIZE = 64
    TARGET_OS=mic
    TARGET_ARCHITECTURE=intel64
else ifeq ($(OUT),miclinux)
    # MIC, Linux, release mode
    SIZE = 64
    TARGET_OS=miclinux
    TARGET_ARCHITECTURE=intel64
else ifeq ($(OUT),mic2linux)
    # MIC, Linux, release mode
    SIZE = 64
    TARGET_OS=miclinux
    TARGET_ARCHITECTURE=intel64
else ifeq ($(findstring mic,$(OUT)),mic)
    # Generic MIC.
    SIZE = 64
    TARGET_OS=miclinux
    TARGET_ARCHITECTURE=intel64
else ifeq ($(OUT),linux64d)
    # Linux 64 debug mode
    SIZE = 64
    OPT ?= -g --no-inline
    TARGET_OS=linux
    TARGET_ARCHITECTURE=intel64
    DEBUG_BUILD=true
else ifeq ($(OUT),linux32d)
    # Linux 32 debug mode
    SIZE = 32
    OPT ?= -g --no-inline
    TARGET_OS=linux
    TARGET_ARCHITECTURE=ia32
    DEBUG_BUILD=true
else ifeq ($(OUT),mac64d)
    # Mac OS 64 debug mode
    SIZE = 64
    OPT ?= -g --no-inline
    TARGET_OS=macos
    TARGET_ARCHITECTURE=intel64
    DEBUG_BUILD=true
else ifeq ($(OUT),mac32d)
    # Mac OS 32 debug mode
    SIZE = 32
    OPT ?= -g --no-inline
    TARGET_OS=macos
    TARGET_ARCHITECTURE=ia32
    DEBUG_BUILD=true
else ifeq ($(OUT),micd)
    # MIC debug mode
    SIZE = 64
    OPT ?= -g --no-inline
    TARGET_OS=mic
    TARGET_ARCHITECTURE=intel64
    DEBUG_BUILD=true
else ifeq ($(OUT),miclinuxd)
    # MIC LInux debug mode
    SIZE = 64
    OPT ?= -g --no-inline
    TARGET_OS=miclinux
    TARGET_ARCHITECTURE=intel64
    DEBUG_BUILD=true
else ifeq ($(OUT),mic2linuxd)
    # MIC2 Linux debug mode
    SIZE = 64
    OPT ?= -g --no-inline
    TARGET_OS=miclinux
    TARGET_ARCHITECTURE=intel64
    DEBUG_BUILD=true
else
    $(error OUT="$(OUT)" not one of linux32[rd], linux64[rd], mac64[rd], mac32[rd], mic[rd], miclinux[rd], mic2linux[rd])
endif

# Target type.
# Any OUT with a mic substring is a MIC target.
ifneq ($(findstring mic,$(OUT)),)
  mic_target=true
else ifeq ($(OUT:d=),mac64)
  macos_target=true
else ifeq ($(OUT:d=),mac32)
  macos_target=true
endif

ifdef macos_target
  OUTDIR = $(TOP)/mac-build
else
  OUTDIR = $(TOP)/unix-build
endif

THIRD_PARTY = $(TOP)/../../3rdparty

# Select "fromdos" for Ubuntu,
# or "dos2unix" for RedHat.
FROMDOS = $(shell which fromdos || which dos2unix)
ifeq ("$(FROMDOS)", "")
    $(warning No text conversion utility found.   This build should not be used to create a kit.)
    # Set FROMDOS to be a no-op so it can continue anyway.
    # Most of the time, we don't care.
    FROMDOS=echo
endif

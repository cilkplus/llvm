#===- lib/cilk/runtime/Makefile.mk -------------------------*- Makefile -*--===#
#
#                     The LLVM Compiler Infrastructure
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#

ModuleName := cilk
SubDirs :=

Sources := $(foreach file, \
             $(wildcard $(Dir)/*.c) $(wildcard $(Dir)/*.cpp),\
           $(notdir $(file)))

ObjNames := $(patsubst %.c,%.o,$(Sources:%.cpp=%.o))

Implementation := Generic

# FIXME: use automatic dependencies?
Dependencies := $(wildcard $(Dir)/../../include/*.h)

# Define a convenience variable for all the cilkrts functions.
CilkrtsFunctions := $(patsubst %.c,%,$(Sources:%.cpp=%))

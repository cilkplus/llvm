#===- lib/cilk/include/Makefile.mk -------------------------*- Makefile -*--===#
#
#                     The LLVM Compiler Infrastructure
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#

ModuleName := cilk
SubDirs :=

ObjNames :=
Implementation :=
Dependencies :=

# Cilk headers for installation
CilkHeaders := $(wildcard $(Dir)/cilk/*.h)

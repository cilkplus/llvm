#  @copyright
#  Copyright (C) 2011-2013, Intel Corporation
#  All rights reserved.
#  
#  @copyright
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
#  @copyright
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

AC_INIT([Cilk Runtime], [2.0], [cilk@intel.com])
AC_PREREQ([2.63])

# Needed to define ${target}.  Needs to be very early to avoid annoying
# warning about calling AC_ARG_PROGRAM before AC_CANONICAL_SYSTEM
AC_CANONICAL_SYSTEM

AM_INIT_AUTOMAKE([1.11 no-define])

# Check whether alloca.h is available
AC_FUNC_ALLOCA

# Build a DLL on Windows
AC_LIBTOOL_WIN32_DLL
AC_PROG_CXX
AC_PROG_CC
AC_PROG_LIBTOOL
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_FILES([Makefile])

# Check whether the target supports protected visibility.
save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -Werror"
AC_TRY_COMPILE([void __attribute__((visibility("protected"))) foo(void) { }],
	       [], libcilkrts_cv_have_attribute_visibility=yes,
	       libcilkrts_cv_have_attribute_visibility=no)
CFLAGS="$save_CFLAGS"
if test $libcilkrts_cv_have_attribute_visibility = yes; then
  AC_DEFINE(HAVE_ATTRIBUTE_VISIBILITY, 1,
    [Define to 1 if the target supports __attribute__((visibility(...))).])
fi

# Set config_dir based on the target.  config_dir specifies where to get
# target-specific files.  The generic implementation is incomplete, but
# contains information on what's needed
case "${target}" in

  x86_64-*-*)
    config_dir="x86"
    ;;

  i[456]86-*-*)
    config_dir="x86"
    ;;

  *)
    config_dir="generic"
    ;;

esac
AC_SUBST(config_dir)

# We have linker scripts for appropriate operating systems
linux_linker_script=no
case "${host}" in
    *-*-linux*)
        linux_linker_script=yes
        ;;
esac
AM_CONDITIONAL(LINUX_LINKER_SCRIPT, test "$linux_linker_script" = "yes")

mac_linker_script=no
case "${host}" in
    *-*-apple*)
        mac_linker_script=yes
        ;;
esac
AM_CONDITIONAL(MAC_LINKER_SCRIPT, test "$mac_linker_script" = "yes")

# Must be last
AC_OUTPUT

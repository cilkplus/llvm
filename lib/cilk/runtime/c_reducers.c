/* c_reducers.c                  -*-C-*-
 *
 *************************************************************************
 *
 * Copyright (C) 2010-2011 , Intel Corporation
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
 * WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 **************************************************************************/

/* Implementation of C reducers */

#include <cilk/reducer_opadd.h>
#include <cilk/reducer_opand.h>
#include <cilk/reducer_opor.h>
#include <cilk/reducer_opxor.h>
#include <cilk/reducer_max.h>
#include <cilk/reducer_min.h>
#include <limits.h>
#include <math.h> /* HUGE_VAL */

#ifndef _MSC_VER
# include <stdint.h> /* WCHAR_MIN */
#else
# include <wchar.h> /* WCHAR_MIN */
#endif

/* Floating-point constants */
#ifndef HUGE_VALF
  static const unsigned int __huge_valf[] = {0x7f800000};
# define HUGE_VALF (*((const float *)__huge_valf))
#endif

#ifndef HUGE_VALL
  static const unsigned int __huge_vall[] = {0, 0, 0x00007f80, 0};
# define HUGE_VALL (*((const long double *)__huge_vall))
#endif

// Disable warning about integer conversions losing significant bits.
// The code is correct as is.
#pragma warning(disable:2259)

CILK_C_REDUCER_OPADD_IMP(char,char)
CILK_C_REDUCER_OPADD_IMP(unsigned char,uchar)
CILK_C_REDUCER_OPADD_IMP(signed char,schar)
CILK_C_REDUCER_OPADD_IMP(wchar_t,wchar_t)
CILK_C_REDUCER_OPADD_IMP(short,short)
CILK_C_REDUCER_OPADD_IMP(unsigned short,ushort)
CILK_C_REDUCER_OPADD_IMP(int,int)
CILK_C_REDUCER_OPADD_IMP(unsigned int,uint)
CILK_C_REDUCER_OPADD_IMP(unsigned int,unsigned) // alternate name
CILK_C_REDUCER_OPADD_IMP(long,long)
CILK_C_REDUCER_OPADD_IMP(unsigned long,ulong)
CILK_C_REDUCER_OPADD_IMP(long long,longlong)
CILK_C_REDUCER_OPADD_IMP(unsigned long long,ulonglong)
CILK_C_REDUCER_OPADD_IMP(float,float)
CILK_C_REDUCER_OPADD_IMP(double,double)
CILK_C_REDUCER_OPADD_IMP(long double,longdouble)

CILK_C_REDUCER_OPAND_IMP(char,char)
CILK_C_REDUCER_OPAND_IMP(unsigned char,uchar)
CILK_C_REDUCER_OPAND_IMP(signed char,schar)
CILK_C_REDUCER_OPAND_IMP(wchar_t,wchar_t)
CILK_C_REDUCER_OPAND_IMP(short,short)
CILK_C_REDUCER_OPAND_IMP(unsigned short,ushort)
CILK_C_REDUCER_OPAND_IMP(int,int)
CILK_C_REDUCER_OPAND_IMP(unsigned int,uint)
CILK_C_REDUCER_OPAND_IMP(unsigned int,unsigned) // alternate name
CILK_C_REDUCER_OPAND_IMP(long,long)
CILK_C_REDUCER_OPAND_IMP(unsigned long,ulong)
CILK_C_REDUCER_OPAND_IMP(long long,longlong)
CILK_C_REDUCER_OPAND_IMP(unsigned long long,ulonglong)

CILK_C_REDUCER_OPOR_IMP(char,char)
CILK_C_REDUCER_OPOR_IMP(unsigned char,uchar)
CILK_C_REDUCER_OPOR_IMP(signed char,schar)
CILK_C_REDUCER_OPOR_IMP(wchar_t,wchar_t)
CILK_C_REDUCER_OPOR_IMP(short,short)
CILK_C_REDUCER_OPOR_IMP(unsigned short,ushort)
CILK_C_REDUCER_OPOR_IMP(int,int)
CILK_C_REDUCER_OPOR_IMP(unsigned int,uint)
CILK_C_REDUCER_OPOR_IMP(unsigned int,unsigned) // alternate name
CILK_C_REDUCER_OPOR_IMP(long,long)
CILK_C_REDUCER_OPOR_IMP(unsigned long,ulong)
CILK_C_REDUCER_OPOR_IMP(long long,longlong)
CILK_C_REDUCER_OPOR_IMP(unsigned long long,ulonglong)

CILK_C_REDUCER_OPXOR_IMP(char,char)
CILK_C_REDUCER_OPXOR_IMP(unsigned char,uchar)
CILK_C_REDUCER_OPXOR_IMP(signed char,schar)
CILK_C_REDUCER_OPXOR_IMP(wchar_t,wchar_t)
CILK_C_REDUCER_OPXOR_IMP(short,short)
CILK_C_REDUCER_OPXOR_IMP(unsigned short,ushort)
CILK_C_REDUCER_OPXOR_IMP(int,int)
CILK_C_REDUCER_OPXOR_IMP(unsigned int,uint)
CILK_C_REDUCER_OPXOR_IMP(unsigned int,unsigned) // alternate name
CILK_C_REDUCER_OPXOR_IMP(long,long)
CILK_C_REDUCER_OPXOR_IMP(unsigned long,ulong)
CILK_C_REDUCER_OPXOR_IMP(long long,longlong)
CILK_C_REDUCER_OPXOR_IMP(unsigned long long,ulonglong)

CILK_C_REDUCER_MAX_IMP(char,char,CHAR_MIN)
CILK_C_REDUCER_MAX_IMP(unsigned char,uchar,0)
CILK_C_REDUCER_MAX_IMP(signed char,schar,SCHAR_MIN)
CILK_C_REDUCER_MAX_IMP(wchar_t,wchar_t,WCHAR_MIN)
CILK_C_REDUCER_MAX_IMP(short,short,SHRT_MIN)
CILK_C_REDUCER_MAX_IMP(unsigned short,ushort,0)
CILK_C_REDUCER_MAX_IMP(int,int,INT_MIN)
CILK_C_REDUCER_MAX_IMP(unsigned int,uint,0)
CILK_C_REDUCER_MAX_IMP(unsigned int,unsigned,0) // alternate name
CILK_C_REDUCER_MAX_IMP(long,long,LONG_MIN)
CILK_C_REDUCER_MAX_IMP(unsigned long,ulong,0)
CILK_C_REDUCER_MAX_IMP(long long,longlong,LLONG_MIN)
CILK_C_REDUCER_MAX_IMP(unsigned long long,ulonglong,0)
CILK_C_REDUCER_MAX_IMP(float,float,-HUGE_VALF)
CILK_C_REDUCER_MAX_IMP(double,double,-HUGE_VAL)
CILK_C_REDUCER_MAX_IMP(long double,longdouble,-HUGE_VALL)
CILK_C_REDUCER_MAX_INDEX_IMP(char,char,CHAR_MIN)
CILK_C_REDUCER_MAX_INDEX_IMP(unsigned char,uchar,0)
CILK_C_REDUCER_MAX_INDEX_IMP(signed char,schar,SCHAR_MIN)
CILK_C_REDUCER_MAX_INDEX_IMP(wchar_t,wchar_t,WCHAR_MIN)
CILK_C_REDUCER_MAX_INDEX_IMP(short,short,SHRT_MIN)
CILK_C_REDUCER_MAX_INDEX_IMP(unsigned short,ushort,0)
CILK_C_REDUCER_MAX_INDEX_IMP(int,int,INT_MIN)
CILK_C_REDUCER_MAX_INDEX_IMP(unsigned int,uint,0)
CILK_C_REDUCER_MAX_INDEX_IMP(unsigned int,unsigned,0) // alternate name
CILK_C_REDUCER_MAX_INDEX_IMP(long,long,LONG_MIN)
CILK_C_REDUCER_MAX_INDEX_IMP(unsigned long,ulong,0)
CILK_C_REDUCER_MAX_INDEX_IMP(long long,longlong,LLONG_MIN)
CILK_C_REDUCER_MAX_INDEX_IMP(unsigned long long,ulonglong,0)
CILK_C_REDUCER_MAX_INDEX_IMP(float,float,-HUGE_VALF)
CILK_C_REDUCER_MAX_INDEX_IMP(double,double,-HUGE_VAL)
CILK_C_REDUCER_MAX_INDEX_IMP(long double,longdouble,-HUGE_VALL)

CILK_C_REDUCER_MIN_IMP(char,char,CHAR_MAX)
CILK_C_REDUCER_MIN_IMP(unsigned char,uchar,CHAR_MIN)
CILK_C_REDUCER_MIN_IMP(signed char,schar,SCHAR_MAX)
CILK_C_REDUCER_MIN_IMP(wchar_t,wchar_t,WCHAR_MAX)
CILK_C_REDUCER_MIN_IMP(short,short,SHRT_MAX)
CILK_C_REDUCER_MIN_IMP(unsigned short,ushort,USHRT_MAX)
CILK_C_REDUCER_MIN_IMP(int,int,INT_MAX)
CILK_C_REDUCER_MIN_IMP(unsigned int,uint,UINT_MAX)
CILK_C_REDUCER_MIN_IMP(unsigned int,unsigned,UINT_MAX) // alternate name
CILK_C_REDUCER_MIN_IMP(long,long,LONG_MAX)
CILK_C_REDUCER_MIN_IMP(unsigned long,ulong,ULONG_MAX)
CILK_C_REDUCER_MIN_IMP(long long,longlong,LLONG_MAX)
CILK_C_REDUCER_MIN_IMP(unsigned long long,ulonglong,ULLONG_MAX)
CILK_C_REDUCER_MIN_IMP(float,float,HUGE_VALF)
CILK_C_REDUCER_MIN_IMP(double,double,HUGE_VAL)
CILK_C_REDUCER_MIN_IMP(long double,longdouble,HUGE_VALL)
CILK_C_REDUCER_MIN_INDEX_IMP(char,char,CHAR_MAX)
CILK_C_REDUCER_MIN_INDEX_IMP(unsigned char,uchar,CHAR_MIN)
CILK_C_REDUCER_MIN_INDEX_IMP(signed char,schar,SCHAR_MAX)
CILK_C_REDUCER_MIN_INDEX_IMP(wchar_t,wchar_t,WCHAR_MAX)
CILK_C_REDUCER_MIN_INDEX_IMP(short,short,SHRT_MAX)
CILK_C_REDUCER_MIN_INDEX_IMP(unsigned short,ushort,USHRT_MAX)
CILK_C_REDUCER_MIN_INDEX_IMP(int,int,INT_MAX)
CILK_C_REDUCER_MIN_INDEX_IMP(unsigned int,uint,UINT_MAX)
CILK_C_REDUCER_MIN_INDEX_IMP(unsigned int,unsigned,UINT_MAX) // alternate name
CILK_C_REDUCER_MIN_INDEX_IMP(long,long,LONG_MAX)
CILK_C_REDUCER_MIN_INDEX_IMP(unsigned long,ulong,ULONG_MAX)
CILK_C_REDUCER_MIN_INDEX_IMP(long long,longlong,LLONG_MAX)
CILK_C_REDUCER_MIN_INDEX_IMP(unsigned long long,ulonglong,ULLONG_MAX)
CILK_C_REDUCER_MIN_INDEX_IMP(float,float,HUGE_VALF)
CILK_C_REDUCER_MIN_INDEX_IMP(double,double,HUGE_VAL)
CILK_C_REDUCER_MIN_INDEX_IMP(long double,longdouble,HUGE_VALL)

/* End reducer_opadd.c */

/*
 * Copyright (C) 2009-2011 , Intel Corporation
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
 */

/*
 * reducer_opor.h
 *
 * Purpose: Reducer hyperobject to compute bitwise OR values
 */

#ifndef REDUCER_OPOR_H_INCLUDED
#define REDUCER_OPOR_H_INCLUDED

#include <cilk/reducer.h>

#ifdef __cplusplus

/* C++ Interface
 *
 * Purpose: Reducer hyperobject to compute bitwise OR values
 *          When bool is passed as 'Type', it computes logical OR
 *          operation.
 *
 * Classes: reducer_opor<Type>
 *
 * Description:
 * ============
 * This component provides a reducer-type hyperobject representation
 * that allows conducting bitwise OR operation to a non-local variable 
 * using the |=, | operators.  A common operation 
 * when traversing a data structure is to bit-wise OR values 
 * into a non-local numeric variable.  When Cilk parallelism is 
 * introduced, however, a data race will occur on the variable holding 
 * the bit-wise OR result.  By replacing the variable with the
 * hyperobject defined in this component, the data race is eliminated.
 *
 * When bool is passed as the 'Type', this reducer conducts logic OR 
 * operation.
 *
 * Usage Example:
 * ==============
 * Assume we wish to traverse an array of objects, performing a bit-wise OR
 * operation on each object and accumulating the result of the operation 
 * into an integer variable.
 *..
 *  unsigned int compute(const X& v);
 *
 *  int test()
 *  {
 *      const std::size_t ARRAY_SIZE = 1000000;
 *      extern X myArray[ARRAY_SIZE];
 *      // ...
 *
 *      unsigned int result = 0;
 *      for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
 *      {
 *          result |= compute(myArray[i]);
 *      }
 *
 *      std::cout << "The result is: " << result << std::endl;
 *
 *      return 0;
 *  }
 *..
 * Changing the 'for' to a 'cilk_for' will cause the loop to run in parallel,
 * but doing so will create a data race on the 'result' variable.
 * The race is solved by changing 'result' to a 'reducer_opor' hyperobject:
 *..
 *  unsigned int compute(const X& v);
 *  
 *   
 *  int test()
 *  {
 *      const std::size_t ARRAY_SIZE = 1000000;
 *      extern X myArray[ARRAY_SIZE];
 *      // ...
 *       
 *      cilk::reducer_opor<unsigned int> result;
 *      cilk_for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
 *      {
 *          *result |= compute(myArray[i]);
 *      }
 *
 *      std::cout << "The result is: " 
 *                << result.get_value() << std::endl;
 *
 *      return 0;
 *  }
 *  
 *
 * Operations provided:
 * ====================
 * Given 'reducer_opor' objects, x and y, the following are
 * valid statements:
 *..
 *  *x |= 5;
 *  *x = *x | 5;
 *..
 * The following are not valid expressions and will result in a run-time error
 * in a debug build:
 *..
 *  x = y;       // Cannot assign one reducer to another
 *  *x = *y | 5; // Mixed reducers
 *  *x = 5 | *x; // operator| is not necessarily commutative
 *..
 *
 * Requirements on the 'Type' parameter
 * ====================================
 * The 'Type' parameter used to instantiate the 'reducer_opor' class must
 * provide a |= operator that meets the requirements for an
 * *associative* *mutating* *operator* as defined in the Cilk++ user manual.
 * The default constructor for 'Type' must yield an OR identity, i.e.,
 * a value (such as unsigned int 0, bool false) that, when performed 
 * OR operation to any other value, yields the other value.
 */

#include <new>

namespace cilk
{

/**
 * @brief A reducer-type hyperobject representation that supports bitwise OR
 * operations on a non-local variable using the |=, | operators.
 *
 * A common operation when traversing a data structure is to bit-wise OR 
 * values into a non-local numeric variable.  When Cilk parallelism is 
 * introduced, however, a data race will occur on the variable holding 
 * the bit-wise OR result.  By replacing the variable with the
 * hyperobject defined in this component, the data race is eliminated.
 *
 * When bool is passed as the 'Type', this reducer conducts logic OR 
 * operation.
 */
template <typename Type>
class reducer_opor
{
  public:
    /// Definition of data view, operation, and identity for reducer_opor
    class Monoid : public monoid_base<Type>
    {
      public:
            /// Combines two views of the data
            static void reduce(Type* left, Type* right);
    };

    /// "PRIVATE" HELPER CLASS
    class temp_or {
        friend class reducer_opor;

        Type* valuePtr_;

        // Default copy constructor, no assignment operator
        temp_or& operator=(const temp_or&);

        explicit temp_or(Type* valuePtr);

      public:
        temp_or& operator|(const Type& x);
    };

  public:

    /// Construct an 'reducer_opor' object with a value of 'Type()'.
    reducer_opor();

    /// Construct an 'reducer_opor' object with the specified initial value.
    explicit reducer_opor(const Type& initial_value);

    /// Return a const reference to the current value of this object.
    ///
    /// @warning If this method is called before the parallel calculation is
    /// complete, the value returned by this method will be a partial result.
    const Type& get_value() const;

    /// Set the value of this object.
    ///
    /// @warning: Setting the value of a reducer such that it violates the 
    /// associative operation algebra will yield results that are likely to 
    /// differ from serial execution and may differ from run to run.
    void set_value(const Type& value);

    /// OR 'x' to the value of this reducer and produce a temporary and object.
    /// The temporary and can be used for additional bit-wise operations 
    /// or assigned back to this reducer.
    temp_or operator|(const Type& x) const;

    /// OR 'x' to the value of this object.
    reducer_opor& operator|=(const Type& x);

    /// Merge the result of OR operation into this object.  The OR operation
    /// must involve this reducer, i.e., x = x + 5; not x = y + 5;
    reducer_opor& operator=(const temp_or& temp);

    reducer_opor&       operator*()       { return *this; }
    reducer_opor const& operator*() const { return *this; }

    reducer_opor*       operator->()       { return this; }
    reducer_opor const* operator->() const { return this; }

  private:
    friend class temp_or;

    // Hyperobject to serve up views
    reducer<Monoid> imp_;

    // Not copyable
    reducer_opor(const reducer_opor&);
    reducer_opor& operator=(const reducer_opor&);
};

/////////////////////////////////////////////////////////////////////////////
// Implementation of inline and template functions
/////////////////////////////////////////////////////////////////////////////

// ------------------------------------
// template class reducer_opor::Monoid
// ------------------------------------

template <typename Type>
void
reducer_opor<Type>::Monoid::reduce(Type* left, Type* right)
{
    *left |= *right;
}

// ----------------------------
// template class reducer_opor
// ----------------------------

template <typename Type>
inline
reducer_opor<Type>::reducer_opor()
    : imp_(Type())
{
}

template <typename Type>
inline
reducer_opor<Type>::reducer_opor(const Type& initial_value)
    : imp_(initial_value)
{
}

template <typename Type>
inline
const Type& reducer_opor<Type>::get_value() const
{
    return imp_.view();
}

template <typename Type>
inline
void reducer_opor<Type>::set_value(const Type& value)
{
    imp_.view() = value;
}

template <typename Type>
inline
typename reducer_opor<Type>::temp_or
reducer_opor<Type>::operator|(const Type& x) const
{
    Type* valuePtr = const_cast<Type*>(&imp_.view());
    *valuePtr = *valuePtr | x;
    return temp_or(valuePtr);
}

template <typename Type>
inline
reducer_opor<Type>& reducer_opor<Type>::operator|=(const Type& x)
{
    imp_.view() |= x;
    return *this;
}

template <typename Type>
inline
reducer_opor<Type>&
reducer_opor<Type>::operator=(
    const typename reducer_opor<Type>::temp_or& temp)
{
    // No-op.  Just test that temp was constructed from this.
    __CILKRTS_ASSERT(&imp_.view() == temp.valuePtr_);
    return *this;
}

// --------------------------------------
// template class reducer_opor::temp_or
// --------------------------------------

template <typename Type>
inline
reducer_opor<Type>::temp_or::temp_or(Type *valuePtr)
    : valuePtr_(valuePtr)
{
}

template <typename Type>
inline
typename reducer_opor<Type>::temp_or&
reducer_opor<Type>::temp_or::operator|(const Type& x)
{
    *valuePtr_ = *valuePtr_ | x;
    return *this;
}

} // namespace cilk

#endif /* __cplusplus */

/* C Interface
 */

__CILKRTS_BEGIN_EXTERN_C

#define CILK_C_REDUCER_OPOR_TYPE(tn)                                         \
    __CILKRTS_MKIDENT(cilk_c_reducer_opor_,tn)
#define CILK_C_REDUCER_OPOR(obj,tn,v)                                        \
    CILK_C_REDUCER_OPOR_TYPE(tn) obj =                                       \
        CILK_C_INIT_REDUCER(_Typeof(obj.value),                              \
                        __CILKRTS_MKIDENT(cilk_c_reducer_opor_reduce_,tn),   \
                        __CILKRTS_MKIDENT(cilk_c_reducer_opor_identity_,tn), \
                        __cilkrts_hyperobject_noop_destroy, v)

/* Declare an instance of the reducer for a specific numeric type */
#define CILK_C_REDUCER_OPOR_INSTANCE(t,tn)                                \
    typedef CILK_C_DECLARE_REDUCER(t)                                     \
        __CILKRTS_MKIDENT(cilk_c_reducer_opor_,tn);                       \
    __CILKRTS_DECLARE_REDUCER_REDUCE(cilk_c_reducer_opor,tn,l,r);         \
    __CILKRTS_DECLARE_REDUCER_IDENTITY(cilk_c_reducer_opor,tn);  

/* Declare an instance of the reducer type for each numeric type */
CILK_C_REDUCER_OPOR_INSTANCE(char,char);
CILK_C_REDUCER_OPOR_INSTANCE(unsigned char,uchar);
CILK_C_REDUCER_OPOR_INSTANCE(signed char,schar);
CILK_C_REDUCER_OPOR_INSTANCE(wchar_t,wchar_t);
CILK_C_REDUCER_OPOR_INSTANCE(short,short);
CILK_C_REDUCER_OPOR_INSTANCE(unsigned short,ushort);
CILK_C_REDUCER_OPOR_INSTANCE(int,int);
CILK_C_REDUCER_OPOR_INSTANCE(unsigned int,uint);
CILK_C_REDUCER_OPOR_INSTANCE(unsigned int,unsigned); /* alternate name */
CILK_C_REDUCER_OPOR_INSTANCE(long,long);
CILK_C_REDUCER_OPOR_INSTANCE(unsigned long,ulong);
CILK_C_REDUCER_OPOR_INSTANCE(long long,longlong);
CILK_C_REDUCER_OPOR_INSTANCE(unsigned long long,ulonglong);
CILK_C_REDUCER_OPOR_INSTANCE(float,float);
CILK_C_REDUCER_OPOR_INSTANCE(double,double);
CILK_C_REDUCER_OPOR_INSTANCE(long double,longdouble);

/* Declare function bodies for the reducer for a specific numeric type */
#define CILK_C_REDUCER_OPOR_IMP(t,tn)                                     \
    __CILKRTS_DECLARE_REDUCER_REDUCE(cilk_c_reducer_opor,tn,l,r)          \
        { *(t*)l |= *(t*)r; }                                              \
    __CILKRTS_DECLARE_REDUCER_IDENTITY(cilk_c_reducer_opor,tn)            \
        { *(t*)v = (t)0; }

/* c_reducers.c contains definitions for all of the monoid functions
   for the C numeric tyeps.  The contents of reducer_opor.c are as follows:

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

*/

__CILKRTS_END_EXTERN_C

#endif /*  REDUCER_OPOR_H_INCLUDED */

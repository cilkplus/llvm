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
 * reducer_opadd.h
 *
 * Purpose: Reducer hyperobject to sum values
 */

#ifndef REDUCER_OPADD_H_INCLUDED
#define REDUCER_OPADD_H_INCLUDED

#include <cilk/reducer.h>

#ifdef __cplusplus

/* C++ Interface
 *
 * Classes: reducer_opadd<Type>
 *
 * Description:
 * ============
 * This component provides a reducer-type hyperobject representation
 * that allows adding values to a non-local variable using the +=, -=,
 * ++, --, +, and - operators.  A common operation when traversing a data
 * structure is to sum values into a non-local numeric variable.  When
 * Cilk parallelism is introduced, however, a data race will occur on
 * the variable holding the sum.  By replacing the variable with the
 * hyperobject defined in this component, the data race is eliminated.
 *
 * Usage Example:
 * ==============
 * Assume we wish to traverse an array of objects, performing an operation on
 * each object and accumulating the result of the operation into an integer
 * variable.
 *..
 *  int compute(const X& v);
 *
 *  int test()
 *  {
 *      const std::size_t ARRAY_SIZE = 1000000;
 *      extern X myArray[ARRAY_SIZE];
 *      // ...
 *
 *      int result = 0;
 *      for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
 *      {
 *          result += compute(myArray[i]);
 *      }
 *
 *      std::cout << "The result is: " << result << std::endl;
 *
 *      return 0;
 *  }
 *..
 * Changing the 'for' to a 'cilk_for' will cause the loop to run in parallel,
 * but doing so will create a data race on the 'result' variable.
 * The race is solved by changing 'result' to a 'reducer_opadd' hyperobject:
 *..
 *  int compute(const X& v);
 *
 *  int test()
 *  {
 *      const std::size_t ARRAY_SIZE = 1000000;
 *      extern X myArray[ARRAY_SIZE];
 *      // ...
 *
 *      cilk::reducer_opadd<int> result;
 *      cilk_for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
 *      {
 *          *result += compute(myArray[i]);
 *      }
 *
 *      std::cout << "The result is: " << result.get_value() << std::endl;
 *
 *      return 0;
 *  }
 *..
 *
 * Operations provided:
 * ====================
 * Given 'reducer_opadd' objects, x and y, the following are
 * valid statements:
 *..
 *  *x += 5;
 *  *x = *x + 5;
 *  *x -= 5;
 *  *y = *y - 5;
 *  ++*x;
 *  --*x;
 *  (*x)++;
 *  (*x)--;
 *..
 * The following are not valid expressions and will result in a run-time error
 * in a debug build:
 *..
 *  x = y;       // Cannot assign one reducer to another
 *  *x = *y + 5; // Mixed reducers
 *  *x = 5 + *x; // operator+ is not necessarily commutative
 *  *x = 5 - *x; // Violates associativity
 *..
 * The the current value of the reducer can be get and set using the
 * 'get_value' and 'set_value' methods, respectively.  As with most reducers,
 * 'set_value' and 'get_value' methods produce deterministic results only if
 * called before the first spawn after creating a 'hyperobject' or when all
 * strands spawned since creating the 'hyperobject' have been synced.  However,
 * the difference two values of the same reducer read twice in the same Cilk
 * strand *is* typically deterministic (assuming the usual relationship between
 * operator '+' and operator '-' for the specified 'Type'):
 *..
 *  cilk::reducer_opadd<int> x;
 *  cilk_spawn func();
 *  int a = x.get_value();
 *  *x += 5;
 *  int b = x.get_value();
 *  assert(b - a == 5);
 *..
 *
 * Requirements on the 'Type' parameter
 * ====================================
 * The 'Type' parameter used to instantiate the 'reducer_opadd' class must
 * provide a += operator that meets the requirements for an
 * *associative* *mutating* *operator* as defined in the Cilk++ user manual.
 * The default constructor for 'Type' must yield an additive identity, i.e.,
 * a value (such as integer zero) that, when added to any other value, yields
 * the other value.  If 'Type' also provides a -= operator, then subtraction
 * is also supported by this reducer. C++ integral types satisfy these
 * requirements.
 *
 * Note that C++ floating-point types do not support truly
 * associative addition in that (a + b) + c will exhibit different
 * round-off error than a + (b + c).  However, for numbers of similar
 * magnitude, a floating-point 'reducer_opadd' may still be useful.
 */

namespace cilk
{

/**
 * @brief A reducer-type hyperobject representation that allows adding values 
 * to a non-local variable using the +=, -=, ++, --, +, and - operators.
 * 
 * A common operation when traversing a data structure is to sum values into a 
 * non-local numeric variable.  When Cilk parallelism is introduced, however, 
 * a data race will occur on the variable holding the sum.  By replacing the 
 * variable with the hyperobject defined in this component, the data race is 
 * eliminated.
 */
template <typename Type>
class reducer_opadd
{
public:
    /// Definition of data view, operation, and identity for reducer_opadd
    class Monoid : public monoid_base<Type>
    {
      public:
        static void reduce(Type* left, Type* right);
    };

    /// "PRIVATE" HELPER CLASS
    class temp_sum {
        friend class reducer_opadd;

        Type* valuePtr_;

        // Default copy constructor, no assignment operator
        temp_sum& operator=(const temp_sum&);

        explicit temp_sum(Type* valuePtr);

      public:
        temp_sum& operator+(const Type& x);
        temp_sum& operator-(const Type& x);
    };

  public:

    /// Construct an 'reducer_opadd' object with a value of 'Type()'.
    reducer_opadd();

    /// Construct an 'reducer_opadd' object with the specified initial value.
    explicit reducer_opadd(const Type& initial_value);

    /// Return a const reference to the current value of this object.
    ///
    /// @warning If this method is called before the parallel calculation is
    /// complete, the value returned by this method will be a partial result.
    const Type& get_value() const;

    /// Set the value of this object.
    ///
    /// @warning Setting the value of a reducer such that it violates the 
    /// associative operation algebra will yield results that are likely to
    /// differ from serial execution and may differ from run to run.
    void set_value(const Type& value);

    /// Add 'x' to the value of this reducer and produce a temporary sum object.
    /// The temporary sum can be used for additional arithmetic or assigned back
    /// to this reducer.
    temp_sum operator+(const Type& x) const;

    /// Subtract 'x' from the value of this reducer and produce a temporary sum
    /// object.  The temporary sum can be used for additional arithmetic or
    /// assigned back to this reducer.
    temp_sum operator-(const Type& x) const;

    /// Add 'x' to the value of this object.
    reducer_opadd& operator+=(const Type& x);

    /// Subtract 'x' from the value of this object.
    reducer_opadd& operator-=(const Type& x);

    /// Increment the value of this object using pre-increment syntax.
    reducer_opadd& operator++();

    /// Increment the value of this object using post-increment syntax.  
    /// Because the reducer is not copy-constructible, it is not possible to
    /// return the previous value.
    void operator++(int);

    /// Decrement the value of this object using pre-decrement syntax.
    reducer_opadd& operator--();

    /// Decrement the value of this object using post-decrement syntax.  
    /// Because the reducer is not copy-constructible, it is not possible to
    /// return the previous value.
    void operator--(int);

    /// Merge the result of an addition into this object.  The addition
    /// must involve this reducer, i.e., x = x + 5; not x = y + 5;
    reducer_opadd& operator=(const temp_sum& temp);

    reducer_opadd&       operator*()       { return *this; }
    reducer_opadd const& operator*() const { return *this; }

    reducer_opadd*       operator->()       { return this; }
    reducer_opadd const* operator->() const { return this; }

  private:
    friend class temp_sum;

    // Hyperobject to serve up views
    reducer<Monoid> imp_;

    // Not copyable
    reducer_opadd(const reducer_opadd&);
    reducer_opadd& operator=(const reducer_opadd&);
};

/////////////////////////////////////////////////////////////////////////////
// Implementation of inline and template functions
/////////////////////////////////////////////////////////////////////////////

// ------------------------------------
// template class reducer_opadd::Monoid
// ------------------------------------

/**
 * Combines two views of the data.
 */
template <typename Type>
void
reducer_opadd<Type>::Monoid::reduce(Type* left, Type* right)
{
    *left += *right;
}

// ----------------------------
// template class reducer_opadd
// ----------------------------

template <typename Type>
inline
reducer_opadd<Type>::reducer_opadd()
    : imp_(Type())
{
}

template <typename Type>
inline
reducer_opadd<Type>::reducer_opadd(const Type& initial_value)
    : imp_(initial_value)
{
}

template <typename Type>
inline
const Type& reducer_opadd<Type>::get_value() const
{
    return imp_.view();
}

template <typename Type>
inline
void reducer_opadd<Type>::set_value(const Type& value)
{
    imp_.view() = value;
}

template <typename Type>
inline
typename reducer_opadd<Type>::temp_sum
reducer_opadd<Type>::operator+(const Type& x) const
{
    Type* valuePtr = const_cast<Type*>(&imp_.view());
    *valuePtr = *valuePtr + x;
    return temp_sum(valuePtr);
}

template <typename Type>
inline
typename reducer_opadd<Type>::temp_sum
reducer_opadd<Type>::operator-(const Type& x) const
{
    Type* valuePtr = const_cast<Type*>(&imp_.view());
    *valuePtr = *valuePtr - x;
    return temp_sum(valuePtr);
}

template <typename Type>
inline
reducer_opadd<Type>& reducer_opadd<Type>::operator+=(const Type& x)
{
    imp_.view() += x;
    return *this;
}

template <typename Type>
inline
reducer_opadd<Type>& reducer_opadd<Type>::operator-=(const Type& x)
{
    imp_.view() -= x;
    return *this;
}

template <typename Type>
inline
reducer_opadd<Type>& reducer_opadd<Type>::operator++()
{
    imp_.view() += 1;
    return *this;
}

template <typename Type>
inline
void reducer_opadd<Type>::operator++(int)
{
    imp_.view() += 1;
}

template <typename Type>
inline
reducer_opadd<Type>& reducer_opadd<Type>::operator--()
{
    imp_.view() -= 1;
    return *this;
}

template <typename Type>
inline
void reducer_opadd<Type>::operator--(int)
{
    imp_.view() -= 1;
}

template <typename Type>
inline
reducer_opadd<Type>&
reducer_opadd<Type>::operator=(
    const typename reducer_opadd<Type>::temp_sum& temp)
{
    // No-op.  Just test that temp was constructed from this.
    __CILKRTS_ASSERT(&imp_.view() == temp.valuePtr_);
    return *this;
}

// --------------------------------------
// template class reducer_opadd::temp_sum
// --------------------------------------

template <typename Type>
inline
reducer_opadd<Type>::temp_sum::temp_sum(Type *valuePtr)
    : valuePtr_(valuePtr)
{
}

template <typename Type>
inline
typename reducer_opadd<Type>::temp_sum&
reducer_opadd<Type>::temp_sum::operator+(const Type& x)
{
    *valuePtr_ = *valuePtr_ + x;
    return *this;
}

template <typename Type>
inline
typename reducer_opadd<Type>::temp_sum&
reducer_opadd<Type>::temp_sum::operator-(const Type& x)
{
    *valuePtr_ = *valuePtr_ - x;
    return *this;
}

} // namespace cilk

#endif // __cplusplus

/* C Interface
 */

__CILKRTS_BEGIN_EXTERN_C

#define CILK_C_REDUCER_OPADD_TYPE(tn)                                         \
    __CILKRTS_MKIDENT(cilk_c_reducer_opadd_,tn)
#define CILK_C_REDUCER_OPADD(obj,tn,v)                                        \
    CILK_C_REDUCER_OPADD_TYPE(tn) obj =                                       \
        CILK_C_INIT_REDUCER(_Typeof(obj.value),                               \
                        __CILKRTS_MKIDENT(cilk_c_reducer_opadd_reduce_,tn),   \
                        __CILKRTS_MKIDENT(cilk_c_reducer_opadd_identity_,tn), \
                        __cilkrts_hyperobject_noop_destroy, v)

/* Declare an instance of the reducer for a specific numeric type */
#define CILK_C_REDUCER_OPADD_INSTANCE(t,tn)                                \
    typedef CILK_C_DECLARE_REDUCER(t)                                      \
        __CILKRTS_MKIDENT(cilk_c_reducer_opadd_,tn);                       \
    __CILKRTS_DECLARE_REDUCER_REDUCE(cilk_c_reducer_opadd,tn,l,r);         \
    __CILKRTS_DECLARE_REDUCER_IDENTITY(cilk_c_reducer_opadd,tn);  

/* Declare an instance of the reducer type for each numeric type */
CILK_C_REDUCER_OPADD_INSTANCE(char,char);
CILK_C_REDUCER_OPADD_INSTANCE(unsigned char,uchar);
CILK_C_REDUCER_OPADD_INSTANCE(signed char,schar);
CILK_C_REDUCER_OPADD_INSTANCE(wchar_t,wchar_t);
CILK_C_REDUCER_OPADD_INSTANCE(short,short);
CILK_C_REDUCER_OPADD_INSTANCE(unsigned short,ushort);
CILK_C_REDUCER_OPADD_INSTANCE(int,int);
CILK_C_REDUCER_OPADD_INSTANCE(unsigned int,uint);
CILK_C_REDUCER_OPADD_INSTANCE(unsigned int,unsigned); /* alternate name */
CILK_C_REDUCER_OPADD_INSTANCE(long,long);
CILK_C_REDUCER_OPADD_INSTANCE(unsigned long,ulong);
CILK_C_REDUCER_OPADD_INSTANCE(long long,longlong);
CILK_C_REDUCER_OPADD_INSTANCE(unsigned long long,ulonglong);
CILK_C_REDUCER_OPADD_INSTANCE(float,float);
CILK_C_REDUCER_OPADD_INSTANCE(double,double);
CILK_C_REDUCER_OPADD_INSTANCE(long double,longdouble);

/* Declare function bodies for the reducer for a specific numeric type */
#define CILK_C_REDUCER_OPADD_IMP(t,tn)                                     \
    __CILKRTS_DECLARE_REDUCER_REDUCE(cilk_c_reducer_opadd,tn,l,r)          \
        { *(t*)l += *(t*)r; }                                              \
    __CILKRTS_DECLARE_REDUCER_IDENTITY(cilk_c_reducer_opadd,tn)            \
        { *(t*)v = 0; }

/* c_reducers.c contains definitions for all of the monoid functions
   for the C numeric tyeps.  The contents of reducer_opadd.c are as follows:

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

*/

__CILKRTS_END_EXTERN_C

#endif /*  REDUCER_OPADD_H_INCLUDED */

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
 * reducer_max.h
 *
 * Purpose: Reducer hyperobject to retain the max value.
 *
 * Classes: reducer_max<Type, Compare=std::less<Type> >
 *          reducer_max_index<Index, Value, Compare=std::less<Type> >
 *
 * Description:
 * ============
 * This component provides reducer-type hyperobject representations that allow
 * the maximum value, or the maximum value and an index, of a group of values to
 * be determined in parallel.
 *
 * Usage Example:
 * ==============
 * Suppose we wish to compute the maximum value in an array of integers.
 *
 *  int test()
 *  {
 *      int a[ARRAY_SIZE];
 *      int max = INT_MAX;
 *
 *      ...
 *
 *      for (int i = 0; i < ARRAY_SIZE; ++i)
 *      {
 *          if (max < a[i])
 *          {
 *              max = a[i];
 *          }
 *      }
 *      std::cout << "max = " << max << std::endl;
 *
 *      ...
 *  }
 *
 * Changing the 'for' to a 'cilk_for' will allow the loop to be run in parallel
 * but will create a data race on the variable 'max'.  The race can be resolved
 * by changing 'max' to a 'reducer_max' hyperobject:
 *
 *  int test()
 *  {
 *      int a[ARRAY_SIZE];
 *      cilk::reducer_max<int> max(INT_MAX);
 *
 *      ...
 *
 *      cilk_for (int i = 0; i < ARRAY_SIZE; ++i)
 *      {
 *          max->calc_max(a[i]);
 *      }
 *      std::cout << "max = " << max->get_value() << std::endl;
 *
 *      ...
 *  }
 *
 * A similar loop which calculates both the maximum value and index would be:
 *
 *  int test()
 *  {
 *      int a[ARRAY_SIZE];
 *      cilk::reducer_max_index<int, int> rmi(INT_MIN, -1);
 *
 *      ...
 *
 *      cilk_for (int i = 0; i < ARRAY_SIZE; ++i)
 *      {
 *          rmi.calc_max(i, a[i]);
 *      }
 *      std::cout << "max = " << rmi->get_value() <<
 *                   ", index = " << rmi->get_index() << std::endl;
 *
 *      ...
 *  }
 *
 *
 * Operations provided:
 * ====================
 * reducer_max and reducer_max_index provide set and get methods that are
 * guaranteed to be deterministic iff they are called prior to the first
 * spawn or after the last sync in a parallel algorithm.  When called during
 * execution, the value returned by get_value (and get_index) may differ from
 * run to run depending on how the routine or loop is scheduled.  Calling
 * set_value anywhere between the first spawn and the last sync may cause the
 * algorithm to produce non-deterministic results.
 *
 * get_value and get_index return imutable values.  The matching get_reference
 * and get_index_reference methods return modifiable references
 *
 * The calc_max method is a comparison operation that sets the reducer to the
 * larger of itself and the object being compared.  The max_of routines are
 * provided for convenience:
 *
 *  cilk::reducer_max<int> rm;
 *
 *  ...
 *
 *  rm.calc_max(55); // alternatively: rm = cilk::max_of(rm, 55);
 *
 *
 * Template parameter restrictions:
 * ================================
 * reducer_max and reducer_max_index require that the 'Type' template parameter 
 * be DefaultConstructible.  The 'Compare' template parameter must
 * implement a strict weak ordering if you want deterministic results.
 *
 * There are no requirements on the 'Index' template parameter of
 * reducer_max_index.  All comparisons will be done on the 'Type' value.
 *
 */

#ifndef REDUCER_MAX_H_INCLUDED
#define REDUCER_MAX_H_INCLUDED

#include <cilk/reducer.h>
#ifdef __cplusplus
# include <cstddef>
# include <functional>
#else
# include <stddef.h>
#endif

#ifdef __cplusplus

/* C++ Interface
 */

namespace cilk {

// Forward declaration
template <typename Type, typename Compare> class reducer_max;

namespace internal {
    // "PRIVATE" HELPER CLASS - uses the type system to make sure that
    // reducer_max instances aren't copied, but we can still allow statements
    // like *max = cilk::max_of(*max, a[i]);
    template <typename Type, typename Compare>
    class temp_max
    {
    private:
        reducer_max<Type,Compare>* m_reducerPtr;

        friend class reducer_max<Type,Compare>;

        // Copyable, not assignable
        temp_max& operator=(const temp_max &);

    public:
        explicit temp_max(reducer_max<Type,Compare> *reducerPtr);

        temp_max calc_max(const Type& x) const;
    };

    template <typename Type, typename Compare>
    inline
    temp_max<Type,Compare>
    max_of(const temp_max<Type,Compare>& tmp, const Type& x)
    {
        return tmp.calc_max(x);
    }

    template <typename Type, typename Compare>
    inline
    temp_max<Type,Compare>
    max_of(const Type& x, const temp_max<Type,Compare>& tmp)
    {
        return tmp.calc_max(x);
    }

} // end namespace internal

/**
 * @brief Class 'reducer_max' is a hyperobject representation of a value that
 * retains the maximum value of all of the values it sees during its lifetime.
 */
template <typename Type, typename Compare=std::less<Type> >
class reducer_max
{
public:
    /// Type of data in a reducer_max
    typedef Type basic_value_type;

public:
    /// Internal representation of the per-strand view of the data for
    /// reducer_max
    struct View
    {
        friend class reducer_max<Type,Compare>;
        friend class monoid_base<View>;

    public:
        /// Constructs a per-strand view instance, initializing it to the
        /// identity value.
        View();

        /// Constructs a per-strand view instance, initializing it to the
        /// specified value.
        View(const Type& v);

        /// Sets this view to the specified value.
        void set(const Type &v);

        /// Returns current value for this view
        const Type &get_value() const;

        /// Returns true if the value has ever been set
        bool is_set() const;

    private:
        Type m_value;
        bool m_isSet;
    };

public:
    /// Definition of data view, operation, and identity for reducer_max
    struct Monoid: monoid_base<View>
    {
        Compare m_comp;
        Monoid() : m_comp() {}
        Monoid(const Compare& comp) : m_comp(comp) {}
        void take_max(View *left, const Type &v) const;
        void reduce(View *left, View *right) const;
    };
private:
    // Hyperobject to serve up views
    reducer<Monoid> m_imp;

public:
    typedef internal::temp_max<Type,Compare> temp_max;

    friend class internal::temp_max<Type,Compare>;

public:
    /// Construct a 'reducer_max' object with a value of 'Type()'.
    reducer_max();

    /// Construct a 'reducer_max' object with the specified initial value.
    explicit reducer_max(const Type& initial_value);

    /// Construct a 'reducer_max' object with the specified initial value and
    /// comparator.
    reducer_max(const Type& initial_value, const Compare& comp);

    /// Return an immutable reference to the value of this object.
    ///
    /// @warning If this method is called before the parallel calculation is
    /// complete, the value returned by this method will be a partial result.
    const Type& get_value() const;

    /// Return a reference to the value of this object.
    ///
    /// @warning If this method is called before the parallel calculation is
    /// complete, the value returned by this method will be a partial result.
    Type&       get_reference();

    /// Return a reference to the value of this object.
    ///
    /// @warning If this method is called before the parallel calculation is
    /// complete, the value returned by this method will be a partial result.
    Type const& get_reference() const;

    /// Returns true if the value has ever been set
    bool is_set() const;

    /// Set the value of this object.
    ///
    /// @warning Setting the value of a reducer such that it violates the
    /// associative operation algebra will yield results that are likely to
    /// differ from serial execution and may differ from run to run.
    void set_value(const Type& value);

    /// Compare the current value with the one passed and retain the
    /// larger of the two.  Return this reducer.
    reducer_max& calc_max(const Type& value);

    /// Merge the result of a 'max' operation into this object.  The
    /// operation must involve this hyperobject, i.e., x = max_of(x, 5);
    reducer_max& operator=(const temp_max &temp);

    reducer_max&       operator*()       { return *this; }
    reducer_max const& operator*() const { return *this; }

    reducer_max*       operator->()       { return this; }
    reducer_max const* operator->() const { return this; }

private:
    // Not copyable
    reducer_max(const reducer_max&);
    reducer_max& operator=(const reducer_max&);
};

// Global "cilk::max_of" functions

using internal::max_of;

template <typename Type, typename Compare>
inline
internal::temp_max<Type,Compare>
max_of(reducer_max<Type,Compare>& r, const Type& x)
{
    return internal::temp_max<Type,Compare>(&r.calc_max(x));
}

template <typename Type, typename Compare>
inline
internal::temp_max<Type,Compare>
max_of(const Type& x, reducer_max<Type,Compare>& r)
{
    return internal::temp_max<Type,Compare>(&r.calc_max(x));
}

/////////////////////////////////////////////////////////////////////////////
// Implementation of inline and template functions
/////////////////////////////////////////////////////////////////////////////

// --------------------------------
// template class reducer_max::View
// --------------------------------

template<typename Type, typename Compare>
reducer_max<Type,Compare>::View::View()
    : m_value()
    , m_isSet(false)
{
}

template<typename Type, typename Compare>
reducer_max<Type,Compare>::View::View(const Type& v)
    : m_value(v)
    , m_isSet(true)
{
}

template<typename Type, typename Compare>
void reducer_max<Type,Compare>::View::set(const Type &v)
{
    m_value = v;
    m_isSet = true;
}

template<typename Type, typename Compare>
const Type &reducer_max<Type,Compare>::View::get_value() const
{
    return m_value;
}

template<typename Type, typename Compare>
bool reducer_max<Type,Compare>::View::is_set() const
{
    return m_isSet;
}

// -------------------------------------------
// template class reducer_max::Monoid
// -------------------------------------------

template<typename Type, typename Compare>
void
reducer_max<Type,Compare>::Monoid::take_max(View *left, const Type &v) const
{
    if (! left->m_isSet || m_comp(left->m_value,v))
    {
        left->m_value = v;
        left->m_isSet = true;
    }
}

template<typename Type, typename Compare>
void
reducer_max<Type,Compare>::Monoid::reduce(View *left, View *right) const
{
    if (right->m_isSet)
    {
        // Take the max of the two values
        take_max (left, right->m_value);
    }
}

// --------------------------------------------
// temp_max private helper class implementation
// --------------------------------------------

template <typename Type, typename Compare> inline
internal::temp_max<Type,Compare>::temp_max(
    reducer_max<Type,Compare> *reducerPtr)
    : m_reducerPtr(reducerPtr)
{
}

template <typename Type, typename Compare> inline
internal::temp_max<Type,Compare>
internal::temp_max<Type,Compare>::calc_max(const Type& x) const
{
    m_reducerPtr->calc_max(x);
    return *this;
}

// --------------------------
// template class reducer_max
// --------------------------

// Default constructor
template <typename Type, typename Compare>
inline
reducer_max<Type,Compare>::reducer_max()
    : m_imp()
{
}

template <typename Type, typename Compare>
inline
reducer_max<Type,Compare>::reducer_max(const Type& initial_value)
    : m_imp(initial_value)
{
}

template <typename Type, typename Compare>
inline
reducer_max<Type,Compare>::reducer_max(const Type& initial_value,
                                       const Compare& comp)
    : m_imp(Monoid(comp), initial_value)
{
}

template <typename Type, typename Compare>
inline
const Type& reducer_max<Type,Compare>::get_value() const
{
    const View &v = m_imp.view();

    return v.m_value;
}

template <typename Type, typename Compare>
inline
Type& reducer_max<Type,Compare>::get_reference()
{
    View &v = m_imp.view();

    return v.m_value;
}

template <typename Type, typename Compare>
inline
Type const& reducer_max<Type,Compare>::get_reference() const
{
    View &v = m_imp.view();

    return v.m_value;
}

template <typename Type, typename Compare>
inline
bool reducer_max<Type,Compare>::is_set() const
{
    const View &v = m_imp.view();

    return v.m_isSet;
}

template <typename Type, typename Compare>
inline
void reducer_max<Type,Compare>::set_value(const Type& value)
{
    View &v = m_imp.view();

    v.set(value);
}

template <typename Type, typename Compare> inline
reducer_max<Type,Compare>&
reducer_max<Type,Compare>::calc_max(const Type& value)
{
    View &v = m_imp.view();
    m_imp.monoid().take_max(&v, value);
    return *this;
}

template <typename Type, typename Compare>
reducer_max<Type,Compare>&
reducer_max<Type,Compare>::operator=(const temp_max& temp)
{
    // Noop.  Just test that temp is the same as this.
    __CILKRTS_ASSERT(this == temp.m_reducerPtr);
    return *this;
}

/*
 * @brief Class 'reducer_max_index' is a hyperobject representation of an
 * index and corresponding value representing the maximum such pair this
 * object has seen.
 */
template <typename Index, typename Value, typename Compare=std::less<Value> >
class reducer_max_index
{
public:
    /// Type of data in a reducer_max
    typedef Value basic_value_type;

public:
    /// Internal representation of the per-strand view of the data for
    /// reducer_max_index
    struct View
    {
        friend class reducer_max_index<Index, Value, Compare>;
        friend class monoid_base<View>;

    public:
        /// Constructs a per-strand view instance, initializing it to the
        /// identity value.
        View();

        /// Construct a per-strand view instance, initializing it to the
        /// specified value and index.
        View(const Index &i, const Value &v);

        /// Sets this view to a specified value and index
        void set(const Index &i, const Value &v);

        /// Returns current index for this view
        const Index &get_index() const;

        /// Returns current value for this view
        const Value &get_value() const;

        /// Returns true if the value has ever been set
        bool is_set() const;

    private:
        Index m_index;
        Value m_value;
        bool  m_isSet;
    };

public:
    /// Definition of data view, operation, and identity for reducer_max_index
    struct Monoid: monoid_base<View>
    {
        Compare m_comp;
        Monoid() : m_comp() {}
        Monoid(const Compare& comp) : m_comp(comp) {}
        void take_max(View *left, const Index &i, const Value &v) const;
        void reduce (View *left, View *right) const;
    };

private:
    // Hyperobject to serve up views
    reducer<Monoid> m_imp;

public:
    /// Construct a 'reducer_max_index' object with a value of 'Type()'.
    reducer_max_index();

    /// Construct a 'reducer_max_index' object with the specified initial
    /// value and index.
    reducer_max_index(const Index& initial_index,
                      const Value& initial_value);

    /// Construct a 'reducer_max_index' object with the specified initial
    /// value, index, and comparator.
    reducer_max_index(const Index& initial_index,
                      const Value& initial_value,
                      const Compare& comp);

    /// Return an immutable reference to the value of this object.
    ///
    /// @warning If this method is called before the parallel calculation is
    /// complete, the value returned by this method will be a partial result.
    const Value& get_value() const;

    /// Return a reference to the value of this object
    ///
    /// @warning If this method is called before the parallel calculation is
    /// complete, the value returned by this method will be a partial result.
    Value&       get_reference();

    /// Return a reference to the value of this object.
    ///
    /// @warning If this method is called before the parallel calculation is
    /// complete, the value returned by this method will be a partial result.
    Value const& get_reference() const;

    /// Return an immutable reference to the maximum index.
    ///
    /// @warning If this method is called before the parallel calculation is
    /// complete, the value returned by this method will be a partial result.
    const Index& get_index() const;

    /// Return a mutable reference to the maximum index
    ///
    /// @warning If this method is called before the parallel calculation is
    /// complete, the value returned by this method will be a partial result.
    Index& get_index_reference();

    /// Returns true if the value has ever been set
    bool is_set() const;

    /// Set the index/value of this object.
    ///
    /// @warning Setting the value of a reducer such that it violates the
    /// associative operation algebra will yield results that are likely to
    /// differ from serial execution and may differ from run to run.
    void set_value(const Index& index,
                   const Value& value);

    /// Compare the current value with the one passed and retain the
    /// larger of the two.  Return this reducer.
    reducer_max_index& calc_max(const Index& index, const Value& value);

    // DEPRECATED.  Use calc_max instead.
    void max_of(const Index& index, const Value& value) {calc_max(index,value);}

    reducer_max_index&       operator*()       { return *this; }
    reducer_max_index const& operator*() const { return *this; }

    reducer_max_index*       operator->()       { return this; }
    reducer_max_index const* operator->() const { return this; }

private:
    // Not copyable
    reducer_max_index(const reducer_max_index&);
    reducer_max_index& operator=(const reducer_max_index&);
};

/////////////////////////////////////////////////////////////////////////////
// Implementation of inline and template functions
/////////////////////////////////////////////////////////////////////////////

// --------------------------------
// template class reducer_max::View
// --------------------------------

template<typename Index, typename Value, typename Compare>
reducer_max_index<Index, Value, Compare>::View::View()
    : m_index()
    , m_value()
    , m_isSet(false)
{
}

template<typename Index, typename Value, typename Compare>
reducer_max_index<Index, Value, Compare>::View::View(const Index &i,
                                                     const Value &v)
    : m_index(i)
    , m_value(v)
    , m_isSet(true)
{
}

template<typename Index, typename Value, typename Compare>
void
reducer_max_index<Index, Value, Compare>::View::set(const Index &i,
                                                    const Value &v)
{
    m_index = i;
    m_value = v;
    m_isSet = true;
}

template<typename Index, typename Value, typename Compare>
const Index &
reducer_max_index<Index, Value, Compare>::View::get_index() const
{
    return m_index;
}

template<typename Index, typename Value, typename Compare>
const Value &
reducer_max_index<Index, Value, Compare>::View::get_value() const
{
    return m_value;
}

template<typename Index, typename Value, typename Compare>
bool
reducer_max_index<Index, Value, Compare>::View::is_set() const
{
    return m_isSet;
}

// -------------------------------------------
// template class reducer_max::Monoid
// -------------------------------------------

template<typename Index, typename Value, typename Compare>
void
reducer_max_index<Index,Value,Compare>::Monoid::take_max(View *left, 
                                                         const Index &i, 
                                                         const Value &v) const
{
    if (! left->m_isSet || m_comp(left->m_value,v))
    {
        left->m_index = i;
        left->m_value = v;
        left->m_isSet = true;
    }
}

template<typename Index, typename Value, typename Compare>
void
reducer_max_index<Index, Value, Compare>::Monoid::reduce(View *left,
                                                         View *right) const
{
    if (right->m_isSet)
        take_max (left, right->m_index, right->m_value);
}

// --------------------------------
// template class reducer_max_index
// --------------------------------

// Default constructor
template <typename Index, typename Value, typename Compare>
inline
reducer_max_index<Index, Value, Compare>::reducer_max_index()
    : m_imp()
{
}

template <typename Index, typename Value, typename Compare>
inline
reducer_max_index<Index, Value, Compare>::reducer_max_index(
    const Index& initial_index, const Value& initial_value)
    : m_imp(initial_index, initial_value)
{
}

template <typename Index, typename Value, typename Compare>
inline
reducer_max_index<Index, Value, Compare>::reducer_max_index(
    const Index& initial_index,
    const Value& initial_value,
    const Compare& comp)
    : m_imp(Monoid(comp), initial_index, initial_value)
{
}

template <typename Index, typename Value, typename Compare>
inline
reducer_max_index<Index, Value, Compare>&
reducer_max_index<Index, Value, Compare>::calc_max(const Index& index,
                                                   const Value& value)
{
    View &v = m_imp.view();
    m_imp.monoid().take_max(&v, index, value);
    return *this;
}

template <typename Index, typename Value, typename Compare>
inline
const Value& reducer_max_index<Index, Value, Compare>::get_value() const
{
    const View &v = m_imp.view();

    return v.m_value;
}

template <typename Index, typename Value, typename Compare>
inline
Value& reducer_max_index<Index, Value, Compare>::get_reference()
{
    View &v = m_imp.view();

    return v.m_value;
}

template <typename Index, typename Value, typename Compare>
inline
Value const& reducer_max_index<Index, Value, Compare>::get_reference() const
{
    const View &v = m_imp.view();

    return v.m_value;
}

template <typename Index, typename Value, typename Compare>
inline
const Index& reducer_max_index<Index, Value, Compare>::get_index() const
{
    const View &v = m_imp.view();

    return v.m_index;
}

template <typename Index, typename Value, typename Compare>
inline
Index& reducer_max_index<Index, Value, Compare>::get_index_reference()
{
    View &v = m_imp.view();

    return v.m_index;
}

template <typename Index, typename Value, typename Compare>
inline
bool reducer_max_index<Index, Value, Compare>::is_set() const
{
    const View &v = m_imp.view();

    return v.m_isSet;
}

template <typename Index, typename Value, typename Compare>
inline
void reducer_max_index<Index, Value, Compare>::set_value(const Index& index,
                                                         const Value& value)
{
    View &v = m_imp.view();

    return v.set(index, value);
}

} // namespace cilk

#endif // __cplusplus

/* C Interface
 */

__CILKRTS_BEGIN_EXTERN_C

/* REDUCER_MAX */

#define CILK_C_REDUCER_MAX_TYPE(tn)                                         \
    __CILKRTS_MKIDENT(cilk_c_reducer_max_,tn)
#define CILK_C_REDUCER_MAX(obj,tn,v)                                        \
    CILK_C_REDUCER_MAX_TYPE(tn) obj =                                       \
        CILK_C_INIT_REDUCER(_Typeof(obj.value),                             \
                        __CILKRTS_MKIDENT(cilk_c_reducer_max_reduce_,tn),   \
                        __CILKRTS_MKIDENT(cilk_c_reducer_max_identity_,tn), \
                        __cilkrts_hyperobject_noop_destroy, v)

/* Declare an instance of the reducer for a specific numeric type */
#define CILK_C_REDUCER_MAX_INSTANCE(t,tn)                                \
    typedef CILK_C_DECLARE_REDUCER(t)                                    \
        __CILKRTS_MKIDENT(cilk_c_reducer_max_,tn);                       \
    __CILKRTS_DECLARE_REDUCER_REDUCE(cilk_c_reducer_max,tn,l,r);         \
    __CILKRTS_DECLARE_REDUCER_IDENTITY(cilk_c_reducer_max,tn);  

/* CILK_C_REDUCER_MAX_CALC(reducer, v) performs the reducer lookup
 * AND calc_max operation, leaving the current view with the max of the
 * previous value and v.
 */
#define CILK_C_REDUCER_MAX_CALC(reducer, v) do {                            \
    _Typeof((reducer).value)* view = &(REDUCER_VIEW(reducer));              \
    _Typeof(v) __value = (v);                                               \
    if (*view < __value) {                                                  \
        *view = __value;                                                    \
    } } while (0)

/* Declare an instance of the reducer type for each numeric type */
CILK_C_REDUCER_MAX_INSTANCE(char,char);
CILK_C_REDUCER_MAX_INSTANCE(unsigned char,uchar);
CILK_C_REDUCER_MAX_INSTANCE(signed char,schar);
CILK_C_REDUCER_MAX_INSTANCE(wchar_t,wchar_t);
CILK_C_REDUCER_MAX_INSTANCE(short,short);
CILK_C_REDUCER_MAX_INSTANCE(unsigned short,ushort);
CILK_C_REDUCER_MAX_INSTANCE(int,int);
CILK_C_REDUCER_MAX_INSTANCE(unsigned int,uint);
CILK_C_REDUCER_MAX_INSTANCE(unsigned int,unsigned); /* alternate name */
CILK_C_REDUCER_MAX_INSTANCE(long,long);
CILK_C_REDUCER_MAX_INSTANCE(unsigned long,ulong);
CILK_C_REDUCER_MAX_INSTANCE(long long,longlong);
CILK_C_REDUCER_MAX_INSTANCE(unsigned long long,ulonglong);
CILK_C_REDUCER_MAX_INSTANCE(float,float);
CILK_C_REDUCER_MAX_INSTANCE(double,double);
CILK_C_REDUCER_MAX_INSTANCE(long double,longdouble);

/* Declare function bodies for the reducer for a specific numeric type */
#define CILK_C_REDUCER_MAX_IMP(t,tn,id)                                  \
    __CILKRTS_DECLARE_REDUCER_REDUCE(cilk_c_reducer_max,tn,l,r)          \
        { if (*(t*)l < *(t*)r) *(t*)l = *(t*)r; }                        \
    __CILKRTS_DECLARE_REDUCER_IDENTITY(cilk_c_reducer_max,tn)            \
        { *(t*)v = id; }

/* c_reducers.c contains definitions for all of the monoid functions
   for the C numeric types.  The contents of reducer_max.c are as follows:

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

*/

/* REDUCER_MAX_INDEX */

#define CILK_C_REDUCER_MAX_INDEX_VIEW(t,tn)                                  \
    typedef struct {                                                         \
        __STDNS ptrdiff_t index;                                             \
        t                 value;                                             \
    } __CILKRTS_MKIDENT(cilk_c_reducer_max_index_view_,tn)

#define CILK_C_REDUCER_MAX_INDEX_TYPE(t) \
    __CILKRTS_MKIDENT(cilk_c_reducer_max_index_,t)
#define CILK_C_REDUCER_MAX_INDEX(obj,t,v)                                    \
    CILK_C_REDUCER_MAX_INDEX_TYPE(t) obj =                                   \
    CILK_C_INIT_REDUCER(_Typeof(obj.value),                                  \
        __CILKRTS_MKIDENT(cilk_c_reducer_max_index_reduce_,t),               \
        __CILKRTS_MKIDENT(cilk_c_reducer_max_index_identity_,t),             \
        __cilkrts_hyperobject_noop_destroy, { 0, v })

/* Declare an instance of the reducer for a specific numeric type */
#define CILK_C_REDUCER_MAX_INDEX_INSTANCE(t,tn)                                  \
    CILK_C_REDUCER_MAX_INDEX_VIEW(t,tn);                                         \
    typedef CILK_C_DECLARE_REDUCER(                                              \
        __CILKRTS_MKIDENT(cilk_c_reducer_max_index_view_,tn))                    \
            __CILKRTS_MKIDENT(cilk_c_reducer_max_index_,tn);                     \
    __CILKRTS_DECLARE_REDUCER_REDUCE(cilk_c_reducer_max_index,tn,l,r);           \
    __CILKRTS_DECLARE_REDUCER_IDENTITY(cilk_c_reducer_max_index,tn);  

/* CILK_C_REDUCER_MAX_INDEX_CALC(reducer, i, v) performs the reducer lookup
 * AND calc_max operation, leaving the current view with the max of the
 * previous value and v.
 */
#define CILK_C_REDUCER_MAX_INDEX_CALC(reducer, i, v) do {                   \
    _Typeof((reducer).value)* view = &(REDUCER_VIEW(reducer));              \
    _Typeof(v) __value = (v);                                               \
    if (view->value < __value) {                                            \
        view->index = (i);                                                  \
        view->value = __value;                                              \
    } } while (0)

/* Declare an instance of the reducer type for each numeric type */
CILK_C_REDUCER_MAX_INDEX_INSTANCE(char,char);
CILK_C_REDUCER_MAX_INDEX_INSTANCE(unsigned char,uchar);
CILK_C_REDUCER_MAX_INDEX_INSTANCE(signed char,schar);
CILK_C_REDUCER_MAX_INDEX_INSTANCE(wchar_t,wchar_t);
CILK_C_REDUCER_MAX_INDEX_INSTANCE(short,short);
CILK_C_REDUCER_MAX_INDEX_INSTANCE(unsigned short,ushort);
CILK_C_REDUCER_MAX_INDEX_INSTANCE(int,int);
CILK_C_REDUCER_MAX_INDEX_INSTANCE(unsigned int,uint);
CILK_C_REDUCER_MAX_INDEX_INSTANCE(unsigned int,unsigned); /* alternate name */
CILK_C_REDUCER_MAX_INDEX_INSTANCE(long,long);
CILK_C_REDUCER_MAX_INDEX_INSTANCE(unsigned long,ulong);
CILK_C_REDUCER_MAX_INDEX_INSTANCE(long long,longlong);
CILK_C_REDUCER_MAX_INDEX_INSTANCE(unsigned long long,ulonglong);
CILK_C_REDUCER_MAX_INDEX_INSTANCE(float,float);
CILK_C_REDUCER_MAX_INDEX_INSTANCE(double,double);
CILK_C_REDUCER_MAX_INDEX_INSTANCE(long double,longdouble);

/* Declare function bodies for the reducer for a specific numeric type */
#define CILK_C_REDUCER_MAX_INDEX_IMP(t,tn,id)                                  \
    __CILKRTS_DECLARE_REDUCER_REDUCE(cilk_c_reducer_max_index,tn,l,r)          \
        { typedef __CILKRTS_MKIDENT(cilk_c_reducer_max_index_view_,tn) view_t; \
          if (((view_t*)l)->value < ((view_t*)r)->value)                       \
              *(view_t*)l = *(view_t*)r; }                                     \
    __CILKRTS_DECLARE_REDUCER_IDENTITY(cilk_c_reducer_max_index,tn)            \
        { typedef __CILKRTS_MKIDENT(cilk_c_reducer_max_index_view_,tn) view_t; \
          ((view_t*)v)->index = 0; ((view_t*)v)->value = id; }

/* c_reducers.c contains definitions for all of the monoid functions
   for the C numeric tyeps.  The contents of reducer_max_index.c are as follows:

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

*/


__CILKRTS_END_EXTERN_C

#endif // defined REDUCER_MAX_H_INCLUDED

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
 * reducer_min.h
 *
 * Purpose: Reducer hyperobject to retain the min value.
 *
 * Classes: reducer_min<Type, Compare=std::less<Type> >
 *          reducer_min_index<Index, Value, Compare=std::less<Type> >
 *
 * Description:
 * ============
 * This component provides reducer-type hyperobject representations that allow
 * the minimum value, or the minimum value and an index, of a group of values to
 * be determined in parallel.
 *
 * Usage Example:
 * ==============
 * Suppose we wish to compute the minimum value in an array of integers.
 *
 *  int test()
 *  {
 *      int a[ARRAY_SIZE];
 *      int min = INT_MIN;
 *
 *      ...
 *
 *      for (int i = 0; i < ARRAY_SIZE; ++i)
 *      {
 *          if (a[i] < min)
 *          {
 *              min = a[i];
 *          }
 *      }
 *      std::cout << "min = " << min << std::endl;
 *
 *      ...
 *  }
 *
 * Changing the 'for' to a 'cilk_for' will allow the loop to be run in parallel
 * but will create a data race on the variable 'min'.  The race can be resolved
 * by changing 'min' to a 'reducer_min' hyperobject:
 *
 *  int test()
 *  {
 *      int a[ARRAY_SIZE];
 *      cilk::reducer_min<int> min(INT_MIN);
 *
 *      ...
 *
 *      cilk_for (int i = 0; i < ARRAY_SIZE; ++i)
 *      {
 *          min->calc_min(a[i]);
 *      }
 *      std::cout << "min = " << min->get_value() << std::endl;
 *
 *      ...
 *  }
 *
 * A similar loop which calculates both the minimum value and index would be:
 *
 *  int test()
 *  {
 *      int a[ARRAY_SIZE];
 *      cilk::reducer_min_index<int, int> rmi(INT_MAX, -1);
 *
 *      ...
 *
 *      cilk_for (int i = 0; i < ARRAY_SIZE; ++i)
 *      {
 *          rmi->calc_min(i, a[i]);
 *      }
 *      std::cout << "min = " << rmi->get_value() <<
 *                   ", index = " << rmi->get_index() << std::endl;
 *
 *      ...
 *  }
 *
 *
 * Operations provided:
 * ====================
 * reducer_min and reducer_min_index provide set and get methods that are
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
 * The calc_min method is a comparison operation that sets the reducer to the
 * smaller of itself and the object being compared.  The min_of routines are
 * provided for convenience:
 *
 *  cilk::reducer_min<int> rm;
 *
 *  ...
 *
 *  rm.calc_min(55); // alternatively: rm = cilk::min_of(rm, 55);
 *
 *
 * Template parameter restrictions:
 * ================================
 * reducer_min and reducer_min_index require that the 'Type' template parameter 
 * be DefaultConstructible.  The 'Compare' template parameter must
 * implement a strict weak ordering if you want deterministic results.
 *
 * There are no requirements on the 'Index' template parameter of
 * reducer_min_index.  All comparisons will be done on the 'Type' value.
 *
 */

#ifndef REDUCER_MIN_H_INCLUDED
#define REDUCER_MIN_H_INCLUDED

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
template <typename Type, typename Compare> class reducer_min;

namespace internal {
    // "PRIVATE" HELPER CLASS - uses the type system to make sure that
    // reducer_max instances aren't copied, but we can still allow statements
    // like *min = cilk::min_of(*min, a[i]);
    template <typename Type, typename Compare>
    class temp_min
    {
    private:
        reducer_min<Type,Compare>* m_reducerPtr;

        friend class reducer_min<Type,Compare>;

        // Copyable, not assignable
        temp_min& operator=(const temp_min &);

    public:
        explicit temp_min(reducer_min<Type,Compare> *reducerPtr);

        temp_min calc_min(const Type& x) const;
    };

    template <typename Type, typename Compare>
    inline
    temp_min<Type,Compare>
    min_of(const internal::temp_min<Type,Compare>& tmp, const Type& x)
    {
        return tmp.calc_min(x);
    }

    template <typename Type, typename Compare>
    inline
    temp_min<Type,Compare>
    min_of(const Type& x, const internal::temp_min<Type,Compare>& tmp)
    {
        return tmp.calc_min(x);
    }

} // end namespace internal

/**
 * @brief Class 'reducer_min' is a hyperobject representation of a value that
 * retains the minimum value of all of the values it sees during its lifetime.
 */
template <typename Type, typename Compare=std::less<Type> >
class reducer_min
{
public:
    /// Type of data in a reducer_min
    typedef Type basic_value_type;

public:
    /// Internal representation of the per-strand view of the data for
    /// reducer_min
    struct View
    {
        friend class reducer_min<Type,Compare>;
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
    /// Definition of data view, operation, and identity for reducer_min
    struct Monoid: monoid_base<View>
    {
        Compare m_comp;
        Monoid() : m_comp() {}
        Monoid(const Compare& comp) : m_comp(comp) {}
        void take_min(View *left, const Type &v) const;
        void reduce(View *left, View *right) const;
    };
private:
    // Hyperobject to serve up views
    reducer<Monoid> m_imp;

public:
    typedef internal::temp_min<Type,Compare> temp_min;

    friend class internal::temp_min<Type,Compare>;

public:
    /// Construct a 'reducer_min' object with a value of 'Type()'.
    reducer_min();

    /// Construct a 'reducer_min' object with the specified initial value.
    explicit reducer_min(const Type& initial_value);

    /// Construct a 'reducer_min' object with the specified initial value and
    /// comparator.
    reducer_min(const Type& initial_value, const Compare& comp);

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
    /// smaller of the two.  Return this reducer.
    reducer_min& calc_min(const Type& value);

    /// Merge the result of a 'min' operation into this object.  The
    /// operation must involve this hyperobject, i.e., x = min_of(x, 5);
    reducer_min& operator=(const temp_min &temp);

    reducer_min&       operator*()       { return *this; }
    reducer_min const& operator*() const { return *this; }

    reducer_min*       operator->()       { return this; }
    reducer_min const* operator->() const { return this; }

private:
    // Not copyable
    reducer_min(const reducer_min&);
    reducer_min& operator=(const reducer_min&);
};

// Global "cilk::min_of" functions

using internal::min_of;

template <typename Type, typename Compare>
inline
internal::temp_min<Type,Compare>
min_of(reducer_min<Type,Compare>& r, const Type& x)
{
    return internal::temp_min<Type,Compare>(&r.calc_min(x));
}

template <typename Type, typename Compare>
inline
internal::temp_min<Type,Compare>
min_of(const Type& x, reducer_min<Type,Compare>& r)
{
    return internal::temp_min<Type,Compare>(&r.calc_min(x));
}

/////////////////////////////////////////////////////////////////////////////
// Implementation of inline and template functions
/////////////////////////////////////////////////////////////////////////////

// --------------------------------
// template class reducer_min::View
// --------------------------------

template<typename Type, typename Compare>
reducer_min<Type,Compare>::View::View()
    : m_value()
    , m_isSet(false)
{
}

template<typename Type, typename Compare>
reducer_min<Type,Compare>::View::View(const Type& v)
    : m_value(v)
    , m_isSet(true)
{
}

template<typename Type, typename Compare>
void reducer_min<Type,Compare>::View::set(const Type &v)
{
    m_value = v;
    m_isSet = true;
}

template<typename Type, typename Compare>
const Type &reducer_min<Type,Compare>::View::get_value() const
{
    return m_value;
}

template<typename Type, typename Compare>
bool reducer_min<Type,Compare>::View::is_set() const
{
    return m_isSet;
}

// -------------------------------------------
// template class reducer_min::Monoid
// -------------------------------------------

template<typename Type, typename Compare>
void
reducer_min<Type,Compare>::Monoid::take_min(View *left, const Type &v) const
{
    if (! left->m_isSet || m_comp(v,left->m_value))
    {
        left->m_value = v;
        left->m_isSet = true;
    }
}

template<typename Type, typename Compare>
void
reducer_min<Type,Compare>::Monoid::reduce(View *left, View *right) const
{
    if (right->m_isSet)
    {
        // Take the min of the two values
        take_min (left, right->m_value);
    }
}

// -----------------------------------
// temp_min private helper class implementation
// -----------------------------------

template <typename Type, typename Compare> inline
internal::temp_min<Type,Compare>::temp_min(
    reducer_min<Type,Compare> *reducerPtr) 
    : m_reducerPtr(reducerPtr)
{
}

template <typename Type, typename Compare> inline
internal::temp_min<Type,Compare>
internal::temp_min<Type,Compare>::calc_min(const Type& x) const
{
    m_reducerPtr->calc_min(x);
    return *this;
}

// --------------------------
// template class reducer_min
// --------------------------

// Default constructor
template <typename Type, typename Compare>
inline
reducer_min<Type,Compare>::reducer_min()
    : m_imp()
{
}

template <typename Type, typename Compare>
inline
reducer_min<Type,Compare>::reducer_min(const Type& initial_value)
    : m_imp(initial_value)
{
}

template <typename Type, typename Compare>
inline
reducer_min<Type,Compare>::reducer_min(const Type& initial_value,
                                       const Compare& comp)
    : m_imp(Monoid(comp), initial_value)
{
}

template <typename Type, typename Compare>
inline
const Type& reducer_min<Type,Compare>::get_value() const
{
    const View &v = m_imp.view();

    return v.m_value;
}

template <typename Type, typename Compare>
inline
Type& reducer_min<Type,Compare>::get_reference()
{
    View &v = m_imp.view();

    return v.m_value;
}

template <typename Type, typename Compare>
inline
Type const& reducer_min<Type,Compare>::get_reference() const
{
    View &v = m_imp.view();

    return v.m_value;
}

template <typename Type, typename Compare>
inline
bool reducer_min<Type,Compare>::is_set() const
{
    const View &v = m_imp.view();

    return v.m_isSet;
}

template <typename Type, typename Compare>
inline
void reducer_min<Type,Compare>::set_value(const Type& value)
{
    View &v = m_imp.view();

    v.set(value);
}

template <typename Type, typename Compare> inline
reducer_min<Type,Compare>&
reducer_min<Type,Compare>::calc_min(const Type& value)
{
    View &v = m_imp.view();

    // If no previous value has been set, always set the value

    m_imp.monoid().take_min(&v, value);

    return *this;
}

template <typename Type, typename Compare>
reducer_min<Type,Compare>&
reducer_min<Type,Compare>::operator=(const temp_min& temp)
{
    // Noop.  Just test that temp is the same as this.
    __CILKRTS_ASSERT(this == temp.m_reducerPtr);
    return *this;
}


/**
 * @brief Class 'reducer_min_index' is a hyperobject representation of an
 * index and corresponding value representing the minimum such pair this
 * object has seen.
 */
template <typename Index, typename Value, typename Compare=std::less<Value> >
class reducer_min_index
{
public:
    /// Type of data in a reducer_min
    typedef Value basic_value_type;

public:
    /// Internal representation of the per-strand view of the data for
    /// reducer_min_index
    struct View
    {
        friend class reducer_min_index<Index, Value, Compare>;
        friend class monoid_base<View>;

    public:
        /// Constructs a per-strand view instance, initializing it to the
        /// identity value.
        View();

        /// Constructs a per-strand view instance, initializing it to the
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
    /// Definition of data view, operation, and identity for reducer_min_index
    struct Monoid: monoid_base<View>
    {
        Compare m_comp;
        Monoid() : m_comp() {}
        Monoid(const Compare& comp) : m_comp(comp) {}
        void take_min(View *left, const Index &i, const Value &v) const;
        void reduce (View *left, View *right) const;
    };

private:
    // Hyperobject to serve up views
    reducer<Monoid> m_imp;

public:
    /// Construct a 'reducer_min_index' object with a value of 'Type()'.
    reducer_min_index();

    /// Construct a 'reducer_min_index' object with the specified initial
    /// value and index.
    reducer_min_index(const Index& initial_index,
                      const Value& initial_value);

    /// Construct a 'reducer_min_index' object with the specified initial
    /// value, index, and comparator.
    reducer_min_index(const Index& initial_index,
                      const Value& initial_value,
                      const Compare& comp);

    /// Return an imutable reference to the value of this object.
    const Value& get_value() const;

    /// Return a reference to the value of this object
    ///
    /// @warning If this method is called before the parallel calculation is
    /// complete, the value returned by this method will be a partial result.
    Value&       get_reference();

    /// Return a reference to the value of this object
    ///
    /// @warning If this method is called before the parallel calculation is
    /// complete, the value returned by this method will be a partial result.
    Value const& get_reference() const;

    /// Return an immutable reference to the minimum index.
    ///
    /// @warning If this method is called before the parallel calculation is
    /// complete, the value returned by this method will be a partial result.
    const Index& get_index() const;

    /// Return a mutable reference to the minimum index
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
    /// smaller of the two.
    void calc_min(const Index& index, const Value& value);

    // DEPRECATED.  Use calc_min instead.
    void min_of(const Index& index, const Value& value)
        { calc_min(index,value); }

    reducer_min_index&       operator*()       { return *this; }
    reducer_min_index const& operator*() const { return *this; }

    reducer_min_index*       operator->()       { return this; }
    reducer_min_index const* operator->() const { return this; }

private:
    // Not copyable
    reducer_min_index(const reducer_min_index&);
    reducer_min_index& operator=(const reducer_min_index&);
};

/////////////////////////////////////////////////////////////////////////////
// Implementation of inline and template functions
/////////////////////////////////////////////////////////////////////////////

// --------------------------------
// template class reducer_min::View
// --------------------------------

template<typename Index, typename Value, typename Compare>
reducer_min_index<Index, Value, Compare>::View::View()
    : m_index()
    , m_value()
    , m_isSet(false)
{
}

template<typename Index, typename Value, typename Compare>
reducer_min_index<Index, Value, Compare>::View::View(const Index &i,
                                                     const Value &v)
    : m_index(i)
    , m_value(v)
    , m_isSet(true)
{
}

template<typename Index, typename Value, typename Compare>
void
reducer_min_index<Index, Value, Compare>::View::set(const Index &i,
                                                    const Value &v)
{
    m_index = i;
    m_value = v;
    m_isSet = true;
}

template<typename Index, typename Value, typename Compare>
const Index &
reducer_min_index<Index, Value, Compare>::View::get_index() const
{
    return m_index;
}

template<typename Index, typename Value, typename Compare>
const Value &
reducer_min_index<Index, Value, Compare>::View::get_value() const
{
    return m_value;
}

template<typename Index, typename Value, typename Compare>
bool
reducer_min_index<Index, Value, Compare>::View::is_set() const
{
    return m_isSet;
}

// -------------------------------------------
// template class reducer_min::Monoid
// -------------------------------------------

template<typename Index, typename Value, typename Compare>
void
reducer_min_index<Index,Value,Compare>::Monoid::take_min(View *left, 
                                                         const Index &i, 
                                                         const Value &v) const
{
    if (! left->m_isSet || m_comp(v,left->m_value ))
    {
        left->m_index = i;
        left->m_value = v;
        left->m_isSet = true;
    }
}

template<typename Index, typename Value, typename Compare>
void
reducer_min_index<Index, Value, Compare>::Monoid::reduce(View *left,
                                                         View *right) const
{
    if (right->m_isSet)
        take_min (left, right->m_index, right->m_value);
}

// --------------------------------
// template class reducer_min_index
// --------------------------------

// Default constructor
template <typename Index, typename Value, typename Compare>
inline
reducer_min_index<Index, Value, Compare>::reducer_min_index()
    : m_imp()
{
}

template <typename Index, typename Value, typename Compare>
inline
reducer_min_index<Index, Value, Compare>::reducer_min_index(
    const Index& initial_index, const Value& initial_value)
    : m_imp(initial_index, initial_value)
{
}

template <typename Index, typename Value, typename Compare>
inline
reducer_min_index<Index, Value, Compare>::reducer_min_index(
    const Index& initial_index,
    const Value& initial_value,
    const Compare& comp)
    : m_imp(Monoid(comp), initial_index, initial_value)
{
}

template <typename Index, typename Value, typename Compare>
inline
void reducer_min_index<Index, Value, Compare>::calc_min(const Index& index,
                                                        const Value& value)
{
    View &v = m_imp.view();

    m_imp.monoid().take_min(&v, index, value);
}

template <typename Index, typename Value, typename Compare>
inline
const Value& reducer_min_index<Index, Value, Compare>::get_value() const
{
    const View &v = m_imp.view();

    return v.m_value;
}

template <typename Index, typename Value, typename Compare>
inline
Value& reducer_min_index<Index, Value, Compare>::get_reference()
{
    View &v = m_imp.view();

    return v.m_value;
}

template <typename Index, typename Value, typename Compare>
inline
Value const& reducer_min_index<Index, Value, Compare>::get_reference() const
{
    const View &v = m_imp.view();

    return v.m_value;
}

template <typename Index, typename Value, typename Compare>
inline
const Index& reducer_min_index<Index, Value, Compare>::get_index() const
{
    const View &v = m_imp.view();

    return v.m_index;
}

template <typename Index, typename Value, typename Compare>
inline
Index& reducer_min_index<Index, Value, Compare>::get_index_reference()
{
    View &v = m_imp.view();

    return v.m_index;
}

template <typename Index, typename Value, typename Compare>
inline
bool reducer_min_index<Index, Value, Compare>::is_set() const
{
    const View &v = m_imp.view();

    return v.m_isSet;
}

template <typename Index, typename Value, typename Compare>
inline
void reducer_min_index<Index, Value, Compare>::set_value(const Index& index,
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

/* REDUCER_MIN */

#define CILK_C_REDUCER_MIN_TYPE(tn)                                         \
    __CILKRTS_MKIDENT(cilk_c_reducer_min_,tn)
#define CILK_C_REDUCER_MIN(obj,tn,v)                                        \
    CILK_C_REDUCER_MIN_TYPE(tn) obj =                                       \
        CILK_C_INIT_REDUCER(_Typeof(obj.value),                             \
                        __CILKRTS_MKIDENT(cilk_c_reducer_min_reduce_,tn),   \
                        __CILKRTS_MKIDENT(cilk_c_reducer_min_identity_,tn), \
                        __cilkrts_hyperobject_noop_destroy, v)

/* Declare an instance of the reducer for a specific numeric type */
#define CILK_C_REDUCER_MIN_INSTANCE(t,tn)                                \
    typedef CILK_C_DECLARE_REDUCER(t)                                    \
        __CILKRTS_MKIDENT(cilk_c_reducer_min_,tn);                       \
    __CILKRTS_DECLARE_REDUCER_REDUCE(cilk_c_reducer_min,tn,l,r);         \
    __CILKRTS_DECLARE_REDUCER_IDENTITY(cilk_c_reducer_min,tn);  

/* CILK_C_REDUCER_MIN_CALC(reducer, v) performs the reducer lookup
 * AND calc_min operation, leaving the current view with the min of the
 * previous value and v.
 */
#define CILK_C_REDUCER_MIN_CALC(reducer, v) do {                            \
    _Typeof((reducer).value)* view = &(REDUCER_VIEW(reducer));              \
    _Typeof(v) __value = (v);                                               \
    if (*view > __value) {                                                  \
        *view = __value;                                                    \
    } } while (0)

/* Declare an instance of the reducer type for each numeric type */
CILK_C_REDUCER_MIN_INSTANCE(char,char);
CILK_C_REDUCER_MIN_INSTANCE(unsigned char,uchar);
CILK_C_REDUCER_MIN_INSTANCE(signed char,schar);
CILK_C_REDUCER_MIN_INSTANCE(wchar_t,wchar_t);
CILK_C_REDUCER_MIN_INSTANCE(short,short);
CILK_C_REDUCER_MIN_INSTANCE(unsigned short,ushort);
CILK_C_REDUCER_MIN_INSTANCE(int,int);
CILK_C_REDUCER_MIN_INSTANCE(unsigned int,uint);
CILK_C_REDUCER_MIN_INSTANCE(unsigned int,unsigned); /* alternate name */
CILK_C_REDUCER_MIN_INSTANCE(long,long);
CILK_C_REDUCER_MIN_INSTANCE(unsigned long,ulong);
CILK_C_REDUCER_MIN_INSTANCE(long long,longlong);
CILK_C_REDUCER_MIN_INSTANCE(unsigned long long,ulonglong);
CILK_C_REDUCER_MIN_INSTANCE(float,float);
CILK_C_REDUCER_MIN_INSTANCE(double,double);
CILK_C_REDUCER_MIN_INSTANCE(long double,longdouble);

/* Declare function bodies for the reducer for a specific numeric type */
#define CILK_C_REDUCER_MIN_IMP(t,tn,id)                                  \
    __CILKRTS_DECLARE_REDUCER_REDUCE(cilk_c_reducer_min,tn,l,r)          \
        { if (*(t*)l > *(t*)r) *(t*)l = *(t*)r; }                        \
    __CILKRTS_DECLARE_REDUCER_IDENTITY(cilk_c_reducer_min,tn)            \
        { *(t*)v = id; }

/* c_reducers.c contains definitions for all of the monoid functions
   for the C numeric tyeps.  The contents of reducer_min.c are as follows:

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

*/

/* REDUCER_MIN_INDEX */

#define CILK_C_REDUCER_MIN_INDEX_VIEW(t,tn)                                  \
    typedef struct {                                                         \
        __STDNS ptrdiff_t index;                                             \
        t                 value;                                             \
    } __CILKRTS_MKIDENT(cilk_c_reducer_min_index_view_,tn)

#define CILK_C_REDUCER_MIN_INDEX_TYPE(t) \
    __CILKRTS_MKIDENT(cilk_c_reducer_min_index_,t)
#define CILK_C_REDUCER_MIN_INDEX(obj,t,v)                                    \
    CILK_C_REDUCER_MIN_INDEX_TYPE(t) obj =                                   \
    CILK_C_INIT_REDUCER(_Typeof(obj.value),                                  \
        __CILKRTS_MKIDENT(cilk_c_reducer_min_index_reduce_,t),               \
        __CILKRTS_MKIDENT(cilk_c_reducer_min_index_identity_,t),             \
        __cilkrts_hyperobject_noop_destroy, { 0, v })

/* Declare an instance of the reducer for a specific numeric type */
#define CILK_C_REDUCER_MIN_INDEX_INSTANCE(t,tn)                                  \
    CILK_C_REDUCER_MIN_INDEX_VIEW(t,tn);                                         \
    typedef CILK_C_DECLARE_REDUCER(                                              \
        __CILKRTS_MKIDENT(cilk_c_reducer_min_index_view_,tn))                    \
            __CILKRTS_MKIDENT(cilk_c_reducer_min_index_,tn);                     \
    __CILKRTS_DECLARE_REDUCER_REDUCE(cilk_c_reducer_min_index,tn,l,r);           \
    __CILKRTS_DECLARE_REDUCER_IDENTITY(cilk_c_reducer_min_index,tn);  

/* CILK_C_REDUCER_MIN_INDEX_CALC(reducer, i, v) performs the reducer lookup
 * AND calc_min operation, leaving the current view with the min of the
 * previous value and v.
 */
#define CILK_C_REDUCER_MIN_INDEX_CALC(reducer, i, v) do {                   \
    _Typeof((reducer).value)* view = &(REDUCER_VIEW(reducer));              \
    _Typeof(v) __value = (v);                                               \
    if (view->value > __value) {                                            \
        view->index = (i);                                                  \
        view->value = __value;                                              \
    } } while (0)

/* Declare an instance of the reducer type for each numeric type */
CILK_C_REDUCER_MIN_INDEX_INSTANCE(char,char);
CILK_C_REDUCER_MIN_INDEX_INSTANCE(unsigned char,uchar);
CILK_C_REDUCER_MIN_INDEX_INSTANCE(signed char,schar);
CILK_C_REDUCER_MIN_INDEX_INSTANCE(wchar_t,wchar_t);
CILK_C_REDUCER_MIN_INDEX_INSTANCE(short,short);
CILK_C_REDUCER_MIN_INDEX_INSTANCE(unsigned short,ushort);
CILK_C_REDUCER_MIN_INDEX_INSTANCE(int,int);
CILK_C_REDUCER_MIN_INDEX_INSTANCE(unsigned int,uint);
CILK_C_REDUCER_MIN_INDEX_INSTANCE(unsigned int,unsigned); /* alternate name */
CILK_C_REDUCER_MIN_INDEX_INSTANCE(long,long);
CILK_C_REDUCER_MIN_INDEX_INSTANCE(unsigned long,ulong);
CILK_C_REDUCER_MIN_INDEX_INSTANCE(long long,longlong);
CILK_C_REDUCER_MIN_INDEX_INSTANCE(unsigned long long,ulonglong);
CILK_C_REDUCER_MIN_INDEX_INSTANCE(float,float);
CILK_C_REDUCER_MIN_INDEX_INSTANCE(double,double);
CILK_C_REDUCER_MIN_INDEX_INSTANCE(long double,longdouble);

/* Declare function bodies for the reducer for a specific numeric type */
#define CILK_C_REDUCER_MIN_INDEX_IMP(t,tn,id)                                  \
    __CILKRTS_DECLARE_REDUCER_REDUCE(cilk_c_reducer_min_index,tn,l,r)          \
        { typedef __CILKRTS_MKIDENT(cilk_c_reducer_min_index_view_,tn) view_t; \
          if (((view_t*)l)->value > ((view_t*)r)->value)                       \
              *(view_t*)l = *(view_t*)r; }                                     \
    __CILKRTS_DECLARE_REDUCER_IDENTITY(cilk_c_reducer_min_index,tn)            \
        { typedef __CILKRTS_MKIDENT(cilk_c_reducer_min_index_view_,tn) view_t; \
          ((view_t*)v)->index = 0; ((view_t*)v)->value = id; }

/* c_reducers.c contains definitions for all of the monoid functions
   for the C numeric tyeps.  The contents of reducer_min_index.c are as follows:

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

*/


__CILKRTS_END_EXTERN_C

#endif // defined REDUCER_MIN_H_INCLUDED

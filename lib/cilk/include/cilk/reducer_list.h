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
 * reducer_list.h
 *
 * Purpose: Reducer hyperobject to accumulate a list of elements.
 *
 * Classes: reducer_list_append<Type, Allocator>
 *          reducer_list_prepend<Type, Allocator>
 *
 * Description:
 * ============
 * This component provides reducer-type hyperobject representations that allow
 * either prepending or appending values to an STL list.  By replacing the
 * variable with the hyperobject defined in this component, the data race is
 * eliminated.
 *
 * Usage Example:
 * ==============
 * Assume we wish to traverse an array of objects, performing an operation on
 * each object and accumulating the result of the operation into an STL list
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
 *      std::list<int> result;
 *      for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
 *      {
 *          result.push_back(compute(myArray[i]));
 *      }
 *
 *      std::cout << "The result is: ";
 *      for (std::list<int>::iterator i = result.begin(); i != result.end();
 *           ++i)
 *      {
 *          std::cout << *i << " " << std::endl;
 *      }
 *
 *      return 0;
 *  }
 *..
 * Changing the 'for' to a 'cilk_for' will cause the loop to run in parallel,
 * but doing so will create a data race on the 'result' list.
 * The race is solved by changing 'result' to a 'reducer_list_append'
 * hyperobject:
 *..
 *  int compute(const X& v);
 *
 *  int test()
 *  {
 *      const std::size_t ARRAY_SIZE = 1000000;
 *      extern X myArray[ARRAY_SIZE];
 *      // ...
 *
 *      cilk::reducer_list_append<int> result;
 *      cilk_for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
 *      {
 *          result->push_back(compute(myArray[i]));
 *      }
 *
 *      std::cout << "The result is: ";
 *      const std::list &r = result->get_value();
 *      for (std::list<int>::const_iterator i = r.begin(); i != r.end(); ++i)
 *      {
 *          std::cout << *i << " " << std::endl;
 *      }
 *
 *      return 0;
 *  }
 *..
 *
 * Operations provided:
 * ====================
 *
 * 'reducer_list_prepend' and 'reducer_list_append' support accumulation of an
 * ordered list of items.  Lists accumulated in Cilk++ strands will be merged
 * to maintain the order of the lists - the order will be the same as if the
 * application was run on a single core.
 *
 * The the current value of the reducer can be gotten and set using the
 * 'get_value', 'get_reference', and 'set_value' methods.  As with most
 * reducers, these methods produce deterministic results only if called before
 * the first spawn after creating a 'hyperobject' or when all strands spawned
 * since creating the 'hyperobject' have been synced.
 */

#ifndef REDUCER_LIST_H_INCLUDED
#define REDUCER_LIST_H_INCLUDED

#include <cilk/reducer.h>
#include <list>

namespace cilk
{

/**
 * @brief Reducer hyperobject to accumulate a list of elements where elements
 * are added to the end of the list.
 */
template<class _Ty,
         class _Ax = std::allocator<_Ty> >
class reducer_list_append
{
public:
    /// std::list reducer_list_prepend is based on
    typedef std::list<_Ty, _Ax> list_type;
    /// Type of elements in a reducer_list_prepend
    typedef _Ty list_value_type;
    /// Type of elements in a reducer_list_prepend
    typedef _Ty basic_value_type;

public:
    /// Definition of data view, operation, and identity for reducer_list_append
    struct Monoid: monoid_base<std::list<_Ty, _Ax> >
    {
        static void reduce (std::list<_Ty, _Ax> *left,
                            std::list<_Ty, _Ax> *right);
    };

private:
    reducer<Monoid> imp_;

public:

    // Default Constructor - Construct a reducer with an empty list
    reducer_list_append();

    // Construct a reducer with an initial list
    reducer_list_append(const std::list<_Ty, _Ax> &initial_value);

    // Return a const reference to the current list
    const std::list<_Ty, _Ax> &get_value() const;

    // Return a reference to the current list
    std::list<_Ty, _Ax> &get_reference();
    std::list<_Ty, _Ax> const &get_reference() const;

    // Replace the list's contents with the given list
    void set_value(const list_type &value);

    // Add an element to the end of the list
    void push_back(const _Ty element);

    reducer_list_append&       operator*()       { return *this; }
    reducer_list_append const& operator*() const { return *this; }

    reducer_list_append*       operator->()       { return this; }
    reducer_list_append const* operator->() const { return this; }

private:
    // Not copyable
    reducer_list_append(const reducer_list_append&);
    reducer_list_append& operator=(const reducer_list_append&);

};  // class reducer_list_append

/////////////////////////////////////////////////////////////////////////////
// Implementation of inline and template functions
/////////////////////////////////////////////////////////////////////////////

// ------------------------------------------
// template class reducer_list_append::Monoid
// ------------------------------------------

/**
 * Appends list from "right" reducer_list onto the end of the "left".
 * When done, the "right" reducer_list is empty.
 *
 * @tparam _Ty - Type of the list elements
 * @tparam _Ax - Allocator object used to define the storage allocation
 * model.  If not specified, the allocator class template for _Ty is used.
 * @param left reducer_list to be reduced into
 * @param right reducer_list to be reduced from
 */
template<class _Ty, class _Ax>
void
reducer_list_append<_Ty, _Ax>::Monoid::reduce(std::list<_Ty, _Ax> *left,
                                              std::list<_Ty, _Ax> *right)
{
    left->splice(left->end(), *right);
}

/**
 * Default constructor - creates an empty list
 *
 * @tparam _Ty - Type of the list elements
 * @tparam _Ax - Allocator object used to define the storage allocation
 * model.  If not specified, the allocator class template for _Ty is used.
 */
template<class _Ty, class _Ax>
reducer_list_append<_Ty, _Ax>::reducer_list_append() :
    imp_()
{
}

/**
 * Construct a reducer_list_append based on a list
 *
 * @tparam _Ty - Type of the list elements
 * @tparam _Ax - Allocator object used to define the storage allocation
 * model.  If not specified, the allocator class template for _Ty is used.
 * @param initial_value - [in] Inital list
 */
template<class _Ty, class _Ax>
reducer_list_append<_Ty, _Ax>::reducer_list_append(const std::list<_Ty, _Ax> &initial_value) :
    imp_(std::list<_Ty, _Ax>(initial_value))
{
}

/**
 * Allows read-only access to the list - same as get_reference()
 *
 * @warning If this method is called before the parallel calculation is
 * complete, the list returned by this method will be a partial result.
 *
 * @tparam _Ty - Type of the list elements
 * @tparam _Ax - Allocator object used to define the storage allocation
 * model.  If not specified, the allocator class template for _Ty is used.
 * @returns A const reference to the list that is the current contents of this view.
 */
template<class _Ty, class _Ax>
const std::list<_Ty, _Ax> &reducer_list_append<_Ty, _Ax>::get_value() const
{
    return imp_.view();
}

/**
 * Allows mutable access to list
 *
 * @warning If this method is called before the parallel calculation is
 * complete, the list returned by this method will be a partial result.
 *
 * @tparam _Ty - Type of the list elements
 * @tparam _Ax - Allocator object used to define the storage allocation
 * model.  If not specified, the allocator class template for _Ty is used.
 * @returns A reference to the list that is the current contents of this view.
 */
template<class _Ty, class _Ax>
std::list<_Ty, _Ax> &reducer_list_append<_Ty, _Ax>::get_reference()
{
    return imp_.view();
}

/**
 * Allows read-only access to list
 *
 * @warning If this method is called before the parallel calculation is
 * complete, the list returned by this method will be a partial result.
 *
 * @tparam _Ty - Type of the list elements
 * @tparam _Ax - Allocator object used to define the storage allocation
 * model.  If not specified, the allocator class template for _Ty is used.
 * @returns A const reference to the list that is the current contents of this view
 */
template<class _Ty, class _Ax>
const std::list<_Ty, _Ax> &reducer_list_append<_Ty, _Ax>::get_reference() const
{
    return imp_.view();
}

/**
 * Replace the list's contents
 *
 * @tparam _Ty - Type of the list elements
 * @tparam _Ax - Allocator object used to define the storage allocation
 * model.  If not specified, the allocator class template for _Ty is used.
 * @param value - The list to replace the current contents of this view
 */
template<class _Ty, class _Ax>
void reducer_list_append<_Ty, _Ax>::set_value(const list_type &value)
{
    // Clean out any value in our list
    imp_.view().clear();

    // If the new list is empty, we're done
    if (value.empty())
        return;

    // Copy each element into our list
    imp_.view() = value;
}

/**
 * Adds an element to the end of the list
 *
 * @tparam _Ty - Type of the list elements
 * @tparam _Ax - Allocator object used to define the storage allocation
 * model.  If not specified, the allocator class template for _Ty is used.
 * @param element - The element to be added to the end of the list
 */
template<class _Ty, class _Ax>
void reducer_list_append<_Ty, _Ax>::push_back(const _Ty element)
{
    imp_.view().push_back(element);
}

/**
 * @brief Reducer hyperobject to accumulate a list of elements where elements are
 * added to the beginning of the list.
 */
template<class _Ty,
         class _Ax = std::allocator<_Ty> >
class reducer_list_prepend
{
public:
    /// std::list reducer_list_prepend is based on
    typedef std::list<_Ty, _Ax> list_type;
    /// Type of elements in a reducer_list_prepend
    typedef _Ty list_value_type;
    /// Type of elements in a reducer_list_prepend
    typedef _Ty basic_value_type;

public:
    /// @brief Definition of data view, operation, and identity for reducer_list_prepend
    struct Monoid: monoid_base<std::list<_Ty, _Ax> >
    {
        static void reduce (std::list<_Ty, _Ax> *left,
                            std::list<_Ty, _Ax> *right);
    };

private:
    reducer<Monoid> imp_;

public:

    // Default Constructor - Construct a reducer with an empty list
    reducer_list_prepend();

    // Construct a reducer with an initial list
    reducer_list_prepend(const std::list<_Ty, _Ax> &initial_value);

    // Return a const reference to the current list
    const std::list<_Ty, _Ax> &get_value() const;

    // Return a reference to the current list
    std::list<_Ty, _Ax> &get_reference();
    std::list<_Ty, _Ax> const &get_reference() const;

    // Replace the list's contents with the given list
    void set_value(const list_type &value);

    // Add an element to the beginning of the list
    void push_front(const _Ty element);

    reducer_list_prepend&       operator*()       { return *this; }
    reducer_list_prepend const& operator*() const { return *this; }

    reducer_list_prepend*       operator->()       { return this; }
    reducer_list_prepend const* operator->() const { return this; }

private:
    // Not copyable
    reducer_list_prepend(const reducer_list_prepend&);
    reducer_list_prepend& operator=(const reducer_list_prepend&);

};  // class reducer_list_prepend

/////////////////////////////////////////////////////////////////////////////
// Implementation of inline and template functions
/////////////////////////////////////////////////////////////////////////////

// ------------------------------------
// template class reducer_list_prepend::Monoid
// ------------------------------------

/**
 * Appends list from "right" reducer_list onto the end of the "left".
 * When done, the "right" reducer_list is empty.
 */
template<class _Ty, class _Ax>
void
reducer_list_prepend<_Ty, _Ax>::Monoid::reduce(std::list<_Ty, _Ax> *left,
                                               std::list<_Ty, _Ax> *right)
{
    left->splice(left->begin(), *right);
}

/**
 * Default constructor - creates an empty list
 */
template<class _Ty, class _Ax>
reducer_list_prepend<_Ty, _Ax>::reducer_list_prepend() :
    imp_(std::list<_Ty, _Ax>())
{
}

/**
 * Construct a reducer_list_prepend based on a list.
 *
 * @param initial_value List used to initialize the reducer_list_prepend
 */
template<class _Ty, class _Ax>
reducer_list_prepend<_Ty, _Ax>::reducer_list_prepend(const std::list<_Ty, _Ax> &initial_value) :
    imp_(std::list<_Ty, _Ax>(initial_value))
{
}

/**
 * Allows read-only access to the list - same as get_reference()
 *
 * @warning If this method is called before the parallel calculation is
 * complete, the list returned by this method will be a partial result.
 *
 * @tparam _Ty - Type of the list elements
 * @tparam _Ax - Allocator object used to define the storage allocation
 * model.  If not specified, the allocator class template for _Ty is used.
 * @returns A const reference to the list that is the current contents of this view.
 */
template<class _Ty, class _Ax>
const std::list<_Ty, _Ax> &reducer_list_prepend<_Ty, _Ax>::get_value() const
{
    return imp_.view();
}

/**
 * Allows mutable access to the list
 *
 * @warning If this method is called before the parallel calculation is
 * complete, the list returned by this method will be a partial result.
 *
 * @tparam _Ty - Type of the list elements
 * @tparam _Ax - Allocator object used to define the storage allocation
 * model.  If not specified, the allocator class template for _Ty is used.
 * @returns A mutable reference to the list that is the current contents of this view.
 */
template<class _Ty, class _Ax>
std::list<_Ty, _Ax> &reducer_list_prepend<_Ty, _Ax>::get_reference()
{
    return imp_.view();
}

/**
 * Allows read-only access to the list
 *
 * @warning If this method is called before the parallel calculation is
 * complete, the list returned by this method will be a partial result.
 *
 * @tparam _Ty - Type of the list elements
 * @tparam _Ax - Allocator object used to define the storage allocation
 * model.  If not specified, the allocator class template for _Ty is used.
 * @returns A read-only reference to the list that is the current contents of this view.
 */
template<class _Ty, class _Ax>
const std::list<_Ty, _Ax> &reducer_list_prepend<_Ty, _Ax>::get_reference() const
{
    return imp_.view();
}

/**
 * Replace the list's contents
 *
 * @tparam _Ty - Type of the list elements
 * @tparam _Ax - Allocator object used to define the storage allocation
 * model.  If not specified, the allocator class template for _Ty is used.
 * @param value - The list to replace the current contents of this view
 */
template<class _Ty, class _Ax>
void reducer_list_prepend<_Ty, _Ax>::set_value(const list_type &value)
{
    // Clean out any value in our list
    imp_.view().clear();

    // If the new list is empty, we're done
    if (value.empty())
        return;

    // Copy each element into our list
    imp_.view() = value;
}

/**
 * Add an element to the beginning of the list
 *
 * @tparam _Ty - Type of the list elements
 * @tparam _Ax - Allocator object used to define the storage allocation
 * model.  If not specified, the allocator class template for _Ty is used.
 * @param element Element to be added to the beginning of the list
 */
template<class _Ty, class _Ax>
void reducer_list_prepend<_Ty, _Ax>::push_front(const _Ty element)
{
    imp_.view().push_front(element);
}

}	// namespace cilk

#endif //  REDUCER_LIST_H_INCLUDED

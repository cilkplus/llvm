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
 * reducer_string.h
 *
 * Purpose: Reducer hyperobject to accumulate a string.
 *
 * Classes: reducer_basic_string<Elem, Traits, Alloc>
 *          reducer_string - convenience name for a string-of-char reducer
 *          reducer_wstring - convenience name for a string-of-wchar_t reducer
 *
 * Description:
 * ============
 * This component provides a reducer-type hyperobject representation that
 * allows appending characters to an STL string.  By replacing the variable
 * with the hyperobject defined in this component, the data race is eliminated.
 *
 * reducer_basic_string is actually implemented using a list to avoid memory
 * fragmentation issues as text is appended to the string.  The string
 * components are assembled into a single string before being returned by
 * get_value().
 *
 * Usage Example:
 * ==============
 * Assume we wish to traverse an array of objects, performing an operation on
 * each object and accumulating the result of the operation into an STL string
 * variable.
 *..
 *  char *compute(const X& v);
 *
 *  int test()
 *  {
 *      const std::size_t ARRAY_SIZE = 1000000;
 *      extern X myArray[ARRAY_SIZE];
 *      // ...
 *
 *      std::string result;
 *      for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
 *      {
 *          result += compute(myArray[i]);
 *      }
 *
 *      std::cout << "The result is: " << result.c_str() << std::endl;
 
 *      return 0;
 *  }
 *..
 * Changing the 'for' to a 'cilk_for' will cause the loop to run in parallel,
 * but doing so will create a data race on the 'result' variable.
 * The race is solved by changing 'result' to a 'reducer_string' hyperobject:
 *..
 *  char *compute(const X& v);
 *
 *  int test()
 *  {
 *      const std::size_t ARRAY_SIZE = 1000000;
 *      extern X myArray[ARRAY_SIZE];
 *      // ...
 *
 *      cilk::reducer_string result;
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
 *
 * 'reducer_string' supports operator+= and append.
 *
 * The the current value of the reducer can be retrieved using the 'get_value'
 * method. As with most reducers, the 'get_value' method produces deterministic
 * results only if called before the first spawn after creating a 'hyperobject'
 * or when all strands spawned since creating the 'hyperobject' have been
 * synced.
 */

#ifndef REDUCER_STRING_H_INCLUDED
#define REDUCER_STRING_H_INCLUDED

#include <cilk/reducer.h>
#include <string>
#include <list>

namespace cilk
{

/**
 * @brief Reducer hyperobject representation of a string.
 *
 * Typedefs for 8-bit character strings (reducer_string) and 16-bit character
 * strings (reducer_wstring) are provided at the end of the file.
 */
template<class _Elem,
         class _Traits = std::char_traits<_Elem>,
         class _Alloc = std::allocator<_Elem> >
class reducer_basic_string
{
public:
    /// Type of the basic_string reducer_basic_string is based on
    typedef std::basic_string<_Elem, _Traits, _Alloc> string_type;

    /// Type of sizes
    typedef typename string_type::size_type size_type;

    /// Character type for reducer_basic_string
    typedef _Elem basic_value_type;

    /// Internal representation of the per-strand view of the data for reducer_basic_string
    struct View
    {
        friend class reducer_basic_string<_Elem, _Traits, _Alloc>;

        /// Type of the basic_string the View is based on
        typedef std::basic_string<_Elem, _Traits, _Alloc> string_type;

        /// Type of sizes
        typedef typename string_type::size_type size_type;

        std::basic_string<_Elem, _Traits, _Alloc> &get_value();

        /// Add a character to the View
        void add_char(_Elem ch) { m_value += ch; }

    private:
        string_type             m_value;   // Holds current string
        std::list<string_type>  m_list;    // List used to accumulate string fragments
    };

public:
    /// Definition of data view, operation, and identity for reducer_basic_string
    struct Monoid: monoid_base< View >
    {
        static void reduce (View *left, View *right);
    };

private:
    // Hyperobject to serve up views
    reducer<Monoid> imp_;

public:

    // Default constructor - Construct an empty reducer_basic_string
    reducer_basic_string();

    // Construct a reducer_basic_string with an initial value
    reducer_basic_string(const _Elem *ptr);
    reducer_basic_string(const _Elem *ptr, const _Alloc &al);
    reducer_basic_string(const _Elem *ptr, size_type count);
    reducer_basic_string(const _Elem *ptr, size_type count, const _Alloc &al);
    reducer_basic_string(const string_type &right, size_type offset, size_type count);
    reducer_basic_string(const string_type &right, size_type offset, size_type count, const _Alloc &al);
    reducer_basic_string(size_type count, _Elem ch);
    reducer_basic_string(size_type count, _Elem ch, const _Alloc &al);

    // Return an immutable reference to the current string
    const string_type &get_value() const;

    // Return a reference to the current string
    string_type&       get_reference();
    string_type const& get_reference() const;

    // Set the string to a specified value
    void set_value(const string_type &value);

    // Append to the string
    void append(const _Elem *ptr);
    void append(const _Elem *ptr, size_type count);
    void append(const string_type &str, size_type offset, size_type count);
    void append(const string_type &str);
    void append(size_type count, _Elem ch);

    // Append to the string
    reducer_basic_string<_Elem, _Traits, _Alloc> &operator+=(_Elem ch);
    reducer_basic_string<_Elem, _Traits, _Alloc> &operator+=(const _Elem *ptr);
    reducer_basic_string<_Elem, _Traits, _Alloc> &operator+=(const string_type &right);

    reducer_basic_string&       operator*()       { return *this; }
    reducer_basic_string const& operator*() const { return *this; }

    reducer_basic_string*       operator->()       { return this; }
    reducer_basic_string const* operator->() const { return this; }

};  // class reducer_basic_string

/////////////////////////////////////////////////////////////////////////////
// Implementation of inline and template functions
/////////////////////////////////////////////////////////////////////////////

// -----------------------------------------
// template class reducer_basic_string::View
// -----------------------------------------

/**
 * Assemble the string from the collected fragments
 *
 * @returns std::basic_string reference to the assembled string
 */
template<class _Elem, class _Traits, class _Alloc>
std::basic_string<_Elem, _Traits, _Alloc> &
reducer_basic_string<_Elem, _Traits, _Alloc>::View::get_value()
{
    // If the list is empty, just return our string
    if (m_list.empty())
        return m_value;

    // First calculate the total length of all of the string fragments
    size_type len = m_value.size();
    typename std::list<string_type>::iterator i;
    for (i = m_list.begin(); i != m_list.end(); ++i)
        len += i->size();

    // Hold onto the string, since it needs to go at the end
    string_type tmp;
    tmp.swap(m_value);

    // Expand the string that to hold all of the string fragments.
    // Allocating it up-front prevents heap fragmentation.
    m_value.reserve(len);

    // Concatenate all of the fragments into the string, then clear out the
    // list
    for (i = m_list.begin(); i != m_list.end(); ++i)
        m_value += *i;
    m_list.clear();

    // Finally, add the string value we saved
    m_value += tmp;
    return m_value;
}

// -------------------------------------------
// template class reducer_basic_string::Monoid
// -------------------------------------------

/**
 * Appends string from "right" reducer_basic_string onto the end of
 * the "left". When done, the "right" reducer_basic_string is empty.
 */
template<class _Elem, class _Traits, class _Alloc>
void
reducer_basic_string<_Elem, _Traits, _Alloc>::Monoid::reduce(View *left,
                                                             View *right)
{
    // Check if there's anything to do
    if (right->m_list.empty() && right->m_value.empty())
        return;

    // If the only thing is the right string, just take it
    if (left->m_list.empty() && right->m_list.empty() & left->m_value.empty())
    {
        left->m_value.swap(right->m_value);
        return;
    }

    // Debugging aid - should be removed before ship!
#ifdef DEBUG_STRING_REDUCER
    std::cout << "Complex merge" << std::endl;
    dump ("Left");
    right->dump("Right");
#endif

    // OK, merge everything together.  If there's anything in our string, it's
    // got to be added to the list first
    if (! left->m_value.empty())
    {
        left->m_list.push_back(left->m_value);
        left->m_value.clear();
    }

    // Now splice the two lists together, then take the right string
    left->m_list.splice(left->m_list.end(), right->m_list);
    left->m_value.swap(right->m_value);

    // Debugging aid - should be removed before ship!
#ifdef DEBUG_STRING_REDUCER
    dump ("Result");
#endif
}

// -----------------------------------
// template class reducer_basic_string
// -----------------------------------

/**
 * Default constructor - doesn't do much
 */
template<class _Elem, class _Traits, class _Alloc>
reducer_basic_string<_Elem, _Traits, _Alloc>::reducer_basic_string():
    imp_()
{
}

/**
 * Construct a reducer_basic_string initializing it from a null-terminated
 * string using the default allocator.
 *
 * @param ptr Null-terminated string to initialize from
 */
template<class _Elem, class _Traits, class _Alloc>
reducer_basic_string<_Elem, _Traits, _Alloc>::reducer_basic_string(const _Elem *ptr) :
    imp_()
{
    string_type str(ptr);

    View &v = imp_.view();
    v.m_value = str;
}

/**
 * Construct a reducer_basic_string initializing it from a null-terminated
 * string specifying an allocator.
 *
 * @param ptr Null-terminated string to initialize from
 * @param al Allocator to be used
 */
template<class _Elem, class _Traits, class _Alloc>
reducer_basic_string<_Elem, _Traits, _Alloc>::reducer_basic_string(const _Elem *ptr,
                                                                   const _Alloc &al) :
    imp_()
{
    string_type str(ptr, al);

    View &v = imp_.view();
    v.m_value = str;
}

/**
 * Construct a reducer_basic_string initializing it from a null-terminated
 * string, copying N characters, using the default allocator.
 *
 * @param ptr Null-terminated string to initialize from
 * @param count Number of characters to copy
 */
template<class _Elem, class _Traits, class _Alloc>
reducer_basic_string<_Elem, _Traits, _Alloc>::reducer_basic_string(const _Elem *ptr,
                                                                   size_type count) :
    imp_()
{
    string_type str(ptr, count);

    View &v = imp_.view();
    v.m_value = str;
}

/**
 * Construct a reducer_basic_string initializing it from a null-terminated
 * string, copying N characters, specifying an allocator.
 *
 * @param ptr Null-terminated string to initialize from
 * @param count Number of characters to copy
 * @param al Allocator to be used
 */
template<class _Elem, class _Traits, class _Alloc>
reducer_basic_string<_Elem, _Traits, _Alloc>::reducer_basic_string(const _Elem *ptr,
                                                                   size_type count,
                                                                   const _Alloc &al) :
    imp_()
{
    string_type str(ptr, count, al);

    View &v = imp_.view();
    v.m_value = str;
}

/**
 * Construct a reducer_basic_string initializing it from a string_type
 * string starting from an offset, copying N characters, using the default
 * allocator.
 *
 * @param right string_type string to initialize from
 * @param offset Character withing right to start copying from
 * @param count Number of characters to copy
 */
template<class _Elem, class _Traits, class _Alloc>
reducer_basic_string<_Elem, _Traits, _Alloc>::reducer_basic_string(const string_type &right,
                                                                   size_type offset,
                                                                   size_type count) :
    imp_()
{
    string_type str(right, offset, count);

    View &v = imp_.view();
    v.m_value = str;
}

/**
 * Construct a reducer_basic_string initializing it from a string_type
 * string starting from an offset, copying N characters, uspecifying an
 * allocator.
 *
 * @param right string_type string to initialize from
 * @param offset Character withing right to start copying from
 * @param count Number of characters to copy
 * @param al Allocator to be used
 */
template<class _Elem, class _Traits, class _Alloc>
reducer_basic_string<_Elem, _Traits, _Alloc>::reducer_basic_string(const string_type &right,
                                                                   size_type offset,
                                                                   size_type count,
                                                                   const _Alloc &al) :
    imp_()
{
    string_type str(right, offset, count, al);

    View &v = imp_.view();
    v.m_value = str;
}

/**
 * Construct a reducer_basic_string initializing it with a character repeated
 * some number of times, using the default allocator.
 *
 * @param count Number of times to repeat the character
 * @param ch Character to initialize reducer_basic_string with
 */
template<class _Elem, class _Traits, class _Alloc>
reducer_basic_string<_Elem, _Traits, _Alloc>::reducer_basic_string(size_type count,
                                                                   _Elem ch) :
    imp_()
{
    string_type str(count, ch);

    View &v = imp_.view();
    v.m_value = str;
}

/**
 * Construct a reducer_basic_string initializing it with a character repeated
 * some number of times, specifying an allocator.
 *
 * @param count Number of times to repeat the character
 * @param ch Character to initialize reducer_basic_string with
 * @param al Allocator to be used
 */
template<class _Elem, class _Traits, class _Alloc>
reducer_basic_string<_Elem, _Traits, _Alloc>::reducer_basic_string(size_type count,
                                                                   _Elem ch,
                                                                   const _Alloc &al) :
    imp_()
{
    string_type str(count, ch, al);

    View &v = imp_.view();
    v.m_value = str;
}

/**
 * Assemble the string from the collected fragments and return a mutable
 * reference to it
 *
 * @returns std::basic_string reference
 */
template<class _Elem, class _Traits, class _Alloc>
std::basic_string<_Elem, _Traits, _Alloc> &
reducer_basic_string<_Elem, _Traits, _Alloc>::get_reference()
{
    View &v = imp_.view();

    return v.get_value();
}

/**
 * Assemble the string from the collected fragments and return an immutable
 * reference to it
 *
 * @returns std::basic_string reference
 */
template<class _Elem, class _Traits, class _Alloc>
const std::basic_string<_Elem, _Traits, _Alloc> &
reducer_basic_string<_Elem, _Traits, _Alloc>::get_reference() const
{
    // Cast away the const-ness and call mutable get_reference to do the work
    reducer_basic_string *pThis = const_cast<reducer_basic_string *>(this);
    return pThis->get_reference();
}

/**
 * Assemble the string from the collected fragments and return an immutable
 * reference to it
 *
 * @returns string_type reference
 */
template<class _Elem, class _Traits, class _Alloc>
inline
const std::basic_string<_Elem, _Traits, _Alloc> &
reducer_basic_string<_Elem, _Traits, _Alloc>::get_value() const
{
    // Delegate to get_reference()
    return this->get_reference();
}

/**
 * Set the string to a specified value
 *
 * @param value string_type to set the reducer_basic_string to
 */
template<class _Elem, class _Traits, class _Alloc>
void reducer_basic_string<_Elem, _Traits, _Alloc>::set_value(const string_type &value)
{
    View &v = imp_.view();

    v.m_list.clear();
    v.m_value.assign(value);
}

/**
 * Add a null-terminated string to the string
 *
 * @param ptr Null-terminated string to be appended
 */
template<class _Elem, class _Traits, class _Alloc>
void reducer_basic_string<_Elem, _Traits, _Alloc>::append(const _Elem *ptr)
{
    View &v = imp_.view();

    v.m_value.append(ptr);
}

/**
 * Add a string_type string to the string
 *
 * @param str string_type to be appended
 */
template<class _Elem, class _Traits, class _Alloc>
void reducer_basic_string<_Elem, _Traits, _Alloc>::append(const string_type &str)
{
    View &v = imp_.view();

    v.m_value.append(str);
}

/**
 * Add a null-terminated string to the string, specifying the maximum number
 * of characters to copy
 *
 * @param ptr Null-terminated string to be appended
 * @param count Maximum number of characters to copy
 */
template<class _Elem, class _Traits, class _Alloc>
void reducer_basic_string<_Elem, _Traits, _Alloc>::append(const _Elem *ptr,
                                                          size_type count)
{
    View &v = imp_.view();

    v.m_value.append(ptr, count);
}

/**
 * Add a string_type string to the string, specifying the starting offset and
 * maximum number of characters to copy
 *
 * @param str Null-terminated string to be appended
 * @param offset Offset in the string_type to start copy at
 * @param count Maximum number of characters to copy
 */
template<class _Elem, class _Traits, class _Alloc>
void reducer_basic_string<_Elem, _Traits, _Alloc>::append(const string_type &str,
                                                          size_type offset,
                                                          size_type count)
{
    View &v = imp_.view();

    v.m_value.append(str, offset, count);
}

/**
 * Add one or more repeated characters to the string
 *
 * @param count Number of times to repeat the character
 * @param ch Character to be added one or more times to the string
 */
// append - add one or more repeated characters to the list
template<class _Elem, class _Traits, class _Alloc>
void reducer_basic_string<_Elem, _Traits, _Alloc>::append(size_type count,
                                                          _Elem ch)
{
    View &v = imp_.view();

    v.m_value.append(count, ch);
}

/**
 * append a single character to the string
 *
 * @param ch Character to be appended
 */
template<class _Elem, class _Traits, class _Alloc>
reducer_basic_string<_Elem, _Traits, _Alloc> &
reducer_basic_string<_Elem, _Traits, _Alloc>::operator+=(_Elem ch)
{
    View &v = imp_.view();

    v.m_value.append(1, ch);
    return *this;
}

/**
 * append a null-terminated string to the string
 *
 * @param ptr Null-terminated string to be appended
 */
template<class _Elem, class _Traits, class _Alloc>
reducer_basic_string<_Elem, _Traits, _Alloc> &
reducer_basic_string<_Elem, _Traits, _Alloc>::operator+=(const _Elem *ptr)
{
    View &v = imp_.view();

    v.m_value.append(ptr);
    return *this;
}

/**
 * append a string-type to the string
 *
 * @param right string-type to be appended
 */
template<class _Elem, class _Traits, class _Alloc>
reducer_basic_string<_Elem, _Traits, _Alloc> &
reducer_basic_string<_Elem, _Traits, _Alloc>::operator+=(const string_type &right)
{
    View &v = imp_.view();

    v.m_value.append(right);
    return *this;
}

/**
 * Convenience typedefs for 8-bit strings
 */
typedef reducer_basic_string<char,
                             std::char_traits<char>,
                             std::allocator<char> >
    reducer_string;

/**
 * Convenience typedefs for 16-bit strings
 */
typedef reducer_basic_string<wchar_t,
                             std::char_traits<wchar_t>,
                             std::allocator<wchar_t> >
    reducer_wstring;

}   // namespace cilk

#endif //  REDUCER_STRING_H_INCLUDED

/** @file reducer_list.h
 *
 *  @brief Defines classes for doing parallel list creation by appending or prepending.
 *
 *  @copyright
 *  Copyright (C) 2012, Intel Corporation
 *  All rights reserved.
 *  
 *  @copyright
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in
 *      the documentation and/or other materials provided with the
 *      distribution.
 *    * Neither the name of Intel Corporation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *  
 *  @copyright
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 *  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 *  AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
 *  WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 *  @ingroup reducers
 *
 *  @see @ref page_reducer_list
 */

#ifndef REDUCER_LIST_H_INCLUDED
#define REDUCER_LIST_H_INCLUDED

#include <cilk/reducer.h>
#include <list>

/** @page page_reducer_list List Reducers
 *
 *  @tableofcontents
 *
 *  Header file reducer_list.h defines the monoid and view classes for creating Cilk reducers
 *  to concatenate a set of lists (or create a list containing a set of values) in parallel.
 *
 *  You should be familiar with @ref pagereducers "Cilk reducers", described in file
 *  reducers.md, and particularly with @ref reducers_using, before trying to use the
 *  information in this file.
 *
 *  @section redlist_usage Usage Example
 *
 *      // Create a list containing the labels of the nodes of a tree in “inorder”
 *      // (left subtree, root, right subtree).
 *
 *      struct Tree { Tree* left; Tree* right; string label; ... };
 *
 *      list<string> x;
 *      cilk::reducer< cilk::op_list_append<string> > xr(cilk::move_in(x));
 *      collect_labels(tree, xr);
 *      xr.move_out(x);
 *      
 *      void collect_labels(Tree* node, cilk::reducer< cilk::op_list_append<string> >& xr)
 *      {
 *          if (node) {
 *              cilk_spawn collect_labels(node->left, xr);
 *              xr->push_back(node->label);
 *              collect_labels(node->right, xr);
 *              cilk_sync;
 *          }
 *      }
 *
 *  @section redlist_classes Classes Defined
 *
 *  @subsection redlist_classes_append Append
 *  *   @ref cilk::op_list_append<Type, Allocator = std::allocator<Type>> (monoid)
 *  *   @ref cilk::op_list_append_view<Type, Allocator = std::allocator<Type>> (view)
 *  *   @ref cilk::reducer< cilk::op_list_append<Type, Allocator = std::allocator<Type>> > (reducer) (defined in
 *      reducer.h)
 *  *   @ref cilk::reducer_list_append<Type, Allocator = std::allocator<Type>> (deprecated reducer)
 *  @subsection redlist_classes_prepend Prepend
 *  *   @ref cilk::op_list_prepend<Type, Allocator = std::allocator<Type>> (monoid)
 *  *   @ref cilk::op_list_prepend_view<Type, Allocator = std::allocator<Type>> (view)
 *  *   @ref cilk::reducer< cilk::op_list_prepend<Type, Allocator = std::allocator<Type>> > (reducer) (defined in
 *      reducer.h)
 *  *   @ref cilk::reducer_list_prepend<Type, Allocator = std::allocator<Type>> (deprecated reducer)
 *
 *  @section redlist_monoid The Monoid
 *
 *  @subsection redlist_monoid_values Value Set
 *
 *  The value set of a list reducer is the set of values of the class 
 *  `std::list<Type, Allocator>`, which we refer to as “the reducer’s list type”.
 *
 *  @subsection redlist_monoid_operator Operator
 *
 *  The operator of a list append reducer is defined as
 *
 *      x CAT y == (every element of x, followed by every element of y)
 *
 *  The operator of a list prepend reducer is defined as
 *
 *      x RCAT y == (every element of y, followed by every element of x)
 *
 *  @subsection redlist_monoid_identity Identity
 *
 *  The identity value of a list reducer is the empty list, which is the value of the expression
 *  `std::list<Type, Allocator>([allocator])`.
 *
 *  @section redlist_operations Operations
 *
 *  In the operation descriptions below, the type name `List` refers to the reducer’s string
 *  type, `std::list<Type, Allocator>`.
 *
 *  @subsection redlist_constructors Constructors
 *
 *  Any argument list which is valid for a `std::list` constructor is valid for a list reducer
 *  constructor. The usual move-in constructor is also provided:
 *
 *      reducer(move_in(List& variable))
 *
 *  A list reducer with no constructor arguments, or with only an allocator argument, will
 *  initially contain the identity value, an empty list. 
 *
 *  @subsection redlist_get_set Set and Get
 *
 *      r.set_value(const List& value)
 *      const List& = r.get_value() const
 *      List& = r.get_value()
 *      r.move_in(List& variable)
 *      r.move_out(List& variable)
 *
 *  @subsection redlist_view_ops View Operations
 *
 *  The view of a list append reducer provides the following member functions:
 *
 *      void push_back(const Type& element) 
 *      void insert_back(List::size_type n, const Type& element) 
 *      template <typename Iter> void insert_back(Iter first, Iter last)
 *      void splice_back(List& x)
 *      void splice_back(List& x, List::iterator i)
 *      void splice_back(List& x, List::iterator first, List::iterator last)
 *  
 *  The view of a list prepend reducer provides the following member functions:
 *
 *      void push_front(const Type& element) 
 *      void insert_front(List::size_type n, const Type& element) 
 *      template <typename Iter> void insert_front(Iter first, Iter last)
 *      void splice_front(List& x)
 *      void splice_front(List& x, List::iterator i)
 *      void splice_front(List& x, List::iterator first, List::iterator last)
 *
 *  The `push_back` and `push_front` functions are the same as the corresponding `std::list`
 *  functions. The `insert_back`, `splice_back`, `insert_front`, and `splice_front` functions
 *  are the same as the `std::list` `insert` and `splice` functions, with the first parameter
 *  fixed to the end or beginning of the list, respectively.
 *
 *  @section redlist_performance Performance Considerations
 *
 *  An efficient reducer requires that combining the values of two views (using the view
 *  `reduce()` function) be a constant-time operations. Two lists can be merged in constant time 
 *  using the `splice()` function if they have the same allocator. Therefore, the lists for new
 *  views are created (by the view identity constructor) using the same allocator as the list
 *  that was created when the reducer was constructed.
 *
 *  The performance of adding elements to a list reducer depends on the view operations that
 *  are used:
 *
 *  *   The `push` functions add a single element to the list, and therefore take
 *      constant time.
 *  *   An `insert` function that inserts _N_ elements adds each of them individually, and
 *      therefore takes _O(N)_ time. 
 *  *   A `splice` function that inserts _N_ elements just adjusts a couple of pointers, and
 *      therefore takes constant time, _if the splice is from a list with the same allocator as
 *      the reducer_. Otherwise, it is equivalent to an `insert`, and takes _O(N)_ time.
 *
 *  This means that for best performance, if you will be adding elements to a list reducer in
 *  batches, you should `splice` them from a list having the same allocator as the reducer.
 *
 *  The reducer `move_in` and `move_out` functions do a constant-time `swap` if the variable
 *  has the same allocator as the reducer, and a linear-time copy otherwise.
 *  
 *  Note that the allocator of a list reducer is determined when the reducer is constructed.
 *  The following two examples may have very different behavior:
 *
 *      list<Element, Allocator> a_list;
 *
 *      reducer< list_append<Element, Allocator> reducer1(move_in(a_list));
 *      ... parallel computation ...
 *      reducer1.move_out(a_list);
 *
 *      reducer< list_append<Element, Allocator> reducer2;
 *      reducer2.move_in(a_list);
 *      ... parallel computation ...
 *      reducer2.move_out(a_list);
 *
 *  *   `reducer1` will be constructed with the same allocator as `a_list`, because the list was
 *      was specified in the constructor. The `move_in` and`move_out` can therefore be done with
 *      a `swap` in constant time.
 *  *   `reducer2` will be constructed with a _default_ allocator, “`Allocator()`”, which may
 *      may not be the same as the allocator of `a_list`. Therefore, the `move_in` and
 *      `move_out` may have to be done with a copy in _O(N)_ time.
 *  
 *  (All instances of an allocator type with no internal state (like `std::allocator`) are “the
 *  same”. You only need to worry about the “same allocator” issue when you create list reducers
 *  with custom allocator types.)
 *
 *  @section redlist_types Type and Operator Requirements
 *
 *  `std::list<Type, Allocator>` must be a valid type.
 */


namespace cilk {

namespace internal {

/** Base class for list append and prepend view classes.
 *
 *  @note   This class provides the definitions that are required for a class that will be
 *          used as the parameter of a @ref list_monoid_base specialization. 
 *
 *  @tparam Type        The type of the list *elements* (not of the list).
 *  @tparam Allocator   The list's allocator class.
 */
template <typename Type, typename Allocator>
class list_view_base
{
protected:
    typedef std::list<Type, Allocator>  list_type;  ///< The type of the contained list
    list_type m_value;                              ///< The list accumulator variable

public:

    /** @name Monoid support.
     */
    //@{
    
    typedef list_type   value_type;             ///< Required by @ref monoid_with_view

    Allocator get_allocator() const             ///< Required by @ref list_monoid_base
    { 
        return m_value.get_allocator(); 
    }
    
    //@}
    
    /** @name Standard list constructors.
     */
    //@{
    
    explicit list_view_base(const Allocator& a = Allocator()) : m_value(a) {}
    explicit list_view_base(
        typename list_type::size_type n, 
        const Type& value = Type(), 
        const Allocator& a = Allocator() ) : m_value(n, value, a) {}
    template <typename Iter> 
    list_view_base(Iter first, Iter last, const Allocator& a = Allocator()) : 
        m_value(first, last, a) {}
    list_view_base(const list_type& list) : m_value(list) {}

    //@}
    
    /** Move-in constructor.
     */
    explicit list_view_base(move_in_wrapper<value_type> w)
        : m_value(w.value().get_allocator())
    {
        m_value.swap(w.value());
    }
    
    /** @name @ref reducer support.
     */
    //@{
    
    void view_move_in(value_type& v)
    {
        if (m_value.get_allocator() == v.get_allocator())
            // Equal allocators. Do a (fast) swap.
            m_value.swap(v);
        else
            // Unequal allocators. Do a (slow) copy.
            m_value = v;
        v.clear();
    }
    
    void view_move_out(value_type& v)
    {
        if (m_value.get_allocator() == v.get_allocator())
            // Equal allocators.  Do a (fast) swap.
            m_value.swap(v);
        else
            // Unequal allocators.  Do a (slow) copy.
            v = m_value;
        m_value.clear();
    }
    
    void view_set_value(const value_type& v) { m_value = v; }

    value_type const& view_get_value()     const { return m_value; }
    value_type      & view_get_reference()       { return m_value; }
    value_type const& view_get_reference() const { return m_value; }
    
    //@}
};


/** Base class for list append and prepend monoid classes.
 *
 *  The key to efficient reducers is that the `identity` operation, which creates a new
 *  per-strand view, and the `reduce` operation, which combines two per-strand views, must be
 *  constant-time operations. Two lists can be concatenated in constant time only if they have
 *  the same allocator. Therefore, all the per-strand list accumulator variables must be created
 *  with the same allocator as the leftmost view list. 
 *
 *  This means that a list reduction monoid must have a copy of the allocator of the leftmost
 *  view’s list, so that it can use it in the `identity` operation. This, in turn, requires that
 *  list reduction monoids have a specialized `construct()` function, which constructs the
 *  leftmost view before the monoid, and then passes the leftmost view’s allocator to the monoid
 *  constructor.
 *
 *  @tparam View    The list append or prepend view class.
 */
template <typename View, bool Align>
class list_monoid_base : public monoid_with_view<View, Align>
{
    typedef typename View::value_type           list_type;
    typedef typename list_type::allocator_type  allocator_type;
    allocator_type                              m_allocator;
    
    using monoid_base<list_type, View>::provisional;
    
public:

    /** Constructor.
     *
     *  There is no default constructor for list monoids, because the allocator must always
     *  be specified.
     *
     *  @param  allocator   The list allocator to be used when identity-constructing new views.
     */
    list_monoid_base(const allocator_type& allocator = allocator_type()) : m_allocator(allocator) {}

    /** Create an identity view.
     *
     *  List view identity constructors take the list allocator as an argument.
     *
     *  @param v    The address of the uninitialized memory in which the view will be
     *  constructed.
     */
    void identity(View *v) const { ::new((void*) v) View(m_allocator); }
    
    /** @name construct functions
     *
     *  All list reducer monoid `construct()` functions first construct the leftmost view, 
     *  using the arguments that were passed in from the reducer constructor. They then
     *  call the view’s `get_allocator()` function to get the list allocator from the list
     *  in the leftmost view, and pass that to the monoid constructor.
     */
    //@{
    
    template <typename Monoid>
    static void construct(Monoid* monoid, View* view)
        { provisional( new ((void*)view) View() ).confirm_if( 
            new ((void*)monoid) Monoid(view->get_allocator()) ); }

    template <typename Monoid, typename T1>
    static void construct(Monoid* monoid, View* view, const T1& x1)
        { provisional( new ((void*)view) View(x1) ).confirm_if( 
            new ((void*)monoid) Monoid(view->get_allocator()) ); }

    template <typename Monoid, typename T1, typename T2>
    static void construct(Monoid* monoid, View* view, const T1& x1, const T2& x2)
        { provisional( new ((void*)view) View(x1, x2) ).confirm_if( 
            new ((void*)monoid) Monoid(view->get_allocator()) ); }

    template <typename Monoid, typename T1, typename T2, typename T3>
    static void construct(Monoid* monoid, View* view, const T1& x1, const T2& x2, 
                            const T3& x3)
        { provisional( new ((void*)view) View(x1, x2, x3) ).confirm_if( 
            new ((void*)monoid) Monoid(view->get_allocator()) ); }

    //@}
};

} // namespace internal


/** The list append reducer view class.
 *
 *  This is the view class for reducers created with 
 *  `cilk::reducer< cilk::op_list_append<Type, Allocator> >`. It holds the accumulator variable
 *  for the reduction, and allows only append operations to be performed on it.
 *
 *  @note   The reducer “dereference” operation (`reducer::operator *()`) yields a reference 
 *          to the view. Thus, for example, the view class’s `push_back` operation would be
 *          used in an expression like `r->push_back(a)`, where `r` is a list append reducer
 *          variable.
 *
 *  @tparam Type        The list element type (not the list type).
 *  @tparam Allocator   The list allocator type.
 *
 *  @see @ref page_reducer_list
 *  @see op_list_append
 */
template <class Type, class Allocator = typename std::list<Type>::allocator_type>
class op_list_append_view : public internal::list_view_base<Type, Allocator>
{
    typedef internal::list_view_base<Type, Allocator>   base;
    typedef std::list<Type, Allocator>                  list_type;
    typedef typename list_type::iterator                iterator;
    
    iterator end() { return this->m_value.end(); }

public:

    /** @name Constructors.
     *
     *  All op_list_append_view constructors simply pass their arguments on to the 
     *  @ref internal::list_view_base base class.
     *
     *  @ref internal::list_view_base supports all the std::list constructor forms, as well as
     *  the reducer move_in constructor form.
     *
     */
    //@{
    
    op_list_append_view() : base() {}
    
    template <typename T1>
    op_list_append_view(const T1& x1) : base(x1) {}
    
    template <typename T1, typename T2>
    op_list_append_view(const T1& x1, const T2& x2) : base(x1, x2) {}
    
    template <typename T1, typename T2, typename T3>
    op_list_append_view(const T1& x1, const T2& x2, const T3& x3) : base(x1, x2, x3) {}

    //@}    

    /** @name View modifier operations.
     */
    //@{
    
    /** Add an element at the end of the list.
     *
     *  Equivalent to `list.push_back(…)`
     */
    void push_back(const Type& element) 
        { this->m_value.push_back(element); }

    /** @name Insert elements at the end of the list.
     *
     *  Equivalent to `list.insert(list.end(), …)`
     */
    //@{

    void insert_back(typename list_type::size_type n, const Type& element) 
        { this->m_value.insert(end(), n, element); }

    template <typename Iter>
    void insert_back(Iter first, Iter last)
        { this->m_value.insert(end(), first, last); }

    //@}
    
    /** @name Splice elements at the end of the list.
     *
     *  Equivalent to `list.splice(list.end(), …)`
     */
    //@{
    
    void splice_back(list_type& x) {
        if (x.get_allocator() == this->m_value.get_allocator())
            this->m_value.splice(end(), x);
        else {
            insert_back(x.begin(), x.end());
            x.clear();
        }
    }

    void splice_back(list_type& x, iterator i) {
        if (x.get_allocator() == this->m_value.get_allocator())
            this->m_value.splice(end(), x, i);
        else {
            push_back(*i);
            x.erase(i);
        }
    }

    void splice_back(list_type& x, iterator first, iterator last) {
        if (x.get_allocator() == this->m_value.get_allocator())
            this->m_value.splice(end(), x, first, last);
        else {
            insert_back(first, last);
            x.erase(first, last);
        }
    }
    
    //@}

    /** Reduce operation.
     *
     *  Required by @ref monoid_base.
     */
    void reduce(op_list_append_view* other)
    {
        __CILKRTS_ASSERT(this->m_value.get_allocator() == other->m_value.get_allocator());
        this->m_value.splice(end(), other->m_value);
    }
};


/** The list prepend reducer view class.
 *
 *  This is the view class for reducers created with 
 *  `cilk::reducer< cilk::op_list_prepend<Type, Allocator> >`. It holds the accumulator variable
 *  for the reduction, and allows only prepend operations to be performed on it.
 *
 *  @note   The reducer “dereference” operation (`reducer::operator *()`) yields a reference 
 *          to the view. Thus, for example, the view class’s `push_front` operation would be
 *          used in an expression like `r->push_front(a)`, where `r` is a list prepend reducer
 *          variable.
 *
 *  @tparam Type        The list element type (not the list type).
 *  @tparam Allocator   The list allocator type.
 *
 *  @see @ref page_reducer_list
 *  @see op_list_prepend
 */
template <class Type, class Allocator = typename std::list<Type>::allocator_type>
class op_list_prepend_view : public internal::list_view_base<Type, Allocator>
{
    typedef internal::list_view_base<Type, Allocator>   base;
    typedef std::list<Type, Allocator>                  list_type;
    typedef typename list_type::iterator                iterator;
    
    iterator begin() { return this->m_value.begin(); }

public:

    /** @name Constructors.
     *
     *  All op_list_prepend_view constructors simply pass their arguments on to the 
     *  @ref internal::list_view_base base class.
     *
     *  @ref internal::list_view_base supports all the std::list constructor forms, as well as
     *  the reducer move_in constructor form.
     *
     */
    //@{
    
    op_list_prepend_view() : base() {}
    
    template <typename T1>
    op_list_prepend_view(const T1& x1) : base(x1) {}
    
    template <typename T1, typename T2>
    op_list_prepend_view(const T1& x1, const T2& x2) : base(x1, x2) {}
    
    template <typename T1, typename T2, typename T3>
    op_list_prepend_view(const T1& x1, const T2& x2, const T3& x3) : base(x1, x2, x3) {}

    //@}    

    /** @name View modifier operations.
     */
    //@{
    
    /** Add an element at the end of the list.
     *
     *  Equivalent to `list.push_front(…)`
     */
    void push_front(const Type& element) 
        { this->m_value.push_front(element); }

    /** @name Insert elements at the end of the list.
     *
     *  Equivalent to `list.insert(list.begin(), …)`
     */
    //@{

    void insert_front(typename list_type::size_type n, const Type& element) 
        { this->m_value.insert(begin(), n, element); }

    template <typename Iter>
    void insert_front(Iter first, Iter last)
        { this->m_value.insert(begin(), first, last); }

    //@}
    
    /** @name Splice elements at the end of the list.
     *
     *  Equivalent to `list.splice(list.begin(), …)`
     */
    //@{
    
    void splice_front(list_type& x) {
        if (x.get_allocator() == this->m_value.get_allocator())
            this->m_value.splice(begin(), x);
        else {
            insert_front(x.begin(), x.begin());
            x.clear();
        }
    }

    void splice_front(list_type& x, iterator i) {
        if (x.get_allocator() == this->m_value.get_allocator())
            this->m_value.splice(begin(), x, i);
        else {
            push_front(*i);
            x.erase(i);
        }
    }

    void splice_front(list_type& x, iterator first, iterator last) {
        if (x.get_allocator() == this->m_value.get_allocator())
            this->m_value.splice(begin(), x, first, last);
        else {
            insert_front(first, last);
            x.erase(first, last);
        }
    }
    
    //@}

    /** Reduce operation.
     *
     *  Required by @ref monoid_base.
     */
    void reduce(op_list_prepend_view* other)
    {
        __CILKRTS_ASSERT(this->m_value.get_allocator() == other->m_value.get_allocator());
        this->m_value.splice(begin(), other->m_value);
    }
};



/** Monoid class for list append reductions. Instantiate the cilk::reducer template
 *  class with a op_list_append monoid to create a list append reducer class. For example, to
 *  create a list of strings:
 *
 *      cilk::reducer< cilk::op_list_append<std::string> > r;
 *
 *  @see @ref page_reducer_list
 *  @see op_list_append_view
 */
template <typename Type, 
          typename Allocator = typename std::list<Type>::allocator_type,
          bool Align = false>
struct op_list_append : 
    public internal::list_monoid_base<op_list_append_view<Type, Allocator>, Align> 
{
    /// Construct with default allocator.
    op_list_append() {}
    /// Construct with specified allocator.
    op_list_append(const Allocator& alloc) : 
        internal::list_monoid_base<op_list_append_view<Type, Allocator>, Align>(alloc) {}
};

/** Monoid class for list prepend reductions. Instantiate the cilk::reducer template
 *  class with a op_list_prepend monoid to create a list prepend reducer class. For example, to
 *  create a list of strings:
 *
 *      cilk::reducer< cilk::op_list_prepend<std::string> > r;
 *
 *  @see @ref page_reducer_list
 *  @see op_list_prepend_view
 */
template <typename Type, 
          typename Allocator = typename std::list<Type>::allocator_type,
          bool Align = false>
struct op_list_prepend : 
    public internal::list_monoid_base<op_list_prepend_view<Type, Allocator>, Align> 
{
    /// Construct with default allocator.
    op_list_prepend() {}
    /// Construct with specified allocator.
    op_list_prepend(const Allocator& alloc) : 
        internal::list_monoid_base<op_list_prepend_view<Type, Allocator>, Align>(alloc) {}
};


/** Deprecated list append reducer class.
 *
 *  reducer_list_append<Type, Allocator> is the same as 
 *  @ref reducer< @ref op_list_append<Type, Allocator> >, except that reducer_list_append is a
 *  proxy for the contained view, so that accumulator variable update operations can be applied
 *  directly to the reducer. For example, an element is appended to a `reducer<op_list_append>`
 *  with `r->push_back(a)`, but an element is appended to a `reducer_list_append` with
 *  `r.push_back(a)`.
 *
 *  @deprecated Users are strongly encouraged to use @ref cilk::reducer\<monoid\> reducers
 *              rather than the old reducers like reducer_list_append. The reducer\<monoid\>
 *              reducers show the reducer/monoid/view architecture more clearly, are more
 *              consistent in their implementation, and present a simpler model for new
 *              user-implemented reducers.
 *
 *  @note   Implicit conversions are provided between `reducer_list_append<Type, Allocator>` and 
 *          `reducer< op_list_append<Type, Allocator> >`. This allows incremental code 
 *          conversion: old code that used  `reducer_list_append` can pass a
 *          `reducer_list_append` to a converted function that now expects a reference to a
 *          `reducer<op_list_append>`, and vice versa.
 *
 *  @tparam Type        The value type of the list.
 *  @tparam Allocator   The allocator type of the list.
 *
 *  @see op_list_append
 *  @see reducer
 *  @see @ref page_reducer_list
 */
template <class Type, class Allocator = std::allocator<Type> >
class reducer_list_append : public reducer<op_list_append<Type, Allocator, true> >
{
    typedef reducer<op_list_append<Type, Allocator, true> > base;
    using base::view;
public:

    /** The reducer’s list type.
     */
    typedef typename base::value_type list_type;

    /** The list’s element type.
     */
    typedef Type list_value_type;

    /** The reducer’s primitive component type.
     */
    typedef Type basic_value_type;

    /** The monoid type.
     */
    typedef typename base::monoid_type Monoid;

    /** Construct a reducer with an empty list.
     */
    reducer_list_append() {}

    /** Construct a reducer with a list value.
     */
    reducer_list_append(const std::list<Type, Allocator> &initial_value) : 
        base(initial_value) {}

    /** Add an element to the end of the list.
     *
     *  @param element  Object to append to list
     */
    void push_back(const Type& element) { view().push_back(element); }

    /** Allow mutable access to the list within the current view.
     * 
     *  @warning    If this method is called before the parallel calculation is complete,
     *              the list returned by this method will be a partial result.
     * 
     *  @returns    A mutable reference to the list within the current view.
     */
    list_type &get_reference() { return view().view_get_reference(); }

    /** Allow read-only access to the list within the current view.
     * 
     *  @warning    If this method is called before the parallel calculation is complete,
     *              the list returned by this method will be a partial result.
     * 
     *  @returns    A const reference to the list within the current view.
     */
    list_type const &get_reference() const { return view().view_get_reference(); }

    /** @name `*reducer == reducer`.
     */
    //@{
    reducer_list_append&       operator*()       { return *this; }
    reducer_list_append const& operator*() const { return *this; }

    reducer_list_append*       operator->()       { return this; }
    reducer_list_append const* operator->() const { return this; }
    //@}
    
    /** “Upcast” to corresponding unaligned reducer.
     *
     *  @note   Upcast to corresponding _aligned_ reducer is a true upcast, so
     *          no conversion operator is necessary.
     */
    operator reducer< op_list_append<Type, Allocator, false> >& ()
    {
        return *reinterpret_cast< reducer< op_list_append<Type, Allocator, false> >* >(this);
    }
    
    /** “Upcast” to corresponding unaligned reducer.
     *
     *  @note   Upcast to corresponding _aligned_ reducer is a true upcast, so
     *          no conversion operator is necessary.
     */
    operator const reducer< op_list_append<Type, Allocator, false> >& () const
    {
        return *reinterpret_cast< const reducer< op_list_append<Type, Allocator, false> >* >(this);
    }
};


/** Deprecated list prepend reducer class.
 *
 *  reducer_list_prepend<Type, Allocator> is the same as 
 *  @ref reducer< @ref op_list_prepend<Type, Allocator> >, except that reducer_list_prepend is a
 *  proxy for the contained view, so that accumulator variable update operations can be applied
 *  directly to the reducer. For example, an element is prepended to a 
 *  `reducer<op_list_prepend>` with `r->push_back(a)`, but an element is prepended to a
 *  `reducer_list_prepend` with `r.push_back(a)`.
 *
 *  @deprecated Users are strongly encouraged to use @ref cilk::reducer\<monoid\> reducers
 *              rather than the old reducers like reducer_list_prepend. The reducer\<monoid\>
 *              reducers show the reducer/monoid/view architecture more clearly, are more
 *              consistent in their implementation, and present a simpler model for new
 *              user-implemented reducers.
 *
 *  @note   Implicit conversions are provided between `reducer_list_prepend<Type, Allocator>`
 *          and `reducer< op_list_prepend<Type, Allocator> >`. This allows incremental code
 *          conversion: old code that used  `reducer_list_prepend` can pass a
 *          `reducer_list_prepend` to a converted function that now expects a reference to a
 *          `reducer<op_list_prepend>`, and vice versa.
 *
 *  @tparam Type        The value type of the list.
 *  @tparam Allocator   The allocator type of the list.
 *
 *  @see op_list_prepend
 *  @see reducer
 *  @see @ref page_reducer_list
 */
template <class Type, class Allocator = std::allocator<Type> >
class reducer_list_prepend : public reducer<op_list_prepend<Type, Allocator, true> >
{
    typedef reducer<op_list_prepend<Type, Allocator, true> > base;
    using base::view;
public:

    /** The reducer’s list type.
     */
    typedef typename base::value_type list_type;

    /** The list’s element type.
     */
    typedef Type list_value_type;

    /** The reducer’s primitive component type.
     */
    typedef Type basic_value_type;

    /** The monoid type.
     */
    typedef typename base::monoid_type Monoid;

    /** Construct a reducer with an empty list.
     */
    reducer_list_prepend() {}

    /** Construct a reducer with a list value.
     */
    reducer_list_prepend(const std::list<Type, Allocator> &initial_value) : 
        base(initial_value) {}

    /** Add an element to the end of the list.
     *
     *  @param element  Object to prepend to list
     */
    void push_front(const Type& element) { view().push_front(element); }

    /** Allow mutable access to the list within the current view.
     * 
     *  @warning    If this method is called before the parallel calculation is complete,
     *              the list returned by this method will be a partial result.
     * 
     *  @returns    A mutable reference to the list within the current view.
     */
    list_type &get_reference() { return view().view_get_reference(); }

    /** Allow read-only access to the list within the current view.
     * 
     *  @warning    If this method is called before the parallel calculation is complete,
     *              the list returned by this method will be a partial result.
     * 
     *  @returns    A const reference to the list within the current view.
     */
    list_type const &get_reference() const { return view().view_get_reference(); }

    /** @name `*reducer == reducer`.
     */
    //@{
    reducer_list_prepend&       operator*()       { return *this; }
    reducer_list_prepend const& operator*() const { return *this; }

    reducer_list_prepend*       operator->()       { return this; }
    reducer_list_prepend const* operator->() const { return this; }
    //@}
    
    /** “Upcast” to corresponding unaligned reducer.
     *
     *  @note   Upcast to corresponding _aligned_ reducer is a true upcast, so
     *          no conversion operator is necessary.
     */
    operator reducer< op_list_prepend<Type, Allocator, false> >& ()
    {
        return *reinterpret_cast< reducer< op_list_prepend<Type, Allocator, false> >* >(this);
    }
    
    /** “Upcast” to corresponding unaligned reducer.
     *
     *  @note   Upcast to corresponding _aligned_ reducer is a true upcast, so
     *          no conversion operator is necessary.
     */
    operator const reducer< op_list_prepend<Type, Allocator, false> >& () const
    {
        return *reinterpret_cast< const reducer< op_list_prepend<Type, Allocator, false> >* >(this);
    }
};

/// @cond internal

/** Metafunction specialization for reducer conversion.
 *
 *  This specialization of the @ref legacy_reducer_downcast template class defined in
 *  reducer.h causes the `reducer< op_list_append<Type, Allocator> >` class to have an 
 *  `operator reducer_list_append<Type, Allocator>& ()` conversion operator that statically
 *  downcasts the `reducer<op_list_append>` to the corresponding `reducer_list_append` type.
 *  (The reverse conversion, from `reducer_list_append` to `reducer<op_list_append>`, is just
 *  an upcast, which is provided for free by the language.)
 */
template <class Type, class Allocator, bool Align>
struct legacy_reducer_downcast<reducer<op_list_append<Type, Allocator, Align> > >
{
    typedef reducer_list_append<Type, Allocator> type;
};

/** Metafunction specialization for reducer conversion.
 *
 *  This specialization of the @ref legacy_reducer_downcast template class defined in
 *  reducer.h causes the `reducer< op_list_prepend<Type, Allocator> >` class to have an 
 *  `operator reducer_list_prepend<Type, Allocator>& ()` conversion operator that statically
 *  downcasts the `reducer<op_list_prepend>` to the corresponding `reducer_list_prepend` type.
 *  (The reverse conversion, from `reducer_list_prepend` to `reducer<op_list_prepend>`, is just
 *  an upcast, which is provided for free by the language.)
 */
template <class Type, class Allocator, bool Align>
struct legacy_reducer_downcast<reducer<op_list_prepend<Type, Allocator, Align> > >
{
    typedef reducer_list_prepend<Type, Allocator> type;
};

/// @endcond

} // Close namespace cilk

#endif //  REDUCER_LIST_H_INCLUDED

/** @file reducer_min_max.h
 *
 *  @brief Defines classes for doing parallel minimum and maximum reductions.
 *
 *  @copyright
 *  Copyright (C) 2009-2012, Intel Corporation
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
 *  @see @ref page_reducer_min_max
 */

#ifndef REDUCER_MIN_MAX_H_INCLUDED
#define REDUCER_MIN_MAX_H_INCLUDED

#include <cilk/reducer.h>

#ifdef __cplusplus

#include <algorithm>
#include <limits>

/** @page page_reducer_min_max Minimum and Maximum Reducers
 *
 *  @tableofcontents
 *
 *  Header file reducer_min_max.h defines the monoid and view classes for creating Cilk reducers
 *  to find the minimum or maximum of a set of values in parallel, and optionally record the
 *  index of the chosen value in the set.
 *
 *  You should be familiar with @ref pagereducers "Cilk reducers", described in file
 *  reducers.md, and particularly with @ref reducers_using, before trying to use the
 *  information in this file.
 *
 *  @section redminmax_usage Usage Examples
 *
 *      cilk::reducer< cilk::op_max<int> > rm;
 *      cilk_for (int i = 0; i < ARRAY_SIZE; ++i)
 *      {
 *          rm->calc_max(a[i]); // or *rm = cilk::max_of(*max, a[i])
 *      }
 *      std::cout << "maximum value is " << rm.get_value() << std::endl;
 *
 *  and
 *
 *      cilk::reducer< cilk::op_min_index<int, double> > rmi;
 *      cilk_for (int i = 0; i < ARRAY_SIZE; ++i)
 *      {
 *          rmi->calc_min(i, a[i]) // or *rmi = cilk::min_of(*rmi, i, a[i]);
 *      }
 *      std::cout << "minimum value a[" << rmi.get_value().first << "] = "
 *                << rmi.get_value().second << std::endl;
 *
 *  @section redminmax_classes Classes Defined
 *
 *  @subsection redminmax_max Maximum
 *  *   @ref cilk::op_max<Type, Compare=std::less<Type> > (monoid)
 *  *   @ref cilk::op_max_view (view)
 *  *   @ref cilk::reducer< cilk::op_max<...> > (reducer) (defined in reducer.h)
 *  *   @ref cilk::reducer_max<Type, Compare=std::less<Type> > (deprecated reducer)
 *
 *  @subsection redminmax_min Minimum
 *  *   @ref cilk::op_min<Type, Compare=std::less<Type> > (monoid)
 *  *   @ref cilk::op_min_view (view)
 *  *   @ref cilk::reducer< cilk::op_min<...> > (reducer) (defined in reducer.h)
 *  *   @ref cilk::reducer_min<Type, Compare=std::less<Type> > (deprecated reducer)
 *
 *  @subsection redminmax_max_index Maximum and Index
 *  *   @ref cilk::op_max_index<Index, Type, Compare=std::less<Type> > (monoid)
 *  *   @ref cilk::op_max_index_view (view)
 *  *   @ref cilk::reducer< cilk::op_max_index<...> > (reducer) (defined in reducer.h)
 *  *   @ref cilk::reducer_max_index<Index, Type, Compare=std::less<Type> > (deprecated reducer)
 *
 *  @subsection redminmax_min_index Minimum and Index
 *  *   @ref cilk::op_min_index<Index, Type, Compare=std::less<Type> > (monoid)
 *  *   @ref cilk::op_min_index_view (view)
 *  *   @ref cilk::reducer< cilk::op_min_index<...> > (reducer) (defined in reducer.h)
 *  *   @ref cilk::reducer_min_index<Index, Type, Compare=std::less<Type> > (deprecated reducer)
 *
 *  @section redminmax_monoid The Monoid
 *
 *  @subsection redminmax_monoid_values Value Set
 *
 *  The value set of a minimum or maximum reducer is the set of values of `Type`,
 *  possibly augmented with a special identity value which is greater than (less than) any value
 *  of `Type`.
 *
 *  @subsection redminmax_monoid_operator Operator
 *
 *  In the most common case, the operator of a minimum reducer is defined as
 *
 *      x MIN y == (x < y) ? x : y
 *
 *  Thus, `a1 MIN a2 MIN … an` is the first `ai` which is not greater than any other `ai`.
 *
 *  The operator of a maximum reducer is defined as
 *
 *      x MAX y == (x > y) ? x : y
 *
 *  Thus, `a1 MAX a2 MAX … an` is the first `ai` which is not less than any other `ai`.
 *
 *  @subsection redminmax_monoid_comparators Comparators
 *
 *  Min/max reducers are not limited to finding the minimum or maximum value determined by the
 *  `<` or `>` operator. In fact, all min/max reducers use a _comparator_, which is either a
 *  function or an object of a function class that defines a [strict weak ordering] 
 *  (http://en.wikipedia.org/wiki/Strict_weak_ordering) on a set of values. (This is exactly the
 *  same as the requirement for the comparison predicate for STL associative containers and
 *  sorting algorithms.)
 *
 *  Just as with STL algorithms and containers, the comparator type parameter for min/max
 *  reducers is optional. If it is omitted, it defaults to `std::less`, which gives the
 *  behavior described in the previous section. Using non-default comparators (anything other
 *  than `std::less`) with min/max reducers is just like using them with STL containers and
 *  algorithms.
 *
 *  Taking comparator objects into account, the reduction operation `MIN` for a minimum reducer
 *  is defined as
 *
 *      x MIN y == compare(x, y) ? x : y
 *
 *  where `compare()` is the reducer’s comparator. Similarly, the reduction operation MAX for a
 *  maximum reducer is defined as
 *
 *      x MAX y == compare(y, x) ? x : y
 *
 *  (If `compare(x, y) == x < y`, then `compare(y, x) == x > y`.) 
 *
 *  @subsection redminmax_monoid_identity Identity
 *
 *  The identity value of the reducer is the value which is greater than (less than) any other 
 *  value in the value set of the reducer. This is the [“special identity value”] 
 *  (#redminmax_monoid_values) if the reducer has one, or the largest (smallest) value in the
 *  value set otherwise.
 *
 *  @section redminmax_index Value and Index Reducers
 *
 *  Min/max reducers come in two families. The _value_ reducers, `op_min` and `op_max`, simply
 *  find the smallest or largest value from a set of values. The _index_ reducers,
 *  `op_min_index` and `op_max_index`, also record an index value associated with the
 *  smallest or largest value.
 *
 *  In the `%op_min_index` usage example [above](#redminmax_usage), the values are taken from an
 *  array, and the index of a value is the index of the array element it comes from. More
 *  generally, though, an index can be any sort of key which identifies a particular value in a
 *  collection of values. For example, if the values were taken from the nodes of a tree, then
 *  the “index” of a value might be a pointer to the node containing that value. 
 *
 *  A min/max index reducer is essentially the same as a min/max value reducer whose value type
 *  is an (index, value) pair, and whose comparator ignores the index part of the pair.
 *  (index, value) pairs are represented by `std::pair<Index, Type>` objects.
 *  This has the consequence that wherever the interface of a min/max value reducer has a
 *  `Type`, the interface of the corresponding min/max index reducer has a 
 *  `std::pair<Index, Type>`. (There are convenience variants of the `reducer(Type)`
 *  constructor and the `calc_min()`, `calc_max()`, `%min_of()`, and `%max_of()` functions that
 *  take an index argument and a value argument instead of an index/value pair.)
 *
 *  @section redminmax_operations Operations
 *
 *  @subsection redminmax_constructors Constructors
 *
 *  @subsubsection redminmax_constructors_value Min/Max Value Reducers
 *
 *      reducer()                           // identity
 *      reducer(const Compare& compare)     // identity
 *      reducer(const Type& value)
 *      reducer(move_in(Type& variable))
 *      reducer(const Type& value, const Compare& compare)
 *      reducer(move_in(Type& variable), const Compare& compare)
 *
 *  @subsubsection redminmax_constructors_index Min/Max Index Reducers
 *
 *      reducer()                           // identity
 *      reducer(const Compare& compare)     // identity
 *      reducer(const std::pair<Index, Type>& pair)
 *      reducer(move_in(std::pair<Index, Type>& variable))
 *      reducer(const Index& index, const Type& value)
 *      reducer(const std::pair<Index, Type>& pair, const Compare& compare)
 *      reducer(move_in(std::pair<Index, Type>& variable), const Compare& compare)
 *      reducer(const Index& index, const Type& value, const Compare& compare)
 *
 *  @subsection redminmax_get_set Set and Get
 *
 *      r.set_value(const Type& value)
 *      Type = r.get_value() const
 *      r.move_in(Type& variable)
 *      r.move_out(Type& variable)
 *
 *  Note that for an index reducer, the `Type` in these operations is actually a
 *  `std::pair<Index, Type>`. (See @ref redminmax_index.)
 *
 *  @subsection redminmax_initial Initial Values and is_set()
 *
 *  A minimum or maximum reducer without a specified initial value, and before any MIN or MAX
 *  operation has been performed on it, will always represent the [identity value]
 *  (#redminmax_monoid_identity) of its monoid. For value reducers with a numeric type and
 *  the default comparator (`std::less`), this will be a well defined value. For example,
 *
 *      reducer< op_max<unsigned> > r1;
 *      // r1.get_value() == 0
 *
 *      reducer< op_min<float> > r2;
 *      // r2.get_value() == std::numeric_limits<float>::infinity
 *
 *  In other cases, though (index reducers, non-numeric type, or non-default comparator),
 *  the actual identity value for the monoid may be unknown, or it may not even be a value 
 *  of the reducer’s type. For example, there is no “largest string” to serve as the initial
 *  value for a `reducer< op_min<std::string> >`. In these cases, the result of calling
 *  `get_value()` is undefined.
 *
 *  To avoid calling `get_value()` when its result would be undefined, you can call the view’s
 *  `is_set()` function, which will return true  if the reducer has a well-defined value —
 *  either because a MIN or MAX operation has been performed, or because it had a well-defined
 *  initial value:
 *
 *      reducer< op_max<unsigned> > r1;
 *      // r1->is_set() == true
 *      // r1.get_value() == 0
 *
 *      reducer< op_min<std::string> > r2;
 *      // r2->is_set() == false
 *      // r2.get_value() is undefined
 *      r2->calc_min("xyzzy");
 *      // r2->is_set() == true
 *      // r2.get_value() == "xyzzy"
 *
 *  >   Note: For an index reducer without a specified initial value, the initial value of the
 *  >   index is the default value of the `Index` type.
 *
 *  @subsection redminmax_view_ops View Operations
 *
 *  The basic reduction operation is `x = x MIN a` for a minimum reducer, or `x = x MAX a` for
 *  a maximum reducer. The basic syntax for these operations uses the `calc_min()` and
 *  `calc_max()` member functions of the view class. An assignment syntax is also provided,
 *  using the %cilk::min_of() and %cilk::max_of() global functions:
 *
 *  Class          | Modifier            | Assignment
 *  ---------------|---------------------|-----------
 *  `op_min`       | `r->calc_min(x)`    | `*r = min_of(*r, x)` or `*r = min_of(x, *r)`
 *  `op_max`       | `r->calc_max(x)`    | `*r = max_of(*r, x)` or `*r = max_of(x, *r)`
 *  `op_min_index` | `r->calc_min(i, x)` | `*r = min_of(*r, i, x)` or `*r = min_of(i, x, *r)`
 *  `op_max_index` | `r->calc_max(i, x)` | `*r = max_of(*r, i, x)` or `*r = max_of(i, x, *r)`
 *
 *  The `calc_min()` and `calc_max()` member functions return a reference to the view,
 *  so they can be chained:
 *
 *      r->calc_max(x).calc_max(y).calc_max(z);
 *
 *  In a `%min_of()` or `%max_of()` assignment, the view on the left-hand side of the
 *  assignment must be the same as the view argument in the call. Otherwise, the behavior is
 *  undefined (but an assertion error will occur if the code is compiled with debugging
 *  enabled).
 *
 *      *r = max_of(*r, x);     // OK
 *      *r1 = max_of(*r2, y);   // ERROR
 *
 *  The `%min_of()` and `%max_of()` functions can take a nested `%min_of()` or
 *  `%max_of()` call in place of the view reference argument:
 *
 *      *r = max_of(max_of(max_of(*r, x), y), z);
 *      *r = min_of(i, a[i], min_of(j, a[j], min_of(k, a[k], *r)));
 *
 *  Wherever an “`i`, `x`” argument pair is shown in the table above, a single
 *  pair argument may be passed instead. For example:
 *
 *      Index index;
 *      Type value;
 *      std::pair<Index, Type> ind_val(index, value);
 *      // The following statements are all equivalent.
 *      r->calc_min(index, value);
 *      r->calc_min(ind_val);
 *      *r = min_of(*r, index, value);
 *      *r = min_of(*r, ind_val);
 *
 *  @section redminmax_compatibility Compatibility Issues
 *
 *  Most Cilk library reducers provide
 *  *   Binary compatibility between `reducer_KIND` reducers compiled with Cilk library
 *      version 0.9 (distributed with Intel® C++ Composer XE version 13.0 and earlier) and the
 *      same reducers compiled with Cilk library version 1.0 and later.
 *  *   Transparent casting between references to `reducer<op_KIND>` and `reducer_KIND`.
 *
 *  This compatibility is not available in all cases for min/max reducers. There are two areas
 *  of incompatibility.
 *
 *  @subsection redminmax_compatibility_stateful Non-empty Comparators
 *
 *  There is no way to provide binary compatibility between the 0.9 and 1.0 definitions of
 *  min/max reducers that use a non-empty comparator class or a comparator function. (Empty
 *  comparator classes like `std::less` are not a problem.) 
 *
 *  To avoid run-time surprises, the legacy `reducer_{min|max}[_index]` classes have been coded
 *  in the 1.0 library so that they will not even compile when instantiated with a
 *  non-empty comparator class.
 *
 *  @subsection redminmax_compatibility_optimized Numeric Optimization
 *
 *  Min/max reducers with a numeric value type and the default comparator can be implemented
 *  slightly more efficiently than other min/max reducers. However, the optimization is
 *  incompatible with the 0.9 library implementation of min/max reducers.
 *
 *  The default min/max reducers implementation in the 1.0 library uses this numeric
 *  optimization. Code using legacy reducers compiled with the 1.0 library can be
 *  safely used in the same program as code compiled with the 0.9 library, but the classes
 *  compiled with the different Cilk libraries will have different “mangled names”, and
 *  therefore will look like different types to the linker.
 *
 *  The simplest solution is just to recompile the code that was compiled with the older version
 *  of Cilk. However, if this is impossible, you can define the
 *  `CILK_LIBRARY_0_9_REDUCER_MINMAX` macro (on the compiler command line, or in your source
 *  code before including `reducer_min_max.h`) when compiling with the new library. This will
 *  cause it to generate numeric reducers that will be less efficient, but will be fully
 *  compatible with previously compiled code. (Note that this macro has no effect on [the
 *  non-empty comparator incompatibility] (redminmax_compatibility_stateful).)
 *
 *  @section redminmax_types Type Requirements
 *
 *  `Type` and `Index` must be `Copy Constructible`, `Default Constructible`, and
 *  `Assignable`.
 *
 *  `Compare` must be `Copy Constructible` if the reducer is constructed with a 
 *  `compare` argument, and `Default Constructible` otherwise.
 *
 *  The `Compare` function must induce a strict weak ordering on the elements of `Type`.
 *
 *  @section redminmax_in_c Minimum and Maximum Reducers in C
 *
 *  These macros can be used to do minimum and maximum reductions in C:
 *
 *  Declaration                  | Type                              | Operation
 *  -----------------------------|-----------------------------------|----------
 * @ref CILK_C_REDUCER_MIN       |@ref CILK_C_REDUCER_MIN_TYPE       |@ref CILK_C_REDUCER_MIN_CALC      
 * @ref CILK_C_REDUCER_MAX       |@ref CILK_C_REDUCER_MAX_TYPE       |@ref CILK_C_REDUCER_MAX_CALC      
 * @ref CILK_C_REDUCER_MIN_INDEX |@ref CILK_C_REDUCER_MIN_INDEX_TYPE |@ref CILK_C_REDUCER_MIN_INDEX_CALC
 * @ref CILK_C_REDUCER_MAX_INDEX |@ref CILK_C_REDUCER_MAX_INDEX_TYPE |@ref CILK_C_REDUCER_MAX_INDEX_CALC
 *
 *  For example:
 *
 *      CILK_C_REDUCER_MIN(r, int, INT_MAX);
 *      CILK_C_REGISTER_REDUCER(r);
 *      cilk_for(int i = 0; i != n; ++i) {
 *          CILK_C_REDUCER_MIN_CALC(r, a[i]);
 *      }
 *      CILK_C_UNREGISTER_REDUCER(r);
 *      printf("The smallest value in a is %d\n", REDUCER_VIEW(r));
 *
 *
 *      CILK_C_REDUCER_MAX_INDEX(r, uint, 0);
 *      CILK_C_REGISTER_REDUCER(r);
 *      cilk_for(int i = 0; i != n; ++i) {
 *          CILK_C_REDUCER_MAX_INDEX_CALC(r, i, a[i]);
 *      }
 *      CILK_C_UNREGISTER_REDUCER(r);
 *      printf("The largest value in a is %u at %d\n", 
 *              REDUCER_VIEW (r).value, REDUCER_VIEW(r).index);
 *
 *  See @ref reducers_c_predefined.
 */

namespace cilk {

namespace internal {
namespace min_max {

/** @defgroup min_max_is_set_optimization The “is_set optimization” for min/max reducers
 *
 *  The obvious definition of the identity value for a max or min reducer is as the smallest
 *  (or largest) value of the value type. However, for an arbitrary comparator and/or an
 *  arbitrary value type, the largest / smallest value may not be known. It may not even be
 *  defined — what is the largest string?
 *
 *  Therefore, min/max reducers represent their value internally as a pair `(value, is_set)`.
 *  When `is_set` is true, the pair represents the known value `value`; when `is_set` is false,
 *  the pair represents the identity value.
 *
 *  This is an effective solution, but the most common use of min/max reducers is probably with
 *  numeric types and the default definition of minimum or maximum (using `std::less`), in which
 *  case there are well-defined, knowable smallest and largest values. Testing `is_set` for
 *  every comparison is then unnecessary and wasteful.
 *
 *  The “is_set optimization” just means generating code that doesn’t use `is_set` when it isn’t
 *  needed. It is implemented using two metaprogramming classes:
 *
 *  -   do_is_set_optimization tests whether the optimization is applicable.
 *  -   identity_value gets the appropriate identity value for a type.
 *  
 *  @ingroup reducers
 */
 //@{

/** Test whether the is_set optimization is applicable.
 *
 *  This class is used to test whether the is_set optimization should be applied for a 
 *  particular reducer. It is instantiated with a value type and a comparator, and defines
 *  a boolean constant, `value`. If the value type is numeric and the comparator is `std::less`,
 *  then `value` is true, meaning the optimization is applicable; otherwise, `value` is false.
 *
 *  For binary compatibility with code that was previously compiled with older reducer
 *  definitions that did not implement the is_set optimization, the user can define the
 *  `CILK_LIBRARY_0_9_REDUCER_MINMAX` macro on the compiler command line. In this case, `value`
 *  will always be false, and the generated code will not use the is_set optimization. (The
 *  is_set optimization flag is a template parameter to the min/max view classes, and,
 *  indirectly, to the monoid and reducer classes, so a reducer class with the optimization
 *  enabled will be a different type from the same reducer class with the optimization disabled,
 *  so modules compiled with and without the macro definition can safely be linked together.)
 *
 *  @tparam Type   The value type for the reducer.
 *  @tparam Compare The comparator type for the reducer.
 *
 *  @result `value` will be `true` if @a Type is a numeric type, @a Compare is 
 *          `std::less<Type>`, and `CILK_LIBRARY_0_9_REDUCER_MINMAX` is not defined.
 *
 *  @see min_max_is_set_optimization
 */
template <  typename Type, 
            typename Compare, 
            bool     = std::numeric_limits<Type>::is_specialized >
struct do_is_set_optimization 
{ 
    static const bool value = false;   ///< False in the general case.
};

#ifndef CILK_LIBRARY_0_9_REDUCER_MINMAX

/** @copydoc do_is_set_optimization
 */
template <typename Type>
struct do_is_set_optimization<Type, std::less<Type>, true> 
{ 
    static const bool value = true;    ///< True in the special case where optimization is possible.
};

#endif


/** Get the identity value when using the is_set optimization.
 *
 *  This class defines a function which assigns the appropriate identity value to a variable
 *  when the is_set optimization is applicable.
 *
 *  @tparam Type    The value type for the reducer.
 *  @tparam Compare The comparator type for the reducer.
 *  @tparam ForMax  `true` to get the identity value for a max reducer (i.e., the smallest
 *                  @a Type), `false` to get the identity value for a min reducer (i.e.,
 *                  the largest @a Type).
 *
 *  @result If @a Type and @a Compare qualify for the is_set optimization, the `set_identity()'
 *  function will set its argument variable to the minimum or maximum value of @a Type,
 *  depending on @a ForMax. Otherwise, `set_identity()` will be a no-op.
 *
 *  @see min_max_is_set_optimization
 */
template <  typename    Type, 
            typename    Compare, 
            bool        ForMax,
            bool        = std::numeric_limits<Type>::is_specialized,
            bool        = std::numeric_limits<Type>::has_infinity >
struct identity_value {
    static void set_identity(Type&) {} ///< No identity value in the general case.
};

/** @copydoc identity_value
 */
template <typename Type>
struct identity_value<Type, std::less<Type>, true, true, true> {
    /// Floating max identity is negative infinity.
    static void set_identity(Type& id) { id = -std::numeric_limits<Type>::infinity(); }
};

/** @copydoc identity_value
 */
template <typename Type>
struct identity_value<Type, std::less<Type>, true, true, false> {
    /// Integer max identity is minimum value of type.
    static void set_identity(Type& id) { id = std::numeric_limits<Type>::min(); }
};

/** @copydoc identity_value
 */
template <typename Type>
struct identity_value<Type, std::less<Type>, false, true, true> {
    /// Floating min identity is positive infinity.
    static void set_identity(Type& id) { id = std::numeric_limits<Type>::infinity(); }
};

/** @copydoc identity_value
 */
template <typename Type>
struct identity_value<Type, std::less<Type>, false, true, false> {
    /// Integer min identity is maximum value of type.
    static void set_identity(Type& id) { id = std::numeric_limits<Type>::max(); }
};

//@}


/** Adapter class to reverse the arguments of a predicate.
 *
 *  Observe that:
 *
 *      (x < y) == (y > x)
 *      max(x, y) == (x < y) ? y : x
 *      min(x, y) == (y < x) ? y : x == (x > y) ? y : x
 *
 *  More generally, if `c` is a predicate defining a `Strict Weak Ordering`, and 
 *  `c*(x, y) == c(y, x)`, then
 *
 *      max(x, y, c) == c(x, y) ? y : x
 *      min(x, y, c) == c(y, x) ? y : x == c*(x, y) ? y : x == max(x, y, c*)
 *
 *  For any predicate `C` with argument type `T`, the template class `reverse_predicate<C, T>`
 *  defines a predicate which is identical to `C`, except that its arguments are reversed. Thus,
 *  we can implement `op_min_view<Type, Compare>` as `op_max_view<Type,
 *  reverse_predicate<Compare, Type> >`. (Actually, op_min_view and op_max_view are both
 *  implemented as subclasses of a common base class, view_base.)
 *
 *  @note   If `C` is an empty functor class, then `reverse_predicate(C)` will also be an
 *          empty functor class.
 *
 *  @tparam Predicate   The predicate whose arguments are to be reversed.
 *  @tparam Argument    @a Predicate’s argument type.
 */
template <typename Predicate, typename Argument = typename Predicate::first_argument_type>
class reverse_predicate : private binary_functor<Predicate>::type {
    typedef typename binary_functor<Predicate>::type base;
public:
    /// Default constructor
    reverse_predicate() : base() {}
    /// Constructor with predicate
    reverse_predicate(const Predicate& p) : base(p) {} 
    ///Predicate operation
    bool operator()(const Argument& x, const Argument& y) const
        { return base::operator()(y, x); }
};


/** Class to represent the comparator for a min/max view class.
 *
 *  This class is intended to accomplish two objectives in the implementation of min/max views.
 *
 *  1.  To minimize data bloat, when we have a reducer with a non-stateless comparator, we want
 *      to keep a single instance of the comparator object in the monoid, and just call it from
 *      the views.
 *  2.  We provide binary compatibility with the original min/max reducer implementation for
 *      reducers with stateless comparators. This means that a view with a stateless comparator
 *      must contain only `value` and `is_set` data members.
 *
 *  To achieve the first objective, we use the typed_indirect_binary_function class defined in
 *  metaprogramming.h to wrap a pointer to the actual comparator. The
 *  typed_indirect_binary_function class will be empty when no pointer is needed because the
 *  comparator is stateless.
 *
 *  To achieve the second objective, we make the indirect comparator class a base class rather
 *  than a data member of the view, so the “empty base class” rule will ensure no that no
 *  additional space is allocated in the view.
 *
 *  We could simply use typed_indirect_binary_function as the base class of the view, but this
 *  would mean writing comparisons as `(*this)(x, y)`, which is just weird. So, instead, we
 *  use this subclass of typed_indirect_binary_function, which provides function `compare()` 
 *  as a synonym for `operator()`.
 *
 *  @tparam Type    The value type of the comparator class.
 *  @tparam Compare A predicate class.
 *
 *  @see typed_indirect_binary_function
 */
template <typename Type, typename Compare>
class comparator_base : private typed_indirect_binary_function<Compare, Type, Type, bool>
{
    typedef typed_indirect_binary_function<Compare, Type, Type, bool> base;
protected:
    comparator_base(const Compare* f) : base(f) {}  ///< Constructor.
    
    /// Comparison function.
    bool compare(const Type& a, const Type& b) const
    {
        return base::operator()(a, b); 
    }
    
    /// Get the comparator pointer.
    const Compare* compare_pointer() const { return base::pointer(); }
};


/** @defgroup view_content_classes Content classes for min/max views
 *
 *  @ingroup reducers
 *
 *  Minimum and maximum reducer view classes inherit from a “view content” class. The content
 *  object contains the actual data members for the view, and provides typedefs and member
 *  functions for accessing the data members as needed to support the view functionality.
 *
 *  There are two content classes, which encapsulate the differences between simple min/max
 *  reducers and min/max with index reducers:
 *
 *  -   view_content
 *  -   index_view_content
 *
 *  @note   An obvious, and arguably simpler, encapsulation strategy would be to just let the
 *          `Type` of a min/max view be an (index, value) pair structure for min_index and
 *          max_index reducers. Then all views would just have a `Type` data member and an
 *          `is_set` data member, and the comparator for min_index and max_index views could be
 *          customized to consider only the value component of the (index, value) `Type` pair.
 *          Unfortunately, this would break binary compatibility with old-style
 *          reducer_max_index and reducer_min_index, because the memory layout of an
 *          (index, value) pair followed by a `bool` is different from the memory layout of an
 *          index data member followed by a value data member followed by a `bool` data
 *          member. The content class is designed to exactly replicate the layout of the views
 *          in the old-style reducers.
 *
 *  A content class `C`, and its objects `c`, must define the following:
 *
 *  Definition                          | Meaning
 *  ------------------------------------|--------
 *  `C::value_type`                     | A typedef for `Type` of the view. (A `std::pair<Index, Type>` for min_index and max_index views).
 *  `C::comp_value_type`                | A typedef for the type of value compared by the view’s `compare()` function.
 *  `C()`                               | Constructs the content with the identity value.
 *  `C(const value_type&)`              | Constructs the content with a specified value.
 *  `c.is_set()`                        | Returns true if the content has a known value.
 *  `c.value()`                         | Returns the content’s value.
 *  `c.set_value(const value_type&)`    | Sets the content’s value. (The value becomes known.)
 *  `c.comp_value()`                    | Returns a const reference to the value or component of the value that is to be compared by the view’s comparator.
 *  `C::comp_value(const value_type&)`  | Returns a const reference to a value or component of a value that is to be compared by the view’s comparator.
 *
 *  @see view_base
 */
//@{

/** Content class for op_min_view and op_max_view.
 *
 *  @tparam Type   The value type of the op_min_view or op_max_view.
 *  @tparam Compare The comparator class specified for the op_min_view or op_max_view. (_Not_
 *                  the derived comparator class actually used by the view_base. For
 *                  example, an `op_min_view<int>` will have comparator class `std::less<int>`,
 *                  but its view_base will be instantiated with a `reverse_predicate<
 *                  std::less<int> >`.)
 *  @tparam ForMax  `true` if this is the content class for an op_max_view, `false` if it is
 *                  for an op_min_view.
 *
 *  @see view_content_classes
 */
template < typename Type
         , typename Compare
         , bool ForMax
         , bool AssumeIsSet = do_is_set_optimization<Type, Compare>::value
         >
class view_content {
    Type    m_value;
    bool    m_is_set;
public:
    typedef Type value_type;        ///< `Type` type of the view.
    typedef Type comp_value_type;   ///< Type compared by the view’s `compare()` function
    
    /// Construct with identity value.
    view_content() : m_value(), m_is_set(false) {}
    
    /// Construct with defined value.
    view_content(const value_type& value) : m_value(value), m_is_set(true) {}
    
    /// Get value.
    value_type value() const { return m_value; }
    
    /// Set value.
    void set_value(const value_type& value) 
    { 
        m_value = value;
        m_is_set = true;
    }
    
    /// Get const reference to comparison value.
    const comp_value_type& comp_value() const { return m_value; }

    /// Get const reference to comparison value from a value.
    static const comp_value_type& comp_value(const value_type& value) { return value; }
    
    /// Get const reference to value.
    const Type& get_reference() const { return m_value; }
    
    /// Get const reference to index (meaningless for non-index reducers, but 
    /// required for view_base.
    const Type& get_index_reference() const { return m_value; }
    
    /// Test if `is_set` is true.
    bool is_set() const { return m_is_set; }
};

/** @copydoc view_content
 *
 *  This is the specialization of the view_content class for cases where
 *  `AssumeIsSet` is true (i.e., where the is_set optimization is applicable).
 */
template < typename Type
         , typename Compare
         , bool ForMax
         >
class view_content<Type, Compare, ForMax, true> {
    typedef identity_value<Type, Compare, ForMax> Identity;
    Type    m_value;
public:
    typedef Type value_type;        ///< `Type` type of the view.
    typedef Type comp_value_type;   ///< Type compared by the view’s `compare()` function
    
    /// Construct with identity value.
    view_content() { Identity::set_identity(m_value); }
    
    /// Construct with defined value.
    view_content(const value_type& value) : m_value(value) {}
    
    /// Get value.
    value_type value() const { return m_value; }
    
    /// Set value.
    void set_value(const value_type& value) 
    { 
        m_value = value;
    }
    
    /// Get const reference to comparison value.
    const comp_value_type& comp_value() const { return m_value; }

    /// Get const reference to comparison value from a value.
    static const comp_value_type& comp_value(const value_type& value) { return value; }
    
    /// Get const reference to value.
    const Type& get_reference() const { return m_value; }
    
    /// Get const reference to index (meaningless for non-index reducers, but 
    /// required for view_base.
    const Type& get_index_reference() const { return m_value; }
    
    /// Test if `is_set` is true.
    bool is_set() const { return true; }
};


/** Content class for op_min_index_view and op_max_index_view.
 *
 *  @tparam Index   The index type of the op_min_index_view or op_max_index_view.
 *  @tparam Type    The value type of the op_min_view or op_max_view. (_Not_ the value type of
 *                  the view_base, which will be `std::pair<Index, Type>`.)
 *  @tparam Compare The comparator class specified for the op_min_index_view or 
 *                  op_max_index_view. (_Not_ the derived comparator class actually used by the
 *                  view_base. For example, an `op_min_index_view<int>` will have
 *                  comparator class `std::less<int>`, but its view_base will be
 *                  instantiated with a `reverse_predicate< std::less<int> >`.)
 *  @tparam ForMax  `true` if this is the content class for an op_max_index_view, `false` if it
 *                  is for an op_min_index_view.
 *
 *  @see view_content_classes
 */
template < typename Index
         , typename Type
         , typename Compare
         , bool ForMax
         >
class index_view_content {
    typedef identity_value<Type, Compare, ForMax> Identity;

    Index   m_index;
    Type    m_value;
    bool    m_is_set;
public:
    typedef std::pair<Index, Type> value_type;  ///< `Type` type of the view.
    typedef Type comp_value_type;  ///< Type compared by the view’s `compare()` function
    
    /// Construct with identity value.
    index_view_content() : m_index(), m_value(), m_is_set(false) {}
    
    /// Construct with index/value pair.
    index_view_content(const value_type& value) : 
        m_index(value.first), m_value(value.second), m_is_set(true) {}
    
    /// Construct with index and value.
    index_view_content(const Index& index, const Type& value) : 
        m_index(index), m_value(value), m_is_set(true) {}
    
    /// Construct with index only.
    index_view_content(const Index& index) : 
        m_index(index), m_value(), m_is_set(false) {}
    
    /// Get value.
    value_type value() const { return value_type(m_index, m_value); }
    
    /// Set value.
    void set_value(const value_type& value) 
    { 
        m_index = value.first; 
        m_value = value.second;
        m_is_set = true;
    }
    
    /// Get const reference to comparison value.
    const comp_value_type& comp_value() const { return m_value; }
    
    /// Get const reference to comparison value from a value.
    static const comp_value_type& comp_value(const value_type& value) 
        { return value.second; }
    
    /// Get const reference to value.
    const Type& get_reference() const { return m_value; }
    
    /// Get const reference to index.
    const Index& get_index_reference() const { return m_index; }
    
    /// Test if `is_set` is true.
    bool is_set() const { return m_is_set; }
};

template <typename View> class rhs_proxy;

template <typename View>
inline rhs_proxy<View> make_proxy(const typename View::value_type& value, const View& view);

template <typename Content, typename Less, typename Compare> class view_base;


/** Class to represent the right-hand side of `*reducer = {min|max}_of(*reducer, value)`.
 *
 *  The only assignment operator for a min/max view class takes a rhs_proxy as its
 *  operand. This results in the syntactic restriction that the only expressions that can be
 *  assigned to a min/max view are ones which generate an rhs_proxy — that is,
 *  expressions of the form `max_of(view, value)` and `min_of(view, value)`.
 *
 *  @warning
 *  The lhs and rhs views in such an assignment must be the same; otherwise, the behavior
 *  will be undefined. (I.e., `*r1 = min_of(*r1, x)` is legal; `*r1 = min_of(*r2, x)` is
 *  illegal.)  This condition will be checked with a runtime assertion when compiled in
 *  debug mode.
 *
 *  @tparam View    The view class (op_{min|max}[_index]_view) that this proxy was 
 *                  created from.
 *
 *  @see view_base
 */
template <typename View>
class rhs_proxy {
    typedef typename View::less_type                less_type;
    typedef typename View::compare_type             compare_type;
    typedef typename View::value_type               value_type;
    typedef typename View::content_type             content_type;
    typedef typename content_type::comp_value_type  comp_value_type;
    
    friend class view_base<content_type, less_type, compare_type>;
    friend rhs_proxy make_proxy<View>(const typename View::value_type& value, const View& view);
    
    typed_indirect_binary_function<compare_type, comp_value_type, comp_value_type, bool>
                                        m_comp;
    const View*                         m_view;
    value_type                          m_value;

    rhs_proxy& operator=(const rhs_proxy&); // Disable assignment operator
    rhs_proxy();                            // Disable default constructor
    
    /** Constructor (called from view_base::make_proxy).
     */
    rhs_proxy(const View* view, 
              const value_type& value,
              const compare_type* compare) : 
        m_view(view), m_value(value), m_comp(compare) {}
        
    /** Check matching view, then return value (called from view_base::assign).
     */
    value_type value(const typename View::base* view) const
    { 
        __CILKRTS_ASSERT(view == m_view); 
        return m_value; 
    }

public:

    /** Support max_of(max_of(view, value), value) and the like.
     */
    rhs_proxy calc(const value_type& x) const
    {
        return rhs_proxy(
            m_view, 
            m_comp(content_type::comp_value(m_value), content_type::comp_value(x)) ? x :
                m_value,
            m_comp.pointer());
    }
};
    
    
/** Create a rhs_proxy.
 */
template <typename View>
static rhs_proxy<View> make_proxy(const typename View::value_type& value, const View& view)
{
    return rhs_proxy<View>(&view, value, view.compare_pointer());
}

//@}

/** Base class for min and max view classes.
 *
 *  This class accumulates the minimum or maximum of a set of values which have occurred as
 *  arguments to the `calc()` function, as determined by a comparator. The accumulated value
 *  will be the first `calc()` argument value `x` such that `compare(x, y)` is false for every
 *  `calc()` argument value `y`.
 *
 *  If the comparator is `std::less`, then the accumulated value is the first argument value
 *  which is not less than any other argument value, i.e., the maximum. Similarly, if the 
 *  comparator is `reverse_predicate<std::less>`, which is equivalent to `std::greater`, then
 *  the accumulated value is the first argument value which is not greater than any other
 *  argument value, i.e., the minimum.
 *  
 *  @note   This class provides the definitions that are required for a class that will be
 *          used as the parameter of a @ref monoid_base specialization. 
 *
 *  @tparam Content     A content class that provides the value types and data members for the
 *                      view.
 *  @tparam Less        A “less than” binary predicate that defines the min or max function.
 *  @tparam Compare     A binary predicate to be used to compare the values. (The same as
 *                      @a Less for max reducers; its reversal for min reducers.)
 *
 *  @see view_content_classes
 *  @see op_max_view
 *  @see op_min_view
 *  @see op_max_index_view
 *  @see op_min_index_view
 */
template <typename Content, typename Less, typename Compare>
class view_base : 
    // comparator_base comes first to ensure that it will get empty base class treatment
    private comparator_base<typename Content::comp_value_type, Compare>, 
    private Content
{
    typedef comparator_base<typename Content::comp_value_type, Compare> base;
    using base::compare;
    using Content::value;
    using Content::set_value;
    using Content::comp_value;
    typedef Content content_type;
    
    template <typename View> friend class rhs_proxy;
    template <typename View>
    friend rhs_proxy<View> make_proxy(const typename View::value_type& value, const View& view);
    
public:
    
    /** @name Monoid support.
     */
    //@{
    
    /** Value type. Required by @ref monoid_with_view.
     */
    typedef typename Content::value_type    value_type;
    
    /** The type of the comparator specified by the user, that defines the ordering on @a Type.
     *  Required by min_max::monoid_base.
     */
    typedef Less                            less_type;
    
    /** The type of the comparator actually used by the view. Required by min_max::monoid_base.
     *  (This is the same as the @ref less_type for a max reducer, or 
     *  `reverse_predicate<less_type>` for a min reducer.)
     */
    typedef Compare                         compare_type;

    /** Reduce operation. Required by @ref monoid_with_view.
     */
    void reduce(view_base* other)
    {
        if (    other->is_set() &&
                (!this->is_set() || compare(this->comp_value(), other->comp_value()) ) )
        {
            this->set_value(other->value());
        }
    }
    
    //@}
    
    /** Default constructor. Initializes to identity value.
     */
    explicit view_base(const compare_type* compare) : base(compare), Content() {}
    
    /** Value constructor.
     */
    template <typename T1>
    view_base(const T1& x1, const compare_type* compare) : 
        base(compare), Content(x1) {}

    /** Value constructor.
     */
    template <typename T1, typename T2>
    view_base(const T1& x1, const T2& x2, const compare_type* compare) : 
        base(compare), Content(x1, x2) {}


    /** Move-in constructor.
     */
    explicit view_base(move_in_wrapper<value_type> w, const compare_type* compare) :
        base(compare), Content(w.value()) {}
    
    /** @name @ref reducer support.
     */
    //@{
    
    void                view_move_in(value_type& v)         { set_value(v); }
    void                view_move_out(value_type& v)        { v = value(); }
    void                view_set_value(const value_type& v) { set_value(v); }
    value_type          view_get_value() const              { return value(); }
    //                  view_get_reference()                NOT SUPPORTED
    
    //@}
    
    /** Is the value defined?
     */
    using Content::is_set;
    
    /** Reference to contained value data member.
     *  @deprecated For legacy reducers only.
     */
    using Content::get_reference;
    
    /** Reference to contained index data member.
     *  (Meaningless for non-index reducers.)
     *  @deprecated For legacy reducers only.
     */
    using Content::get_index_reference;
    
protected:

    /** Update the min/max value.
     */
    void calc(const value_type& x)
    {
        if (!is_set() || compare(comp_value(), comp_value(x))) set_value(x);
    }
    
    /** Assign the result of a `{min|max}_of(view, value)` expression to the view.
     *
     *  @see rhs_proxy
     */
    template <typename View>
    void assign(const rhs_proxy<View>& rhs)
    {
        calc(rhs.value(this));
    }
    
};


/** Base class for min and max monoid classes.
 *
 *  The unique characteristic of minimum and maximum reducers is that they incorporate a
 *  comparator functor that defines what “minimum” or “maximum” means. The monoid for a reducer
 *  contains the comparator that will be used for the reduction. If the comparator is a function
 *  or a class with state, then each view will have a pointer to the comparator.
 *
 *  This means that the `construct()` functions first construct the monoid (possibly with an
 *  explicit comparator argument), and then construct the view with a pointer to the monoid’s
 *  comparator.
 *
 *  @tparam View    The view class.
 *  @tparam Align   If true, reducers instantiated on this monoid will be
 *                  aligned. By default, library reducers (unlike legacy
 *                  library reducer _wrappers_) are unaligned.
 *
 *  @see view_base
 */
template <typename View, bool Align = false>
class monoid_base : public monoid_with_view<View, Align>
{
    typedef typename View::compare_type compare_type;
    typedef typename View::less_type    less_type;
    const compare_type                  m_compare;

    const compare_type*                 compare_pointer() const { return &m_compare; }
    
    using cilk::monoid_base<typename View::value_type, View>::provisional;
    
public:

    /** Default constructor uses default comparator.
     */
    monoid_base() : m_compare() {}

    /** Constructor.
     *
     *  @param  compare The comparator to use.
     */
    monoid_base(const compare_type& compare) : m_compare(compare) {}

    /** Create an identity view.
     *
     *  List view identity constructors take the list allocator as an argument.
     *
     *  @param v    The address of the uninitialized memory in which the view will be
     *  constructed.
     */
    void identity(View *v) const { ::new((void*) v) View(compare_pointer()); }
    
    /** @name construct functions
     *
     *  Min/max monoid `construct()` functions optionally take one or two value arguments, a
     *  @ref move_in argument, and/or a comparator argument.
     */
    //@{
    
    template <typename Monoid>
    static void construct(Monoid* monoid, View* view)
        { provisional( new ((void*)monoid) Monoid() ).confirm_if( 
            new ((void*)view) View(monoid->compare_pointer()) ); }

    template <typename Monoid, typename T1>
    static void construct(Monoid* monoid, View* view, const T1& x1)
        { provisional( new ((void*)monoid) Monoid() ).confirm_if( 
            new ((void*)view) View(x1, monoid->compare_pointer()) ); }

    template <typename Monoid, typename T1, typename T2>
    static void construct(Monoid* monoid, View* view, const T1& x1, const T2& x2)
        { provisional( new ((void*)monoid) Monoid() ).confirm_if( 
            new ((void*)view) View(x1, x2, monoid->compare_pointer()) ); }

    template <typename Monoid>
    static void construct(Monoid* monoid, View* view, const less_type& compare)
        { provisional( new ((void*)monoid) Monoid(compare) ).confirm_if( 
            new ((void*)view) View(monoid->compare_pointer()) ); }

    template <typename Monoid, typename T1>
    static void construct(Monoid* monoid, View* view, const T1& x1, const less_type& compare)
        { provisional( new ((void*)monoid) Monoid(compare) ).confirm_if( 
            new ((void*)view) View(x1, monoid->compare_pointer()) ); }

    template <typename Monoid, typename T1, typename T2>
    static void construct(Monoid* monoid, View* view, const T1& x1, const T2& x2, const less_type& compare)
        { provisional( new ((void*)monoid) Monoid(compare) ).confirm_if( 
            new ((void*)view) View(x1, x2, monoid->compare_pointer()) ); }

    //@}
};

} //namespace min_max
} // namespace internal


/** The maximum reducer view class.
 *
 *  This is the view class for reducers created with `cilk::reducer< cilk::op_max<Type,
 *  Compare> >`. It accumulates the maximum, as determined by a comparator, of a set of values
 *  which have occurred as arguments to the `calc_max()` function. The accumulated value
 *  will be the first argument `x` such that `compare(x, y)` is false for every argument `y`.
 *
 *  If the comparator is `std::less`, then the accumulated value is the first argument value
 *  which is not less than any other argument value, i.e., the maximum.
 *
 *  @note   The reducer “dereference” operation (`reducer::operator *()`) yields a reference 
 *          to the view. Thus, for example, the view class’s `calc_max()` function would be
 *          used in an expression like `r->calc_max(a)` where `r` is an op_max reducer variable.
 *
 *  @tparam Type    The type of the values compared by the reducer. This will be the value type
 *                  of a monoid_with_view that is instantiated with this view.
 *  @tparam Compare A `Strict Weak Ordering` whose argument type is @a Type. It defines the
 *                  “less than” relation used to compute the maximum.
 *
 *  @see @ref page_reducer_min_max
 *  @see op_max
 */
template <typename Type, typename Compare>
class op_max_view : public internal::min_max::view_base<
    internal::min_max::view_content<Type, Compare, true>, 
    Compare,
    Compare>
{
    typedef internal::min_max::view_base<
        internal::min_max::view_content<Type, Compare, true>, 
        Compare,
        Compare> base;
    using base::calc;
    using base::assign;
    friend class internal::min_max::rhs_proxy<op_max_view>;
    
public:

    /** @name Constructors.
     *
     *  All op_max_view constructors simply pass their arguments on to the 
     *  @ref view_base base class.
     */
    //@{
    
    op_max_view() : base() {}
    
    template <typename T1>
    op_max_view(const T1& x1) : base(x1) {}
    
    template <typename T1, typename T2>
    op_max_view(const T1& x1, const T2& x2) : base(x1, x2) {}
    
    //@}    

    /** @name View modifier operations.
     */
    //@{
    
    /** Maximize with a value.
     *
     *  If @a x is greater than the current value of the view (as defined by the reducer’s
     *  comparator), or if the view was created without an initial value and its value has never
     *  been updated (with `calc_max()` or `= max_of()`), then the value of the view is set 
     *  to @a x.
     *
     *  @param  x   The value to maximize the view’s value with.
     *
     *  @return     A reference to the view. (Allows `view.comp_max(a).comp_max(b)…`.)
     */
    op_max_view& calc_max(const Type& x) { calc(x); return *this; }

    /** Assign the result of a `max_of(view, value)` expression to the view.
     *
     *  @param  rhs An rhs_proxy value created by a `max_of(view, value)` expression.
     *
     *  @return     A reference to the view.
     *
     *  @see internal::min_max::view_base::rhs_proxy
     */
    op_max_view& operator=(const internal::min_max::rhs_proxy<op_max_view>& rhs) 
        { assign(rhs); return *this; }
};


/** Compute the maximum of the value in a view and another value.
 *
 *  The result of this computation can only be assigned back to the original view or
 *  used in another max_of() call. For example,
 *
 *      *reducer = max_of(*reducer, x);
 *      *reducer = max_of(x, *reducer);
 *
 *  @see internal::min_max::max_max_view_base::rhs_proxy
 */
//@{
template <typename Type, typename Compare>
inline internal::min_max::rhs_proxy< op_max_view<Type, Compare> >
max_of(const op_max_view<Type, Compare>& view, const Type& value)
{
    return internal::min_max::make_proxy(value, view);
}

template <typename Type, typename Compare>
inline internal::min_max::rhs_proxy< op_max_view<Type, Compare> >
max_of(const Type& value, const op_max_view<Type, Compare>& view)
{
    return internal::min_max::make_proxy(value, view);
}
//@}

/** Nested computation of the maximum of the value in a view and other values.
 *
 *  Compute the maximum of the result of a max_of() call and another value.
 *
 *  The result of this computation can only be assigned back to the original view or
 *  used in another max_of() call. For example,
 *
 *      *reducer = max_of(x, max_of(y, *reducer));
 *      *reducer = max_of(max_of(*reducer, x), y);
 *
 *  @see internal::min_max::max_max_view_base::rhs_proxy
 */
//@{
template <typename Type, typename Compare>
inline internal::min_max::rhs_proxy< op_max_view<Type, Compare> >
max_of(const internal::min_max::rhs_proxy< op_max_view<Type, Compare> >& proxy, 
       const Type& value)
{
    return proxy.calc(value);
}

template <typename Type, typename Compare>
inline internal::min_max::rhs_proxy< op_max_view<Type, Compare> >
max_of(const Type& value, 
       const internal::min_max::rhs_proxy< op_max_view<Type, Compare> >& proxy)
{
    return proxy.calc(value);
}
//@}


/** Monoid class for maximum reductions. Instantiate the cilk::reducer template class
 *  with an op_max monoid to create a maximum reducer class. For example, to compute
 *  the maximum of a set of `int` values:
 *
 *      cilk::reducer< cilk::op_max<int> > r;
 *
 *  @see @ref page_reducer_min_max
 *  @see op_max_view
 */
template <typename Type, typename Compare=std::less<Type>, bool Align = false>
class op_max : public internal::min_max::monoid_base<op_max_view<Type, Compare>, Align> {
    typedef internal::min_max::monoid_base<op_max_view<Type, Compare>, Align> base;
public:
    /// Construct with default comparator.
    op_max() {}
    /// Construct with specified comparator.
    op_max(const Compare& compare) : base(compare) {}
};


/** The minimum reducer view class.
 *
 *  This is the view class for reducers created with `cilk::reducer< cilk::op_min<Type,
 *  Compare> >`. It accumulates the minimum, as determined by a comparator, of a set of values
 *  which have occurred as arguments to the `calc_min()` function. The accumulated value
 *  will be the first argument `x` such that `compare(y, x)` is false for every argument `y`.
 *
 *  If the comparator is `std::less`, then the accumulated value is the first argument value
 *  which no other argument value is less than, i.e., the minimum.
 *
 *  @note   The reducer “dereference” operation (`reducer::operator *()`) yields a reference 
 *          to the view. Thus, for example, the view class’s `calc_min()` function would be
 *          used in an expression like `r->calc_min(a)` where `r` is an op_min reducer variable.
 *
 *  @tparam Type    The type of the values compared by the reducer. This will be the value type
 *                  of a monoid_with_view that is instantiated with this view.
 *  @tparam Compare A `Strict Weak Ordering` whose argument type is @a Type. It defines the
 *                  “less than” relation used to compute the minimum.
 *
 *  @see @ref page_reducer_min_max
 *  @see op_min
 */
template <typename Type, typename Compare>
class op_min_view : public internal::min_max::view_base<
    internal::min_max::view_content<Type, Compare, false>, 
    Compare,
    internal::min_max::reverse_predicate<Compare, Type> >
{
    typedef internal::min_max::view_base<
        internal::min_max::view_content<Type, Compare, false>, 
        Compare,
        internal::min_max::reverse_predicate<Compare, Type> > base;
    using base::calc;
    using base::assign;
    friend class internal::min_max::rhs_proxy<op_min_view>;

public:
    /** @name Constructors.
     *
     *  All op_min_view constructors simply pass their arguments on to the 
     *  @ref view_base base class.
     */
    //@{
    
    op_min_view() : base() {}
    
    template <typename T1>
    op_min_view(const T1& x1) : base(x1) {}
    
    template <typename T1, typename T2>
    op_min_view(const T1& x1, const T2& x2) : base(x1, x2) {}
    
    //@}    

    /** @name View modifier operations.
     */
    //@{
    
    /** Minimize with a value.
     *
     *  If @a x is less than the current value of the view (as defined by the reducer’s
     *  comparator), or if the view was created without an initial value and its value has never
     *  been updated (with `calc_min()` or `= min_of()`), then the value of the view is set 
     *  to @a x.
     *
     *  @param  x   The value to minimize the view’s value with.
     *
     *  @return     A reference to the view. (Allows `view.comp_min(a).comp_min(b)…`.)
     */
    op_min_view& calc_min(const Type& x) { calc(x); return *this; }

    /** Assign the result of a `min_of(view, value)` expression to the view.
     *
     *  @param  rhs An rhs_proxy value created by a `min_of(view, value)` expression.
     *
     *  @return     A reference to the view.
     *
     *  @see internal::min_max::view_base::rhs_proxy
     */
    op_min_view& operator=(const internal::min_max::rhs_proxy<op_min_view>& rhs) 
        { assign(rhs); return *this; }
};


/** Compute the minimum of the value in a view and another value.
 *
 *  The result of this computation can only be assigned back to the original view or
 *  used in another min_of() call. For example,
 *
 *      *reducer = min_of(*reducer, x);
 *      *reducer = min_of(x, *reducer);
 *
 *  @see internal::min_max::view_base::rhs_proxy
 */
//@{
template <typename Type, typename Compare>
inline internal::min_max::rhs_proxy< op_min_view<Type, Compare> >
min_of(const op_min_view<Type, Compare>& view, const Type& value)
{
    return internal::min_max::make_proxy(value, view);
}

template <typename Type, typename Compare>
inline internal::min_max::rhs_proxy< op_min_view<Type, Compare> >
min_of(const Type& value, const op_min_view<Type, Compare>& view)
{
    return internal::min_max::make_proxy(value, view);
}
//@}

/** Nested computation of the minimum of the value in a view and other values.
 *
 *  Compute the minimum of the result of a min_of() call and another value.
 *
 *  The result of this computation can only be assigned back to the original view or
 *  used in another min_of() call. For example,
 *
 *      *reducer = min_of(x, min_of(y, *reducer));
 *      *reducer = min_of(min_of(*reducer, x), y);
 *
 *  @see internal::min_max::view_base::rhs_proxy
 */
//@{
template <typename Type, typename Compare>
inline internal::min_max::rhs_proxy< op_min_view<Type, Compare> >
min_of(const internal::min_max::rhs_proxy< op_min_view<Type, Compare> >& proxy, 
       const Type& value)
{
    return proxy.calc(value);
}

template <typename Type, typename Compare>
inline internal::min_max::rhs_proxy< op_min_view<Type, Compare> >
min_of(const Type& value, 
       const internal::min_max::rhs_proxy< op_min_view<Type, Compare> >& proxy)
{
    return proxy.calc(value);
}
//@}


/** Monoid class for minimum reductions. Instantiate the cilk::reducer template class
 *  with an op_min monoid to create a minimum reducer class. For example, to compute
 *  the minimum of a set of `int` values:
 *
 *      cilk::reducer< cilk::op_min<int> > r;
 *
 *  @see @ref page_reducer_min_max
 *  @see op_min_view
 */
template <typename Type, typename Compare=std::less<Type>, bool Align = false>
class op_min : public internal::min_max::monoid_base<op_min_view<Type, Compare>, Align> {
    typedef internal::min_max::monoid_base<op_min_view<Type, Compare>, Align> base;
public:
    /// Construct with default comparator.
    op_min() {}
    /// Construct with specified comparator.
    op_min(const Compare& compare) : base(compare) {}
};


/** The maximum index reducer view class.
 *
 *  This is the view class for reducers created with `cilk::reducer< cilk::op_max_index<Index,
 *  Type, Compare> >`. It accumulates the maximum, as determined by a comparator, of a set of
 *  values which have occurred as arguments to the `calc_max()` function, and records the index
 *  of the maximum value. The accumulated value will be the first argument `x` such that
 *  `compare(x, y)` is false for every argument `y`.
 *
 *  If the comparator is `std::less`, then the accumulated value is the first argument value
 *  which is not less than any other argument value, i.e., the maximum.
 *
 *  @note   The reducer “dereference” operation (`reducer::operator *()`) yields a reference 
 *          to the view. Thus, for example, the view class’s `calc_max()` function would be
 *          used in an expression like `r->calc_max(i, a)`where `r` is an op_max_index reducer
 *          variable.
 *
 *  @note   The word “index” suggests an integer index into an array, but there is no
 *          restriction on the index type or how it should be used. In general, it may be
 *          convenient to use it for any kind of key that can be used to locate the maximum 
 *          value in the collection that it came from — for example:
 *              -   An index into an array.
 *              -   A key into an STL map.
 *              -   An iterator into any STL container.
 *
 *  @note   A max_index reducer is essentially a max reducer whose value type is a
 *          `std::pair<Index, Type>`. This fact is camouflaged in the view `calc_max` function,
 *          the global `max_of` functions, and the reducer value constructor, which can all take
 *          an index argument and a value argument as an alternative to a single `std::pair`
 *          argument. However, the reducer `set_value()`, `get_value()`, `move_in()`, and
 *          `move_out()` functions work only with pairs, not with individual value
 *          and/or index arguments.
 *
 *  @tparam Index   The type of the indices associated with the values.
 *  @tparam Type    The type of the values compared by the reducer. This will be the value type
 *                  of a monoid_with_view that is instantiated with this view.
 *  @tparam Compare Used to compare the values. It must be a binary predicate. If it is omitted,
 *                  then the view computes the conventional arithmetic maximum.
 *
 *  @see @ref page_reducer_min_max
 *  @see op_max_index
 */
template <typename Index, typename Type, typename Compare>
class op_max_index_view : public internal::min_max::view_base<
    internal::min_max::index_view_content<Index, Type, Compare, true>,
    Compare,
    Compare>
{
    typedef internal::min_max::view_base<
        internal::min_max::index_view_content<Index, Type, Compare, true>,
        Compare,
        Compare> base;
    using base::calc;
    using base::assign;
    typedef std::pair<Index, Type> pair_type;
    friend class internal::min_max::rhs_proxy<op_max_index_view>;

public:
    /** @name Constructors.
     *
     *  All op_max_index_view constructors simply pass their arguments on to the 
     *  @ref view_base base class, except for the `(index, value [, compare])`
     *  constructors, which create a `std::pair` containing the index and value.
     */
    //@{
    
    op_max_index_view() : base() {}
    
    template <typename T1>
    op_max_index_view(const T1& x1) : base(x1) {}
    
    template <typename T1, typename T2>
    op_max_index_view(const T1& x1, const T2& x2) : base(x1, x2) {}
    
    template <typename T1, typename T2, typename T3>
    op_max_index_view(const T1& x1, const T2& x2, const T3& x3) : base(x1, x2, x3) {}
    
    op_max_index_view(const Index& i, const Type& v) : base(pair_type(i, v)) {}
    
    op_max_index_view(const Index& i, const Type& v, const typename base::compare_type* c) : 
        base(pair_type(i, v), c) {}
    
    //@}    

    /** Maximize with a value and index.
     *
     *  If @a x is greater than the current value of the view (as defined by the reducer’s
     *  comparator), or if the view was created without an initial value and its value has never
     *  been updated (with `calc_max()` or `= max_of()`), then the value of the view is set 
     *  to @a x, and the index is set to @a i..
     *
     *  @param  i   The index of the value @a x.
     *  @param  x   The value to maximize the view’s value with.
     *
     *  @return     A reference to the view. (Allows `view.comp_max(i, a).comp_max(j, b)…`.)
     */
    op_max_index_view& calc_max(const Index& i, const Type& x) 
        { calc(pair_type(i, x)); return *this; }

    /** Maximize with an index/value pair.
     *
     *  If @a x is greater than the current value of the view (as defined by the reducer’s
     *  comparator), or if the view was created without an initial value and its value has never
     *  been updated (with `calc_max()` or `= max_of()`), then the value of the view is set 
     *  to @a x, and the index is set to @a i..
     *
     *  @param  pair    A pair containing a value to maximize the view’s value with and its 
     *                  associated index.
     *
     *  @return     A reference to the view. (Allows `view.comp_max(p1).comp_max(p2)…`.)
     */
    op_max_index_view& calc_max(const pair_type& pair) 
        { calc(pair); return *this; }

    /** Assign the result of a `max_of(view, index, value)` expression to the view.
     *
     *  @param  rhs An rhs_proxy value created by a `max_of(view, index, value)` expression.
     *
     *  @return     A reference to the view.
     *
     *  @see internal::min_max::view_base::rhs_proxy
     */
    op_max_index_view& operator=(const internal::min_max::rhs_proxy<op_max_index_view>& rhs) 
        { assign(rhs); return *this; }
};


/** Compute the maximum of the value in a view and another value.
 *
 *  The result of this computation can only be assigned back to the original view or
 *  used in another max_of() call. For example,
 *
 *      *reducer = max_of(*reducer, i, x);
 *      *reducer = max_of(i, x, *reducer);
 *
 *  @see internal::min_max::max_max_view_base::rhs_proxy
 */
//@{
template <typename Index, typename Type, typename Compare>
inline internal::min_max::rhs_proxy< op_max_index_view<Index, Type, Compare> >
max_of(const op_max_index_view<Index, Type, Compare>& view,
       const Index& index, const Type& value)
{
    return internal::min_max::make_proxy(std::pair<Index, Type>(index, value), view);
}

template <typename Index, typename Type, typename Compare>
inline internal::min_max::rhs_proxy< op_max_index_view<Index, Type, Compare> >
max_of(const Index& index, const Type& value,
       const op_max_index_view<Index, Type, Compare>& view)
{
    return internal::min_max::make_proxy(std::pair<Index, Type>(index, value), view);
}

template <typename Index, typename Type, typename Compare>
inline internal::min_max::rhs_proxy< op_max_index_view<Index, Type, Compare> >
max_of(const op_max_index_view<Index, Type, Compare>& view,
       const std::pair<Index, Type>& pair)
{
    return internal::min_max::make_proxy(pair, view);
}

template <typename Index, typename Type, typename Compare>
inline internal::min_max::rhs_proxy< op_max_index_view<Index, Type, Compare> >
max_of(const std::pair<Index, Type>& pair,
       const op_max_index_view<Index, Type, Compare>& view)
{
    return internal::min_max::make_proxy(pair, view);
}
//@}

/** Nested computation of the maximum of the value in a view and other values.
 *
 *  Compute the maximum of the result of a max_of() call and another value.
 *
 *  The result of this computation can only be assigned back to the original view or
 *  used in another max_of() call. For example,
 *
 *      *reducer = max_of(x, max_of(y, *reducer));
 *      *reducer = max_of(max_of(*reducer, x), y);
 *
 *  @see internal::min_max::max_max_view_base::rhs_proxy
 */
//@{
template <typename Index, typename Type, typename Compare>
inline internal::min_max::rhs_proxy< op_max_index_view<Index, Type, Compare> >
max_of(const internal::min_max::rhs_proxy< op_max_index_view<Index, Type, Compare> >& proxy,
       const Index& index, const Type& value)
{
    return proxy.calc(std::pair<Index, Type>(index, value));
}

template <typename Index, typename Type, typename Compare>
inline internal::min_max::rhs_proxy< op_max_index_view<Index, Type, Compare> >
max_of(const Index& index, const Type& value,
       const internal::min_max::rhs_proxy< op_max_index_view<Index, Type, Compare> >& proxy)
{
    return proxy.calc(std::pair<Index, Type>(index, value));
}

template <typename Index, typename Type, typename Compare>
inline internal::min_max::rhs_proxy< op_max_index_view<Index, Type, Compare> >
max_of(const internal::min_max::rhs_proxy< op_max_index_view<Index, Type, Compare> >& proxy,
       const std::pair<Index, Type>& pair)
{
    return proxy.calc(pair);
}

template <typename Index, typename Type, typename Compare>
inline internal::min_max::rhs_proxy< op_max_index_view<Index, Type, Compare> >
max_of(const std::pair<Index, Type>& pair,
       const internal::min_max::rhs_proxy< op_max_index_view<Index, Type, Compare> >& proxy)
{
    return proxy.calc(pair);
}
//@}


/** Monoid class for maximum reductions with index. Instantiate the cilk::reducer template class
 *  with an op_max_index monoid to create a max_index reducer class. For example, to compute
 *  the maximum of a set of `int` values and the index of the max value:
 *
 *      cilk::reducer< cilk::op_max_index<unsigned, int> > r;
 *
 *  @see @ref page_reducer_min_max
 *  @see op_max_index_view
 */
template < typename Index, typename Type, typename Compare=std::less<Type>, bool Align = false>
class op_max_index : public internal::min_max::monoid_base<op_max_index_view<Index, Type, Compare>, Align> {
    typedef internal::min_max::monoid_base<op_max_index_view<Index, Type, Compare>, Align> base;
public:
    /// Construct with default comparator.
    op_max_index() {}
    /// Construct with specified comparator.
    op_max_index(const Compare& compare) : base(compare) {}
};



/** The minimum index reducer view class.
 *
 *  This is the view class for reducers created with `cilk::reducer< cilk::op_min_index<Index,
 *  Type, Compare> >`. It accumulates the minimum, as determined by a comparator, of a set of
 *  values which have occurred as arguments to the `calc_min()` function, and records the index
 *  of the minimum value. The accumulated value will be the first argument `x` such that
 *  `compare(y, x)` is false for every argument `y`.
 *
 *  If the comparator is `std::less`, then the accumulated value is the first argument value
 *  which no other argument value is less than, i.e., the minimum.
 *
 *  @note   The reducer “dereference” operation (`reducer::operator *()`) yields a reference 
 *          to the view. Thus, for example, the view class’s `calc_min()` function would be
 *          used in an expression like `r->calc_min(i, a)`where `r` is an op_min_index reducer
 *          variable.
 *
 *  @note   The word “index” suggests an integer index into an array, but there is no
 *          restriction on the index type or how it should be used. In general, it may be
 *          convenient to use it for any kind of key that can be used to locate the minimum 
 *          value in the collection that it came from — for example:
 *              -   An index into an array.
 *              -   A key into an STL map.
 *              -   An iterator into any STL container.
 *
 *  @note   A min_index reducer is essentially a min reducer whose value type is a
 *          `std::pair<Index, Type>`. This fact is camouflaged in the view `calc_min` function,
 *          the global `min_of` functions, and the reducer value constructor, which can all take
 *          an index argument and a value argument as an alternative to a single `std::pair`
 *          argument. However, the reducer `set_value()`, `get_value()`, `move_in()`, and
 *          `move_out()` functions work only with pairs, not with individual value
 *          and/or index arguments.
 *
 *  @tparam Index   The type of the indices associated with the values.
 *  @tparam Type    The type of the values compared by the reducer. This will be the value type
 *                  of a monoid_with_view that is instantiated with this view.
 *  @tparam Compare Used to compare the values. It must be a binary predicate. If it is omitted,
 *                  then the view computes the conventional arithmetic minimum.
 *
 *  @see @ref page_reducer_min_max
 *  @see op_min_index
 */
template <typename Index, typename Type, typename Compare>
class op_min_index_view : public internal::min_max::view_base<
    internal::min_max::index_view_content<Index, Type, Compare, false>,
    Compare,
    internal::min_max::reverse_predicate<Compare, Type> >
{
    typedef internal::min_max::view_base<
        internal::min_max::index_view_content<Index, Type, Compare, false>,
        Compare,
        internal::min_max::reverse_predicate<Compare, Type> > base;
    using base::calc;
    using base::assign;
    typedef std::pair<Index, Type> pair_type;
    friend class internal::min_max::rhs_proxy<op_min_index_view>;

public:
    /** @name Constructors.
     *
     *  All op_min_index_view constructors simply pass their arguments on to the 
     *  @ref view_base base class, except for the `(index, value [, compare])`
     *  constructors, which create a `std::pair` containing the index and value.
     */
    //@{
    
    op_min_index_view() : base() {}
    
    template <typename T1>
    op_min_index_view(const T1& x1) : base(x1) {}
    
    template <typename T1, typename T2>
    op_min_index_view(const T1& x1, const T2& x2) : base(x1, x2) {}
    
    template <typename T1, typename T2, typename T3>
    op_min_index_view(const T1& x1, const T2& x2, const T3& x3) : base(x1, x2, x3) {}
    
    op_min_index_view(const Index& i, const Type& v) : base(pair_type(i, v)) {}
    
    op_min_index_view(const Index& i, const Type& v, const typename base::compare_type* c) : 
        base(pair_type(i, v), c) {}
    
    //@}    

    /** Minimize with a value and index.
     *
     *  If @a x is greater than the current value of the view (as defined by the reducer’s
     *  comparator), or if the view was created without an initial value and its value has never
     *  been updated (with `calc_min()` or `= min_of()`), then the value of the view is set 
     *  to @a x, and the index is set to @a i..
     *
     *  @param  i   The index of the value @a x.
     *  @param  x   The value to minimize the view’s value with.
     *
     *  @return     A reference to the view. (Allows `view.comp_min(i, a).comp_min(j, b)…`.)
     */
    op_min_index_view& calc_min(const Index& i, const Type& x) 
        { calc(pair_type(i, x)); return *this; }

    /** Maximize with an index/value pair.
     *
     *  If @a x is greater than the current value of the view (as defined by the reducer’s
     *  comparator), or if the view was created without an initial value and its value has never
     *  been updated (with `calc_min()` or `= min_of()`), then the value of the view is set 
     *  to @a x, and the index is set to @a i..
     *
     *  @param  pair    A pair containing a value to minimize the view’s value with and its 
     *                  associated index.
     *
     *  @return     A reference to the view. (Allows `view.comp_min(p1).comp_min(p2)…`.)
     */
    op_min_index_view& calc_min(const pair_type& pair) 
        { calc(pair); return *this; }

    /** Assign the result of a `min_of(view, index, value)` expression to the view.
     *
     *  @param  rhs An rhs_proxy value created by a `min_of(view, index, value)` expression.
     *
     *  @return     A reference to the view.
     *
     *  @see internal::min_max::view_base::rhs_proxy
     */
    op_min_index_view& operator=(const internal::min_max::rhs_proxy<op_min_index_view>& rhs) 
        { assign(rhs); return *this; }
};


/** Compute the minimum of the value in a view and another value.
 *
 *  The result of this computation can only be assigned back to the original view or
 *  used in another min_of() call. For example,
 *
 *      *reducer = min_of(*reducer, i, x);
 *      *reducer = min_of(i, x, *reducer);
 *
 *  @see internal::min_max::min_min_view_base::rhs_proxy
 */
//@{
template <typename Index, typename Type, typename Compare>
inline internal::min_max::rhs_proxy< op_min_index_view<Index, Type, Compare> >
min_of(const op_min_index_view<Index, Type, Compare>& view,
       const Index& index, const Type& value)
{
    return internal::min_max::make_proxy(std::pair<Index, Type>(index, value), view);
}

template <typename Index, typename Type, typename Compare>
inline internal::min_max::rhs_proxy< op_min_index_view<Index, Type, Compare> >
min_of(const Index& index, const Type& value,
       const op_min_index_view<Index, Type, Compare>& view)
{
    return internal::min_max::make_proxy(std::pair<Index, Type>(index, value), view);
}

template <typename Index, typename Type, typename Compare>
inline internal::min_max::rhs_proxy< op_min_index_view<Index, Type, Compare> >
min_of(const op_min_index_view<Index, Type, Compare>& view,
       const std::pair<Index, Type>& pair)
{
    return internal::min_max::make_proxy(pair, view);
}

template <typename Index, typename Type, typename Compare>
inline internal::min_max::rhs_proxy< op_min_index_view<Index, Type, Compare> >
min_of(const std::pair<Index, Type>& pair,
       const op_min_index_view<Index, Type, Compare>& view)
{
    return internal::min_max::make_proxy(pair, view);
}
//@}

/** Nested computation of the minimum of the value in a view and other values.
 *
 *  Compute the minimum of the result of a min_of() call and another value.
 *
 *  The result of this computation can only be assigned back to the original view or
 *  used in another min_of() call. For example,
 *
 *      *reducer = min_of(x, min_of(y, *reducer));
 *      *reducer = min_of(min_of(*reducer, x), y);
 *
 *  @see internal::min_max::min_min_view_base::rhs_proxy
 */
//@{
template <typename Index, typename Type, typename Compare>
inline internal::min_max::rhs_proxy< op_min_index_view<Index, Type, Compare> >
min_of(const internal::min_max::rhs_proxy< op_min_index_view<Index, Type, Compare> >& proxy,
       const Index& index, const Type& value)
{
    return proxy.calc(std::pair<Index, Type>(index, value));
}

template <typename Index, typename Type, typename Compare>
inline internal::min_max::rhs_proxy< op_min_index_view<Index, Type, Compare> >
min_of(const Index& index, const Type& value,
       const internal::min_max::rhs_proxy< op_min_index_view<Index, Type, Compare> >& proxy)
{
    return proxy.calc(std::pair<Index, Type>(index, value));
}

template <typename Index, typename Type, typename Compare>
inline internal::min_max::rhs_proxy< op_min_index_view<Index, Type, Compare> >
min_of(const internal::min_max::rhs_proxy< op_min_index_view<Index, Type, Compare> >& proxy,
       const std::pair<Index, Type>& pair)
{
    return proxy.calc(pair);
}

template <typename Index, typename Type, typename Compare>
inline internal::min_max::rhs_proxy< op_min_index_view<Index, Type, Compare> >
min_of(const std::pair<Index, Type>& pair,
       const internal::min_max::rhs_proxy< op_min_index_view<Index, Type, Compare> >& proxy)
{
    return proxy.calc(pair);
}
//@}


/** Monoid class for minimum reductions with index. Instantiate the cilk::reducer template class
 *  with an op_min_index monoid to create a min_index reducer class. For example, to compute
 *  the minimum of a set of `int` values and the index of the min value:
 *
 *      cilk::reducer< cilk::op_min_index<unsigned, int> > r;
 *
 *  @see @ref page_reducer_min_max
 *  @see op_min_index_view
 */
template <typename Index, typename Type, typename Compare=std::less<Type>, bool Align = false>
class op_min_index : public internal::min_max::monoid_base<op_min_index_view<Index, Type, Compare>, Align> {
    typedef internal::min_max::monoid_base<op_min_index_view<Index, Type, Compare>, Align> base;
public:
    /// Construct with default comparator.
    op_min_index() {}
    /// Construct with specified comparator.
    op_min_index(const Compare& compare) : base(compare) {}
};



/** Macro test for binary compatibility.
 *
 *  If the macro CILK_LIBRARY_0_9_REDUCER_MINMAX is defined, then we generate reducer code and
 *  data structures which are binary-compatible with code that was compiled with the old min/max
 *  reducer definitions, so we want the mangled names of the legacy REDUCER_{MIN_MAX}[_INDEX]
 *  classes to be the same as the names produced by the old definitions.
 *
 *  Conversely, if the macro is not defined, then we generate binary-incompatible code, so we
 *  want different mangled names, to make sure that the linker does not allow new and old
 *  compiled legacy reducers to be passed to one another. (Global variables are a different, and
 *  probably insoluble, problem.)
 *
 *  The trick is to add an additional parameter to the reducer’s template parameter list to
 *  give it a different mangled name.
 *
 *  @ingroup min_max_is_set_optimization
 */
#ifdef CILK_LIBRARY_0_9_REDUCER_MINMAX
#   define __CILKRTS_MANGLE_DIFFERENT
# else
#   define __CILKRTS_MANGLE_DIFFERENT , bool Compatible_0_9 = true
#endif

/** Deprecated maximum reducer class.
 *
 *  reducer_max\<Type\> is the same as @ref cilk::reducer< @ref op_max\<Type\> >, except that 
 *  reducer_max is a proxy for the contained view, so that accumulator variable update 
 *  operations can be applied directly to the reducer. For example, where a `reducer<op_max>`
 *  is updated using `r->calc_max(a)`, you increment a reducer_max with `r.calc_max(a)`.
 *
 *  @deprecated Users are strongly encouraged to use @ref cilk::reducer\<monoid\> reducers
 *              rather than the old reducers like reducer_max. The reducer\<monoid\> reducers
 *              show the reducer/monoid/view architecture more clearly, are more consistent in
 *              their implementation, and present a simpler model for new user-implemented
 *              reducers.
 *
 *  @note   Implicit conversions are provided between `reducer_max<T>` and 
 *          `reducer< op_max<T> >`. This allows incremental code conversion: old code that
 *          used `reducer_max` can pass a `reducer_max` to a converted function that now
 *          expects a reference to a `reducer<op_max>`, and vice versa. **But see 
 *          @ref redminmax_compatibility.**
 *
 *  @tparam Type    The value type of the reducer.
 *  @tparam Compare The “less than” comparator type for the reducer.
 *
 *  @see op_max
 *  @see op_max_view
 *  @see reducer
 *  @see @ref page_reducer_min_max
 */
template <typename Type, typename Compare=std::less<Type> __CILKRTS_MANGLE_DIFFERENT >
class reducer_max : public reducer< op_max<Type, Compare, true> >
{
    __CILKRTS_STATIC_ASSERT(
        internal::class_is_empty< typename internal::binary_functor<Compare>::type >::value, 
        "cilk::reducer_max<Type, Compare> only works with an empty Compare class");
    typedef reducer< op_max<Type, Compare, true> > base;
public:
    
    typedef Type basic_value_type;                      ///< Type of data in a reducer_max.
    typedef typename base::view_type        view_type;  ///< The view type for the reducer.
    typedef typename base::view_type        View;       ///< The view type for the reducer.
    typedef typename base::monoid_type      monoid_type;///< The monoid type for the reducer.
    typedef typename base::monoid_type      Monoid      ;///< The monoid type for the reducer.
    typedef internal::min_max::rhs_proxy<View> rhs_proxy; ///< The view’s rhs proxy type.          
    using base::view;

    /// Construct with an unspecified initial value.
    reducer_max() : base() {}

    /// Construct with a specified initial value.
    explicit reducer_max(const Type& initial_value) : base(initial_value) {}

    /// Construct with a specified comparator.
    explicit reducer_max(const Compare& comp) : base(comp) {}

    /// Construct with a specified initial value and comparator.
    reducer_max(const Type& initial_value, const Compare& comp)
    :   base(initial_value, comp) {}

    /// Returns true if the value has ever been set.
    bool is_set() const { return view().is_set(); }

    /// Return const reference to value data member in view.
    const Type& get_reference() const { return view().get_reference(); }
    
    /// Max operation.
    reducer_max& calc_max(const Type& value) 
        { view().calc_max(value); return *this; }

    /// Assignment of `max_of()`.
    reducer_max& operator=(const rhs_proxy& rhs)
        { view() = rhs; return *this; }

    /** @name `*reducer == reducer`.
     */
    //@{
    reducer_max&       operator*()       { return *this; }
    reducer_max const& operator*() const { return *this; }

    reducer_max*       operator->()       { return this; }
    reducer_max const* operator->() const { return this; }
    //@}
    
    /** “Upcast” to corresponding unaligned reducer.
     *
     *  @note   Upcast to corresponding _aligned_ reducer is a true upcast, so
     *          no conversion operator is necessary.
     */
    operator reducer< op_max<Type, Compare, false> >& ()
    {
        return *reinterpret_cast< reducer< op_max<Type, Compare, false> >* >(this);
    }
    
    /** “Upcast” to corresponding unaligned reducer.
     *
     *  @note   Upcast to corresponding _aligned_ reducer is a true upcast, so
     *          no conversion operator is necessary.
     */
    operator const reducer< op_max<Type, Compare, false> >& () const
    {
        return *reinterpret_cast< const reducer< op_max<Type, Compare, false> >* >(this);
    }
};


/** Compute the maximum of a reducer and a value.
 *
 *  @deprecated Because reducer_max is deprecated.
 */
//@{
template <typename Type, typename Compare>
inline internal::min_max::rhs_proxy< op_max_view<Type, Compare> >
max_of(const reducer_max<Type, Compare>& r, const Type& value)
{
    return internal::min_max::make_proxy(value, r.view());
}

template <typename Type, typename Compare>
inline internal::min_max::rhs_proxy< op_max_view<Type, Compare> >
max_of(const Type& value, const reducer_max<Type, Compare>& r)
{
    return internal::min_max::make_proxy(value, r.view());
}
//@}


/// @cond internal
/** Metafunction specialization for reducer conversion.
 *
 *  This specialization of the @ref legacy_reducer_downcast template class defined in
 *  reducer.h causes the `reducer< op_max<Type> >` class to have an 
 *  `operator reducer_max<Type>& ()` conversion operator that statically downcasts the 
 *  `reducer<op_max>` to the corresponding `reducer_max` type. (The reverse conversion,
 *  from `reducer_max` to `reducer<op_max>`, is just an upcast, which is provided for free
 *  by the language.)
 */
template <typename Type, typename Compare, bool Align>
struct legacy_reducer_downcast< reducer< op_max<Type, Compare, Align> > >
{
    typedef reducer_max<Type> type;
};
/// @endcond


/** Deprecated minimum reducer class.
 *
 *  reducer_min\<Type\> is the same as @ref cilk::reducer< @ref op_min\<Type\> >, except that 
 *  reducer_min is a proxy for the contained view, so that accumulator variable update 
 *  operations can be applied directly to the reducer. For example, where a `reducer<op_min>`
 *  is updated using `r->calc_min(a)`, you increment a reducer_min with `r.calc_min(a)`.
 *
 *  @deprecated Users are strongly encouraged to use @ref cilk::reducer\<monoid\> reducers
 *              rather than the old reducers like reducer_min. The reducer\<monoid\> reducers
 *              show the reducer/monoid/view architecture more clearly, are more consistent in
 *              their implementation, and present a simpler model for new user-implemented
 *              reducers.
 *
 *  @note   Implicit conversions are provided between `reducer_min<T>` and 
 *          `reducer< op_min<T> >`. This allows incremental code conversion: old code that
 *          used `reducer_min` can pass a `reducer_min` to a converted function that now
 *          expects a reference to a `reducer<op_min>`, and vice versa. **But see 
 *          @ref redminmax_compatibility.**
 *
 *  @tparam Type    The value type of the reducer.
 *  @tparam Compare The “less than” comparator type for the reducer.
 *
 *  @see op_min
 *  @see op_min_view
 *  @see reducer
 *  @see @ref page_reducer_min_max
 */
template <typename Type, typename Compare=std::less<Type> __CILKRTS_MANGLE_DIFFERENT>
class reducer_min : public reducer< op_min<Type, Compare, true> >
{
    __CILKRTS_STATIC_ASSERT(
        internal::class_is_empty< typename internal::binary_functor<Compare>::type >::value, 
        "cilk::reducer_min<Type, Compare> only works with an empty Compare class");
    typedef reducer< op_min<Type, Compare, true> > base;
public:
    
    typedef Type basic_value_type;                      ///< Type of data in a reducer_min.
    typedef typename base::view_type        view_type;  ///< The view type for the reducer.
    typedef typename base::view_type        View;       ///< The view type for the reducer.
    typedef typename base::monoid_type      monoid_type;///< The monoid type for the reducer.
    typedef typename base::monoid_type      Monoid      ;///< The monoid type for the reducer.
    typedef internal::min_max::rhs_proxy<View> rhs_proxy; ///< The view’s rhs proxy type.          
    using base::view;


    /// Construct with an unspecified initial value.
    reducer_min() : base() {}

    /// Construct with a specified initial value.
    explicit reducer_min(const Type& initial_value) : base(initial_value) {}

    /// Construct with a specified comparator.
    explicit reducer_min(const Compare& comp) : base(comp) {}

    /// Construct with a specified initial value and comparator.
    reducer_min(const Type& initial_value, const Compare& comp)
    :   base(initial_value, comp) {}

    /// Returns true if the value has ever been set.
    bool is_set() const { return view().is_set(); }

    /// Return const reference to value data member in view.
    const Type& get_reference() const { return view().get_reference(); }
    
    /// Min operation.
    reducer_min& calc_min(const Type& value) 
        { view().calc_min(value); return *this; }

    /// Assignment of `min_of()`.
    reducer_min& operator=(const rhs_proxy& rhs)
        { view() = rhs; return *this; }

    /** @name `*reducer == reducer`.
     */
    //@{
    reducer_min&       operator*()       { return *this; }
    reducer_min const& operator*() const { return *this; }

    reducer_min*       operator->()       { return this; }
    reducer_min const* operator->() const { return this; }
    //@}
    
    /** “Upcast” to corresponding unaligned reducer.
     *
     *  @note   Upcast to corresponding _aligned_ reducer is a true upcast, so
     *          no conversion operator is necessary.
     */
    operator reducer< op_min<Type, Compare, false> >& ()
    {
        return *reinterpret_cast< reducer< op_min<Type, Compare, false> >* >(this);
    }
    
    /** “Upcast” to corresponding unaligned reducer.
     *
     *  @note   Upcast to corresponding _aligned_ reducer is a true upcast, so
     *          no conversion operator is necessary.
     */
    operator const reducer< op_min<Type, Compare, false> >& () const
    {
        return *reinterpret_cast< const reducer< op_min<Type, Compare, false> >* >(this);
    }
};


/** Compute the minimum of a reducer and a value.
 *
 *  @deprecated Because reducer_min is deprecated.
 */
//@{
template <typename Type, typename Compare>
inline internal::min_max::rhs_proxy< op_min_view<Type, Compare> >
min_of(const reducer_min<Type, Compare>& r, const Type& value)
{
    return internal::min_max::make_proxy(value, r.view());
}

template <typename Type, typename Compare>
inline internal::min_max::rhs_proxy< op_min_view<Type, Compare> >
min_of(const Type& value, const reducer_min<Type, Compare>& r)
{
    return internal::min_max::make_proxy(value, r.view());
}
//@}


/// @cond internal
/** Metafunction specialization for reducer conversion.
 *
 *  This specialization of the @ref legacy_reducer_downcast template class defined in
 *  reducer.h causes the `reducer< op_min<Type> >` class to have an 
 *  `operator reducer_min<Type>& ()` conversion operator that statically downcasts the 
 *  `reducer<op_min>` to the corresponding `reducer_min` type. (The reverse conversion,
 *  from `reducer_min` to `reducer<op_min>`, is just an upcast, which is provided for free
 *  by the language.)
 */
template <typename Type, typename Compare, bool Align>
struct legacy_reducer_downcast< reducer< op_min<Type, Compare, Align> > >
{
    typedef reducer_min<Type> type;
};
/// @endcond


/** Deprecated maximum with index reducer class.
 *
 *  reducer_max_index<Index, Type> is the same as @ref cilk::reducer< @ref op_max_index<Index,
 *  Type> >, except that reducer_max_index is a proxy for the contained view, so that
 *  accumulator variable update operations can be applied directly to the reducer. For example,
 *  where a `reducer<op_max_index>` is updated using `r->calc_max(i, a)`, you increment a
 *  reducer_max_index with `r.calc_max(i, a)`.
 *
 *  @deprecated Users are strongly encouraged to use @ref cilk::reducer\<monoid\> reducers
 *              rather than the old reducers like reducer_max_index. The reducer\<monoid\>
 *              reducers show the reducer/monoid/view architecture more clearly, are more
 *              consistent in their implementation, and present a simpler model for new
 *              user-implemented reducers.
 *
 *  @note   Implicit conversions are provided between `reducer_max_index<T>` and 
 *          `reducer< op_max_index<T> >`. This allows incremental code conversion: old code that
 *          used `reducer_max_index` can pass a `reducer_max_index` to a converted function that
 *          now expects a reference to a `reducer<op_max_index>`, and vice versa. **But see 
 *          @ref redminmax_compatibility.**
 *
 *  @tparam Index       The index type of the reducer.
 *  @tparam Type        The value type of the reducer.
 *  @tparam Compare     The “less than” predicate for the reducer.
 *
 *  @see op_max_index
 *  @see op_max_index_view
 *  @see reducer
 *  @see @ref page_reducer_min_max
 */
template < typename Index
         , typename Type
         , typename Compare = std::less<Type>
           __CILKRTS_MANGLE_DIFFERENT 
         >
class reducer_max_index : public reducer< op_max_index<Index, Type, Compare, true> >
{
    __CILKRTS_STATIC_ASSERT(
        internal::class_is_empty< typename internal::binary_functor<Compare>::type >::value, 
        "cilk::reducer_max_index<Type, Compare> only works with an empty Compare class");
    typedef reducer< op_max_index<Index, Type, Compare, true> > base;
public:
    
    typedef Type basic_value_type;                      ///< Type of data in a reducer_max.
    typedef typename base::view_type        view_type;  ///< The view type for the reducer.
    typedef typename base::view_type        View;       ///< The view type for the reducer.
    typedef typename base::monoid_type      monoid_type;///< The monoid type for the reducer.
    typedef typename base::monoid_type      Monoid      ;///< The monoid type for the reducer.
    typedef internal::min_max::rhs_proxy<View> rhs_proxy; ///< The view’s rhs proxy type.          
    using base::view;

    /// Construct with an unspecified initial index and value.
    reducer_max_index() : base() {}

    /// Construct with a specified initial index and value.
    reducer_max_index(const Index& initial_index,
                      const Type& initial_value)
    : base(initial_index, initial_value) {}

    /// Construct with a specified comparator.
    explicit reducer_max_index(const Compare& comp) : base(comp) {}

    /// Construct with a specified initial index, value, and comparator.
    reducer_max_index(const Index& initial_index,
                      const Type& initial_value,
                      const Compare& comp)
    : base(initial_index, initial_value, comp) {}

    /// Set the index/value of this object.
    void set_value(const Index& index, const Type& value)
        { base::set_value(std::make_pair(index, value)); }

    /// Return the maximum value.
    Type get_value() const { return base::get_value().second; }

    /// Return the maximum index.
    Index get_index() const { return base::get_value().first; }

    /// Return const reference to value data member in view.
    const Type& get_reference() const { return view().get_reference(); }
    
    /// Return const reference to index data member in view.
    const Index& get_index_reference() const { return view().get_index_reference(); }
    
    /// Returns true if the value has ever been set.
    bool is_set() const { return view().is_set(); }

    /// Max operation.
    reducer_max_index& calc_max(const Index& index, const Type& value) 
        { view().calc_max(index, value); return *this; }

    /// Assignment of `max_of()`.
    reducer_max_index& operator=(const rhs_proxy& rhs)
        { view() = rhs; return *this; }

    /** @name `*reducer == reducer`.
     */
    //@{
    reducer_max_index&       operator*()       { return *this; }
    reducer_max_index const& operator*() const { return *this; }

    reducer_max_index*       operator->()       { return this; }
    reducer_max_index const* operator->() const { return this; }
    //@}
    
    /** “Upcast” to corresponding unaligned reducer.
     *
     *  @note   Upcast to corresponding _aligned_ reducer is a true upcast, so
     *          no conversion operator is necessary.
     */
    operator reducer< op_max_index<Index, Type, Compare, false> >& ()
    {
        return *reinterpret_cast< reducer< op_max_index<Index, Type, Compare, false> >* >(this);
    }
    
    /** “Upcast” to corresponding unaligned reducer.
     *
     *  @note   Upcast to corresponding _aligned_ reducer is a true upcast, so
     *          no conversion operator is necessary.
     */
    operator const reducer< op_max_index<Index, Type, Compare, false> >& () const
    {
        return *reinterpret_cast< const reducer< op_max_index<Index, Type, Compare, false> >* >(this);
    }
};


/** Compute the maximum of a reducer and a value.
 *
 *  @deprecated Because reducer_max is deprecated.
 */
//@{
template <typename Index, typename Type, typename Compare>
inline internal::min_max::rhs_proxy< op_max_index_view<Index, Type, Compare> >
max_of(const reducer_max_index<Index, Type, Compare>& r,
       const Index& index, const Type& value)
{
    return internal::min_max::make_proxy(std::pair<Index, Type>(index, value), r.view());
}

template <typename Index, typename Type, typename Compare>
inline internal::min_max::rhs_proxy< op_max_index_view<Index, Type, Compare> >
max_of(const Index& index, const Type& value,
       const reducer_max_index<Index, Type, Compare>& r)
{
    return internal::min_max::make_proxy(std::pair<Index, Type>(index, value), r.view());
}

template <typename Index, typename Type, typename Compare>
inline internal::min_max::rhs_proxy< op_max_index_view<Index, Type, Compare> >
max_of(const reducer_max_index<Index, Type, Compare>& r,
       const std::pair<Index, Type>& pair)
{
    return internal::min_max::make_proxy(pair, r.view());
}

template <typename Index, typename Type, typename Compare>
inline internal::min_max::rhs_proxy< op_max_index_view<Index, Type, Compare> >
max_of(const std::pair<Index, Type>& pair,
       const reducer_max_index<Index, Type, Compare>& r)
{
    return internal::min_max::make_proxy(pair, r.view());
}
//@}


/// @cond internal
/** Metafunction specialization for reducer conversion.
 *
 *  This specialization of the @ref legacy_reducer_downcast template class defined in
 *  reducer.h causes the `reducer< op_max_index<Index, Type> >` class to have an 
 *  `operator reducer_min<Index, Type>& ()` conversion operator that statically downcasts the 
 *  `reducer<op_max_index>` to the corresponding `reducer_max_index` type. (The reverse
 *  conversion, from `reducer_max_index` to `reducer<op_max_index>`, is just an upcast, which is
 *  provided for free by the language.)
 */
template <typename Index, typename Type, typename Compare, bool Align>
struct legacy_reducer_downcast< reducer< op_max_index<Index, Type, Compare, Align> > >
{
    typedef reducer_max_index<Index, Type> type;
};
/// @endcond


/** Deprecated minimum with index reducer class.
 *
 *  reducer_min_index<Index, Type> is the same as @ref cilk::reducer< @ref op_min_index<Index,
 *  Type> >, except that reducer_min_index is a proxy for the contained view, so that
 *  accumulator variable update operations can be applied directly to the reducer. For example,
 *  where a `reducer<op_min_index>` is updated using `r->calc_min(i, a)`, you increment a
 *  reducer_min_index with `r.calc_min(i, a)`.
 *
 *  @deprecated Users are strongly encouraged to use @ref cilk::reducer\<monoid\> reducers
 *              rather than the old reducers like reducer_min_index. The reducer\<monoid\>
 *              reducers show the reducer/monoid/view architecture more clearly, are more
 *              consistent in their implementation, and present a simpler model for new
 *              user-implemented reducers.
 *
 *  @note   Implicit conversions are provided between `reducer_min_index<T>` and 
 *          `reducer< op_min_index<T> >`. This allows incremental code conversion: old code that
 *          used `reducer_min_index` can pass a `reducer_min_index` to a converted function that
 *          now expects a reference to a `reducer<op_min_index>`, and vice versa. **But see 
 *          @ref redminmax_compatibility.**
 *
 *  @tparam Index       The index type of the reducer.
 *  @tparam Type        The value type of the reducer.
 *  @tparam Compare     The “less than” predicate for the reducer.
 *
 *  @see op_min_index
 *  @see op_min_index_view
 *  @see reducer
 *  @see @ref page_reducer_min_max
 */
template < typename Index
         , typename Type
         , typename Compare = std::less<Type>
           __CILKRTS_MANGLE_DIFFERENT 
         >
class reducer_min_index : public reducer< op_min_index<Index, Type, Compare, true> >
{
    __CILKRTS_STATIC_ASSERT(
        internal::class_is_empty< typename internal::binary_functor<Compare>::type >::value, 
        "cilk::reducer_min_index<Type, Compare> only works with an empty Compare class");
    typedef reducer< op_min_index<Index, Type, Compare, true> > base;
public:
    
    typedef Type basic_value_type;                      ///< Type of data in a reducer_min.
    typedef typename base::view_type        view_type;  ///< The view type for the reducer.
    typedef typename base::view_type        View;       ///< The view type for the reducer.
    typedef typename base::monoid_type      monoid_type;///< The monoid type for the reducer.
    typedef typename base::monoid_type      Monoid      ;///< The monoid type for the reducer.
    typedef internal::min_max::rhs_proxy<View> rhs_proxy; ///< The view’s rhs proxy type.          
    using base::view;

    /// Construct with an unspecified initial index and value.
    reducer_min_index() : base() {}

    /// Construct with a specified initial index and value.
    reducer_min_index(const Index& initial_index,
                      const Type& initial_value)
    : base(initial_index, initial_value) {}

    /// Construct with a specified comparator.
    explicit reducer_min_index(const Compare& comp) : base(comp) {}

    /// Construct with a specified initial index, value, and comparator.
    reducer_min_index(const Index& initial_index,
                      const Type& initial_value,
                      const Compare& comp)
    : base(initial_index, initial_value, comp) {}

    /// Set the index/value of this object.
    void set_value(const Index& index, const Type& value)
        { base::set_value(std::make_pair(index, value)); }

    /// Return the minimum value.
    Type get_value() const { return base::get_value().second; }

    /// Return the minimum index.
    Index get_index() const { return base::get_value().first; }

    /// Return const reference to value data member in view.
    const Type& get_reference() const { return view().get_reference(); }
    
    /// Return const reference to index data member in view.
    const Index& get_index_reference() const { return view().get_index_reference(); }
    
    /// Returns true if the value has ever been set.
    bool is_set() const { return view().is_set(); }

    /// Min operation.
    reducer_min_index& calc_min(const Index& index, const Type& value) 
        { view().calc_min(index, value); return *this; }

    /// Assignment of `min_of()`.
    reducer_min_index& operator=(const rhs_proxy& rhs)
        { view() = rhs; return *this; }

    /** @name `*reducer == reducer`.
     */
    //@{
    reducer_min_index&       operator*()       { return *this; }
    reducer_min_index const& operator*() const { return *this; }

    reducer_min_index*       operator->()       { return this; }
    reducer_min_index const* operator->() const { return this; }
    //@}
    
    /** “Upcast” to corresponding unaligned reducer.
     *
     *  @note   Upcast to corresponding _aligned_ reducer is a true upcast, so
     *          no conversion operator is necessary.
     */
    operator reducer< op_min_index<Index, Type, Compare, false> >& ()
    {
        return *reinterpret_cast< reducer< op_min_index<Index, Type, Compare, false> >* >(this);
    }
    
    /** “Upcast” to corresponding unaligned reducer.
     *
     *  @note   Upcast to corresponding _aligned_ reducer is a true upcast, so
     *          no conversion operator is necessary.
     */
    operator const reducer< op_min_index<Index, Type, Compare, false> >& () const
    {
        return *reinterpret_cast< const reducer< op_min_index<Index, Type, Compare, false> >* >(this);
    }
};


/** Compute the minimum of a reducer and a value.
 *
 *  @deprecated Because reducer_min is deprecated.
 */
//@{
template <typename Index, typename Type, typename Compare>
inline internal::min_max::rhs_proxy< op_min_index_view<Index, Type, Compare> >
min_of(const reducer_min_index<Index, Type, Compare>& r,
       const Index& index, const Type& value)
{
    return internal::min_max::make_proxy(std::pair<Index, Type>(index, value), r.view());
}

template <typename Index, typename Type, typename Compare>
inline internal::min_max::rhs_proxy< op_min_index_view<Index, Type, Compare> >
min_of(const Index& index, const Type& value,
       const reducer_min_index<Index, Type, Compare>& r)
{
    return internal::min_max::make_proxy(std::pair<Index, Type>(index, value), r.view());
}

template <typename Index, typename Type, typename Compare>
inline internal::min_max::rhs_proxy< op_min_index_view<Index, Type, Compare> >
min_of(const reducer_min_index<Index, Type, Compare>& r,
       const std::pair<Index, Type>& pair)
{
    return internal::min_max::make_proxy(pair, r.view());
}

template <typename Index, typename Type, typename Compare>
inline internal::min_max::rhs_proxy< op_min_index_view<Index, Type, Compare> >
min_of(const std::pair<Index, Type>& pair,
       const reducer_min_index<Index, Type, Compare>& r)
{
    return internal::min_max::make_proxy(pair, r.view());
}
//@}


/// @cond internal
/** Metafunction specialization for reducer conversion.
 *
 *  This specialization of the @ref legacy_reducer_downcast template class defined in
 *  reducer.h causes the `reducer< op_min_index<Index, Type> >` class to have an 
 *  `operator reducer_min<Index, Type>& ()` conversion operator that statically downcasts the 
 *  `reducer<op_min_index>` to the corresponding `reducer_min_index` type. (The reverse
 *  conversion, from `reducer_min_index` to `reducer<op_min_index>`, is just an upcast, which is
 *  provided for free by the language.)
 */
template <typename Index, typename Type, typename Compare, bool Align>
struct legacy_reducer_downcast< reducer< op_min_index<Index, Type, Compare, Align> > >
{
    typedef reducer_min_index<Index, Type> type;
};
/// @endcond


} // namespace cilk

#undef __CILKRTS_MANGLE_DIFFERENT

#endif // __cplusplus


/** @name C language reducer macros
 *
 *  These macros are used to declare and work with numeric minimum and maximum reducers in C
 *  code.
 *
 *  @see @ref page_reducers_in_c
 */
 //@{
 

#ifdef CILK_C_DEFINE_REDUCERS

/* Integer min/max constants */
#include <limits.h>

/* Wchar_t min/max constants */
#if defined(_MSC_VER) || defined(ANDROID)
#   include <wchar.h>
#else
#   include <stdint.h>
#endif

/* Floating-point min/max constants */
#include <math.h>
#ifndef HUGE_VALF
    static const unsigned int __huge_valf[] = {0x7f800000};
#   define HUGE_VALF (*((const float *)__huge_valf))
#endif

#ifndef HUGE_VALL
    static const unsigned int __huge_vall[] = {0, 0, 0x00007f80, 0};
#   define HUGE_VALL (*((const long double *)__huge_vall))
#endif

#endif

/** Max reducer type name.
 *
 *  This macro expands into the identifier which is the name of the max reducer
 *  type for a specified numeric type.
 *
 *  @param  tn  The @ref reducers_c_type_names "numeric type name" specifying the type of the
 *              reducer.
 *
 *  @see @ref reducers_c_predefined
 */
#define CILK_C_REDUCER_MAX_TYPE(tn)                                         \
    __CILKRTS_MKIDENT(cilk_c_reducer_max_,tn)

/** Declare a max reducer object.
 *
 *  This macro expands into a declaration of a max reducer object for a specified numeric
 *  type. For example:
 *
 *      CILK_C_REDUCER_MAX(my_reducer, double, -DBL_MAX);
 *
 *  @param  obj The variable name to be used for the declared reducer object.
 *  @param  tn  The @ref reducers_c_type_names "numeric type name" specifying the type of the
 *              reducer.
 *  @param  v   The initial value for the reducer. (A value which can be assigned to the 
 *              numeric type represented by @a tn.)
 *
 *  @see @ref reducers_c_predefined
 */
#define CILK_C_REDUCER_MAX(obj,tn,v)                                        \
    CILK_C_REDUCER_MAX_TYPE(tn) obj =                                       \
        CILK_C_INIT_REDUCER(_Typeof(obj.value),                             \
                        __CILKRTS_MKIDENT(cilk_c_reducer_max_reduce_,tn),   \
                        __CILKRTS_MKIDENT(cilk_c_reducer_max_identity_,tn), \
                        __cilkrts_hyperobject_noop_destroy, v)

/** Maximize with a value.
 *
 *  `CILK_C_REDUCER_MAX_CALC(reducer, v)` sets the current view of the
 *  reducer to the max of its previous value and a specified new value.
 *  This is equivalent to
 *
 *      REDUCER_VIEW(reducer) = max(REDUCER_VIEW(reducer), v)
 *
 *  @param reducer  The reducer whose contained value is to be updated.
 *  @param v        The value that it is to be maximized with.
 */
#define CILK_C_REDUCER_MAX_CALC(reducer, v) do {                            \
    _Typeof((reducer).value)* view = &(REDUCER_VIEW(reducer));              \
    _Typeof(v) __value = (v);                                               \
    if (*view < __value) {                                                  \
        *view = __value;                                                    \
    } } while (0)

/// @cond internal

/** Declare the max reducer functions for a numeric type.
 *
 *  This macro expands into external function declarations for functions which implement
 *  the reducer functionality for the max reducer type for a specified numeric type.
 *
 *  @param  t   The value type of the reducer.
 *  @param  tn  The value “type name” identifier, used to construct the reducer type name,
 *              function names, etc.
 */
#define CILK_C_REDUCER_MAX_DECLARATION(t,tn,id)                             \
    typedef CILK_C_DECLARE_REDUCER(t) CILK_C_REDUCER_MAX_TYPE(tn);       \
    __CILKRTS_DECLARE_REDUCER_REDUCE(cilk_c_reducer_max,tn,l,r);         \
    __CILKRTS_DECLARE_REDUCER_IDENTITY(cilk_c_reducer_max,tn);
 
/** Define the max reducer functions for a numeric type.
 *
 *  This macro expands into function definitions for functions which implement the
 *  reducer functionality for the max reducer type for a specified numeric type.
 *
 *  @param  t   The value type of the reducer.
 *  @param  tn  The value “type name” identifier, used to construct the reducer type name,
 *              function names, etc.
 */
#define CILK_C_REDUCER_MAX_DEFINITION(t,tn,id)                           \
    typedef CILK_C_DECLARE_REDUCER(t) CILK_C_REDUCER_MAX_TYPE(tn);       \
    __CILKRTS_DECLARE_REDUCER_REDUCE(cilk_c_reducer_max,tn,l,r)          \
        { if (*(t*)l < *(t*)r) *(t*)l = *(t*)r; }                        \
    __CILKRTS_DECLARE_REDUCER_IDENTITY(cilk_c_reducer_max,tn)            \
        { *(t*)v = id; }
 
//@{
/** @def CILK_C_REDUCER_MAX_INSTANCE 
 *  @brief Declare or define implementation functions for a reducer type.
 *
 *  In the runtime source file c_reducers.c, the macro CILK_C_DEFINE_REDUCERS will be defined, and
 *  this macro will generate reducer implementation functions. Everywhere else, CILK_C_DEFINE_REDUCERS
 *  will be undefined, and this macro will expand into external declarations for the functions.
 */
#ifdef CILK_C_DEFINE_REDUCERS
#   define CILK_C_REDUCER_MAX_INSTANCE(t,tn,id)  \
        CILK_C_REDUCER_MAX_DEFINITION(t,tn,id)
#else
#   define CILK_C_REDUCER_MAX_INSTANCE(t,tn,id)  \
        CILK_C_REDUCER_MAX_DECLARATION(t,tn,id)
#endif
//@}

/*  Declare or define an instance of the reducer type and its functions for each 
 *  numeric type.
 */
__CILKRTS_BEGIN_EXTERN_C
CILK_C_REDUCER_MAX_INSTANCE(char,               char,       CHAR_MIN)
CILK_C_REDUCER_MAX_INSTANCE(unsigned char,      uchar,      0)
CILK_C_REDUCER_MAX_INSTANCE(signed char,        schar,      SCHAR_MIN)
CILK_C_REDUCER_MAX_INSTANCE(wchar_t,            wchar_t,    WCHAR_MIN)
CILK_C_REDUCER_MAX_INSTANCE(short,              short,      SHRT_MIN)
CILK_C_REDUCER_MAX_INSTANCE(unsigned short,     ushort,     0)
CILK_C_REDUCER_MAX_INSTANCE(int,                int,        INT_MIN)
CILK_C_REDUCER_MAX_INSTANCE(unsigned int,       uint,       0)
CILK_C_REDUCER_MAX_INSTANCE(unsigned int,       unsigned,   0) // alternate name
CILK_C_REDUCER_MAX_INSTANCE(long,               long,       LONG_MIN)
CILK_C_REDUCER_MAX_INSTANCE(unsigned long,      ulong,      0)
CILK_C_REDUCER_MAX_INSTANCE(long long,          longlong,   LLONG_MIN)
CILK_C_REDUCER_MAX_INSTANCE(unsigned long long, ulonglong,  0)
CILK_C_REDUCER_MAX_INSTANCE(float,              float,      -HUGE_VALF)
CILK_C_REDUCER_MAX_INSTANCE(double,             double,     -HUGE_VAL)
CILK_C_REDUCER_MAX_INSTANCE(long double,        longdouble, -HUGE_VALL)
__CILKRTS_END_EXTERN_C

/// @endcond

/** Max_index reducer type name.
 *
 *  This macro expands into the identifier which is the name of the max_index reducer
 *  type for a specified numeric type.
 *
 *  @param  tn  The @ref reducers_c_type_names "numeric type name" specifying the type of the
 *              reducer.
 *
 *  @see @ref reducers_c_predefined
 */
#define CILK_C_REDUCER_MAX_INDEX_TYPE(tn)                                         \
    __CILKRTS_MKIDENT(cilk_c_reducer_max_index_,tn)

/** Declare an op_max_index reducer object.
 *
 *  This macro expands into a declaration of a max_index reducer object for a specified
 *  numeric type. For example:
 *
 *      CILK_C_REDUCER_MAX_INDEX(my_reducer, double, -DBL_MAX_INDEX);
 *
 *  @param  obj The variable name to be used for the declared reducer object.
 *  @param  tn  The @ref reducers_c_type_names "numeric type name" specifying the type of the
 *              reducer.
 *  @param  v   The initial value for the reducer. (A value which can be assigned to the 
 *              numeric type represented by @a tn.)
 *
 *  @see @ref reducers_c_predefined
 */
#define CILK_C_REDUCER_MAX_INDEX(obj,tn,v)                                        \
    CILK_C_REDUCER_MAX_INDEX_TYPE(tn) obj =                                       \
        CILK_C_INIT_REDUCER(_Typeof(obj.value),                             \
                        __CILKRTS_MKIDENT(cilk_c_reducer_max_index_reduce_,tn),   \
                        __CILKRTS_MKIDENT(cilk_c_reducer_max_index_identity_,tn), \
                        __cilkrts_hyperobject_noop_destroy, {0, v})

/** Maximize with a value.
 *
 *  `CILK_C_REDUCER_MAX_INDEX_CALC(reducer, i, v)` sets the current view of the
 *  reducer to the max of its previous value and a specified new value.
 *  This is equivalent to
 *
 *      REDUCER_VIEW(reducer) = max_index(REDUCER_VIEW(reducer), v)
 *
 *  If the value of the reducer is changed to @a v, then the index of the reducer is
 *  changed to @a i. 
 *
 *  @param reducer  The reducer whose contained value and index are to be updated.
 *  @param i        The index associated with the new value.
 *  @param v        The value that it is to be maximized with.
 */
#define CILK_C_REDUCER_MAX_INDEX_CALC(reducer, i, v) do {                   \
    _Typeof((reducer).value)* view = &(REDUCER_VIEW(reducer));              \
    _Typeof(v) __value = (v);                                               \
    if (view->value < __value) {                                            \
        view->index = (i);                                                  \
        view->value = __value;                                              \
    } } while (0)

/// @cond internal

/** Declare the max_index view type.
 *
 *  The view of a max_index reducer is a structure containing both the
 *  maximum value for the reducer and the index that was associated with
 *  that value in the sequence of input values.
 */
#define CILK_C_REDUCER_MAX_INDEX_VIEW(t,tn)                                  \
    typedef struct {                                                         \
        __STDNS ptrdiff_t index;                                             \
        t                 value;                                             \
    } __CILKRTS_MKIDENT(cilk_c_reducer_max_index_view_,tn)

/** Declare the max_index reducer functions for a numeric type.
 *
 *  This macro expands into external function declarations for functions which implement
 *  the reducer functionality for the max_index reducer type for a specified numeric type.
 *
 *  @param  t   The value type of the reducer.
 *  @param  tn  The value “type name” identifier, used to construct the reducer type name,
 *              function names, etc.
 */
#define CILK_C_REDUCER_MAX_INDEX_DECLARATION(t,tn,id)                       \
    CILK_C_REDUCER_MAX_INDEX_VIEW(t,tn);                                    \
    typedef CILK_C_DECLARE_REDUCER(                                         \
        __CILKRTS_MKIDENT(cilk_c_reducer_max_index_view_,tn))               \
            CILK_C_REDUCER_MAX_INDEX_TYPE(tn);                              \
    __CILKRTS_DECLARE_REDUCER_REDUCE(cilk_c_reducer_max_index,tn,l,r);      \
    __CILKRTS_DECLARE_REDUCER_IDENTITY(cilk_c_reducer_max_index,tn);
 
/** Define the max_index reducer functions for a numeric type.
 *
 *  This macro expands into function definitions for functions which implement the
 *  reducer functionality for the max_index reducer type for a specified numeric type.
 *
 *  @param  t   The value type of the reducer.
 *  @param  tn  The value “type name” identifier, used to construct the reducer type name,
 *              function names, etc.
 */
#define CILK_C_REDUCER_MAX_INDEX_DEFINITION(t,tn,id)                           \
    CILK_C_REDUCER_MAX_INDEX_VIEW(t,tn);                                    \
    typedef CILK_C_DECLARE_REDUCER(                                         \
        __CILKRTS_MKIDENT(cilk_c_reducer_max_index_view_,tn))               \
            CILK_C_REDUCER_MAX_INDEX_TYPE(tn);                              \
    __CILKRTS_DECLARE_REDUCER_REDUCE(cilk_c_reducer_max_index,tn,l,r)          \
        { typedef __CILKRTS_MKIDENT(cilk_c_reducer_max_index_view_,tn) view_t; \
          if (((view_t*)l)->value < ((view_t*)r)->value)                       \
              *(view_t*)l = *(view_t*)r; }                                     \
    __CILKRTS_DECLARE_REDUCER_IDENTITY(cilk_c_reducer_max_index,tn)            \
        { typedef __CILKRTS_MKIDENT(cilk_c_reducer_max_index_view_,tn) view_t; \
          ((view_t*)v)->index = 0; ((view_t*)v)->value = id; }
 
//@{
/** @def CILK_C_REDUCER_MAX_INDEX_INSTANCE 
 *  @brief Declare or define implementation functions for a reducer type.
 *
 *  In the runtime source file c_reducers.c, the macro CILK_C_DEFINE_REDUCERS will be defined, and
 *  this macro will generate reducer implementation functions. Everywhere else, CILK_C_DEFINE_REDUCERS
 *  will be undefined, and this macro will expand into external declarations for the functions.
 */
#ifdef CILK_C_DEFINE_REDUCERS
#   define CILK_C_REDUCER_MAX_INDEX_INSTANCE(t,tn,id)  \
        CILK_C_REDUCER_MAX_INDEX_DEFINITION(t,tn,id)
#else
#   define CILK_C_REDUCER_MAX_INDEX_INSTANCE(t,tn,id)  \
        CILK_C_REDUCER_MAX_INDEX_DECLARATION(t,tn,id)
#endif
//@}

/*  Declare or define an instance of the reducer type and its functions for each 
 *  numeric type.
 */
__CILKRTS_BEGIN_EXTERN_C
CILK_C_REDUCER_MAX_INDEX_INSTANCE(char,               char,       CHAR_MIN)
CILK_C_REDUCER_MAX_INDEX_INSTANCE(unsigned char,      uchar,      0)
CILK_C_REDUCER_MAX_INDEX_INSTANCE(signed char,        schar,      SCHAR_MIN)
CILK_C_REDUCER_MAX_INDEX_INSTANCE(wchar_t,            wchar_t,    WCHAR_MIN)
CILK_C_REDUCER_MAX_INDEX_INSTANCE(short,              short,      SHRT_MIN)
CILK_C_REDUCER_MAX_INDEX_INSTANCE(unsigned short,     ushort,     0)
CILK_C_REDUCER_MAX_INDEX_INSTANCE(int,                int,        INT_MIN)
CILK_C_REDUCER_MAX_INDEX_INSTANCE(unsigned int,       uint,       0)
CILK_C_REDUCER_MAX_INDEX_INSTANCE(unsigned int,       unsigned,   0) // alternate name
CILK_C_REDUCER_MAX_INDEX_INSTANCE(long,               long,       LONG_MIN)
CILK_C_REDUCER_MAX_INDEX_INSTANCE(unsigned long,      ulong,      0)
CILK_C_REDUCER_MAX_INDEX_INSTANCE(long long,          longlong,   LLONG_MIN)
CILK_C_REDUCER_MAX_INDEX_INSTANCE(unsigned long long, ulonglong,  0)
CILK_C_REDUCER_MAX_INDEX_INSTANCE(float,              float,      -HUGE_VALF)
CILK_C_REDUCER_MAX_INDEX_INSTANCE(double,             double,     -HUGE_VAL)
CILK_C_REDUCER_MAX_INDEX_INSTANCE(long double,        longdouble, -HUGE_VALL)
__CILKRTS_END_EXTERN_C

/// @endcond

/** Min reducer type name.
 *
 *  This macro expands into the identifier which is the name of the min reducer
 *  type for a specified numeric type.
 *
 *  @param  tn  The @ref reducers_c_type_names "numeric type name" specifying the type of the
 *              reducer.
 *
 *  @see @ref reducers_c_predefined
 */
#define CILK_C_REDUCER_MIN_TYPE(tn)                                         \
    __CILKRTS_MKIDENT(cilk_c_reducer_min_,tn)

/** Declare a min reducer object.
 *
 *  This macro expands into a declaration of a min reducer object for a specified numeric
 *  type. For example:
 *
 *      CILK_C_REDUCER_MIN(my_reducer, double, DBL_MAX);
 *
 *  @param  obj The variable name to be used for the declared reducer object.
 *  @param  tn  The @ref reducers_c_type_names "numeric type name" specifying the type of the
 *              reducer.
 *  @param  v   The initial value for the reducer. (A value which can be assigned to the 
 *              numeric type represented by @a tn.)
 *
 *  @see @ref reducers_c_predefined
 */
#define CILK_C_REDUCER_MIN(obj,tn,v)                                        \
    CILK_C_REDUCER_MIN_TYPE(tn) obj =                                       \
        CILK_C_INIT_REDUCER(_Typeof(obj.value),                             \
                        __CILKRTS_MKIDENT(cilk_c_reducer_min_reduce_,tn),   \
                        __CILKRTS_MKIDENT(cilk_c_reducer_min_identity_,tn), \
                        __cilkrts_hyperobject_noop_destroy, v)

/** Minimize with a value.
 *
 *  `CILK_C_REDUCER_MIN_CALC(reducer, v)` sets the current view of the
 *  reducer to the min of its previous value and a specified new value.
 *  This is equivalent to
 *
 *      REDUCER_VIEW(reducer) = min(REDUCER_VIEW(reducer), v)
 *
 *  @param reducer  The reducer whose contained value is to be updated.
 *  @param v        The value that it is to be minimized with.
 */
#define CILK_C_REDUCER_MIN_CALC(reducer, v) do {                            \
    _Typeof((reducer).value)* view = &(REDUCER_VIEW(reducer));              \
    _Typeof(v) __value = (v);                                               \
    if (*view > __value) {                                                  \
        *view = __value;                                                    \
    } } while (0)

/// @cond internal

/** Declare the min reducer functions for a numeric type.
 *
 *  This macro expands into external function declarations for functions which implement
 *  the reducer functionality for the min reducer type for a specified numeric type.
 *
 *  @param  t   The value type of the reducer.
 *  @param  tn  The value “type name” identifier, used to construct the reducer type name,
 *              function names, etc.
 */
#define CILK_C_REDUCER_MIN_DECLARATION(t,tn,id)                             \
    typedef CILK_C_DECLARE_REDUCER(t) CILK_C_REDUCER_MIN_TYPE(tn);       \
    __CILKRTS_DECLARE_REDUCER_REDUCE(cilk_c_reducer_min,tn,l,r);         \
    __CILKRTS_DECLARE_REDUCER_IDENTITY(cilk_c_reducer_min,tn);
 
/** Define the min reducer functions for a numeric type.
 *
 *  This macro expands into function definitions for functions which implement the
 *  reducer functionality for the min reducer type for a specified numeric type.
 *
 *  @param  t   The value type of the reducer.
 *  @param  tn  The value “type name” identifier, used to construct the reducer type name,
 *              function names, etc.
 */
#define CILK_C_REDUCER_MIN_DEFINITION(t,tn,id)                           \
    typedef CILK_C_DECLARE_REDUCER(t) CILK_C_REDUCER_MIN_TYPE(tn);       \
    __CILKRTS_DECLARE_REDUCER_REDUCE(cilk_c_reducer_min,tn,l,r)          \
        { if (*(t*)l > *(t*)r) *(t*)l = *(t*)r; }                        \
    __CILKRTS_DECLARE_REDUCER_IDENTITY(cilk_c_reducer_min,tn)            \
        { *(t*)v = id; }
 
//@{
/** @def CILK_C_REDUCER_MIN_INSTANCE 
 *  @brief Declare or define implementation functions for a reducer type.
 *
 *  In the runtime source file c_reducers.c, the macro CILK_C_DEFINE_REDUCERS will be defined, and
 *  this macro will generate reducer implementation functions. Everywhere else, CILK_C_DEFINE_REDUCERS
 *  will be undefined, and this macro will expand into external declarations for the functions.
 */
#ifdef CILK_C_DEFINE_REDUCERS
#   define CILK_C_REDUCER_MIN_INSTANCE(t,tn,id)  \
        CILK_C_REDUCER_MIN_DEFINITION(t,tn,id)
#else
#   define CILK_C_REDUCER_MIN_INSTANCE(t,tn,id)  \
        CILK_C_REDUCER_MIN_DECLARATION(t,tn,id)
#endif
//@}

/*  Declare or define an instance of the reducer type and its functions for each 
 *  numeric type.
 */
__CILKRTS_BEGIN_EXTERN_C
CILK_C_REDUCER_MIN_INSTANCE(char,               char,       CHAR_MAX)
CILK_C_REDUCER_MIN_INSTANCE(unsigned char,      uchar,      CHAR_MAX)
CILK_C_REDUCER_MIN_INSTANCE(signed char,        schar,      SCHAR_MAX)
CILK_C_REDUCER_MIN_INSTANCE(wchar_t,            wchar_t,    WCHAR_MAX)
CILK_C_REDUCER_MIN_INSTANCE(short,              short,      SHRT_MAX)
CILK_C_REDUCER_MIN_INSTANCE(unsigned short,     ushort,     USHRT_MAX)
CILK_C_REDUCER_MIN_INSTANCE(int,                int,        INT_MAX)
CILK_C_REDUCER_MIN_INSTANCE(unsigned int,       uint,       UINT_MAX)
CILK_C_REDUCER_MIN_INSTANCE(unsigned int,       unsigned,   UINT_MAX) // alternate name
CILK_C_REDUCER_MIN_INSTANCE(long,               long,       LONG_MAX)
CILK_C_REDUCER_MIN_INSTANCE(unsigned long,      ulong,      ULONG_MAX)
CILK_C_REDUCER_MIN_INSTANCE(long long,          longlong,   LLONG_MAX)
CILK_C_REDUCER_MIN_INSTANCE(unsigned long long, ulonglong,  ULLONG_MAX)
CILK_C_REDUCER_MIN_INSTANCE(float,              float,      HUGE_VALF)
CILK_C_REDUCER_MIN_INSTANCE(double,             double,     HUGE_VAL)
CILK_C_REDUCER_MIN_INSTANCE(long double,        longdouble, HUGE_VALL)
__CILKRTS_END_EXTERN_C

/// @endcond

/** Min_index reducer type name.
 *
 *  This macro expands into the identifier which is the name of the min_index reducer
 *  type for a specified numeric type.
 *
 *  @param  tn  The @ref reducers_c_type_names "numeric type name" specifying the type of the
 *              reducer.
 *
 *  @see @ref reducers_c_predefined
 */
#define CILK_C_REDUCER_MIN_INDEX_TYPE(tn)                                         \
    __CILKRTS_MKIDENT(cilk_c_reducer_min_index_,tn)

/** Declare an op_min_index reducer object.
 *
 *  This macro expands into a declaration of a min_index reducer object for a specified
 *  numeric type. For example:
 *
 *      CILK_C_REDUCER_MIN_INDEX(my_reducer, double, -DBL_MIN_INDEX);
 *
 *  @param  obj The variable name to be used for the declared reducer object.
 *  @param  tn  The @ref reducers_c_type_names "numeric type name" specifying the type of the
 *              reducer.
 *  @param  v   The initial value for the reducer. (A value which can be assigned to the 
 *              numeric type represented by @a tn.)
 *
 *  @see @ref reducers_c_predefined
 */
#define CILK_C_REDUCER_MIN_INDEX(obj,tn,v)                                        \
    CILK_C_REDUCER_MIN_INDEX_TYPE(tn) obj =                                       \
        CILK_C_INIT_REDUCER(_Typeof(obj.value),                             \
                        __CILKRTS_MKIDENT(cilk_c_reducer_min_index_reduce_,tn),   \
                        __CILKRTS_MKIDENT(cilk_c_reducer_min_index_identity_,tn), \
                        __cilkrts_hyperobject_noop_destroy, {0, v})

/** Minimize with a value.
 *
 *  `CILK_C_REDUCER_MIN_INDEX_CALC(reducer, i, v)` sets the current view of the
 *  reducer to the min of its previous value and a specified new value.
 *  This is equivalent to
 *
 *      REDUCER_VIEW(reducer) = min_index(REDUCER_VIEW(reducer), v)
 *
 *  If the value of the reducer is changed to @a v, then the index of the reducer is
 *  changed to @a i. 
 *
 *  @param reducer  The reducer whose contained value and index are to be updated.
 *  @param i        The index associated with the new value.
 *  @param v        The value that it is to be minimized with.
 */
#define CILK_C_REDUCER_MIN_INDEX_CALC(reducer, i, v) do {                   \
    _Typeof((reducer).value)* view = &(REDUCER_VIEW(reducer));              \
    _Typeof(v) __value = (v);                                               \
    if (view->value > __value) {                                            \
        view->index = (i);                                                  \
        view->value = __value;                                              \
    } } while (0)

/// @cond internal

/** Declare the min_index view type.
 *
 *  The view of a min_index reducer is a structure containing both the
 *  minimum value for the reducer and the index that was associated with
 *  that value in the sequence of input values.
 */
#define CILK_C_REDUCER_MIN_INDEX_VIEW(t,tn)                                  \
    typedef struct {                                                         \
        __STDNS ptrdiff_t index;                                             \
        t                 value;                                             \
    } __CILKRTS_MKIDENT(cilk_c_reducer_min_index_view_,tn)

/** Declare the min_index reducer functions for a numeric type.
 *
 *  This macro expands into external function declarations for functions which implement
 *  the reducer functionality for the min_index reducer type for a specified numeric type.
 *
 *  @param  t   The value type of the reducer.
 *  @param  tn  The value “type name” identifier, used to construct the reducer type name,
 *              function names, etc.
 */
#define CILK_C_REDUCER_MIN_INDEX_DECLARATION(t,tn,id)                       \
    CILK_C_REDUCER_MIN_INDEX_VIEW(t,tn);                                    \
    typedef CILK_C_DECLARE_REDUCER(                                         \
        __CILKRTS_MKIDENT(cilk_c_reducer_min_index_view_,tn))               \
            CILK_C_REDUCER_MIN_INDEX_TYPE(tn);                              \
    __CILKRTS_DECLARE_REDUCER_REDUCE(cilk_c_reducer_min_index,tn,l,r);      \
    __CILKRTS_DECLARE_REDUCER_IDENTITY(cilk_c_reducer_min_index,tn);
 
/** Define the min_index reducer functions for a numeric type.
 *
 *  This macro expands into function definitions for functions which implement the
 *  reducer functionality for the min_index reducer type for a specified numeric type.
 *
 *  @param  t   The value type of the reducer.
 *  @param  tn  The value “type name” identifier, used to construct the reducer type name,
 *              function names, etc.
 */
#define CILK_C_REDUCER_MIN_INDEX_DEFINITION(t,tn,id)                           \
    CILK_C_REDUCER_MIN_INDEX_VIEW(t,tn);                                    \
    typedef CILK_C_DECLARE_REDUCER(                                         \
        __CILKRTS_MKIDENT(cilk_c_reducer_min_index_view_,tn))               \
            CILK_C_REDUCER_MIN_INDEX_TYPE(tn);                              \
    __CILKRTS_DECLARE_REDUCER_REDUCE(cilk_c_reducer_min_index,tn,l,r)          \
        { typedef __CILKRTS_MKIDENT(cilk_c_reducer_min_index_view_,tn) view_t; \
          if (((view_t*)l)->value > ((view_t*)r)->value)                       \
              *(view_t*)l = *(view_t*)r; }                                     \
    __CILKRTS_DECLARE_REDUCER_IDENTITY(cilk_c_reducer_min_index,tn)            \
        { typedef __CILKRTS_MKIDENT(cilk_c_reducer_min_index_view_,tn) view_t; \
          ((view_t*)v)->index = 0; ((view_t*)v)->value = id; }
 
//@{
/** @def CILK_C_REDUCER_MIN_INDEX_INSTANCE 
 *  @brief Declare or define implementation functions for a reducer type.
 *
 *  In the runtime source file c_reducers.c, the macro CILK_C_DEFINE_REDUCERS will be defined, and
 *  this macro will generate reducer implementation functions. Everywhere else, CILK_C_DEFINE_REDUCERS
 *  will be undefined, and this macro will expand into external declarations for the functions.
 */
#ifdef CILK_C_DEFINE_REDUCERS
#   define CILK_C_REDUCER_MIN_INDEX_INSTANCE(t,tn,id)  \
        CILK_C_REDUCER_MIN_INDEX_DEFINITION(t,tn,id)
#else
#   define CILK_C_REDUCER_MIN_INDEX_INSTANCE(t,tn,id)  \
        CILK_C_REDUCER_MIN_INDEX_DECLARATION(t,tn,id)
#endif
//@}

/*  Declare or define an instance of the reducer type and its functions for each 
 *  numeric type.
 */
__CILKRTS_BEGIN_EXTERN_C
CILK_C_REDUCER_MIN_INDEX_INSTANCE(char,               char,       CHAR_MAX)
CILK_C_REDUCER_MIN_INDEX_INSTANCE(unsigned char,      uchar,      CHAR_MAX)
CILK_C_REDUCER_MIN_INDEX_INSTANCE(signed char,        schar,      SCHAR_MAX)
CILK_C_REDUCER_MIN_INDEX_INSTANCE(wchar_t,            wchar_t,    WCHAR_MAX)
CILK_C_REDUCER_MIN_INDEX_INSTANCE(short,              short,      SHRT_MAX)
CILK_C_REDUCER_MIN_INDEX_INSTANCE(unsigned short,     ushort,     USHRT_MAX)
CILK_C_REDUCER_MIN_INDEX_INSTANCE(int,                int,        INT_MAX)
CILK_C_REDUCER_MIN_INDEX_INSTANCE(unsigned int,       uint,       UINT_MAX)
CILK_C_REDUCER_MIN_INDEX_INSTANCE(unsigned int,       unsigned,   UINT_MAX) // alternate name
CILK_C_REDUCER_MIN_INDEX_INSTANCE(long,               long,       LONG_MAX)
CILK_C_REDUCER_MIN_INDEX_INSTANCE(unsigned long,      ulong,      ULONG_MAX)
CILK_C_REDUCER_MIN_INDEX_INSTANCE(long long,          longlong,   LLONG_MAX)
CILK_C_REDUCER_MIN_INDEX_INSTANCE(unsigned long long, ulonglong,  ULLONG_MAX)
CILK_C_REDUCER_MIN_INDEX_INSTANCE(float,              float,      HUGE_VALF)
CILK_C_REDUCER_MIN_INDEX_INSTANCE(double,             double,     HUGE_VAL)
CILK_C_REDUCER_MIN_INDEX_INSTANCE(long double,        longdouble, HUGE_VALL)
__CILKRTS_END_EXTERN_C

/// @endcond

//@}

#endif // defined REDUCER_MAX_H_INCLUDED

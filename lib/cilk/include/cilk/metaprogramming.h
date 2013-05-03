/** @file metaprogramming.h
 *
 *  @brief Defines metaprogramming utility classes used in thr Cilk library.
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
 *  @ingroup metaprogramming
 */

/** @defgroup metaprogramming Meta-Programming Utility Classes
 *
 *  These are C++ utility classes which provide meta-programming features — that is,
 *  conditional compilation capabilities based on properties of template arguments.
 */
 
#ifndef METAPROGRAMMING_H_INCLUDED
#define METAPROGRAMMING_H_INCLUDED

#ifdef __cplusplus

#include <functional>
#include <new>
#include <cstdlib>
#ifdef _WIN32
#include <malloc.h>
#endif
#include <algorithm>

namespace cilk {

namespace internal {

/** Test if a class is empty.
 *
 *  If @a Class is an empty (and therefore necessarily stateless) class, then the “empty
 *  base-class optimization” guarantees that `sizeof(check_for_empty_class<Class>) ==
 *  sizeof(char)`. Conversely, if `sizeof(check_for_empty_class<Class>) > sizeof(char)`,
 *  then @a Class is not empty, and we must discriminate distinct instances of @a Class.
 *
 *  Typical usage:
 *
 *      // General definition of A<B> for non-empty B:
 *      template <typename B, bool BIsEmpty = class_is_empty<B>::value> >
 *      class A { ... };
 *
 *      // Specialized definition of A<B> for empty B:
 *      template <typename B>
 *      class A<B, true> { ... };
 *
 *  @tparam Class   The class to be tested for emptiness.
 *
 *  @result         The `value` member will be `true` if @a Class is empty, `false` otherwise.
 *
 *  @ingroup metaprogramming
 */
template <class Class>
class class_is_empty { 
    class check_for_empty_class : public Class
    {
        char m_data;
    public:
        // Declared but not defined
        check_for_empty_class();
        check_for_empty_class(const check_for_empty_class&);
        check_for_empty_class& operator=(const check_for_empty_class&);
        ~check_for_empty_class();
    };
public:

    /** Constant is true if and only if @a Class is empty.
     */
    static const bool value = (sizeof(check_for_empty_class) == sizeof(char));
};


/** Compute the alignment of a type. (More precisely, the alignment of a data
 *  member of the type in a structure.)
 *
 *  For example:
 *
 *      align_of<double>::value == 8
 *
 *  Adapted from the [AlignOf](http://llvm.org/doxygen/AlignOf_8h_source.html)
 *  class used in [LLVM](http://llvm.org).
 *
 *  @tparam T   The type whose alignment is to be computed.
 *
 *  @result     `value` will be the alignment for type @a T.
 *
 *  @see alignof()
 *
 *  @ingroup metaprogramming
 */
template <typename T>
class align_of {
    
    struct impl {
      char x;
      T t;
      impl();   // Never instantiate.
      impl(const impl&);
    };

public:
    enum {
        /** The alignment of the type @a T.
         */
        value = static_cast<std::size_t>(sizeof(impl) - sizeof(T))
    };
};


/** Get the functor class corresponding to a binary function type.
 *
 *  The `binary_functor` template class class can be instantiated with a binary functor class or
 *  with a real binary function, and will yield an equivalent binary functor class class in
 *  either case.
 *
 *  @tparam F   A binary functor class, a binary function type, or a pointer to binary function
 *              type.
 *
 *  @result     `binary_functor<F>::%type` will be the same as @a F if @a F is a class. It will
 *              be a `std::pointer_to_binary_function` wrapper if @a F is a binary function or
 *              binary function pointer type. (It will _not_ necessarily be an `Adaptable
 *              Binary Function` class, since @a F might be a non-adaptable binary functor
 *              class.)
 *
 *  @ingroup metaprogramming
 */
template <typename F>
struct binary_functor {
     /// The binary functor class equivalent to @a F.
     typedef F type;
};

/// @copydoc binary_functor
/// Specialization for binary function.
template <typename R, typename A, typename B>
struct binary_functor<R(A,B)> {
     /// The binary functor class equivalent to @a F.
    typedef std::pointer_to_binary_function<A, B, R> type;
};

/// @copydoc binary_functor
/// Specialization for pointer to binary function.
template <typename R, typename A, typename B>
struct binary_functor<R(*)(A,B)> {
     /// The binary functor class equivalent to @a F.
    typedef std::pointer_to_binary_function<A, B, R> type;
};


/** Indirect binary function class with specified types.
 *
 *  `typed_indirect_binary_function<F>` is an `Adaptable Binary Function` class based on an
 *  existing binary functor class or binary function type @a F. If @a F is a stateless class,
 *  then this class will be empty, and its `operator()` will invoke @a F’s `operator()`.
 *  Otherwise, an object of this class will hold a pointer to an object of type @a F, and will
 *  refer its `operator()` calls to the pointed-to @a F object.
 *
 *  That is, suppose that we have the declarations:
 *
 *      F *p;
 *      typed_indirect_binary_function<F, int, int, bool> ibf(p);
 *
 *  Then:
 *
 *  -   `ibf(x, y) == (*p)(x, y)`.
 *  -   `ibf(x, y)` will not do a pointer dereference if `F` is an empty class.
 *
 *  @note       Just to repeat: if `F` is an empty class, then
 *              `typed_indirect_binary_function\<F\>' is also an empty class. This is critical
 *              for its use in the @ref min_max::view_base "min/max reducer view classes", where
 *              it allows the view to call a comparison functor in the monoid without actually
 *              having to allocate a pointer in the view class when the comparison class is
 *              empty.
 *
 *  @note       If you have an `Adaptable Binary Function` class or a binary function type,
 *              then you can use the @ref indirect_binary_function class, which derives the
 *              argument and result types parameter type instead of requiring you to specify
 *              them as template arguments.
 *
 *  @tparam F   A binary functor class, a binary function type, or a pointer to binary function
 *              type.
 *  @param A1   The first argument type.
 *  @param A2   The second argument type.
 *  @param R    The result type.
 *
 *  @see min_max::comparator_base
 *  @see indirect_binary_function
 *
 *  @ingroup metaprogramming
 */
template <  typename F
         ,  typename A1
         ,  typename A2
         ,  typename R
         ,  typename Functor    = typename binary_functor<F>::type
         ,  bool FunctorIsEmpty = class_is_empty<Functor>::value
         >
class typed_indirect_binary_function : std::binary_function<A1, A2, R>
{
    const F* f;
public:
    /// Constructor captures a pointer to the wrapped function.
    typed_indirect_binary_function(const F* f) : f(f) {}
    
    /// Return the comparator pointer, or `NULL` if the comparator is stateless.
    const F* pointer() const { return f; }

    /// Apply the pointed-to functor to the arguments.
    R operator()(const A1& a1, const A2& a2) const { return (*f)(a1, a2); }
};


/// @copydoc typed_indirect_binary_function
/// Specialization for an empty functor class. (This is only possible if @a F itself is an empty
/// class. If @a F is a function or pointer-to-function type, then the functor will contain a
/// pointer.)
template <typename F, typename A1, typename A2, typename R, typename Functor>
class typed_indirect_binary_function<F, A1, A2, R, Functor, true> : 
    std::binary_function<A1, A2, R>
{
public:
    /// Return `NULL` for the comparator pointer of a stateless comparator.
    const F* pointer() const { return 0; }

    /// Constructor discards the pointer to a stateless functor class.
    typed_indirect_binary_function(const F* f) {}
    
    /// Create an instance of the stateless functor class and apply it to the arguments.
    R operator()(const A1& a1, const A2& a2) const { return F()(a1, a2); }
};


/** Indirect binary function class with inferred types.
 *
 *  This is identical to @ref typed_indirect_binary_function, except that it derives the binary
 *  function argument and result types from the parameter type @a F instead of taking them as
 *  additional template parameters. If @a F is a class type, then it must be an `Adaptable
 *  Binary Function`.
 *
 *  @see typed_indirect_binary_function
 *
 *  @ingroup metaprogramming
 */
template <typename F, typename Functor = typename binary_functor<F>::type>
class indirect_binary_function : 
    typed_indirect_binary_function< F
                                  , typename Functor::first_argument_type
                                  , typename Functor::second_argument_type
                                  , typename Functor::result_type
                                  > 
{
    typedef     typed_indirect_binary_function< F
                                  , typename Functor::first_argument_type
                                  , typename Functor::second_argument_type
                                  , typename Functor::result_type
                                  > 
                base;
public:
    indirect_binary_function(const F* f) : base(f) {} ///< Constructor
};


/** Choose a type based on a boolean constant.
 *
 *  This metafunction is identical to C++11’s condition metafunction.
 *  It needs to be here until we can reasonably assume that users will be compiling with 
 *  C++11.
 *
 *  @tparam Cond    A boolean constant.
 *  @tparam IfTrue  A type.
 *  @tparam IfFalse A type.
 *  @result         The `type` member will be a typedef of @a IfTrue if @a Cond is true,
 *                  and a typedef of @a IfFalse if @a Cond is false.
 *
 *  @ingroup metaprogramming
 */
template <bool Cond, typename IfTrue, typename IfFalse>
struct condition
{
    typedef IfTrue type;    ///< The type selected by the condition.
};

/// @copydoc condition
/// Specialization for @a Cond == `false`.
template <typename IfTrue, typename IfFalse>
struct condition<false, IfTrue, IfFalse>
{
    typedef IfFalse type;   ///< The type selected by the condition.
};


/** @def __CILKRTS_STATIC_ASSERT
 *
 *  @brief Compile-time assertion.
 *
 *  Causes a compilation error if a compile-time constant expression is false.
 *
 *  @par    Usage example.
 *          This assertion  is used in reducer_min_max.h to avoid defining legacy reducer
 *          classes that would not be binary-compatible with the same classes compiled with
 *          earlier versions of the reducer library.
 *
 *              __CILKRTS_STATIC_ASSERT(
 *                  internal::class_is_empty< internal::binary_functor<Compare> >::value, 
 *                  "cilk::reducer_max<Value, Compare> only works with an empty Compare class");
 *
 *  @note   In a C++11 compiler, this is just the language predefined `static_assert`
 *          macro.
 *
 *  @note   In a non-C++11 compiler, the @a Msg string is not directly included in the
 *          compiler error message, but it may appear if the compiler prints the
 *          source line that the error occurred on.
 *
 *  @param  Cond    The expression to test.
 *  @param  Msg     A string explaining the failure.
 *
 *  @ingroup metaprogramming
 */
#if defined(__INTEL_CXX11_MODE__) || defined(__GXX_EXPERIMENTAL_CXX0X__)
#   define  __CILKRTS_STATIC_ASSERT(Cond, Msg) static_assert(Cond, Msg)
#else
#   define __CILKRTS_STATIC_ASSERT(Cond, Msg)                                   \
        typedef int __CILKRTS_STATIC_ASSERT_DUMMY_TYPE                          \
            [::cilk::internal::static_assert_failure<(Cond)>::Success]

/// @cond internal
    template <bool> struct static_assert_failure { };
    template <> struct static_assert_failure<true> { enum { Success = 1 }; };

#   define  __CILKRTS_STATIC_ASSERT_DUMMY_TYPE \
            __CILKRTS_STATIC_ASSERT_DUMMY_TYPE1(__cilkrts_static_assert_, __LINE__)
#   define  __CILKRTS_STATIC_ASSERT_DUMMY_TYPE1(a, b) \
            __CILKRTS_STATIC_ASSERT_DUMMY_TYPE2(a, b)
#   define  __CILKRTS_STATIC_ASSERT_DUMMY_TYPE2(a, b) a ## b
/// @endcond

#endif

/** @name Aligned heap management.
 */
//@{

/** Implementation-specific aligned memory allocation function.
 *
 *  @param  size        The minimum number of bytes to allocate.
 *  @param  alignment   The required alignment (must be a power of 2).
 *  @return             The address of a block of memory of at least @a size
 *                      bytes. The address will be a multiple of @a alignment.
 *                      `NULL` if the allocation fails.
 *
 *  @see                deallocate_aligned()
 */
inline void* allocate_aligned(std::size_t size, std::size_t alignment)
{
#ifdef _WIN32
    return _aligned_malloc(size, alignment);
#else
    void* ptr;
    return (posix_memalign(&ptr, std::max(alignment, sizeof(void*)), size) == 0) ? ptr : 0;
#endif        
}

/** Implementation-specific aligned memory deallocation function.
 *
 *  @param  ptr A pointer which was returned by a call to alloc_aligned().
 */
inline void deallocate_aligned(void* ptr)
{
#ifdef _WIN32
    _aligned_free(ptr);
#else
    std::free(ptr);
#endif        
}

/** Class to allocate and guard an aligned pointer.
 *
 *  A new_aligned_pointer object allocates aligned heap-allocated memory when
 *  it is created, and automatically deallocates it when it is destroyed 
 *  unless its `ok()` function is called.
 *
 *  @tparam T   The type of the object to allocate on the heap. The allocated
 *              will have the size and alignment of an object of type T.
 */
template <typename T>
class new_aligned_pointer {
    void* m_ptr;
public:
    /// Constructor allocates the pointer.
    new_aligned_pointer() : 
        m_ptr(allocate_aligned(sizeof(T), internal::align_of<T>::value)) {}
    /// Destructor deallocates the pointer.
    ~new_aligned_pointer() { if (m_ptr) deallocate_aligned(m_ptr); }
    /// Get the pointer.
    operator void*() { return m_ptr; }
    /// Return the pointer and release the guard.
    T* ok() { 
        T* ptr = static_cast<T*>(m_ptr);
        m_ptr = 0;
        return ptr;
    }
};

//@}

} // namespace internal

/** @name Allocate an aligned data structure on the heap.
 *
 *  `cilk::aligned_new<T>([args])` is equivalent to `new T([args])`, except
 *  that it guarantees that the returned pointer will be at least as aligned
 *  as the alignment requirements of type `T`.
 */
//@{

template <typename T>
T* aligned_new()
{
    internal::new_aligned_pointer<T> ptr;
    new (ptr) T();
    return ptr.ok();
}

template <typename T, typename T1>
T* aligned_new(const T1& x1)
{
    internal::new_aligned_pointer<T> ptr;
    new (ptr) T(x1);
    return ptr.ok();
}

template <typename T, typename T1, typename T2>
T* aligned_new(const T1& x1, const T2& x2)
{
    internal::new_aligned_pointer<T> ptr;
    new (ptr) T(x1, x2);
    return ptr.ok();
}

template <typename T, typename T1, typename T2, typename T3>
T* aligned_new(const T1& x1, const T2& x2, const T3& x3)
{
    internal::new_aligned_pointer<T> ptr;
    new (ptr) T(x1, x2, x3);
    return ptr.ok();
}

template <typename T, typename T1, typename T2, typename T3, typename T4>
T* aligned_new(const T1& x1, const T2& x2, const T3& x3, const T4& x4)
{
    internal::new_aligned_pointer<T> ptr;
    new (ptr) T(x1, x2, x3, x4);
    return ptr.ok();
}

template <typename T, typename T1, typename T2, typename T3, typename T4, typename T5>
T* aligned_new(const T1& x1, const T2& x2, const T3& x3, const T4& x4, const T5& x5)
{
    internal::new_aligned_pointer<T> ptr;
    new (ptr) T(x1, x2, x3, x4, x5);
    return ptr.ok();
}

//@}


template <typename T>
void aligned_delete(const T* ptr)
{
    ptr->~T();
    internal::deallocate_aligned((void*)ptr);
}

} // namespace cilk

#endif // __cplusplus

#endif // METAPROGRAMMING_H_INCLUDED
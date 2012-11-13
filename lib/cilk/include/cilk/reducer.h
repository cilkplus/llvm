/*  reducer.h                  -*-C++-*-
 *
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

#ifndef CILK_REDUCER_H_INCLUDED
#define CILK_REDUCER_H_INCLUDED

#include "cilk/hyperobject_base.h"

/*
 * C++ and C interfaces for Cilk reducer hyperobjects
 */

/* Utility macros */
#define __CILKRTS_MKIDENT(a,b) __CILKRTS_MKIDENT_IMP(a,b,)
#define __CILKRTS_MKIDENT3(a,b,c) __CILKRTS_MKIDENT_IMP(a,b,c)
#define __CILKRTS_MKIDENT_IMP(a,b,c) a ## b ## c

#ifdef __cplusplus

//===================== C++ interfaces ===================================

#include <new>

#ifdef CILK_STUB
// Stub implementations are in the cilk::stub namespace
namespace cilk {
    namespace stub { }
    using namespace stub;
}
#endif

// MONOID CONCEPT AND monoid_base CLASS TEMPLATE
//
// In mathematics, a "monoid" comprises a set of values (type), an associative
// operation on that set, and an identity value for that set and that
// operation.  So for example (integer, +, 0) is a monoid, as is (real, *, 1).
// The 'Monoid' concept in Cilk++ has a typedef and three functions that
// represent a that map to a monoid, (T, OP, IDENTITY), as follows:
//..
//  value_type          is a typedef for T
//  reduce(left,right)  evaluates '*left = *left OP *right'
//  identity(p)         constructs IDENTITY value into the uninitilized '*p'
//  destroy(p)          calls the destructor on the object pointed-to by 'p'
//  allocate(size)      return a pointer to size bytes of raw memory
//  deallocate(p)       deallocate the raw memory at p
//..
// 'left', 'right', and 'p' are all pointers to objects of type 'value_type'.
// All functions must be either 'static' or 'const'.  A class that meets the
// requirements of the 'Monoid' concept is usually stateless, but will
// sometimes contain state used to initialize the identity object.

namespace cilk {

/// The 'monoid_base' class template is a useful base class for a large set
/// of monoid classes for which the identity value is a default-constructed
/// value of type 'T', allocated using operator new.  A derived class of
/// 'monoid_base' need only declare and implement the 'reduce' function.
template <class T>
class monoid_base
{
public:
    /// Type of value for this monoid
    typedef T value_type;

    /// Constructs IDENTITY value into the uninitilized '*p'
    void identity(T* p) const { new ((void*) p) T(); }

    /// Calls the destructor on the object pointed-to by 'p'
    void destroy(T* p) const { p->~T(); }

    /// Return a pointer to size bytes of raw memory
    void* allocate(size_t s) const { return operator new(s); }

    /// Deallocate the raw memory at p
    void deallocate(void* p) const { operator delete(p); }
};

} // end namspace cilk

#ifndef CILK_STUB

namespace cilk {

/// reducer CLASS TEMPLATE
///
/// A reducer is instantiated on a Monoid.  The Monoid provides the value
/// type, associative reduce function, and identity for the reducer.  Function
/// view(), operator*(), and operator()() return the current view of the
/// reducer, although operator()() is deprecated.
template <class Monoid>
class reducer
{
    typedef typename Monoid::value_type value_type;

    __cilkrts_hyperobject_base  base_;
    const Monoid                monoid_; // implementation of monoid interface
    void*                       initialThis_; // Sanity checker

    // Primary (leftmost) view, on its own cache line to avoid false sharing.
    // IMPORTANT: Even though this view is known in advance, access to it from
    // outside the reducer should be through the __cilkrts_hyper_lookup()
    // function only (which is called by the view() function.  This
    // restriction is necessary so that the compiler can assume that
    // __cilkrts_hyper_lookup() is the ONLY source of the address of this
    // object, and can therefore optimize as if it had no aliases.
    __CILKRTS_CACHE_ALIGNED(value_type leftmost_);

    // Wrappers around C monoid dispatch functions
    static void reduce_wrapper(void* r, void* lhs, void* rhs);
    static void identity_wrapper(void* r, void* view);
    static void destroy_wrapper(void* r, void* view);
    static void* allocate_wrapper(void* r, __STDNS size_t bytes);
    static void deallocate_wrapper(void* r, void* view);

    // Used for certain asserts
    bool reducer_is_cache_aligned() const
        { return 0 == ((std::size_t) this & (__CILKRTS_CACHE_LINE__ - 1)); }

    void init();

    // disable copy
    reducer(const reducer&);
    reducer& operator=(const reducer&);

  public:
    reducer() : monoid_(), leftmost_()
    {
        init();
    }

    /// Special case: allow reducer(A) construction from both const and
    /// non-const reference to A.  Allowing this for all argument combinations
    /// is desirable but would result in at least 93 overloads.
    template <typename A>
    explicit reducer(A& a)
        : base_(), monoid_(), leftmost_(a)
    {
        init();
    }

    template <typename A>
    explicit reducer(const A& a)
      : base_(), monoid_(), leftmost_(a)
    {
        init();
    }

    template <typename A, typename B>
    reducer(const A& a, const B& b)
        : base_(), monoid_(), leftmost_(a,b)
    {
        init();
    }

    template <typename A, typename B, typename C>
    reducer(const A& a, const B& b, const C& c)
      : base_(), monoid_(), leftmost_(a,b,c)
    {
        init();
    }

    template <typename A, typename B, typename C, typename D>
    reducer(const A& a, const B& b, const C& c, const D& d)
      : base_(), monoid_(), leftmost_(a,b,c,d)
    {
        init();
    }

    template <typename A, typename B, typename C, typename D, typename E>
    reducer(const A& a, const B& b, const C& c, const D& d, const E& e)
      : base_(), monoid_(), leftmost_(a,b,c,d,e)
    {
        init();
    }

    // Special case: both const and non-const Monoid reference are needed
    // so that reducer(Monoid&) is more specialised than
    // template <typename A> explicit reducer(A& a) and
    // reducer(const Monoid&) is more specialised than
    // template <typename A> explicit reducer(const A& a)
    explicit reducer(Monoid& hmod)
        : base_(), monoid_(hmod), leftmost_()
    {
        init();
    }

    explicit reducer(const Monoid& hmod)
        : base_(), monoid_(hmod), leftmost_()
    {
        init();
    }

    // Special case: allow reducer(Monoid,A) construction from both const and
    // non-const references to A.  Allowing this for all argument combinations
    // is desirable but would result in at least 93 overloads.
    template <typename A>
    reducer(const Monoid& hmod, A& a)
      : base_(), monoid_(hmod), leftmost_(a)
    {
        init();
    }

    template <typename A>
    reducer(const Monoid& hmod, const A& a)
      : base_(), monoid_(hmod), leftmost_(a)
    {
        init();
    }

    template <typename A, typename B>
    reducer(const Monoid& hmod, const A& a, const B& b)
      : base_(), monoid_(hmod), leftmost_(a,b)
    {
        init();
    }

    template <typename A, typename B, typename C>
    reducer(const Monoid& hmod, const A& a, const B& b, const C& c)
      : base_(), monoid_(hmod), leftmost_(a,b,c)
    {
        init();
    }

    template <typename A, typename B, typename C, typename D>
    reducer(const Monoid& hmod, const A& a, const B& b, const C& c,
                const D& d)
      : base_(), monoid_(hmod), leftmost_(a,b,c,d)
    {
        init();
    }

    template <typename A, typename B, typename C, typename D, typename E>
    reducer(const Monoid& hmod, const A& a, const B& b, const C& c,
                const D& d, const E& e)
      : base_(), monoid_(hmod), leftmost_(a,b,c,d,e)
    {
        init();
    }

    __CILKRTS_STRAND_STALE(~reducer());

    /* access the unwrapped object */
    value_type& view() {
        // Look up reducer in current map.  IMPORTANT: Even though the
        // leftmost view is known in advance, access to it should be through
        // the __cilkrts_hyper_lookup() function only.  This restriction is
        // necessary so that the compiler can assume that
        // __cilkrts_hyper_lookup() is the ONLY source of the address of this
        // object, and can therefore optimize as if it had no aliases.
        return *static_cast<value_type *>(__cilkrts_hyper_lookup(&base_));
    }

    value_type const& view() const {
        /* look up reducer in current map */
        return const_cast<reducer*>(this)->view();
    }

    /// "Dereference" reducer to return the current view.
    value_type&       operator*()       { return view(); }
    value_type const& operator*() const { return view(); }

    /// "Dereference" reducer to return the current view.
    value_type*       operator->()       { return &view(); }
    value_type const* operator->() const { return &view(); }

    /// operator()() is deprecated.  Use operator*() instead.
    value_type&       operator()()       { return view(); }
    value_type const& operator()() const { return view(); }

    const Monoid& monoid() const { return monoid_; }
};

template <typename Monoid>
void reducer<Monoid>::init()
{
    static const cilk_c_monoid c_monoid_initializer = {
        (cilk_c_reducer_reduce_fn_t)     &reduce_wrapper,
        (cilk_c_reducer_identity_fn_t)   &identity_wrapper,
        (cilk_c_reducer_destroy_fn_t)    &destroy_wrapper,
        (cilk_c_reducer_allocate_fn_t)   &allocate_wrapper,
        (cilk_c_reducer_deallocate_fn_t) &deallocate_wrapper
    };

#ifdef CILK_CHECK_REDUCER_ALIGNMENT
    // ASSERT THAT LEFTMOST VIEW IS CACHE-LINE ALIGNED:
    // We use an attribute to ensure that the leftmost view, and therefore the
    // entire reducer object, is cache-line (64-byte) aligned.  The compiler
    // enforces this alignment for static- and automatic-duration objects.
    // However, if a reducer or a structure containing a reducer is allocated
    // from the heap using a custom allocator (which typically guarantee only
    // 8- or 16-byte alignment), the compiler cannot guarantee this cache-line
    // alignment.  Certain vector instructions require that the operands be
    // aligned on vector boundaries (up to 16-bytes in SSE, 32-bytes in AVX
    // and 64-bytes in MIC).  At high optimazation levels, the allocator's
    // failure to keep the promised alignment can cause a program to fault
    // mysteriously in a vector instruction.  The assertion is intended to
    // catch this situation.  If the assertion fails, the user is advised
    // to change the way that reducer or the the structure containing the
    // reducer is allocated such that it is guaranteed to be on a 64-byte
    // boundary, thus preventing both the possible crash and false sharing.
    __CILKRTS_ASSERT(reducer_is_cache_aligned());
#endif // CILK_CHECK_REDUCER_ALIGNMENT

    base_.__c_monoid = c_monoid_initializer;
    base_.__flags = 0;
    base_.__view_offset = (char*) &leftmost_ - (char*) this;
    base_.__view_size = sizeof(value_type);
    initialThis_ = this;

    __cilkrts_hyper_create(&base_);
}

template <typename Monoid>
void reducer<Monoid>::reduce_wrapper(void* r, void* lhs, void* rhs)
{
    reducer* self = static_cast<reducer*>(r);
    self->monoid_.reduce(static_cast<value_type*>(lhs),
                         static_cast<value_type*>(rhs));
}

template <typename Monoid>
void reducer<Monoid>::identity_wrapper(void* r, void* view)
{
    reducer* self = static_cast<reducer*>(r);
    self->monoid_.identity(static_cast<value_type*>(view));
}

template <typename Monoid>
void reducer<Monoid>::destroy_wrapper(void* r, void* view)
{
    reducer* self = static_cast<reducer*>(r);
    self->monoid_.destroy(static_cast<value_type*>(view));
}

template <typename Monoid>
void* reducer<Monoid>::allocate_wrapper(void* r, __STDNS size_t bytes)
{
    reducer* self = static_cast<reducer*>(r);
    return self->monoid_.allocate(bytes);
}

template <typename Monoid>
void reducer<Monoid>::deallocate_wrapper(void* r, void* view)
{
    reducer* self = static_cast<reducer*>(r);
    self->monoid_.deallocate(static_cast<value_type*>(view));
}

template <typename Monoid>
__CILKRTS_STRAND_STALE(reducer<Monoid>::~reducer())
{
    // Make sure we haven't been memcopy'd or corrupted
    __CILKRTS_ASSERT(this == initialThis_);
    __cilkrts_hyper_destroy(&base_);
}

} // end namespace cilk

#else // if defined(CILK_STUB)

/**************************************************************************
 *        Stub reducer implementation
 **************************************************************************/

namespace cilk {
namespace stub {

template <class Monoid>
class reducer {
    typedef typename Monoid::value_type value_type;

    const Monoid monoid_;
    value_type   obj_;

    /* disable copy */
    reducer(const reducer&);
    reducer& operator=(const reducer&);

  public:
    reducer() : monoid_(), obj_() { }

    // Special case: allow reducer(A) construction from both const and
    // non-const reference to A.  Allowing this for all argument combinations
    // is desirable but would result in at least 93 overloads.
    template <typename A>
    explicit reducer(A& a)
      : monoid_(), obj_(a) {
    }

    template <typename A>
    explicit reducer(const A& a)
      : monoid_(), obj_(a) {
    }

    template <typename A, typename B>
    reducer(const A& a, const B& b)
      : monoid_(), obj_(a, b) {
    }

    template <typename A, typename B, typename C>
    reducer(const A& a, const B& b, const C& c)
      : monoid_(), obj_(a, b, c)
    {
    }

    template <typename A, typename B, typename C, typename D>
    reducer(const A& a, const B& b, const C& c, const D& d)
      : monoid_(), obj_(a, b, c, d)
    {
    }

    template <typename A, typename B, typename C, typename D, typename E>
    reducer(const A& a, const B& b, const C& c, const D& d, const E& e)
      : monoid_(), obj_(a, b, c, d, e)
    {
    }

    // Special case: both const and non-const Monoid reference are needed
    // so that reducer(Monoid&) is more specialised than
    // template <typename A> explicit reducer(A& a) and
    // reducer(const Monoid&) is more specialised than
    // template <typename A> explicit reducer(const A& a)
    explicit reducer(Monoid& m) : monoid_(m), obj_() { }
    explicit reducer(const Monoid& m) : monoid_(m), obj_() { }

    // Special case: allow reducer(Monoid,A) construction from both const and
    // non-const references to A.  Allowing this for all argument combinations
    // is desirable but would result in at least 93 overloads.
    template <typename A>
    reducer(const Monoid& m, A& a)
      : monoid_(m), obj_(a) {
    }

    template <typename A>
    reducer(const Monoid& m, const A& a)
      : monoid_(m), obj_(a) {
    }

    template <typename A, typename B>
    reducer(const Monoid& m, const A& a, const B& b)
      : monoid_(m), obj_(a, b) {
    }

    template <typename A, typename B, typename C>
    reducer(const Monoid& m, const A& a, const B& b, const C& c)
      : monoid_(m), obj_(a, b, c)
    {
    }

    template <typename A, typename B, typename C, typename D>
    reducer(const Monoid& m, const A& a, const B& b, const C& c, const D& d)
      : monoid_(m), obj_(a, b, c, d)
    {
    }

    template <typename A, typename B, typename C, typename D, typename E>
    reducer(const Monoid& m, const A& a, const B& b, const C& c,
                const D& d, const E& e)
      : monoid_(m), obj_(a, b, c, d, e)
    {
    }

    ~reducer() { }

    value_type&       view()       { return obj_; }
    value_type const& view() const { return obj_; }

    value_type&       operator()()       { return view(); }
    value_type const& operator()() const { return view(); }

    const Monoid& monoid() const { return monoid_; }

}; // stub::reducer

} // end namespace stub
} // end namespace cilk

#endif // CILK_STUB

#endif /* __cplusplus */

/*===================== C interfaces ===================================*/

#if defined(__GNUC__) && !defined(__INTEL_COMPILER)
# define _Typeof __typeof__
#endif

/* MACROS FOR DEFINING AND USING C REDUCERS
 *
 * Example use of these macros
 *
 *  double array[ARRAY_LEN];
 *  double sum()
 *  {
 *      extern void* double_summing_identity();
 *      extern void double_summing_reduce(void* lhs, void* rhs);
 *
 *      CILK_C_DECLARE_REDUCER(double) total =
 *          CILK_C_INIT_REDUCER(sizeof(double),
 *                              double_summing_reduce,
 *                              double_summing_identity,
 *                              free,
 *                              0);
 *      int i;
 *
 *      CILK_C_REGISTER_REDUCER(total);
 *
 *      cilk_for (i = 0; i < ARRAY_LEN; ++i)
 *          REDUCER_VIEW(total) += array[i];
 *
 *      CILK_C_UNREGISTER_REDUCER(total);
 *
 *      // Never access total.value directly -- the compiler optimizer assumes
 *      // that REDUCER_VIEW(total) is the ONLY way to refer to the value. 
 *      return REDUCER_VIEW(total);
 *  }
 */

/***************************************************************************
 *              Common to real and stub implementations
 ***************************************************************************/

__CILKRTS_BEGIN_EXTERN_C

#define __CILKRTS_DECLARE_REDUCER_IDENTITY(name,tn)  CILK_EXPORT         \
    void __CILKRTS_MKIDENT3(name,_identity_,tn)(void* key, void* v)
#define __CILKRTS_DECLARE_REDUCER_REDUCE(name,tn,l,r) CILK_EXPORT        \
    void __CILKRTS_MKIDENT3(name,_reduce_,tn)(void* key, void* l, void* r)
#define __CILKRTS_DECLARE_REDUCER_DESTROY(name,tn,p) CILK_EXPORT         \
    void __CILKRTS_MKIDENT3(name,_destroy_,tn)(void* key, void* p)

__CILKRTS_END_EXTERN_C


#ifndef CILK_STUB

/***************************************************************************
 *              Real implementation
 ***************************************************************************/

__CILKRTS_BEGIN_EXTERN_C

/* Declare a reducer with 'Type' value type */
#define CILK_C_DECLARE_REDUCER(Type) struct {                      \
        __cilkrts_hyperobject_base   __cilkrts_hyperbase;          \
        __CILKRTS_CACHE_ALIGNED(Type value);                       \
    }

/* Initialize a reducer using the Identity, Reduce, and Destroy functions
 * (the monoid) and with an arbitrary-length comma-separated initializer list.
 */
#define CILK_C_INIT_REDUCER(T,Reduce,Identity,Destroy, ...)                  \
    { { { Reduce,Identity,Destroy,                                           \
          __cilkrts_hyperobject_alloc,__cilkrts_hyperobject_dealloc },       \
                0, __CILKRTS_CACHE_LINE__, sizeof(T) }, __VA_ARGS__ }

/* Register a local reducer. */
#define CILK_C_REGISTER_REDUCER(Expr) \
    __cilkrts_hyper_create(&(Expr).__cilkrts_hyperbase)

/* Unregister a local reducer. */
#define CILK_C_UNREGISTER_REDUCER(Expr) \
    __cilkrts_hyper_destroy(&(Expr).__cilkrts_hyperbase)

/* Get the current view for a reducer */
#define REDUCER_VIEW(Expr) (*(_Typeof((Expr).value)*)               \
    __cilkrts_hyper_lookup(&(Expr).__cilkrts_hyperbase))

__CILKRTS_END_EXTERN_C

#else /* if defined(CILK_STUB) */

/***************************************************************************
 *              Stub implementation
 ***************************************************************************/

__CILKRTS_BEGIN_EXTERN_C

/* Declare a reducer with 'Type' value type */
#define CILK_C_DECLARE_REDUCER(Type) struct {                      \
        Type                    value;                             \
    }

/* Initialize a reducer using the Identity, Reduce, and Destroy functions
 * (the monoid) and with an arbitrary-length comma-separated initializer list.
 */
#define CILK_C_INIT_REDUCER(T,Identity,Reduce,Destroy, ...)        \
    { __VA_ARGS__ }

/* Register a local reducer. */
#define CILK_C_REGISTER_REDUCER(Expr) ((void) Expr)

/* Unregister a local reducer. */
#define CILK_C_UNREGISTER_REDUCER(Expr) ((void) Expr)

/* Get the current view for a reducer */
#define REDUCER_VIEW(Expr) ((Expr).value)

__CILKRTS_END_EXTERN_C

#endif /* CILK_STUB */

#endif // CILK_REDUCER_H_INCLUDED

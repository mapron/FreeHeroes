/**
  * This file based mostly on libc++ code. (https://libcxx.llvm.org/ - project URL)
  * libcxx license:
  *
  * Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
  * See https://llvm.org/LICENSE.txt for license information.
  * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
  *
  */

#include <cstddef>
#include <cstdint>

#include <utility>
#include <limits>
#include <climits>

// all inside hacked_std is just copy-paste from libc++ <random>
// that is done for missing binary serialization, and also to be serialized data consistent between implementations.

namespace hacked_libcxx {
using namespace std;
template <class _Sseq, class _Engine>
struct __is_seed_sequence
{
    static constexpr const bool value =
              !is_convertible<_Sseq, typename _Engine::result_type>::value &&
              !is_same<typename remove_cv<_Sseq>::type, _Engine>::value;
};

template <class _UIntType, size_t __w, size_t __n, size_t __m, size_t __r,
          _UIntType __a, size_t __u, _UIntType __d, size_t __s,
          _UIntType __b, size_t __t, _UIntType __c, size_t __l, _UIntType __f>
class mersenne_twister_engine;


template <class _UIntType, size_t __w, size_t __n, size_t __m, size_t __r,
          _UIntType __a, size_t __u, _UIntType __d, size_t __s,
          _UIntType __b, size_t __t, _UIntType __c, size_t __l, _UIntType __f>
class mersenne_twister_engine
{
public:
    // types
    typedef _UIntType result_type;

private:
    result_type __x_[__n];
    size_t      __i_;

    static_assert(  0 <  __m, "mersenne_twister_engine invalid parameters");
    static_assert(__m <= __n, "mersenne_twister_engine invalid parameters");
    static constexpr const result_type _Dt = numeric_limits<result_type>::digits;
    static_assert(__w <= _Dt, "mersenne_twister_engine invalid parameters");
    static_assert(  2 <= __w, "mersenne_twister_engine invalid parameters");
    static_assert(__r <= __w, "mersenne_twister_engine invalid parameters");
    static_assert(__u <= __w, "mersenne_twister_engine invalid parameters");
    static_assert(__s <= __w, "mersenne_twister_engine invalid parameters");
    static_assert(__t <= __w, "mersenne_twister_engine invalid parameters");
    static_assert(__l <= __w, "mersenne_twister_engine invalid parameters");
public:
    static constexpr const result_type _Min = 0;
    static constexpr const result_type _Max = __w == _Dt ? result_type(~0) :
                                                      (result_type(1) << __w) - result_type(1);
    static_assert(_Min < _Max, "mersenne_twister_engine invalid parameters");
    static_assert(__a <= _Max, "mersenne_twister_engine invalid parameters");
    static_assert(__b <= _Max, "mersenne_twister_engine invalid parameters");
    static_assert(__c <= _Max, "mersenne_twister_engine invalid parameters");
    static_assert(__d <= _Max, "mersenne_twister_engine invalid parameters");
    static_assert(__f <= _Max, "mersenne_twister_engine invalid parameters");

    // engine characteristics
    static constexpr const size_t word_size = __w;
    static constexpr const size_t state_size = __n;
    static constexpr const size_t shift_size = __m;
    static constexpr const size_t mask_bits = __r;
    static constexpr const result_type xor_mask = __a;
    static constexpr const size_t tempering_u = __u;
    static constexpr const result_type tempering_d = __d;
    static constexpr const size_t tempering_s = __s;
    static constexpr const result_type tempering_b = __b;
    static constexpr const size_t tempering_t = __t;
    static constexpr const result_type tempering_c = __c;
    static constexpr const size_t tempering_l = __l;
    static constexpr const result_type initialization_multiplier = __f;
    inline
    static constexpr result_type min() { return _Min; }
    inline
    static constexpr result_type max() { return _Max; }
    static constexpr const result_type default_seed = 5489u;

    // constructors and seeding functions
    inline
    explicit mersenne_twister_engine(result_type __sd = default_seed)
        {seed(__sd);}
    template<class _Sseq>
        inline
        explicit mersenne_twister_engine(_Sseq& __q,
        typename enable_if<__is_seed_sequence<_Sseq, mersenne_twister_engine>::value>::type* = 0)
        {seed(__q);}
    void seed(result_type __sd = default_seed);
    template<class _Sseq>
        inline
        typename enable_if
        <
            __is_seed_sequence<_Sseq, mersenne_twister_engine>::value,
            void
        >::type
        seed(_Sseq& __q)
            {__seed(__q, integral_constant<unsigned, 1 + (__w - 1) / 32>());}

    // generating functions
    result_type operator()();
    inline
    void discard(unsigned long long __z) {for (; __z; --__z) operator()();}


private:

    template<class _Sseq>
        void __seed(_Sseq& __q, integral_constant<unsigned, 1>);
    template<class _Sseq>
        void __seed(_Sseq& __q, integral_constant<unsigned, 2>);

    template <size_t __count>
        inline
        static
        typename enable_if
        <
            __count < __w,
            result_type
        >::type
        __lshift(result_type __x) {return (__x << __count) & _Max;}

    template <size_t __count>
        inline
        static
        typename enable_if
        <
            (__count >= __w),
            result_type
        >::type
        __lshift(result_type) {return result_type(0);}

    template <size_t __count>
        inline
        static
        typename enable_if
        <
            __count < _Dt,
            result_type
        >::type
        __rshift(result_type __x) {return __x >> __count;}

    template <size_t __count>
        inline
        static
        typename enable_if
        <
            (__count >= _Dt),
            result_type
        >::type
        __rshift(result_type) {return result_type(0);}
};

template <class _UIntType, size_t __w, size_t __n, size_t __m, size_t __r,
          _UIntType __a, size_t __u, _UIntType __d, size_t __s,
          _UIntType __b, size_t __t, _UIntType __c, size_t __l, _UIntType __f>
    constexpr const size_t
    mersenne_twister_engine<_UIntType, __w, __n, __m, __r, __a, __u, __d, __s, __b, __t, __c, __l, __f>::word_size;

template <class _UIntType, size_t __w, size_t __n, size_t __m, size_t __r,
          _UIntType __a, size_t __u, _UIntType __d, size_t __s,
          _UIntType __b, size_t __t, _UIntType __c, size_t __l, _UIntType __f>
    constexpr const size_t
    mersenne_twister_engine<_UIntType, __w, __n, __m, __r, __a, __u, __d, __s, __b, __t, __c, __l, __f>::state_size;

template <class _UIntType, size_t __w, size_t __n, size_t __m, size_t __r,
          _UIntType __a, size_t __u, _UIntType __d, size_t __s,
          _UIntType __b, size_t __t, _UIntType __c, size_t __l, _UIntType __f>
    constexpr const size_t
    mersenne_twister_engine<_UIntType, __w, __n, __m, __r, __a, __u, __d, __s, __b, __t, __c, __l, __f>::shift_size;

template <class _UIntType, size_t __w, size_t __n, size_t __m, size_t __r,
          _UIntType __a, size_t __u, _UIntType __d, size_t __s,
          _UIntType __b, size_t __t, _UIntType __c, size_t __l, _UIntType __f>
    constexpr const size_t
    mersenne_twister_engine<_UIntType, __w, __n, __m, __r, __a, __u, __d, __s, __b, __t, __c, __l, __f>::mask_bits;

template <class _UIntType, size_t __w, size_t __n, size_t __m, size_t __r,
          _UIntType __a, size_t __u, _UIntType __d, size_t __s,
          _UIntType __b, size_t __t, _UIntType __c, size_t __l, _UIntType __f>
    constexpr const typename mersenne_twister_engine<_UIntType, __w, __n, __m, __r, __a, __u, __d, __s, __b, __t, __c, __l, __f>::result_type
    mersenne_twister_engine<_UIntType, __w, __n, __m, __r, __a, __u, __d, __s, __b, __t, __c, __l, __f>::xor_mask;

template <class _UIntType, size_t __w, size_t __n, size_t __m, size_t __r,
          _UIntType __a, size_t __u, _UIntType __d, size_t __s,
          _UIntType __b, size_t __t, _UIntType __c, size_t __l, _UIntType __f>
    constexpr const size_t
    mersenne_twister_engine<_UIntType, __w, __n, __m, __r, __a, __u, __d, __s, __b, __t, __c, __l, __f>::tempering_u;

template <class _UIntType, size_t __w, size_t __n, size_t __m, size_t __r,
          _UIntType __a, size_t __u, _UIntType __d, size_t __s,
          _UIntType __b, size_t __t, _UIntType __c, size_t __l, _UIntType __f>
    constexpr const typename mersenne_twister_engine<_UIntType, __w, __n, __m, __r, __a, __u, __d, __s, __b, __t, __c, __l, __f>::result_type
    mersenne_twister_engine<_UIntType, __w, __n, __m, __r, __a, __u, __d, __s, __b, __t, __c, __l, __f>::tempering_d;

template <class _UIntType, size_t __w, size_t __n, size_t __m, size_t __r,
          _UIntType __a, size_t __u, _UIntType __d, size_t __s,
          _UIntType __b, size_t __t, _UIntType __c, size_t __l, _UIntType __f>
    constexpr const size_t
    mersenne_twister_engine<_UIntType, __w, __n, __m, __r, __a, __u, __d, __s, __b, __t, __c, __l, __f>::tempering_s;

template <class _UIntType, size_t __w, size_t __n, size_t __m, size_t __r,
          _UIntType __a, size_t __u, _UIntType __d, size_t __s,
          _UIntType __b, size_t __t, _UIntType __c, size_t __l, _UIntType __f>
    constexpr const typename mersenne_twister_engine<_UIntType, __w, __n, __m, __r, __a, __u, __d, __s, __b, __t, __c, __l, __f>::result_type
    mersenne_twister_engine<_UIntType, __w, __n, __m, __r, __a, __u, __d, __s, __b, __t, __c, __l, __f>::tempering_b;

template <class _UIntType, size_t __w, size_t __n, size_t __m, size_t __r,
          _UIntType __a, size_t __u, _UIntType __d, size_t __s,
          _UIntType __b, size_t __t, _UIntType __c, size_t __l, _UIntType __f>
    constexpr const size_t
    mersenne_twister_engine<_UIntType, __w, __n, __m, __r, __a, __u, __d, __s, __b, __t, __c, __l, __f>::tempering_t;

template <class _UIntType, size_t __w, size_t __n, size_t __m, size_t __r,
          _UIntType __a, size_t __u, _UIntType __d, size_t __s,
          _UIntType __b, size_t __t, _UIntType __c, size_t __l, _UIntType __f>
    constexpr const typename mersenne_twister_engine<_UIntType, __w, __n, __m, __r, __a, __u, __d, __s, __b, __t, __c, __l, __f>::result_type
    mersenne_twister_engine<_UIntType, __w, __n, __m, __r, __a, __u, __d, __s, __b, __t, __c, __l, __f>::tempering_c;

template <class _UIntType, size_t __w, size_t __n, size_t __m, size_t __r,
          _UIntType __a, size_t __u, _UIntType __d, size_t __s,
          _UIntType __b, size_t __t, _UIntType __c, size_t __l, _UIntType __f>
    constexpr const size_t
    mersenne_twister_engine<_UIntType, __w, __n, __m, __r, __a, __u, __d, __s, __b, __t, __c, __l, __f>::tempering_l;

template <class _UIntType, size_t __w, size_t __n, size_t __m, size_t __r,
          _UIntType __a, size_t __u, _UIntType __d, size_t __s,
          _UIntType __b, size_t __t, _UIntType __c, size_t __l, _UIntType __f>
    constexpr const typename mersenne_twister_engine<_UIntType, __w, __n, __m, __r, __a, __u, __d, __s, __b, __t, __c, __l, __f>::result_type
    mersenne_twister_engine<_UIntType, __w, __n, __m, __r, __a, __u, __d, __s, __b, __t, __c, __l, __f>::initialization_multiplier;

template <class _UIntType, size_t __w, size_t __n, size_t __m, size_t __r,
          _UIntType __a, size_t __u, _UIntType __d, size_t __s,
          _UIntType __b, size_t __t, _UIntType __c, size_t __l, _UIntType __f>
    constexpr const typename mersenne_twister_engine<_UIntType, __w, __n, __m, __r, __a, __u, __d, __s, __b, __t, __c, __l, __f>::result_type
    mersenne_twister_engine<_UIntType, __w, __n, __m, __r, __a, __u, __d, __s, __b, __t, __c, __l, __f>::default_seed;

template <class _UIntType, size_t __w, size_t __n, size_t __m, size_t __r,
          _UIntType __a, size_t __u, _UIntType __d, size_t __s,
          _UIntType __b, size_t __t, _UIntType __c, size_t __l, _UIntType __f>
void
mersenne_twister_engine<_UIntType, __w, __n, __m, __r, __a, __u, __d, __s, __b,
    __t, __c, __l, __f>::seed(result_type __sd)

{   // __w >= 2
    __x_[0] = __sd & _Max;
    for (size_t __i = 1; __i < __n; ++__i)
        __x_[__i] = (__f * (__x_[__i-1] ^ __rshift<__w - 2>(__x_[__i-1])) + __i) & _Max;
    __i_ = 0;
}

template <class _UIntType, size_t __w, size_t __n, size_t __m, size_t __r,
          _UIntType __a, size_t __u, _UIntType __d, size_t __s,
          _UIntType __b, size_t __t, _UIntType __c, size_t __l, _UIntType __f>
template<class _Sseq>
void
mersenne_twister_engine<_UIntType, __w, __n, __m, __r, __a, __u, __d, __s, __b,
    __t, __c, __l, __f>::__seed(_Sseq& __q, integral_constant<unsigned, 1>)
{
    const unsigned __k = 1;
    uint32_t __ar[__n * __k];
    __q.generate(__ar, __ar + __n * __k);
    for (size_t __i = 0; __i < __n; ++__i)
        __x_[__i] = static_cast<result_type>(__ar[__i] & _Max);
    const result_type __mask = __r == _Dt ? result_type(~0) :
                                       (result_type(1) << __r) - result_type(1);
    __i_ = 0;
    if ((__x_[0] & ~__mask) == 0)
    {
        for (size_t __i = 1; __i < __n; ++__i)
            if (__x_[__i] != 0)
                return;
        __x_[0] = result_type(1) << (__w - 1);
    }
}

template <class _UIntType, size_t __w, size_t __n, size_t __m, size_t __r,
          _UIntType __a, size_t __u, _UIntType __d, size_t __s,
          _UIntType __b, size_t __t, _UIntType __c, size_t __l, _UIntType __f>
template<class _Sseq>
void
mersenne_twister_engine<_UIntType, __w, __n, __m, __r, __a, __u, __d, __s, __b,
    __t, __c, __l, __f>::__seed(_Sseq& __q, integral_constant<unsigned, 2>)
{
    const unsigned __k = 2;
    uint32_t __ar[__n * __k];
    __q.generate(__ar, __ar + __n * __k);
    for (size_t __i = 0; __i < __n; ++__i)
        __x_[__i] = static_cast<result_type>(
            (__ar[2 * __i] + ((uint64_t)__ar[2 * __i + 1] << 32)) & _Max);
    const result_type __mask = __r == _Dt ? result_type(~0) :
                                       (result_type(1) << __r) - result_type(1);
    __i_ = 0;
    if ((__x_[0] & ~__mask) == 0)
    {
        for (size_t __i = 1; __i < __n; ++__i)
            if (__x_[__i] != 0)
                return;
        __x_[0] = result_type(1) << (__w - 1);
    }
}

template <class _UIntType, size_t __w, size_t __n, size_t __m, size_t __r,
          _UIntType __a, size_t __u, _UIntType __d, size_t __s,
          _UIntType __b, size_t __t, _UIntType __c, size_t __l, _UIntType __f>
_UIntType
mersenne_twister_engine<_UIntType, __w, __n, __m, __r, __a, __u, __d, __s, __b,
    __t, __c, __l, __f>::operator()()
{
    const size_t __j = (__i_ + 1) % __n;
    const result_type __mask = __r == _Dt ? result_type(~0) :
                                       (result_type(1) << __r) - result_type(1);
    const result_type _Yp = (__x_[__i_] & ~__mask) | (__x_[__j] & __mask);
    const size_t __k = (__i_ + __m) % __n;
    __x_[__i_] = __x_[__k] ^ __rshift<1>(_Yp) ^ (__a * (_Yp & 1));
    result_type __z = __x_[__i_] ^ (__rshift<__u>(__x_[__i_]) & __d);
    __i_ = __j;
    __z ^= __lshift<__s>(__z) & __b;
    __z ^= __lshift<__t>(__z) & __c;
    return __z ^ __rshift<__l>(__z);
}

//typedef mersenne_twister_engine<uint_fast32_t, 32, 624, 397, 31,
//                                0x9908b0df, 11, 0xffffffff,
//                                7,  0x9d2c5680,
//                                15, 0xefc60000,
//                                18, 1812433253>                         mt19937;
typedef mersenne_twister_engine<uint_fast64_t, 64, 312, 156, 31,
                                0xb5026f5aa96619e9ULL, 29, 0x5555555555555555ULL,
                                17, 0x71d67fffeda60000ULL,
                                37, 0xfff7eee000000000ULL,
                                43, 6364136223846793005ULL>          mt19937_64;




template <unsigned long long _Xp, size_t _Rp>
struct __log2_imp
{
    static const size_t value = _Xp & ((unsigned long long)(1) << _Rp) ? _Rp
                                           : __log2_imp<_Xp, _Rp - 1>::value;
};

template <unsigned long long _Xp>
struct __log2_imp<_Xp, 0>
{
    static const size_t value = 0;
};

template <size_t _Rp>
struct __log2_imp<0, _Rp>
{
    static const size_t value = _Rp + 1;
};

template <class _UIntType, _UIntType _Xp>
struct __log2
{
    static const size_t value = __log2_imp<_Xp,
                                         sizeof(_UIntType) * CHAR_BIT  - 1>::value;
};

template<class _Engine, class _UIntType>
class __independent_bits_engine
{
public:
    // types
    typedef _UIntType result_type;

private:
    typedef typename _Engine::result_type _Engine_result_type;
    typedef typename conditional
        <
            sizeof(_Engine_result_type) <= sizeof(result_type),
                result_type,
                _Engine_result_type
        >::type _Working_result_type;

    _Engine& __e_;
    size_t __w_;
    size_t __w0_;
    size_t __n_;
    size_t __n0_;
    _Working_result_type __y0_;
    _Working_result_type __y1_;
    _Engine_result_type __mask0_;
    _Engine_result_type __mask1_;

    static constexpr const _Working_result_type _Rp = _Engine::max() - _Engine::min()
                                                      + _Working_result_type(1);

    static constexpr const size_t __m = __log2<_Working_result_type, _Rp>::value;
    static constexpr const size_t _WDt = numeric_limits<_Working_result_type>::digits;
    static constexpr const size_t _EDt = numeric_limits<_Engine_result_type>::digits;

public:
    // constructors and seeding functions
    __independent_bits_engine(_Engine& __e, size_t __w);

    // generating functions
    result_type operator()() {return __eval(integral_constant<bool, _Rp != 0>());}

private:
    result_type __eval(false_type);
    result_type __eval(true_type);
};

template<class _Engine, class _UIntType>
__independent_bits_engine<_Engine, _UIntType>
    ::__independent_bits_engine(_Engine& __e, size_t __w)
        : __e_(__e),
          __w_(__w)
{
    __n_ = __w_ / __m + (__w_ % __m != 0);
    __w0_ = __w_ / __n_;
    if (_Rp == 0)
        __y0_ = _Rp;
    else if (__w0_ < _WDt)
        __y0_ = (_Rp >> __w0_) << __w0_;
    else
        __y0_ = 0;
    if (_Rp - __y0_ > __y0_ / __n_)
    {
        ++__n_;
        __w0_ = __w_ / __n_;
        if (__w0_ < _WDt)
            __y0_ = (_Rp >> __w0_) << __w0_;
        else
            __y0_ = 0;
    }
    __n0_ = __n_ - __w_ % __n_;
    if (__w0_ < _WDt - 1)
        __y1_ = (_Rp >> (__w0_ + 1)) << (__w0_ + 1);
    else
        __y1_ = 0;
    __mask0_ = __w0_ > 0 ? _Engine_result_type(~0) >> (_EDt - __w0_) :
                          _Engine_result_type(0);
    __mask1_ = __w0_ < _EDt - 1 ?
                               _Engine_result_type(~0) >> (_EDt - (__w0_ + 1)) :
                               _Engine_result_type(~0);
}

template<class _Engine, class _UIntType>
inline
_UIntType
__independent_bits_engine<_Engine, _UIntType>::__eval(false_type)
{
    return static_cast<result_type>(__e_() & __mask0_);
}

template<class _Engine, class _UIntType>
_UIntType
__independent_bits_engine<_Engine, _UIntType>::__eval(true_type)
{
    const size_t _WRt = numeric_limits<result_type>::digits;
    result_type _Sp = 0;
    for (size_t __k = 0; __k < __n0_; ++__k)
    {
        _Engine_result_type __u;
        do
        {
            __u = __e_() - _Engine::min();
        } while (__u >= __y0_);
        if (__w0_ < _WRt)
            _Sp <<= __w0_;
        else
            _Sp = 0;
        _Sp += __u & __mask0_;
    }
    for (size_t __k = __n0_; __k < __n_; ++__k)
    {
        _Engine_result_type __u;
        do
        {
            __u = __e_() - _Engine::min();
        } while (__u >= __y1_);
        if (__w0_ < _WRt - 1)
            _Sp <<= __w0_ + 1;
        else
            _Sp = 0;
        _Sp += __u & __mask1_;
    }
    return _Sp;
}


template<class _IntType = int>
class uniform_int_distribution
{
public:
    // types
    typedef _IntType result_type;

    class param_type
    {
        result_type __a_;
        result_type __b_;
    public:
        typedef uniform_int_distribution distribution_type;

        explicit param_type(result_type __a = 0,
                            result_type __b = numeric_limits<result_type>::max())
            : __a_(__a), __b_(__b) {}

        result_type a() const {return __a_;}
        result_type b() const {return __b_;}

        friend bool operator==(const param_type& __x, const param_type& __y)
            {return __x.__a_ == __y.__a_ && __x.__b_ == __y.__b_;}
        friend bool operator!=(const param_type& __x, const param_type& __y)
            {return !(__x == __y);}
    };

private:
    param_type __p_;

public:
    // constructors and reset functions
    explicit uniform_int_distribution(result_type __a = 0,
                                      result_type __b = numeric_limits<result_type>::max())
        : __p_(param_type(__a, __b)) {}
    explicit uniform_int_distribution(const param_type& __p) : __p_(__p) {}
    void reset() {}

    // generating functions
    template<class _URNG> result_type operator()(_URNG& __g)
        {return (*this)(__g, __p_);}
    template<class _URNG> result_type operator()(_URNG& __g, const param_type& __p);

    // property functions
    result_type a() const {return __p_.a();}
    result_type b() const {return __p_.b();}

    param_type param() const {return __p_;}
    void param(const param_type& __p) {__p_ = __p;}

    result_type min() const {return a();}
    result_type max() const {return b();}

    friend bool operator==(const uniform_int_distribution& __x,
                           const uniform_int_distribution& __y)
        {return __x.__p_ == __y.__p_;}
    friend bool operator!=(const uniform_int_distribution& __x,
                           const uniform_int_distribution& __y)
            {return !(__x == __y);}
};

template<class _IntType>
template<class _URNG>
typename uniform_int_distribution<_IntType>::result_type
uniform_int_distribution<_IntType>::operator()(_URNG& __g, const param_type& __p)
{
    typedef typename conditional<sizeof(result_type) <= sizeof(uint32_t),
                                            uint32_t, uint64_t>::type _UIntType;
    const _UIntType _Rp = _UIntType(__p.b()) - _UIntType(__p.a()) + _UIntType(1);
    if (_Rp == 1)
        return __p.a();
    const size_t _Dt = numeric_limits<_UIntType>::digits;
    typedef __independent_bits_engine<_URNG, _UIntType> _Eng;
    if (_Rp == 0)
        return static_cast<result_type>(_Eng(__g, _Dt)());
    size_t __w = _Dt - __hacked_clz(_Rp) - 1;
    if ((_Rp & (std::numeric_limits<_UIntType>::max() >> (_Dt - __w))) != 0)
        ++__w;
    _Eng __e(__g, __w);
    _UIntType __u;
    do
    {
        __u = __e();
    } while (__u >= _Rp);
    return static_cast<result_type>(__u + __p.a());
}

} // end of hacked_libcxx

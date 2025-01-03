
//  (C) Copyright John Maddock 2006.
//  (C) Copyright Matt Borland 2024.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_SPECIAL_HERMITE_HPP
#define BOOST_MATH_SPECIAL_HERMITE_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <boost/math/tools/config.hpp>
#include <boost/math/tools/promotion.hpp>
#include <boost/math/special_functions/math_fwd.hpp>
#include <boost/math/policies/error_handling.hpp>

namespace boost{
namespace math{

// Recurrence relation for Hermite polynomials:
template <class T1, class T2, class T3>
BOOST_MATH_GPU_ENABLED inline typename tools::promote_args<T1, T2, T3>::type 
   hermite_next(unsigned n, T1 x, T2 Hn, T3 Hnm1)
{
   using promoted_type = tools::promote_args_t<T1, T2, T3>;
   return (2 * promoted_type(x) * promoted_type(Hn) - 2 * n * promoted_type(Hnm1));
}

namespace detail{

// Implement Hermite polynomials via recurrence:
template <class T>
BOOST_MATH_GPU_ENABLED T hermite_imp(unsigned n, T x)
{
   T p0 = 1;
   T p1 = 2 * x;

   if(n == 0)
      return p0;

   unsigned c = 1;

   while(c < n)
   {
      BOOST_MATH_GPU_SAFE_SWAP(p0, p1);
      p1 = static_cast<T>(hermite_next(c, x, p0, p1));
      ++c;
   }
   return p1;
}

} // namespace detail

template <class T, class Policy>
BOOST_MATH_GPU_ENABLED inline typename tools::promote_args<T>::type 
   hermite(unsigned n, T x, const Policy&)
{
   typedef typename tools::promote_args<T>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   return policies::checked_narrowing_cast<result_type, Policy>(detail::hermite_imp(n, static_cast<value_type>(x)), "boost::math::hermite<%1%>(unsigned, %1%)");
}

template <class T>
BOOST_MATH_GPU_ENABLED inline typename tools::promote_args<T>::type 
   hermite(unsigned n, T x)
{
   return boost::math::hermite(n, x, policies::policy<>());
}

} // namespace math
} // namespace boost

#endif // BOOST_MATH_SPECIAL_HERMITE_HPP




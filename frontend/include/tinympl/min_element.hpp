
/*
//@HEADER
// ************************************************************************
//
//                             min_element.hpp                             
//                         darma_mockup
//              Copyright (C) 2015 Sandia Corporation
// This file was adapted from its original form in the tinympl library.
// The original file bore the following copyright:
//   Copyright (C) 2013, Ennio Barbaro.
// See LEGAL.md for more information.
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact David S. Hollman (dshollm@sandia.gov)
//
// ************************************************************************
//@HEADER
*/


#ifndef TINYMPL_MIN_ELEMENT_HPP
#define TINYMPL_MIN_ELEMENT_HPP

#include "variadic/min_element.hpp"
#include "as_sequence.hpp"
#include "sequence.hpp"
#include "less.hpp"
#include "at.hpp"

namespace tinympl {


/**
 * \ingroup SeqMaxMin
 * \class min_element
 * \brief Compute the index of the smallest element in a sequence
 * \param Sequence the input sequence
 * \param Cmp the comparator function; `Cmp<A,B>::type::value` must be
convertible to bool. Defaults to \ref tinympl::less
 * \return `min_element<...>::type` is an
`std::integral_constant<std::size_t,v>` where `v` is the 0-based index of the
minimum element
 * \sa variadic::min_element
 */
template<class Sequence, template<class ... > class Cmp = less>
struct min_element : min_element<as_sequence_t<Sequence>, Cmp > {};

template<template<class ... > class Cmp, class ... Args>
struct min_element<sequence<Args...>, Cmp> :
    variadic::min_element<Cmp, Args...> {};

template <typename Arg1, typename Arg2, typename... Args>
struct min
  : at<min_element<sequence<Arg1, Arg2, Args...>, less>::type::value,
      sequence<Arg1, Arg2, Args...>
    >::type
{ };

template <typename... Args>
using min_t = typename min<Args...>::type;

} // namespace tinympl

#endif // TINYMPL_MIN_ELEMENT_HPP

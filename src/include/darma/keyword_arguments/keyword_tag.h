/*
//@HEADER
// ************************************************************************
//
//                          keyword_tag.h
//                         darma_new
//              Copyright (C) 2017 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
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
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#ifndef SRC_KEYWORD_ARGUMENTS_KEYWORD_TAG_H_
#define SRC_KEYWORD_ARGUMENTS_KEYWORD_TAG_H_

#include <type_traits>

namespace darma {

namespace detail {

// Tags inherit from this (though this fact isn't used anywhere yet)
struct keyword_tag { };

// To be specialized by keyword arguments
template <typename Tag>
struct tag_data
{ /* intentionally empty */ };

template <class T>
struct is_tag
  : public std::is_base_of<detail::keyword_tag, T>::type { };

template <class T>
struct extract_tag {
  static_assert(is_tag<T>::value, "invalid keyword tag type");
  typedef T type;
};

template <typename T, typename Enable=void>
struct extract_type_if_tag {
  // If it's not a tag, then it's the type we want
  typedef T type;
};

template <typename T>
struct extract_type_if_tag<
  T, typename std::enable_if<is_tag<T>::value>::type
>
{
  typedef typename tag_data<T>::value_t type;
};

struct unknown_type { };


}} // end namespace darma::detail



#endif /* SRC_KEYWORD_ARGUMENTS_KEYWORD_TAG_H_ */

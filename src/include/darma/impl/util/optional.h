/*
//@HEADER
// ************************************************************************
//
//                      optional.h
//                         DARMA
//              Copyright (C) 2017 Sandia Corporation
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

#ifndef DARMA_IMPL_UTIL_OPTIONAL_H
#define DARMA_IMPL_UTIL_OPTIONAL_H

// TODO configuration/compatibility check to make sure optional is available, and our own implementation to use if not

#include <experimental/optional>

namespace darma {
namespace detail {

using nullopt_t = std::experimental::nullopt_t;
static constexpr auto nullopt = std::experimental::nullopt;

template <typename T>
using optional = std::experimental::optional<T>;

template <typename T>
constexpr
optional<std::decay_t<T>> make_optional( T&& value ) {
  return std::experimental::make_optional(std::forward<T>(value));
}

template <typename T, typename... Args>
constexpr
optional<T> make_optional( Args&&... args ) {
  return std::experimental::make_optional<T>(std::forward<Args>(args)...);
}

template <typename T, typename U, typename... Args>
constexpr
optional<T> make_optional( std::initializer_list<U> il, Args&&... args ) {
  return std::experimental::make_optional<T>(il, std::forward<Args>(args)...);
}

static constexpr auto in_place = std::experimental::in_place;

} // end namespace detail
} // end namespace darma

#endif //DARMA_IMPL_UTIL_OPTIONAL_H

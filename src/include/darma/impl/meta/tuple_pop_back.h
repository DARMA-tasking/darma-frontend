/*
//@HEADER
// ************************************************************************
//
//                       tuple_pop_back.hpp
//                         dharma
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

#ifndef DARMA_IMPL_META_TUPLE_POP_BACK_H
#define DARMA_IMPL_META_TUPLE_POP_BACK_H

#include <utility>
#include <tuple>

namespace darma {

namespace meta {

namespace _tuple_pop_back_impl {

template <size_t... Is, typename Tuple>
auto
_impl( std::index_sequence<Is...>, Tuple&& tup ) {
  return std::forward_as_tuple( std::get<Is>( std::forward<Tuple>(tup))... );
};

} // end namespace impl

template <typename Tuple>
auto
tuple_pop_back(Tuple&& tup) {
  static_assert(std::tuple_size<std::decay_t<Tuple>>::value > 0,
    "Can't pop back empty tuple"
  );
  return _tuple_pop_back_impl::_impl(
    std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value - 1>(),
    std::forward<Tuple>(tup)
  );
}


} // end namespace meta

} // end namespace darma


#endif //DARMA_IMPL_META_TUPLE_POP_BACK_H

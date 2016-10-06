/*
//@HEADER
// ************************************************************************
//
//                      tagged_constant.h
//                         DARMA
//              Copyright (C) 2016 Sandia Corporation
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

#ifndef DARMA_IMPL_META_TAGGED_CONSTANT_H
#define DARMA_IMPL_META_TAGGED_CONSTANT_H

#include <cstdlib>

namespace darma_runtime {

namespace meta {

struct tagged_constant { };

namespace detail {

template <typename T /* = tagged_constant (always...) */>
std::size_t& _get_tagged_constant_index_offset() {
  static std::size_t _value = 0;
  return _value;
}

template <typename Catagory>
std::size_t& _get_catagory_index() {
  static std::size_t _cat_value = 0;
  return _cat_value;
}

template <typename Tag, typename Catagory>
std::size_t _get_tagged_constant_value_for_catagory() {
  static std::size_t _this_value = _get_catagory_index<Catagory>()++;
  return _this_value;
};

template <typename Tag>
std::size_t _get_tagged_constant_value() {
  static std::size_t _this_value = _get_tagged_constant_index_offset<meta::tagged_constant>()++;
  return _this_value;
};

} // end namespace detail

} // end namespace meta

} // end namespace darma_runtime

#define DARMA_CREATE_TAGGED_CONSTANT(constant, catagory) \
  namespace detail { \
    struct constant##_tag_t : catagory { \
      static ::std::size_t value_in_catagory() { \
        return ::darma_runtime::meta::detail::_get_tagged_constant_value_for_catagory<constant##_tag_t, catagory>(); \
      } \
      static ::std::size_t value() { \
        return ::darma_runtime::meta::detail::_get_tagged_constant_value<constant##_tag_t>(); \
      } \
    }; \
  } /* end namespace detail */ \
  static constexpr detail::constant##_tag_t constant = { }

#define DARMA_CREATE_TAGGED_CONSTANT_CATAGORY(catagory) \
  struct catagory : ::darma_runtime::meta::tagged_constant { }

#endif //DARMA_IMPL_META_TAGGED_CONSTANT_H
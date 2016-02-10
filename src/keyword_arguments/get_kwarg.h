/*
//@HEADER
// ************************************************************************
//
//                          get_kwarg.h
//                         dharma_new
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

#ifndef SRC_KEYWORD_ARGUMENTS_GET_KWARG_H_
#define SRC_KEYWORD_ARGUMENTS_GET_KWARG_H_

#include <tuple>

#include <tinympl/find_if.hpp>
#include <tinympl/lambda.hpp>
#include <tinympl/bind.hpp>
#include <tinympl/size.hpp>

#include "kwarg_expression.h"

#include "keyword_tag.h"

// TODO validate arg/kwarg expression in terms of rules

namespace dharma_runtime { namespace detail {

namespace m = tinympl;
namespace mv = tinympl::variadic;
namespace mp = tinympl::placeholders;

namespace _get_kwarg_impl {

template <typename Tag, typename... Args>
using var_tag_spot = mv::find_if<
  m::lambda<is_kwarg_expression_with_tag<mp::_, Tag>>::template apply,
  Args...
>;

template <typename Tag, typename Seq>
using seq_tag_spot = m::find_if<
  Seq, m::lambda<is_kwarg_expression_with_tag<mp::_, Tag>>::template apply
>;

} // end namespace _get_kwarg_impl

////////////////////////////////////////////////////////////////////////////////

/* get_typed_kwarg                                                       {{{1 */ #if 1 // begin fold

template <typename Tag>
struct get_typed_kwarg {
  static_assert(tag_data<Tag>::has_type, "Can't get_typed_kwarg from untyped tag");

  typedef typename tag_data<Tag>::value_t return_t;

  template <typename... Args>
  return_t
  operator()(Args&&... args) {
    static constexpr size_t spot = _get_kwarg_impl::var_tag_spot<Tag, Args...>::value;
    // TODO error message readability and compile-time debugability
    static_assert(spot < sizeof...(Args), "missing required keyword argument");
    return std::get<spot>(std::forward_as_tuple(args...)).value();
  }

};

/*                                                                            */ #endif // end fold

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////

/* get_typed_kwarg_with_default                                          {{{1 */ #if 1 // begin fold

namespace _get_kwarg_impl {

template <
  typename Tag, typename ArgsTuple, typename ForwardedArgsTuple, typename ReturnType,
  typename Enable=void /* => kwarg found */
>
struct _default_select_impl {
  static constexpr size_t spot = seq_tag_spot<Tag, ArgsTuple>::value;
  template <typename ReturnTypeCastable>
  ReturnType
  operator()(ReturnTypeCastable&& default_val, ForwardedArgsTuple&& tup) const {
    return std::get<spot>(std::forward<ForwardedArgsTuple>(tup)).value();
  }
};

template <typename Tag, typename ArgsTuple, typename ForwardedArgsTuple, typename ReturnType>
struct _default_select_impl<
  Tag, ArgsTuple, ForwardedArgsTuple, ReturnType,
  std::enable_if_t<seq_tag_spot<Tag, ArgsTuple>::value == m::size<ArgsTuple>::value>
> {
  template <typename ReturnTypeCastable>
  ReturnType
  operator()(ReturnTypeCastable&& default_val, ForwardedArgsTuple&& tup) const {
    return default_val;
  }
};

} // end namespace _get_kwarg_impl

template <typename Tag>
struct get_typed_kwarg_with_default {
  static_assert(tag_data<Tag>::has_type, "Can't get_typed_kwarg from untyped tag");

  typedef typename tag_data<Tag>::value_t return_t;

  template <typename ReturnTCastable, typename... Args>
  return_t
  operator()(ReturnTCastable&& default_val, Args&&... args) const {
    return _get_kwarg_impl::_default_select_impl<Tag,
        std::tuple<Args...>,
        decltype(std::forward_as_tuple(args...)), return_t
    >()(std::forward<ReturnTCastable>(default_val), std::forward_as_tuple(args...));
  }

};

/*                                                                            */ #endif // end fold

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////

/* get_typeless_kwarg_as                                                 {{{1 */ #if 1 // begin fold

template <typename Tag, typename AsType>
struct get_typeless_kwarg_as {
  template <typename... Args>
  AsType
  operator()(Args&&... args) const {
    static constexpr size_t spot = _get_kwarg_impl::var_tag_spot<Tag, Args...>::value;
    // TODO error message readability and compile-time debugability
    static_assert(spot < sizeof...(Args), "missing required keyword argument");
    return std::get<spot>(std::forward_as_tuple(args...)).template value_as<AsType>();
  }
};

/*                                                                            */ #endif // end fold

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////

/* get_typeless_kwarg_as                                                 {{{1 */ #if 1 // begin fold

//namespace _get_kwarg_impl {
//
//template <
//  typename Tag, typename ArgsTuple, typename ForwardedArgsTuple, typename ReturnType,
//  typename Enable=void /* => kwarg found */
//>
//struct _typeless_default_select_impl {
//  static constexpr size_t spot = seq_tag_spot<Tag, ArgsTuple>::value;
//  template <typename ReturnTypeCastable>
//  ReturnType
//  operator()(ReturnTypeCastable&& default_val, ForwardedArgsTuple&& tup) const {
//    return std::get<spot>(std::forward<ForwardedArgsTuple>(tup)).value();
//  }
//};
//
//template <typename Tag, typename ArgsTuple, typename ForwardedArgsTuple, typename ReturnType>
//struct _typeless_default_select_impl<
//  Tag, ArgsTuple, ForwardedArgsTuple, ReturnType,
//  std::enable_if_t<seq_tag_spot<Tag, ArgsTuple>::value == m::size<ArgsTuple>::value>
//> {
//  template <typename ReturnTypeCastable>
//  ReturnType
//  operator()(ReturnTypeCastable&& default_val, ForwardedArgsTuple&& tup) const {
//    return default_val;
//  }
//};
//
//} // end namespace _get_kwarg_impl
//
//template <typename Tag, typename AsType>
//struct get_typeless_kwarg_with_default_as {
//  template <typename AsTypeConvertible, typename... Args>
//  AsType
//  operator()(AsTypeConvertible&& default_val, Args&&... args) const {
//    static constexpr size_t spot = _get_kwarg_impl::var_tag_spot<Tag, Args...>::value;
//  }
//};

/*                                                                            */ #endif // end fold

////////////////////////////////////////////////////////////////////////////////

}} // end namespace dharma_runtime::detail



#endif /* SRC_KEYWORD_ARGUMENTS_GET_KWARG_H_ */

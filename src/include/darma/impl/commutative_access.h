/*
//@HEADER
// ************************************************************************
//
//                      commutative_access.h
//                         DARMA
//              Copyright (C) 2017 Sandia Corporation
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

#ifndef DARMA_IMPL_COMMUTATIVE_ACCESS_H
#define DARMA_IMPL_COMMUTATIVE_ACCESS_H

#include <darma/impl/feature_testing_macros.h>

#if _darma_has_feature(commutative_access_handles)

#include <darma/impl/meta/detection.h> // nonesuch
#include <darma/impl/keyword_arguments/macros.h>
#include <darma/impl/keyword_arguments/parse.h>
#include <darma/impl/handle.h> // is_access_handle
#include "flow_handling.h"
#include "use.h"


DeclareDarmaTypeTransparentKeyword(commutative_access, to_handle);

namespace darma_runtime {

namespace detail {

template <typename T, typename... TraitFlags>
struct _commutative_access_impl {
  template <typename AccessHandleT,
    typename=std::enable_if_t<is_access_handle<std::decay_t<AccessHandleT>>::value>
  >
  decltype(auto)
  operator()(AccessHandleT&& in_handle) const {
    static_assert(
      is_access_handle<std::decay_t<AccessHandleT>>::value
      and std::is_rvalue_reference<AccessHandleT&&>::value,
      "Argument to commutative_access(to_handle=...) must be an rvalue"
        " AccessHandle.  Use std::move() if necessary"
    );
    static_assert(std::is_same<T, meta::nonesuch>::value
      or std::is_same<T, typename std::decay_t<AccessHandleT>::value_type>::value,
      "Don't need to give a template parameter for commutative_access(to_handle=...),"
        " but if you do, it must match the type of the existing parameter"
    );
    static_assert(
      std::decay_t<AccessHandleT>::is_compile_time_scheduling_modifiable,
      "Can't create commutative_access to handle that is marked with scheduling"
        " permissions less than Modify at compile time"
    );
    static_assert(
      std::decay_t<AccessHandleT>::traits::static_scheduling_permissions
        != AccessHandlePermissions::Commutative,
      "Can't create commutative_access to handle that is already marked"
        " commutative at compile time"
    );
    static_assert(
      std::decay_t<AccessHandleT>::traits::static_immediate_permissions
        != AccessHandlePermissions::Commutative,
      "Can't create commutative_access to handle that is already marked"
        " commutative at compile time"
    );
    static_assert(
      std::decay_t<AccessHandleT>::traits::required_scheduling_permissions
        != AccessHandlePermissions::Commutative,
      "Can't create commutative_access to handle that is already marked"
        " commutative at compile time"
    );
    static_assert(
      std::decay_t<AccessHandleT>::traits::required_immediate_permissions
        != AccessHandlePermissions::Commutative,
      "Can't create commutative_access to handle that is already marked"
        " commutative at compile time"
    );
    DARMA_ASSERT_MESSAGE(
      in_handle.current_use_.get() != nullptr,
      "commutative_access(to_handle=...) called on uninitialized or released AccessHandle"
    );
    DARMA_ASSERT_MESSAGE(
      in_handle.current_use_->use->immediate_permissions_ == detail::HandleUse::None,
      "commutative_access(to_handle=...) called on use with immediate permissions that aren't None"
    );
    DARMA_ASSERT_MESSAGE(
      in_handle.current_use_->use->scheduling_permissions_ == detail::HandleUse::Modify,
      "commutative_access(to_handle=...) called on use without scheduling modify permissions"
    );

    using in_handle_t = std::decay_t<AccessHandleT>;
    using out_handle_t = typename in_handle_t::template with_traits<
      typename make_access_handle_traits<
        typename in_handle_t::value_type, TraitFlags...
      >::template from_traits<
        make_access_handle_traits_t<typename in_handle_t::value_type,
          static_scheduling_permissions<AccessHandlePermissions::Commutative>,
          required_scheduling_permissions<AccessHandlePermissions::Commutative>,
          copy_assignable_handle<false>
        >
      >::type
    >;

    auto* rt = abstract::backend::get_backend_runtime();

    auto comm_reg_out = detail::make_next_flow_ptr(
      in_handle.current_use_->use->in_flow_, rt
    );

    // Use the allocator from the out handle (which is the allocator from the
    // in handle unless one is given for the new handle, which is not yet
    // supported anyway)
    auto new_use_ptr = std::allocate_shared<typename out_handle_t::use_holder_t>(
      in_handle.get_allocator(),
      HandleUse(
        in_handle.var_handle_,
        in_handle.current_use_->use->in_flow_ ,
        comm_reg_out,
        HandleUse::Commutative, /* scheduling permissions */
        HandleUse::None /* immediate permissions */
      )
    );
    assert(in_handle.current_use_->use->suspended_out_flow_ == nullptr);
    new_use_ptr->use->suspended_out_flow_ = in_handle.current_use_->use->out_flow_;
    in_handle.current_use_->could_be_alias = false;

    if(in_handle.current_use_->is_registered) {
      new_use_ptr->do_register();
    }

    // Hold the variable handle so that it doesn't expire
    auto var_handle = in_handle.var_handle_;

    // release the use from the expired handle
    in_handle.current_use_ = nullptr;
    in_handle.var_handle_ = nullptr;

    return out_handle_t(
      var_handle, new_use_ptr
    );
  }

  decltype(auto)
  _impl(types::key_t&& key) const {
    auto* rt = abstract::backend::get_backend_runtime();

    // TODO use allocator

    auto var_h = detail::make_shared<detail::VariableHandle<T>>(std::move(key));

    auto in_flow = detail::make_flow_ptr(
      rt->make_initial_flow( var_h )
    );

    auto null_flow = detail::make_flow_ptr(
      rt->make_null_flow( var_h )
    );

    auto out_flow = detail::make_next_flow_ptr(
      in_flow, rt
    );

    return AccessHandle<T,
      typename make_access_handle_traits<T, TraitFlags...>::template from_traits<
        make_access_handle_traits_t<T,
          static_scheduling_permissions<AccessHandlePermissions::Commutative>,
          required_scheduling_permissions<AccessHandlePermissions::Commutative>,
          copy_assignable_handle<false>
        >
      >::type
    >(
      var_h, in_flow, out_flow, detail::HandleUse::Commutative, detail::HandleUse::None,
      null_flow
    );
  }

  template <typename KeyPart1, typename... KeyParts,
    typename=std::enable_if_t<not is_access_handle<KeyPart1>::value>
  >
  decltype(auto)
  operator()(
    variadic_arguments_begin_tag,
    KeyPart1&& kp1, KeyParts&&... kps
  ) const {
    return _impl(darma_runtime::make_key(
      std::forward<KeyPart1>(kp1),
      std::forward<KeyParts>(kps)...
    ));
  };

  template <typename _Ignored_SFINAE=void,
    typename=std::enable_if_t<std::is_void<_Ignored_SFINAE>::value>
  >
  decltype(auto)
  operator()(
    variadic_arguments_begin_tag
  ) const {
    return _impl(
      key_traits<types::key_t>::make_awaiting_backend_assignment_key()
    );
  };

};

} // end namespace detail


template <typename T=meta::nonesuch,
  typename... TraitFlags,
  typename... Args
>
auto
commutative_access(Args&&... args) {

  // TODO allocator kwarg

  using namespace darma_runtime::detail;
  using parser = detail::kwarg_parser<
    overload_description<
      // for converting an existing handle into commutative mode:
      _keyword<
        deduced_parameter, keyword_tags_for_commutative_access::to_handle
      >
    >,
    // for creating a new parameter in commutative mode:
    variadic_positional_overload_description<>
  >;
  using _______________see_calling_context_on_next_line________________ = typename parser::template static_assert_valid_invocation<Args...>;

  return parser()
    .parse_args(std::forward<Args>(args)...)
    .invoke(
      detail::_commutative_access_impl<T, TraitFlags...>{}
    );

};


} // end namespace darma_runtime

#endif // _darma_has_feature(commutative_access_handles)

#endif //DARMA_IMPL_COMMUTATIVE_ACCESS_H

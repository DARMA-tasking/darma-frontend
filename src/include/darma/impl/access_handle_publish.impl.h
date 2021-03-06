/*
//@HEADER
// ************************************************************************
//
//                      access_handle_publish.impl.h.h
//                         DARMA
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

#ifndef DARMA_IMPL_ACCESS_HANDLE_PUBLISH_IMPL_H
#define DARMA_IMPL_ACCESS_HANDLE_PUBLISH_IMPL_H

#include <cstdlib> // size_t

#include <darma/impl/flow_handling.h>
#include <darma/keyword_arguments/parse.h>
#include <darma/impl/capture.h>

// TODO move this to access_handle sub-directory when we're at a stable merge point

namespace darma {

#if _darma_has_feature(publish_fetch)
namespace detail {

template <typename AccessHandleT>
struct _publish_impl {

  AccessHandleT const& this_;

  explicit _publish_impl(AccessHandleT const& ah) : this_(ah) { }

  void operator()(
    types::key_t version, size_t n_readers, bool out
  ) {
    _impl(version, n_readers, out);
  }

  template <typename ReaderIndex, typename RegionContext>
  void operator()(
    ReaderIndex&& idx, RegionContext&& reg_ctxt,
    types::key_t version, size_t n_readers, bool out
  ) {
    _impl(version, n_readers, out);
  }

  template <typename T, typename Traits>
  void _impl(
    AccessHandle<T, Traits> version, size_t n_readers, bool is_publish_out
  ) {
    _impl(make_key(version.get_value()), n_readers, is_publish_out);
  }

  void _impl(
    types::key_t version, size_t n_readers, bool is_publish_out
  ) {
    auto* backend_runtime = abstract::backend::get_backend_runtime();
    detail::PublicationDetails dets(version, n_readers, not is_publish_out);

    #if _darma_has_feature(task_collection_token)
    DARMA_ASSERT_MESSAGE(
      this_.other_private_members_.can_be_published_dynamic(),
      "Tried to publish AccessHandle that is not descended from a"
        " AccessHandleCollection local access"
    );
    dets.token_ = this_.other_private_members_.task_collection_token();
    #endif // _darma_has_feature(task_collection_token)

    auto publish_use_holder = make_captured_use_holder(
      this_.var_handle_base_,
      frontend::Permissions::None,
      frontend::Permissions::Read,
      this_.get_current_use()
    );

    backend_runtime->publish_use(
      publish_use_holder->relinquish_into_destructible_use(),
      &dets
    );
  }
};


} // end namespace detail

template < typename T, typename Traits >
template < typename _Ignored, typename... PublishExprParts >
std::enable_if_t<
  AccessHandle<T, Traits>::is_compile_time_scheduling_readable
    and std::is_same<_Ignored, void>::value
>
AccessHandle<T, Traits>::publish(
  PublishExprParts&&... parts
) const {
  _check_use_exists("publish()");
  DARMA_ASSERT_MESSAGE(
    get_current_use()->use()->scheduling_permissions_ != frontend::Permissions::None,
    "publish() called on handle that can't schedule at least read usage on data (most likely "
      "because it was already released"
  );

  using detail::HandleUse;

  using namespace darma::detail;
  using parser = detail::kwarg_parser<
    overload_description<
      _optional_keyword<converted_parameter, keyword_tags_for_publication::version>,
      _optional_keyword<std::size_t, keyword_tags_for_publication::n_readers>,
      _optional_keyword<bool, keyword_tags_for_publication::out>
    >,
    overload_description<
      _keyword<deduced_parameter, keyword_tags_for_publication::reader_hint>,
      _keyword<deduced_parameter, keyword_tags_for_publication::region_context>,
      _optional_keyword<converted_parameter, keyword_tags_for_publication::version>,
      _optional_keyword<std::size_t, keyword_tags_for_publication::n_readers>,
      _optional_keyword<bool, keyword_tags_for_publication::out>
    >
  >;

  using _______________see_calling_context_on_next_line________________ = typename parser::template static_assert_valid_invocation<PublishExprParts...>;

  parser()
    .with_default_generators(
      keyword_arguments_for_publication::version=[]{ return make_key(); },
      keyword_arguments_for_publication::n_readers=[]{ return 1ul; },
      keyword_arguments_for_publication::out=[]{ return false; }
    )
    .with_converters(
      [](auto&&... key_parts) {
        return make_key(std::forward<decltype(key_parts)>(key_parts)...);
      }
    )
    .parse_args(std::forward<PublishExprParts>(parts)...)
    .invoke(detail::_publish_impl<AccessHandle>(*this));

}
#endif // _darma_has_feature(publish_fetch)

} // end namespace darma


#endif //DARMA_IMPL_ACCESS_HANDLE_PUBLISH_IMPL_H

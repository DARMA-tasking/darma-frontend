/*
//@HEADER
// ************************************************************************
//
//                          read_access.h
//                         dharma_new
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

#ifndef SRC_INCLUDE_DARMA_INTERFACE_APP_READ_ACCESS_H_
#define SRC_INCLUDE_DARMA_INTERFACE_APP_READ_ACCESS_H_

#include <darma/impl/feature_testing_macros.h>

#if OLD_CODE
#if _darma_has_feature(arbitrary_publish_fetch)

#include <darma/interface/app/access_handle.h>
#include <darma/impl/handle_attorneys.h>
#include <darma/impl/flow_handling.h>
#include <darma/impl/compatibility.h>
#include <darma/keyword_arguments/check_allowed_kwargs.h>


namespace darma {

namespace detail {

template <typename U>
struct _read_access_helper {
  template <typename... Args>
  decltype(auto)
  operator()(
    types::key_t const& version_key,
    darma::detail::variadic_arguments_begin_tag,
    Args&&... args
  ) const {
    auto backend_runtime = darma::abstract::backend::get_backend_runtime();
    auto key = darma::make_key(std::forward<decltype(args)>(args)...);
    auto var_h = darma::detail::make_shared<
      darma::detail::VariableHandle<U>
    >(key);

    using namespace darma::detail::flow_relationships;
    using namespace darma::abstract::frontend;

    auto use_holder = std::make_shared<detail::UseHolder>(
      detail::HandleUse(
        var_h,
        frontend::Permissions::Read, frontend::Permissions::None,
        detail::FlowRelationshipImpl(
          abstract::frontend::FlowRelationship::Fetching,
          /* related flow = */ nullptr,
          /* related_is_in = */ false,
          /* version key = */ &version_key,
          /* index = */ 0,
          /* anti_related = */ nullptr,
          /* anti_rel_is_in = */ false
        ),
        null_flow(),
        null_flow(),
        detail::FlowRelationshipImpl(
          abstract::frontend::FlowRelationship::Fetching,
          /* related flow = */ nullptr,
          /* related_is_in = */ false,
          /* version key = */ &version_key,
          /* index = */ 0,
          /* anti_related = */ nullptr,
          /* anti_rel_is_in = */ false
        )
      ), true, false
    );
    use_holder->could_be_alias = true;

    return darma::ReadAccessHandle<U>(
      var_h, std::move(use_holder)
    );

  }
};

} // end namespace detail

template <
  typename U=void,
  typename... KeyExprParts
>
DARMA_ATTRIBUTE_DEPRECATED_WITH_MESSAGE(
  "arbitrary publish fetch is being removed very soon"
)
auto
read_access(
  KeyExprParts&&... parts
) {
  using namespace darma::detail;
  using parser = detail::kwarg_parser<
    variadic_positional_overload_description<
      _optional_keyword<converted_parameter, keyword_tags_for_publication::version>
    >
  >;
  using _______________see_calling_context_on_next_line________________ = typename parser::template static_assert_valid_invocation<KeyExprParts...>;

  return parser()
    .with_converters(
      [](auto&&... parts) {
        return darma::make_key(std::forward<decltype(parts)>(parts)...);
      }
    )
    .with_default_generators(
      keyword_arguments_for_publication::version=[]{ return make_key(); }
    )
    .parse_args(std::forward<KeyExprParts>(parts)...)
    .invoke(detail::_read_access_helper<U>{});
}

} // end namespace darma

#endif // _darma_has_feature(arbitrary_publish_fetch)
#endif // OLD CODE

#endif /* SRC_INCLUDE_DARMA_INTERFACE_APP_READ_ACCESS_H_ */

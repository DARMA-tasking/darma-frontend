/*
//@HEADER
// ************************************************************************
//
//                      allreduce.h
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

#ifndef DARMA_IMPL_COLLECTIVE_ALLREDUCE_H
#define DARMA_IMPL_COLLECTIVE_ALLREDUCE_H

#include <darma/impl/feature_testing_macros.h>

#if _darma_has_feature(simple_collectives)

#include <tinympl/bool.hpp>

#include <darma/impl/keyword_arguments/macros.h>
#include <darma/impl/keyword_arguments/keyword_argument_name.h>
#include <darma/impl/keyword_arguments/check_allowed_kwargs.h>
#include <darma/impl/keyword_arguments/get_kwarg.h>
#include <darma/impl/keyword_arguments/parse.h>
#include <darma/interface/frontend/collective_details.h>

#include <darma/impl/handle.h> // is_access_handle
#include <darma/impl/use.h> // HandleUse
#include <darma/impl/capture.h> // make_captured_use_holder


#include "details.h"


DeclareDarmaTypeTransparentKeyword(collectives, input);
DeclareDarmaTypeTransparentKeyword(collectives, output);
DeclareDarmaTypeTransparentKeyword(collectives, in_out);
DeclareDarmaTypeTransparentKeyword(collectives, piece);
DeclareDarmaTypeTransparentKeyword(collectives, n_pieces);
DeclareDarmaTypeTransparentKeyword(collectives, tag);

namespace darma_runtime {


namespace detail {

struct op_not_given { };

// Umm... this will probably need to change if the value types of InHandle and
// OutHandle differ...
template <typename Op, typename InputHandle, typename OutputHandle>
using _get_collective_details_t = SimpleCollectiveDetails<
  std::conditional_t<
    std::is_same<Op, op_not_given>::value,
    typename darma_runtime::default_reduce_op<
      typename std::decay_t<OutputHandle>::value_type
    >::type,
    Op
  >,
  typename std::decay_t<OutputHandle>::value_type
>;


template<typename Op>
struct all_reduce_impl {

  template <
    typename InputHandle,
    typename OutputHandle
  >
  inline
  std::enable_if_t<
    is_access_handle<std::decay_t<InputHandle>>::value
      and is_access_handle<std::decay_t<OutputHandle>>::value
  >
  operator()(
    InputHandle&& input, OutputHandle&& output, types::key_t const& tag,
    size_t piece, size_t n_pieces
  ) const {

    DARMA_ASSERT_MESSAGE(
      input.current_use_->use.scheduling_permissions_ != HandleUse::None,
      "allreduce() called on handle that can't schedule at least Read usage on "
        "data"
    );
    DARMA_ASSERT_MESSAGE(
      output.current_use_->use.scheduling_permissions_ != HandleUse::None
      and output.current_use_->use.scheduling_permissions_ != HandleUse::Read,
      "allreduce() called on handle that can't schedule at least Write usage on "
        "data"
    );
    // TODO disallow same handle

    auto* backend_runtime = abstract::backend::get_backend_runtime();

    // This is a read capture of the InputHandle and a write capture of the
    // output handle

    auto input_use_holder = detail::make_captured_use_holder(
      input.var_handle_,
      /* requested_scheduling_permissions */
      HandleUse::None,
      /* requested_immediate_permissions */
      HandleUse::Read,
      input.current_use_
    );

    auto output_use_holder = detail::make_captured_use_holder(
      output.var_handle_,
      /* requested_scheduling_permissions */
      HandleUse::None,
      /* requested_immediate_permissions */
      // TODO change this to Write once that is implemented
      HandleUse::Modify,
      output.current_use_
    );

    _get_collective_details_t<
      Op, std::decay_t<InputHandle>, std::decay_t<OutputHandle>
    > details(piece, n_pieces);

    backend_runtime->allreduce_use(
      &(input_use_holder->use),
      &(output_use_holder->use),
      &details, tag
    );

    // TODO decide if the release is necessary here (same as with publish)
    input_use_holder->do_release();
    output_use_holder->do_release();

  }

  //============================================================================

  template <
    typename InOutHandle
  >
  inline
  std::enable_if_t<
    is_access_handle<std::decay_t<InOutHandle>>::value
  >
  operator()(
    InOutHandle&& in_out, types::key_t const& tag,
    size_t piece, size_t n_pieces
  ) const {

    DARMA_ASSERT_MESSAGE(
      in_out.current_use_->use.scheduling_permissions_ != HandleUse::None,
      "allreduce() called on handle that can't schedule at least read usage on "
      "data (most likely because it was already released"
    );
    DARMA_ASSERT_MESSAGE(
      in_out.current_use_->use.scheduling_permissions_ == HandleUse::Permissions::Modify,
      "Can't do an allreduce capture of a handle as in_out without Modify"
      " scheduling permissions"
    );

    auto* backend_runtime = abstract::backend::get_backend_runtime();

    _get_collective_details_t<
      Op, std::decay_t<InOutHandle>, std::decay_t<InOutHandle>
    > details(piece, n_pieces);

    // This is a mod capture.  Need special behavior if we have modify
    // immediate permissions (i.e., forwarding)

    auto collective_use_holder = detail::make_captured_use_holder(
      in_out.var_handle_,
      /* requested_scheduling_permissions */
      HandleUse::None,
      /* requested_immediate_permissions */
      HandleUse::Modify,
      in_out.current_use_
    );

    backend_runtime->allreduce_use(
      &(collective_use_holder->use),
      &(collective_use_holder->use),
      &details, tag
    );

    // Release the captured use
    // TODO decide if this is necessary here (same as with publish)
    collective_use_holder->do_release();

  }

};

} // end namespace detail


template <typename Op = detail::op_not_given, typename... KWArgs>
void allreduce(
  KWArgs&&... kwargs
) {

  using namespace darma_runtime::detail;
  using parser = detail::kwarg_parser<
    overload_description<
      _positional_or_keyword<deduced_parameter, keyword_tags_for_collectives::input>,
      _positional_or_keyword<deduced_parameter, keyword_tags_for_collectives::output>,
      _optional_keyword<converted_parameter, keyword_tags_for_collectives::tag>,
      _optional_keyword<size_t, keyword_tags_for_collectives::piece>,
      _optional_keyword<size_t, keyword_tags_for_collectives::n_pieces>
    >,
    overload_description<
      _positional_or_keyword<deduced_parameter, keyword_tags_for_collectives::in_out>,
      _optional_keyword<converted_parameter, keyword_tags_for_collectives::tag>,
      _optional_keyword<size_t, keyword_tags_for_collectives::piece>,
      _optional_keyword<size_t, keyword_tags_for_collectives::n_pieces>
    >
  >;

  using _______________see_calling_context_on_next_line________________ = typename parser::template static_assert_valid_invocation<KWArgs...>;

  parser()
    .with_default_generators(
      keyword_arguments_for_collectives::tag=[]{ return make_key(); },
      keyword_arguments_for_collectives::piece=[]{
        return abstract::frontend::CollectiveDetails::unknown_contribution();
      },
      keyword_arguments_for_collectives::n_pieces=[] {
        return abstract::frontend::CollectiveDetails::unknown_contribution();
      }
    )
    .with_converters(
      [](auto&&... key_parts) {
        return make_key(std::forward<decltype(key_parts)>(key_parts)...);
      }
    )
    .parse_args(
      std::forward<KWArgs>(kwargs)...
    )
    .invoke(detail::all_reduce_impl<Op>());

};

} // end namespace darma_runtime

#endif // _darma_has_feature(simple_collectives)

#endif //DARMA_IMPL_COLLECTIVE_ALLREDUCE_H

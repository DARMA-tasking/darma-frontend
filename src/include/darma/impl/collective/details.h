/*
//@HEADER
// ************************************************************************
//
//                      details.h
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

#ifndef DARMA_IMPL_COLLECTIVE_DETAILS_H
#define DARMA_IMPL_COLLECTIVE_DETAILS_H

#include <darma/interface/frontend/collective_details.h>
#include <darma/utility/compressed_pair.h>

#include "reduce_op.h"

namespace darma {
namespace detail {

namespace _impl {

template <typename ReduceOp>
ReduceOp const*
_get_static_reduce_op_instance() {
  static const ReduceOp instance = { };
  return &instance;
}

} // end namespace _impl




template <typename ReduceOp, typename T>
class SimpleCollectiveDetails
  : public abstract::frontend::CollectiveDetails

{
  private:

    size_t piece_;
    size_t n_pieces_;

    using wrapper_t = detail::ReduceOperationWrapper<ReduceOp, T>;

#if _darma_has_feature(task_collection_token)
    types::task_collection_token_t token_;
#endif // _darma_has_feature(task_collection_token)

  public:

    SimpleCollectiveDetails(size_t piece, size_t n_pieces)
      : piece_(piece), n_pieces_(n_pieces)
    { }

#if _darma_has_feature(task_collection_token)
    SimpleCollectiveDetails(size_t piece, size_t n_pieces,
      types::task_collection_token_t const& token
    )
      : piece_(piece), n_pieces_(n_pieces), token_(token)
    { }
#endif // _darma_has_feature(task_collection_token)

    size_t
    this_contribution() const override { return piece_; }

    size_t
    n_contributions() const override { return n_pieces_; }

    bool
    is_indexed() const override {
      return wrapper_t::is_indexed;
    }

    abstract::frontend::ReduceOp const*
    reduce_operation() const override {
      return _impl::_get_static_reduce_op_instance<wrapper_t>();
    }

#if _darma_has_feature(task_collection_token)
    types::task_collection_token_t const&
    get_task_collection_token() const override {
      return token_;
    }
#endif // _darma_has_feature(task_collection_token)

};

} // end namespace detail
} // end namespace darma

#endif //DARMA_IMPL_COLLECTIVE_DETAILS_H

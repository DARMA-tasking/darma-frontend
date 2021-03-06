/*
//@HEADER
// ************************************************************************
//
//                      publication_details.h
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

#ifndef DARMA_IMPL_PUBLICATION_DETAILS_H
#define DARMA_IMPL_PUBLICATION_DETAILS_H

#include <darma/interface/frontend/publication_details.h>

namespace darma {
namespace detail {

class PublicationDetails
  : public darma::abstract::frontend::PublicationDetails
{
  public:

    types::key_t version_name;
    size_t n_fetchers;

    types::key_t const&
    get_version_name() const override {
      return version_name;
    }

    size_t
    get_n_fetchers() const override {
      return n_fetchers;
    }

    PublicationDetails(
      types::key_t const& version_name_in,
      size_t n_fetchers_in,
      bool is_within_region = true
    ) : version_name(version_name_in),
        n_fetchers(n_fetchers_in)
    { }

#if _darma_has_feature(task_collection_token)

    types::task_collection_token_t token_;

    types::task_collection_token_t const&
    get_task_collection_token() const override {
      return token_;
    }

#endif // _darma_has_feature(task_collection_token)

};


} // end namespace detail
} // end namespace darma

#endif //DARMA_IMPL_PUBLICATION_DETAILS_H

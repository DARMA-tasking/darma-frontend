/*
//@HEADER
// ************************************************************************
//
//                      darma_regions.h
//                         DARMA
//              Copyright (C) 2018 Sandia Corporation
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

#ifndef DARMAFRONTEND_INTERFACE_BACKEND_DARMA_REGIONS_H
#define DARMAFRONTEND_INTERFACE_BACKEND_DARMA_REGIONS_H

#include <darma/impl/feature_testing_macros.h>
#include <darma_types.h>


namespace darma {
namespace backend {

#if _darma_has_feature(darma_regions)

void
initialize_with_arguments(int& argc, char**& argv);

void
finalize();

types::runtime_instance_token_t
initialize_runtime_instance();

void
register_runtime_instance_quiescence_callback(
  types::runtime_instance_token_t& token,
  std::function<void()> callback
);

//void
//activate_runtime_instance(
//  types::runtime_instance_token_t& token
//);

void
with_active_runtime_instance(
  types::runtime_instance_token_t& token,
  std::function<void()> callback
);

//void
//deactivate_runtime_instance(
//  types::runtime_instance_token_t& token
//);

//void
//await_runtime_instance_quiescence(
//  types::runtime_instance_token_t& token
//);

//void
//destroy_runtime_instance(
//  types::runtime_instance_token_t& token
//);

#endif // _darma_has_feature(darma_regions)

} // end namespace backend
} // end namespace darma

#endif //DARMAFRONTEND_INTERFACE_BACKEND_DARMA_REGIONS_H

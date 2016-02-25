/*
//@HEADER
// ************************************************************************
//
//                          types.h
//                         darma_new
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

#ifndef SRC_ABSTRACT_BACKEND_TYPES_H_
#define SRC_ABSTRACT_BACKEND_TYPES_H_

#include <utility> // std::forward
#include "../defaults/key_fwd.h" // key_traits

#ifdef DARMA_BACKEND_TYPES_INCLUDE
#include DARMA_BACKEND_TYPES_INCLUDE
#endif

// TODO Make the backend specify either DARMA_BACKEND_USE_CUSTOM_KEY or DARMA_BACKEND_USE_DEFAULT_KEY
#ifndef DARMA_BACKEND_CUSTOM_KEY_TYPE
#include "../defaults/key_fwd.h"
#include "../defaults/key.h"
namespace darma_runtime { namespace types {
  typedef darma_runtime::defaults::Key key_t;
}} // end namespace darma_runtime::types
#endif

namespace darma_runtime {

// Key utility functions

template <typename... Args>
inline types::key_t
make_key(Args&&... args) {
  return darma_runtime::detail::key_traits<types::key_t>::maker()(std::forward<Args>(args)...);
}

template <typename TupleType>
inline types::key_t
make_key_from_tuple(TupleType&& tup) {
  return darma_runtime::detail::key_traits<types::key_t>::maker_from_tuple()(std::forward<TupleType>(tup));
}

} // end namespace darma_runtime

#ifndef DARMA_BACKEND_CUSTOM_VERSION_TYPE
#include "../defaults/version.h"
namespace darma_runtime { namespace types {
  typedef darma_runtime::defaults::Version version_t;
}} // end namespace darma_runtime::types
#endif

#ifndef DARMA_BACKEND_CUSTOM_SMART_POINTERS
#include <memory>
namespace darma_runtime { namespace types {
  template <typename... Ts>
  using shared_ptr_template = std::shared_ptr<Ts...>;
  template <typename... Ts>
  using unique_ptr_template = std::unique_ptr<Ts...>;
}} // end namespace darma_runtime::types
#endif

#ifndef DARMA_BACKEND_SPMD_NAME_PREFIX
#define DARMA_BACKEND_SPMD_NAME_PREFIX "___spmd_top_level"
#endif



#endif /* SRC_ABSTRACT_BACKEND_TYPES_H_ */

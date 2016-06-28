/*
//@HEADER
// ************************************************************************
//
//                      task.crtp_impl.h
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

#ifndef DARMA_TASK_CRTP_IMPL_H
#define DARMA_TASK_CRTP_IMPL_H

#include <darma/interface/frontend/types.h>

#include <darma/interface/frontend/task.h>
#include <darma/interface/frontend/use.h>

#include <darma/interface/frontend/frontend_fwd.h>

namespace darma_runtime {
namespace abstract {
namespace frontend {

template <typename ConcreteTask>
inline types::handle_container_template<use_t*> const&
Task<ConcreteTask>::get_dependencies() const {
  return static_cast<ConcreteTask const*>(this)->get_dependencies();
}

template <typename ConcreteTask>
template <typename ReturnType>
inline ReturnType
Task<ConcreteTask>::run() {
  return static_cast<ConcreteTask*>(this)->template run<ReturnType>();
}

template <typename ConcreteTask>
inline types::key_t const&
Task<ConcreteTask>::get_name() const {
  return static_cast<ConcreteTask const*>(this)->get_name();
}

template <typename ConcreteTask>
inline void
Task<ConcreteTask>::set_name(types::key_t const& name_key) {
  return static_cast<ConcreteTask*>(this)->set_name(name_key);
}

template <typename ConcreteTask>
inline bool
Task<ConcreteTask>::is_migratable() const {
  return static_cast<ConcreteTask const*>(this)->is_migratable();
}

template <typename ConcreteTask>
inline size_t
Task<ConcreteTask>::get_packed_size() const {
  return static_cast<ConcreteTask const*>(this)->get_packed_size();
}

template <typename ConcreteTask>
inline void
Task<ConcreteTask>::pack(void* allocated) const {
  return static_cast<ConcreteTask const*>(this)->pack(allocated);
}

} // end namespace frontend
} // end namespace abstract
} // end namespace darma_runtime

#endif //DARMA_TASK_CRTP_IMPL_H
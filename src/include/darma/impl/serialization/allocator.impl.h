/*
//@HEADER
// ************************************************************************
//
//                      allocator.impl.h
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

#ifndef DARMA_IMPL_SERIALIZATION_ALLOCATOR_IMPL_H
#define DARMA_IMPL_SERIALIZATION_ALLOCATOR_IMPL_H

#include <darma/impl/runtime.h>

#include "allocator.h"

namespace darma_runtime {
namespace serialization {

template <typename T, typename BaseAllocator>
typename darma_allocator<T, BaseAllocator>::pointer
darma_allocator<T, BaseAllocator>::allocate(
  size_type n, const_void_pointer _ignored
) {
  return static_cast<darma_allocator<T, BaseAllocator>::pointer>(
    darma_runtime::abstract::backend::get_backend_memory_manager()->allocate(
      n*sizeof(T), detail::DefaultMemoryRequirementDetails{}
    )
  );
};

template <typename T, typename BaseAllocator>
void
darma_allocator<T, BaseAllocator>::deallocate(
  pointer ptr, size_type n
) noexcept {
  darma_runtime::abstract::backend::get_backend_memory_manager()->deallocate(
    static_cast<void*>(ptr), n*sizeof(T)
  );
};

} // end namespace serialization
} // end namespace darma_runtime


#endif //DARMA_IMPL_SERIALIZATION_ALLOCATOR_IMPL_H

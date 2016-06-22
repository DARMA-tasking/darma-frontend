/*
//@HEADER
// ************************************************************************
//
//                      method_runnable.h
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

#ifndef DARMA_METHOD_RUNNABLE_H
#define DARMA_METHOD_RUNNABLE_H

#include <darma/impl/runnable.h>
#include <darma/impl/oo/oo_fwd.h>

namespace darma_runtime {
namespace oo {
namespace detail {

template <
  typename CaptureStruct, typename... Args
>
class MethodRunnable
  : public darma_runtime::detail::RunnableBase
{
  private:

    CaptureStruct captured_;

    // TODO add method arguments

  public:

    // Allow construction from the class that this is a method of
    template <typename OfClassDeduced,
      typename = std::enable_if_t<
        std::is_convertible<OfClassDeduced, typename CaptureStruct::of_class_t>::value
          or is_darma_method_of_class<
            std::decay_t<OfClassDeduced>,
        typename CaptureStruct::of_class_t
      >::value
    >
    >
    constexpr inline explicit
    MethodRunnable(OfClassDeduced&& val)
      : captured_(std::forward<OfClassDeduced>(val))
    { }

    bool run() override {
      captured_.run();
      return true;
    }

    // TODO implement this
    size_t get_index() const  { DARMA_ASSERT_NOT_IMPLEMENTED(); }
};

} // end namespace detail
} // end namespace oo

} // end namespace darma_runtime

#endif //DARMA_METHOD_RUNNABLE_H

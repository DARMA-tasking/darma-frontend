/*
//@HEADER
// ************************************************************************
//
//                      oo.h
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

#ifndef DARMA_OO_H
#define DARMA_OO_H

#include <darma/impl/feature_testing_macros.h>

#if _darma_has_feature(oo_interface)

namespace darma_runtime {
namespace oo {

template <typename ClassName, typename... Args>
struct darma_class;

template <typename OfClass, typename... Args>
struct darma_method;

template <typename OfClass, typename... Args>
struct darma_constructors;

// Useful alias
template <typename OfClass, typename... Args>
using darma_constructor = darma_constructors<OfClass, Args...>;

template <typename... Args>
struct public_methods;

template <typename... Args>
struct private_fields;

template <typename... Args>
struct public_fields;

template <typename... Args>
struct reads_;

template <typename... Args>
struct reads_value_;
template <typename... Args>
using reads_values_ = reads_value_<Args...>;

template <typename... Args>
struct modifies_;

template <typename... Args>
struct modifies_value_;
template <typename... Args>
using modifies_values_ = modifies_value_<Args...>;

} // end namespace oo
} // end namespace darma_runtime

#include <darma/impl/oo/class.h>
#include <darma/impl/oo/method.h>
#include <darma/impl/oo/field.h>
#include <darma/impl/oo/macros.h>

#endif // _darma_has_feature(oo_interface)

#endif //DARMA_OO_H

/*
//@HEADER
// ************************************************************************
//
//                      test_serialization_common.h
//                         DARMA
//              Copyright (C) 2017 Sandia Corporation
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

#ifndef DARMAFRONTEND_TEST_SERIALIZATION_COMMON_H
#define DARMAFRONTEND_TEST_SERIALIZATION_COMMON_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>

class TestSerialize
  : public ::testing::Test
{ };

#define STATIC_ASSERT_SIZABLE(archive, type...) \
static_assert( \
  not darma_runtime::serialization::impl::get_serializer_style< \
    type, archive \
  >::uses_multiple_styles, \
  "Too many sizing interfaces detected for " #type " with " #archive \
); \
static_assert( \
  not darma_runtime::serialization::impl::get_serializer_style< \
    type, archive \
  >::unserializable, \
  #type " detected as unserializable with " #archive \
); \
static_assert( \
  darma_runtime::serialization::is_sizable_with_archive< \
    type, archive \
  >::value, \
  #type " not marked as sizable with " #archive \
)

#define STATIC_ASSERT_PACKABLE(archive, type...) \
static_assert( \
  not darma_runtime::serialization::impl::get_serializer_style< \
    type, archive \
  >::uses_multiple_styles, \
  "Too many packing interfaces detected for " #type " with " #archive \
); \
static_assert( \
  not darma_runtime::serialization::impl::get_serializer_style< \
    type, archive \
  >::unserializable, \
  #type " detected as unserializable with " #archive \
); \
static_assert( \
  darma_runtime::serialization::is_packable_with_archive< \
    type, archive \
  >::value, \
  #type " not marked as packable with " #archive \
)

#define STATIC_ASSERT_UNPACKABLE(archive, type...) \
static_assert( \
  not darma_runtime::serialization::impl::get_serializer_style< \
    type, archive \
  >::uses_multiple_styles, \
  "Too many unpacking interfaces detected for " #type " with " #archive \
); \
static_assert( \
  not darma_runtime::serialization::impl::get_serializer_style< \
    type, archive \
  >::unserializable, \
  #type " detected as unserializable with " #archive \
); \
static_assert( \
  darma_runtime::serialization::is_unpackable_with_archive< \
    type, archive \
  >::value, \
  #type " not marked as unpackable with " #archive \
)

#define STATIC_ASSERT_DIRECTLY_SERIALIZABLE(type...) \
static_assert( \
  darma_runtime::serialization::is_directly_serializable< \
    type \
  >::value, \
  #type " not marked as directly serializable" \
)

#endif //DARMAFRONTEND_TEST_SERIALIZATION_COMMON_H

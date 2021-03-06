/*
//@HEADER
// ************************************************************************
//
//                      test_sso_key.cc
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

#include <gtest/gtest.h>
#include <gmock/gmock.h>


#include <darma/key/SSO_key.h>
#include <darma/key/key_concept.h>
#include <darma/utility/static_assertions.h>
#include <darma/impl/serialization/manager.h>


#include <darma/impl/darma.h>

using namespace darma;
using namespace darma::detail;

////////////////////////////////////////////////////////////////////////////////

DARMA_STATIC_ASSERT_VALID_KEY_TYPE(SSOKey<>);
STATIC_ASSERT_SIZE_IS(SSOKey<>, 64);


using sso_key_t = SSOKey<>;

////////////////////////////////////////////////////////////////////////////////

#include "test_frontend.h"
#include "mock_backend.h"

//
////////////////////////////////////////////////////////////////////////////////

class TestSSOKey
  : public TestFrontend
{
  protected:

    virtual void SetUp() {
      using namespace ::testing;
      setup_mock_runtime<::testing::NiceMock>();
      TestFrontend::SetUp();
    }

    virtual void TearDown() {
      TestFrontend::TearDown();
    }

};

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSSOKey, sso_int) {
  using namespace darma::detail;
  auto maker = typename key_traits<sso_key_t>::maker{};
  sso_key_t k = maker(2,4,8);
  ASSERT_EQ(k.component<0>().as<int>(), 2);
  ASSERT_EQ(k.component<1>().as<int>(), 4);
  ASSERT_EQ(k.component<2>().as<int>(), 8);
  ASSERT_EQ(k.component(0).as<int>(), 2);
  ASSERT_EQ(k.component(1).as<int>(), 4);
  ASSERT_EQ(k.component(2).as<int>(), 8);
}

TEST_F(TestSSOKey, simple_string) {
  using namespace darma::detail;
  auto maker = typename key_traits<sso_key_t>::maker{};
  sso_key_t k = maker("hello", 2, "world!");
  ASSERT_EQ(k.component<0>().as<std::string>(), "hello");
  ASSERT_EQ(k.component<1>().as<int>(), 2);
  ASSERT_EQ(k.component<2>().as<std::string>(), "world!");
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSSOKey, string_split) {
  using namespace darma::detail;
  auto maker = typename key_traits<sso_key_t>::maker{};


  sso_key_t k = maker("hello", 2, 3, 4, 5, 6, "world!", "How is it going today?");
  ASSERT_EQ(k.component<0>().as<std::string>(), "hello");
  ASSERT_EQ(k.component<1>().as<int>(), 2);
  ASSERT_EQ(k.component<6>().as<std::string>(), "world!");
  ASSERT_EQ(k.component<7>().as<std::string>(), "How is it going today?");
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSSOKey, ints_split) {
  using namespace darma::detail;
  auto maker = typename key_traits<sso_key_t>::maker{};
  sso_key_t k = maker(
    1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096
  );
  ASSERT_EQ(k.component<1>().as<int>(), 2);
  ASSERT_EQ(k.component<12>().as<int>(), 4096);
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSSOKey, ints_exact) {
  using namespace darma::detail;
  auto maker = typename key_traits<sso_key_t>::maker{};
  sso_key_t k = maker(
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, "A"
  );
  ASSERT_EQ(k.component<1>().as<int>(), 2);
  ASSERT_EQ(k.component<10>().as<std::string>(), "A");
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSSOKey, ints_exact_2) {
  using namespace darma::detail;
  auto maker = typename key_traits<sso_key_t>::maker{};
  sso_key_t k = maker(
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, "Ab",
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, "AB"
  );
  ASSERT_EQ(k.component<1>().as<int>(), 2);
  ASSERT_EQ(k.component<23>().as<std::string>(), "AB");
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSSOKey, string_span_3) {
  using namespace darma::detail;
  auto maker = typename key_traits<sso_key_t>::maker{};
  const std::string s = ""
    "hello, there.  How is it going today? "
    " Blah blah blah blah blah blah blah 42";
  sso_key_t k = maker(s, 42, s);
  ASSERT_EQ(k.component<2>().as<std::string>(), s);
}
////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSSOKey, equal_multipart) {
  using namespace darma::detail;
  auto maker = typename key_traits<sso_key_t>::maker{};
  sso_key_t k1 = maker("hello", 2, 3, 4, 5, 6, "world!", "How is it going today?");
  sso_key_t k2 = maker("hello", 2, 3, 4, 5, 6, "world!", "How is it going today?");
  ASSERT_TRUE(key_traits<sso_key_t>::key_equal()(k1, k2));
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSSOKey, hash_multipart) {
  using namespace darma::detail;
  auto maker = typename key_traits<sso_key_t>::maker{};
  sso_key_t k1 = maker("hello", 2, 3, 4, 5, 6, "world!", "How is it going today?");
  sso_key_t k2 = maker("hello", 2, 3, 4, 5, 6, "world!", "How is it going today?");
  ASSERT_EQ(key_traits<sso_key_t>::hasher()(k1), key_traits<sso_key_t>::hasher()(k2));
}

////////////////////////////////////////////////////////////////////////////////

typedef enum EnumTestA { OneA=1, TwoA=2, ThreeA=3 } EnumTestA;
typedef enum EnumTestB { OneB=1, TwoB=2, ThreeB=3 } EnumTestB;

TEST_F(TestSSOKey, enums) {
  using namespace darma::detail;
  using namespace ::testing;
  auto maker = typename key_traits<sso_key_t>::maker{};
  auto kA = maker(OneA, TwoB, ThreeA);
  auto kB1 = maker(OneB, TwoA, ThreeB);
  auto kB2 = maker(OneB, TwoA, ThreeB);
  EXPECT_THAT(kA, Not(Eq(kB1)));
  EXPECT_EQ(kB1, kB2);
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSSOKey, equal_key_key) {
  using namespace darma::detail;
  auto maker = typename key_traits<sso_key_t>::maker{};
  sso_key_t k1 = maker("hello", 2, 3, 4, 5, 6, "world!", "How is it going today?");
  sso_key_t k2 = maker(k1);
  ASSERT_TRUE(key_traits<sso_key_t>::key_equal()(k1, k2));
  sso_key_t k3 = maker("hello", 2, 3, 4, 5, 6, "world!", "How is it going today?", "done");
  sso_key_t k4 = maker(k1, "done");
  sso_key_t k5 = maker(k2, "done");
  ASSERT_TRUE(key_traits<sso_key_t>::key_equal()(k3, k4));
  ASSERT_TRUE(key_traits<sso_key_t>::key_equal()(k3, k5));

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSSOKey, equal_backend_assigned) {
  using namespace darma::detail;
  using namespace ::testing;
  auto maker = typename key_traits<sso_key_t>::maker{};
  sso_key_t backendk1 = maker();
  sso_key_t backendk2 = maker();
  ASSERT_EQ(backendk1, backendk2);
  auto backend_maker = typename key_traits<sso_key_t>::backend_maker{};
  auto backendk3 = backend_maker(0);
  auto backendk4 = backend_maker(1);
  ASSERT_THAT(backendk3, Not(Eq(backendk4)));
  ASSERT_THAT(backendk2, Not(Eq(backendk4)));
}

////////////////////////////////////////////////////////////////////////////////


TEST_F(TestSSOKey, serialize_long) {
  using namespace darma::detail;
  using namespace darma::serialization;
  using serialization_handler_t = darma::serialization::SimpleSerializationHandler<>;

  auto maker = typename key_traits<sso_key_t>::maker{};
  sso_key_t k1 = maker("hello", 2, 3, 4, 5, 6, "world!", "How is it going today?");

  auto buff = serialization_handler_t::serialize(k1);

  auto k2 = serialization_handler_t::template deserialize<sso_key_t>(buff);

  ASSERT_TRUE(key_traits<sso_key_t>::key_equal()(k1, k2));
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSSOKey, serialize_small) {
  using namespace darma::detail;
  using namespace darma::serialization;
  using serialization_handler_t = darma::serialization::SimpleSerializationHandler<>;

  auto maker = typename key_traits<sso_key_t>::maker{};
  sso_key_t k1 = maker("me", 2);

  auto buff = serialization_handler_t::serialize(k1);

  auto k2 = serialization_handler_t::template deserialize<sso_key_t>(buff);

  ASSERT_TRUE(key_traits<sso_key_t>::key_equal()(k1, k2));
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestSSOKey, serialize_backend_assigned) {
  using namespace darma::detail;
  using namespace darma::serialization;
  using serialization_handler_t = darma::serialization::SimpleSerializationHandler<>;

  auto maker = typename key_traits<sso_key_t>::backend_maker{};
  sso_key_t k1 = maker(314ul);

  auto buff = serialization_handler_t::serialize(k1);

  auto k2 = serialization_handler_t::template deserialize<sso_key_t>(buff);

  ASSERT_TRUE(key_traits<sso_key_t>::key_equal()(k1, k2));
}

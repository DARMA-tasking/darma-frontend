/*
//@HEADER
// ************************************************************************
//
//                          test_initial_access.cc
//                         dharma_new
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


#include <gtest/gtest.h>

#include <darma/impl/handle.h>
#include <darma/interface/app/initial_access.h>

#include "mock_backend.h"
#include "test_frontend.h"

////////////////////////////////////////////////////////////////////////////////

namespace {

class TestInitialAccess
  : public TestFrontend
{
  protected:

    virtual void SetUp() {
      setup_mock_runtime<::testing::StrictMock>();
      TestFrontend::SetUp();
    }

    virtual void TearDown() {
      TestFrontend::TearDown();
    }
};

} // end anonymous namespace

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestInitialAccess, call_sequence) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;

  Sequence s1;

  auto hm1 = make_same_handle_matcher();

  EXPECT_CALL(*mock_runtime, register_handle(Truly(hm1)))
    .Times(Exactly(1))
    .InSequence(s1);

  // Release read-only usage should happen immedately for initial access,
  // i.e., before runnign the next line of code that triggers the next register_handle
  EXPECT_CALL(*mock_runtime, release_read_only_usage(Truly(hm1)))
    .Times(Exactly(1))
    .InSequence(s1);

  EXPECT_CALL(*mock_runtime, handle_done_with_version_depth(Truly(hm1)))
    .Times(Exactly(1))
    .InSequence(s1);

  EXPECT_CALL(*mock_runtime, release_handle(Truly(hm1)))
    .Times(Exactly(1))
    .InSequence(s1);

  {
    auto tmp = initial_access<int>("hello");
    ASSERT_THAT(hm1.handle, NotNull());
    ASSERT_THAT(hm1.handle, Eq(detail::create_work_attorneys::for_AccessHandle::get_dep_handle(tmp)));
  } // tmp deleted

}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestInitialAccess, call_sequence_assign) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace darma_runtime::keyword_arguments_for_publication;

  Sequence s1, s2;

  auto hm1 = make_same_handle_matcher();
  auto hm2 = make_same_handle_matcher();

  EXPECT_CALL(*mock_runtime, register_handle(Truly(hm1)))
    .InSequence(s1);
  // Release read-only usage should happen immedately for initial access,
  // i.e., before runnign the next line of code that triggers the next register_handle
  EXPECT_CALL(*mock_runtime, release_read_only_usage(Truly(hm1)))
    .InSequence(s1);
  EXPECT_CALL(*mock_runtime, register_handle(AllOf(Truly(hm2), Not(Eq(hm1.handle)))))
    .InSequence(s1);
  // Release read-only usage should happen immedately for initial access,
  // i.e., before runnign the next line of code that triggers the next register_handle
  EXPECT_CALL(*mock_runtime, release_read_only_usage(Truly(hm2)))
    .InSequence(s1);
  EXPECT_CALL(*mock_runtime, handle_done_with_version_depth(Truly(hm1)))
    .InSequence(s1);
  EXPECT_CALL(*mock_runtime, release_handle(Truly(hm1)))
    .InSequence(s1);
  EXPECT_CALL(*mock_runtime, handle_done_with_version_depth(Truly(hm2)))
    .InSequence(s1);
  EXPECT_CALL(*mock_runtime, release_handle(Truly(hm2)))
    .InSequence(s1);

  {

    auto tmp1 = initial_access<int>("hello");

    // Replace tmp1
    tmp1 = initial_access<int>("world");

  } // tmp1

}

////////////////////////////////////////////////////////////////////////////////

// TODO Assert that if a version tag is given, an error should result

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


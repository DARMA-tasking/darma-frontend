/*
//@HEADER
// ************************************************************************
//
//                          test_runtime_release_handle.cc
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

#ifdef TEST_BACKEND_INCLUDE
#  include TEST_BACKEND_INCLUDE
#endif

#include "mock_frontend.h"
#include "helpers.h"

using namespace darma_runtime;

// to disable a test, add a line like the one below
//#define some_test_name DISABLED_some_test_name

namespace {

class RuntimeRelease
  : public ::testing::Test
{
  protected:

    virtual void SetUp() {
      // Emulate argc and argv
      argc_ = 1;
      argv_ = new char*[1];
      argv_[0] = new char[256];
      sprintf(argv_[0], "<mock frontend test>");
      // Make a mock task pointer
      std::unique_ptr<typename abstract::backend::runtime_t::task_t> top_level_task =
          std::make_unique<::testing::NiceMock<mock_frontend::MockTask>>();

      abstract::backend::darma_backend_initialize(
        argc_, argv_, detail::backend_runtime,
        std::move(top_level_task)
      );
      backend_finalized = false;
    }

    virtual void TearDown() {
      if(!backend_finalized) {
        // Clean up from failed tests
        detail::backend_runtime->finalize();
      }
      delete detail::backend_runtime;
      detail::backend_runtime = 0;
      delete[] argv_[0];
      delete[] argv_;
    }

    int argc_;
    char** argv_;
    std::string program_name;

    bool backend_finalized;

    virtual ~RuntimeRelease() { }
};

} // end anonymous namespace

// satisfy subsequent ++v
TEST_F(RuntimeRelease, satisfy_same_depth) {
  using namespace ::testing;

  auto h_0 = make_handle<int, true, true>(MockDependencyHandle::version_t(), "the_key");
  auto next_version = h_0->get_version();
  ++next_version;
  auto h_1 = make_handle<int, true, false>(next_version, "the_key");

  EXPECT_CALL(*h_0.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_0.get(), get_data_block())
    .Times(AtLeast(1));  // when running write-only task

  EXPECT_CALL(*h_1.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_1.get(), get_data_block())
    .Times(AtLeast(1));
  EXPECT_CALL(*h_1.get(), allow_writes())
    .Times(AtMost(1));

  detail::backend_runtime->register_handle(h_0.get());
  detail::backend_runtime->register_handle(h_1.get());

  detail::backend_runtime->release_read_only_usage(h_0.get());

  int value = 42;

  register_write_only_capture(h_0.get(), [&,value]{
    ASSERT_TRUE(h_0->is_satisfied());
    ASSERT_TRUE(h_0->is_writable());
    abstract::backend::DataBlock* data_block = h_0->get_data_block();
    ASSERT_THAT(data_block, NotNull());
    void* data = data_block->get_data();
    ASSERT_THAT(data, NotNull());
    memcpy(data, &value, sizeof(int));
    detail::backend_runtime->release_handle(h_0.get());
  });

  detail::backend_runtime->finalize();
  backend_finalized = true;

  ASSERT_TRUE(h_1->is_satisfied());
  abstract::backend::DataBlock* data_block = h_1->get_data_block();
  ASSERT_THAT(data_block, NotNull());
  void* data = data_block->get_data();
  ASSERT_THAT(data, NotNull());
  ASSERT_THAT(*((int*)data), Eq(value));

  detail::backend_runtime->release_read_only_usage(h_1.get());
  detail::backend_runtime->release_handle(h_1.get());
}

// satisfy subsequent ++(v.pop_subversion())
TEST_F(RuntimeRelease, satisfy_prev_depth) {
  using namespace ::testing;

  auto first_version = MockDependencyHandle::version_t();
  first_version.push_subversion();
  auto h_0 = make_handle<int, true, true>(first_version, "the_key");
  auto next_version = first_version;
  next_version.pop_subversion();
  ++next_version;
  auto h_1 = make_handle<int, true, false>(next_version, "the_key");

  EXPECT_CALL(*h_0.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_0.get(), get_data_block())
    .Times(AtLeast(1));  // when running write-only task

  EXPECT_CALL(*h_1.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_1.get(), get_data_block())
    .Times(AtLeast(1));
  EXPECT_CALL(*h_1.get(), allow_writes())
    .Times(AtMost(1));

  detail::backend_runtime->register_handle(h_0.get());
  detail::backend_runtime->register_handle(h_1.get());

  detail::backend_runtime->handle_done_with_version_depth(h_0.get());
  detail::backend_runtime->release_read_only_usage(h_0.get());

  int value = 42;

  register_write_only_capture(h_0.get(), [&,value]{
    ASSERT_TRUE(h_0->is_satisfied());
    ASSERT_TRUE(h_0->is_writable());
    abstract::backend::DataBlock* data_block = h_0->get_data_block();
    ASSERT_THAT(data_block, NotNull());
    void* data = data_block->get_data();
    ASSERT_THAT(data, NotNull());
    memcpy(data, &value, sizeof(int));
    detail::backend_runtime->release_handle(h_0.get());
  });

  detail::backend_runtime->finalize();
  backend_finalized = true;

  ASSERT_TRUE(h_1->is_satisfied());
  abstract::backend::DataBlock* data_block = h_1->get_data_block();
  ASSERT_THAT(data_block, NotNull());
  void* data = data_block->get_data();
  ASSERT_THAT(data, NotNull());
  ASSERT_THAT(*((int*)data), Eq(value));

  detail::backend_runtime->release_read_only_usage(h_1.get());
  detail::backend_runtime->release_handle(h_1.get());
}

// satisfy subsequent ++(v.push_subversion())
TEST_F(RuntimeRelease, satisfy_next_depth) {
  using namespace ::testing;

  auto h_0 = make_handle<int, true, true>(MockDependencyHandle::version_t(), "the_key");
  auto next_version = h_0->get_version();
  next_version.push_subversion();
  ++next_version;
  auto next_version2 = h_0->get_version();
  ++next_version2;
  auto h_1 = make_handle<int, true, false>(next_version, "the_key");
  // h_2 must exist even though we're not directly using it
  auto h_2 = make_handle<int, true, false>(next_version2, "the_key");

  EXPECT_CALL(*h_0.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_0.get(), get_data_block())
    .Times(AtLeast(1));  // when running write-only task

  EXPECT_CALL(*h_1.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_1.get(), get_data_block())
    .Times(AtLeast(1));
  EXPECT_CALL(*h_1.get(), allow_writes())
    .Times(AtMost(1));

  detail::backend_runtime->register_handle(h_0.get());
  detail::backend_runtime->register_handle(h_1.get());
  detail::backend_runtime->register_handle(h_2.get());

  detail::backend_runtime->release_read_only_usage(h_0.get());

  int value = 42;

  register_write_only_capture(h_0.get(), [&,value]{
    ASSERT_TRUE(h_0->is_satisfied());
    ASSERT_TRUE(h_0->is_writable());
    abstract::backend::DataBlock* data_block = h_0->get_data_block();
    ASSERT_THAT(data_block, NotNull());
    void* data = data_block->get_data();
    ASSERT_THAT(data, NotNull());
    memcpy(data, &value, sizeof(int));
    detail::backend_runtime->release_handle(h_0.get());
  });

  detail::backend_runtime->finalize();
  backend_finalized = true;

  ASSERT_TRUE(h_1->is_satisfied());
  abstract::backend::DataBlock* data_block = h_1->get_data_block();
  ASSERT_THAT(data_block, NotNull());
  void* data = data_block->get_data();
  ASSERT_THAT(data, NotNull());
  ASSERT_THAT(*((int*)data), Eq(value));

  detail::backend_runtime->release_read_only_usage(h_1.get());
  detail::backend_runtime->release_handle(h_1.get());
  detail::backend_runtime->release_read_only_usage(h_2.get());
  detail::backend_runtime->release_handle(h_2.get());
}

// satisfy subsequent ++(v.push_subversion()) even though
// handle_done_with_version_depth was called
TEST_F(RuntimeRelease, satisfy_next_depth_when_last_at_depth) {
  using namespace ::testing;

  auto h_0 = make_handle<int, true, true>(MockDependencyHandle::version_t(), "the_key");
  auto next_version = h_0->get_version();
  next_version.push_subversion();
  ++next_version;
  auto h_1 = make_handle<int, true, false>(next_version, "the_key");

  EXPECT_CALL(*h_0.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_0.get(), get_data_block())
    .Times(AtLeast(1));  // when running write-only task

  EXPECT_CALL(*h_1.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_1.get(), get_data_block())
    .Times(AtLeast(1));
  EXPECT_CALL(*h_1.get(), allow_writes())
    .Times(AtMost(1));

  detail::backend_runtime->register_handle(h_0.get());
  detail::backend_runtime->register_handle(h_1.get());

  detail::backend_runtime->handle_done_with_version_depth(h_0.get());
  detail::backend_runtime->release_read_only_usage(h_0.get());

  int value = 42;

  register_write_only_capture(h_0.get(), [&,value]{
    ASSERT_TRUE(h_0->is_satisfied());
    ASSERT_TRUE(h_0->is_writable());
    abstract::backend::DataBlock* data_block = h_0->get_data_block();
    ASSERT_THAT(data_block, NotNull());
    void* data = data_block->get_data();
    ASSERT_THAT(data, NotNull());
    memcpy(data, &value, sizeof(int));
    detail::backend_runtime->release_handle(h_0.get());
  });

  detail::backend_runtime->finalize();
  backend_finalized = true;

  ASSERT_TRUE(h_1->is_satisfied());
  abstract::backend::DataBlock* data_block = h_1->get_data_block();
  ASSERT_THAT(data_block, NotNull());
  void* data = data_block->get_data();
  ASSERT_THAT(data, NotNull());
  ASSERT_THAT(*((int*)data), Eq(value));

  detail::backend_runtime->release_read_only_usage(h_1.get());
  detail::backend_runtime->release_handle(h_1.get());
}

// satisfy ++(v.push_subversion()) and then ++v
TEST_F(RuntimeRelease, satisfy_next_then_prev) {
  using namespace ::testing;

  auto h_0 = make_handle<int, true, true>(MockDependencyHandle::version_t(), "the_key");
  auto next_version = h_0->get_version();
  next_version.push_subversion();
  ++next_version;
  auto next_version2 = h_0->get_version();
  ++next_version2;
  auto h_1 = make_handle<int, true, false>(next_version, "the_key");
  auto h_2 = make_handle<int, true, false>(next_version2, "the_key");

  EXPECT_CALL(*h_0.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_0.get(), get_data_block())
    .Times(AtLeast(1));  // when running write-only task

  EXPECT_CALL(*h_1.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_1.get(), get_data_block())
    .Times(AtLeast(1));
  EXPECT_CALL(*h_1.get(), allow_writes())
    .Times(Exactly(1));

  EXPECT_CALL(*h_2.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_2.get(), get_data_block())
    .Times(AtLeast(1));
  EXPECT_CALL(*h_2.get(), allow_writes())
    .Times(AtMost(1));

  detail::backend_runtime->register_handle(h_0.get());
  detail::backend_runtime->register_handle(h_1.get());
  detail::backend_runtime->register_handle(h_2.get());

  detail::backend_runtime->release_read_only_usage(h_0.get());

  int value = 42;

  register_write_only_capture(h_0.get(), [&,value]{
    ASSERT_TRUE(h_0->is_satisfied());
    ASSERT_TRUE(h_0->is_writable());
    abstract::backend::DataBlock* data_block = h_0->get_data_block();
    ASSERT_THAT(data_block, NotNull());
    void* data = data_block->get_data();
    ASSERT_THAT(data, NotNull());
    memcpy(data, &value, sizeof(int));
    detail::backend_runtime->release_handle(h_0.get());
  });

  detail::backend_runtime->release_read_only_usage(h_1.get());

  register_read_write_capture(h_1.get(), [&,value]{
    ASSERT_TRUE(h_1->is_satisfied());
    ASSERT_TRUE(h_1->is_writable());
    abstract::backend::DataBlock* data_block = h_1->get_data_block();
    ASSERT_THAT(data_block, NotNull());
    void* data = data_block->get_data();
    ASSERT_THAT(data, NotNull());
    ASSERT_THAT(*((int*)data), Eq(value));
    *((int*)data) = value*2;
    detail::backend_runtime->handle_done_with_version_depth(h_1.get());
    detail::backend_runtime->release_handle(h_1.get());
  });

  detail::backend_runtime->finalize();
  backend_finalized = true;

  ASSERT_TRUE(h_2->is_satisfied());
  abstract::backend::DataBlock* data_block = h_2->get_data_block();
  ASSERT_THAT(data_block, NotNull());
  void* data = data_block->get_data();
  ASSERT_THAT(data, NotNull());
  ASSERT_THAT(*((int*)data), Eq(value*2));

  detail::backend_runtime->release_read_only_usage(h_2.get());
  detail::backend_runtime->release_handle(h_2.get());
}

// satisfy subsequent of subsequent, each at one less depth, when the
// first subsequent has already been released
SERIAL_DISABLED_TEST_F(RuntimeRelease, satisfy_prev_already_released) {
  using namespace ::testing;

  auto first_version = MockDependencyHandle::version_t();
  first_version.push_subversion();
  first_version.push_subversion();
  auto h_0 = make_handle<int, true, true>(first_version, "the_key");
  auto next_version = first_version;
  next_version.pop_subversion();
  ++next_version;
  auto h_1 = make_handle<int, true, false>(next_version, "the_key");
  auto next_version2 = next_version;
  next_version2.pop_subversion();
  ++next_version2;
  auto h_2 = make_handle<int, true, false>(next_version2, "the_key");

  EXPECT_CALL(*h_0.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_0.get(), get_data_block())
    .Times(AtLeast(1));  // when running write-only task

  EXPECT_CALL(*h_2.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_2.get(), get_data_block())
    .Times(AtLeast(1));
  EXPECT_CALL(*h_2.get(), allow_writes())
    .Times(AtMost(1));

  detail::backend_runtime->register_handle(h_0.get());
  detail::backend_runtime->register_handle(h_1.get());
  detail::backend_runtime->register_handle(h_2.get());

  detail::backend_runtime->handle_done_with_version_depth(h_0.get());
  detail::backend_runtime->release_read_only_usage(h_0.get());

  detail::backend_runtime->handle_done_with_version_depth(h_1.get());
  detail::backend_runtime->release_read_only_usage(h_1.get());
  detail::backend_runtime->release_handle(h_1.get());
  // h_0's subsequent, h_1, was just released prematurely, so h_0 needs
  // to satisfy h_2 instead 

  int value = 42;

  register_write_only_capture(h_0.get(), [&,value]{
    ASSERT_TRUE(h_0->is_satisfied());
    ASSERT_TRUE(h_0->is_writable());
    abstract::backend::DataBlock* data_block = h_0->get_data_block();
    ASSERT_THAT(data_block, NotNull());
    void* data = data_block->get_data();
    ASSERT_THAT(data, NotNull());
    memcpy(data, &value, sizeof(int));
    detail::backend_runtime->release_handle(h_0.get());
  });

  detail::backend_runtime->finalize();
  backend_finalized = true;

  ASSERT_TRUE(h_2->is_satisfied());
  abstract::backend::DataBlock* data_block = h_2->get_data_block();
  ASSERT_THAT(data_block, NotNull());
  void* data = data_block->get_data();
  ASSERT_THAT(data, NotNull());
  ASSERT_THAT(*((int*)data), Eq(value));

  detail::backend_runtime->release_read_only_usage(h_2.get());
  detail::backend_runtime->release_handle(h_2.get());
}

// satisfy subsequent of subsequent, the first at the same depth and
// the second at the previous depth, when the first subsequent has
// already been released
SERIAL_DISABLED_TEST_F(RuntimeRelease, satisfy_same_already_released) {
  using namespace ::testing;

  auto first_version = MockDependencyHandle::version_t();
  first_version.push_subversion();
  first_version.push_subversion();
  auto h_0 = make_handle<int, true, true>(first_version, "the_key");
  auto next_version = first_version;
  ++next_version;
  auto h_1 = make_handle<int, true, false>(next_version, "the_key");
  auto next_version2 = first_version;
  next_version2.pop_subversion();
  ++next_version2;
  auto h_2 = make_handle<int, true, false>(next_version2, "the_key");

  EXPECT_CALL(*h_0.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_0.get(), get_data_block())
    .Times(AtLeast(1));  // when running write-only task

  EXPECT_CALL(*h_2.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_2.get(), get_data_block())
    .Times(AtLeast(1));
  EXPECT_CALL(*h_2.get(), allow_writes())
    .Times(AtMost(1));

  detail::backend_runtime->register_handle(h_0.get());
  detail::backend_runtime->register_handle(h_1.get());
  detail::backend_runtime->register_handle(h_2.get());

  detail::backend_runtime->release_read_only_usage(h_0.get());

  detail::backend_runtime->handle_done_with_version_depth(h_1.get());
  detail::backend_runtime->release_read_only_usage(h_1.get());
  detail::backend_runtime->release_handle(h_1.get());
  // h_0's subsequent, h_1, was just released prematurely, so h_0 needs
  // to satisfy h_2 instead 

  int value = 42;

  register_write_only_capture(h_0.get(), [&,value]{
    ASSERT_TRUE(h_0->is_satisfied());
    ASSERT_TRUE(h_0->is_writable());
    abstract::backend::DataBlock* data_block = h_0->get_data_block();
    ASSERT_THAT(data_block, NotNull());
    void* data = data_block->get_data();
    ASSERT_THAT(data, NotNull());
    memcpy(data, &value, sizeof(int));
    detail::backend_runtime->release_handle(h_0.get());
  });

  detail::backend_runtime->finalize();
  backend_finalized = true;

  ASSERT_TRUE(h_2->is_satisfied());
  abstract::backend::DataBlock* data_block = h_2->get_data_block();
  ASSERT_THAT(data_block, NotNull());
  void* data = data_block->get_data();
  ASSERT_THAT(data, NotNull());
  ASSERT_THAT(*((int*)data), Eq(value));

  detail::backend_runtime->release_read_only_usage(h_2.get());
  detail::backend_runtime->release_handle(h_2.get());
}

// satisfy subsequent of subsequent of subsequent, each at one less depth
// when the first two subsequents have already been released
SERIAL_DISABLED_TEST_F(RuntimeRelease, satisfy_prev_two_already_released) {
  using namespace ::testing;

  auto first_version = MockDependencyHandle::version_t();
  first_version.push_subversion();
  first_version.push_subversion();
  first_version.push_subversion();
  first_version.push_subversion();
  auto h_0 = make_handle<int, true, true>(first_version, "the_key");
  auto next_version = first_version;
  next_version.pop_subversion();
  ++next_version;
  auto h_1 = make_handle<int, true, false>(next_version, "the_key");
  auto next_version2 = next_version;
  next_version2.pop_subversion();
  ++next_version2;
  auto h_2 = make_handle<int, true, false>(next_version2, "the_key");
  auto next_version3 = next_version2;
  next_version3.pop_subversion();
  ++next_version3;
  auto h_3 = make_handle<int, true, false>(next_version3, "the_key");

  EXPECT_CALL(*h_0.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_0.get(), get_data_block())
    .Times(AtLeast(1));  // when running write-only task

  EXPECT_CALL(*h_3.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_3.get(), get_data_block())
    .Times(AtLeast(1));
  EXPECT_CALL(*h_3.get(), allow_writes())
    .Times(AtMost(1));

  detail::backend_runtime->register_handle(h_0.get());
  detail::backend_runtime->register_handle(h_1.get());
  detail::backend_runtime->register_handle(h_2.get());
  detail::backend_runtime->register_handle(h_3.get());

  detail::backend_runtime->handle_done_with_version_depth(h_0.get());
  detail::backend_runtime->release_read_only_usage(h_0.get());

  detail::backend_runtime->handle_done_with_version_depth(h_2.get());
  detail::backend_runtime->release_read_only_usage(h_2.get());
  detail::backend_runtime->release_handle(h_2.get());
  detail::backend_runtime->handle_done_with_version_depth(h_1.get());
  detail::backend_runtime->release_read_only_usage(h_1.get());
  detail::backend_runtime->release_handle(h_1.get());
  // h_1's subsequent, h_2, was just released prematurely
  // h_0's subsequent, h_1, was also just released prematurely
  // so h_0 needs to satisfy h_3 instead

  int value = 42;

  register_write_only_capture(h_0.get(), [&,value]{
    ASSERT_TRUE(h_0->is_satisfied());
    ASSERT_TRUE(h_0->is_writable());
    abstract::backend::DataBlock* data_block = h_0->get_data_block();
    ASSERT_THAT(data_block, NotNull());
    void* data = data_block->get_data();
    ASSERT_THAT(data, NotNull());
    memcpy(data, &value, sizeof(int));
    detail::backend_runtime->release_handle(h_0.get());
  });

  detail::backend_runtime->finalize();
  backend_finalized = true;

  ASSERT_TRUE(h_3->is_satisfied());
  abstract::backend::DataBlock* data_block = h_3->get_data_block();
  ASSERT_THAT(data_block, NotNull());
  void* data = data_block->get_data();
  ASSERT_THAT(data, NotNull());
  ASSERT_THAT(*((int*)data), Eq(value));

  detail::backend_runtime->release_read_only_usage(h_3.get());
  detail::backend_runtime->release_handle(h_3.get());
}

// no subsequent only because we run out of version depth
SERIAL_DISABLED_TEST_F(RuntimeRelease, satisfy_none_due_to_min_depth) {
  using namespace ::testing;

  auto first_version = MockDependencyHandle::version_t();
  first_version.push_subversion();
  first_version.push_subversion();
  first_version.push_subversion();  // 0.0.0.0
  auto h_0 = make_handle<int, true, true>(first_version, "the_key");
  auto next_version = first_version;
  next_version.pop_subversion();
  ++next_version;  // 0.0.1
  auto h_1 = make_handle<int, true, false>(next_version, "the_key");
  auto next_version2 = next_version;
  next_version2.pop_subversion();
  ++next_version2;  // 0.1
  auto h_2 = make_handle<int, true, false>(next_version2, "the_key");
  auto next_version3 = next_version2;
  next_version3.pop_subversion();
  ++next_version3;  // 1
  auto h_3 = make_handle<int, true, false>(next_version3, "the_key");

  EXPECT_CALL(*h_0.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_0.get(), get_data_block())
    .Times(AtLeast(1));  // when running write-only task

  detail::backend_runtime->register_handle(h_0.get());
  detail::backend_runtime->register_handle(h_1.get());
  detail::backend_runtime->register_handle(h_2.get());
  detail::backend_runtime->register_handle(h_3.get());

  detail::backend_runtime->handle_done_with_version_depth(h_0.get());
  detail::backend_runtime->release_read_only_usage(h_0.get());

  detail::backend_runtime->handle_done_with_version_depth(h_3.get());
  detail::backend_runtime->release_read_only_usage(h_3.get());
  detail::backend_runtime->release_handle(h_3.get());

  detail::backend_runtime->handle_done_with_version_depth(h_2.get());
  detail::backend_runtime->release_read_only_usage(h_2.get());
  detail::backend_runtime->release_handle(h_2.get());

  detail::backend_runtime->handle_done_with_version_depth(h_1.get());
  detail::backend_runtime->release_read_only_usage(h_1.get());
  detail::backend_runtime->release_handle(h_1.get());

  // h_2's subsequent, h_3, was just released prematurely
  // h_1's subsequent, h_2, was also just released prematurely
  // h_0's subsequent, h_1, was also just released prematurely

  // we would satisfy h_3's subsequent, except it is already at
  // minimum version depth, so it has no subsequent

  int value = 42;

  register_write_only_capture(h_0.get(), [&,value]{
    ASSERT_TRUE(h_0->is_satisfied());
    ASSERT_TRUE(h_0->is_writable());
    abstract::backend::DataBlock* data_block = h_0->get_data_block();
    ASSERT_THAT(data_block, NotNull());
    void* data = data_block->get_data();
    ASSERT_THAT(data, NotNull());
    memcpy(data, &value, sizeof(int));
    detail::backend_runtime->release_handle(h_0.get());
  });

  detail::backend_runtime->finalize();
  backend_finalized = true;
}

// satisfy subsequent using different version increment behavior
// (make sure runtime doesn't assume constant behavior)
TEST_F(RuntimeRelease, satisfy_different_version_incr_behavior) {
  using namespace ::testing;

  auto first_version = MockDependencyHandle::version_t();
  auto h_0 = make_handle<int, true, true>(MockDependencyHandle::version_t(), "the_key");  // 0

  EXPECT_CALL(*h_0.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_0.get(), get_data_block())
    .Times(AtLeast(1));  // when running write-only task

  detail::backend_runtime->register_handle(h_0.get());
  detail::backend_runtime->release_read_only_usage(h_0.get());

  // register with v=0 but release with v=0.0.0
  h_0->increase_version_depth(2);

  auto next_version = h_0->get_version();
  ++next_version;  // 0.0.1
  auto h_1 = make_handle<int, true, false>(next_version, "the_key");

  EXPECT_CALL(*h_1.get(), satisfy_with_data_block(_))
    .Times(Exactly(1));
  EXPECT_CALL(*h_1.get(), get_data_block())
    .Times(AtLeast(1));
  EXPECT_CALL(*h_1.get(), allow_writes())
    .Times(AtMost(1));

  detail::backend_runtime->register_handle(h_1.get());

  int value = 42;

  register_write_only_capture(h_0.get(), [&,value]{
    ASSERT_TRUE(h_0->is_satisfied());
    ASSERT_TRUE(h_0->is_writable());
    abstract::backend::DataBlock* data_block = h_0->get_data_block();
    ASSERT_THAT(data_block, NotNull());
    void* data = data_block->get_data();
    ASSERT_THAT(data, NotNull());
    memcpy(data, &value, sizeof(int));
    detail::backend_runtime->release_handle(h_0.get());
  });

  detail::backend_runtime->finalize();
  backend_finalized = true;

  ASSERT_TRUE(h_1->is_satisfied());
  abstract::backend::DataBlock* data_block = h_1->get_data_block();
  ASSERT_THAT(data_block, NotNull());
  void* data = data_block->get_data();
  ASSERT_THAT(data, NotNull());
  ASSERT_THAT(*((int*)data), Eq(value));

  detail::backend_runtime->release_read_only_usage(h_1.get());
  detail::backend_runtime->release_handle(h_1.get());
}


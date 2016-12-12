/*
//@HEADER
// ************************************************************************
//
//                   test_create_condition_be.cc
//                         darma
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
// Questions? Contact Nicole Slattengren (nlslatt@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <atomic>

#ifdef TEST_BACKEND_INCLUDE
#  include TEST_BACKEND_INCLUDE
#endif

#include "test_backend.h"
#include "helpers.h"

using namespace darma_runtime;

class TestCreateConditionBE
  : public TestBackend
{
 protected:
  virtual void SetUp() {
    TestBackend::SetUp();
  }

  virtual void TearDown() {
    TestBackend::TearDown();
  }
};

//////////////////////////////////////////////////////////////////////////////////

// test create_condition without any access handles
// builds upon TestCreateWorkBE::no_access()
TEST_F(TestCreateConditionBE, trivial_true){
  static std::shared_ptr<std::atomic<int>> execution_order_check(new std::atomic<int>(0));
  struct test_task {
    void operator()(std::vector<std::string> args) {
      *execution_order_check = 0;
      {
        // schedule a trivial condition task
        bool ret = create_condition([=]{
          (*execution_order_check)++;
          return true;
        });
        // check the return value for correctness
        EXPECT_TRUE(ret);
      }
    }
  };
  DARMA_REGISTER_TOP_LEVEL_FUNCTOR(test_task);
  backend_init_finalize(argc_, argv_);
  // make sure condition task not still queued
  ASSERT_TRUE(execution_order_check.unique());
  // make sure condition task actually ran
  ASSERT_EQ(execution_order_check->load(), 1);
}

// test create_condition without any access handles
// builds upon TestCreateWorkBE::no_access()
TEST_F(TestCreateConditionBE, trivial_false){
  static std::shared_ptr<std::atomic<int>> execution_order_check(new std::atomic<int>(0));
  struct test_task {
    void operator()(std::vector<std::string> args) {
      *execution_order_check = 0;
      {
        // schedule a trivial condition task
        bool ret = create_condition([=]{
          (*execution_order_check)++;
          return false;
        });
        // check the return value for correctness
        EXPECT_FALSE(ret);
      }
    }
  };
  DARMA_REGISTER_TOP_LEVEL_FUNCTOR(test_task);
  backend_init_finalize(argc_, argv_);
  // make sure condition task not still queued
  ASSERT_TRUE(execution_order_check.unique());
  // make sure condition task actually ran
  ASSERT_EQ(execution_order_check->load(), 1);
}

// test create_condition with an access handle
// builds upon TestCreateWorkBE::initial_access_alloc()
TEST_F(TestCreateConditionBE, read_access){
  static std::shared_ptr<std::atomic<int>> execution_order_check(new std::atomic<int>(0));
  struct test_task {
    void operator()(std::vector<std::string> args) {
      *execution_order_check = 0;
      {
        auto h = initial_access<mydata>("a");
        create_work([=]{
          // make sure this task runs first
          EXPECT_EQ((*execution_order_check)++, 0);
          h.set_value(7);
          // make sure other task didn't start running yet
          EXPECT_EQ((*execution_order_check)++, 1);
        });
        // schedule a condition task
        bool ret = create_condition(reads(h),[=]{
          // make sure this task runs last
          EXPECT_EQ((*execution_order_check)++, 2);
          // make sure we see the correct initial value
          EXPECT_EQ(h.get_value(), 7);
          return (h.get_value() == 7);
        });
        // check the return value for correctness
        EXPECT_TRUE(ret);
      }
    }
  };
  DARMA_REGISTER_TOP_LEVEL_FUNCTOR(test_task);
  backend_init_finalize(argc_, argv_);
  // make sure condition task not still queued
  ASSERT_TRUE(execution_order_check.unique());
  // make sure condition task actually ran
  ASSERT_EQ(execution_order_check->load(), 3);
}

// test create_condition with an access handle
// builds upon TestCreateWorkBE::initial_access_alloc()
TEST_F(TestCreateConditionBE, rw_access){
  static std::shared_ptr<std::atomic<int>> execution_order_check(new std::atomic<int>(0));
  struct test_task {
    void operator()(std::vector<std::string> args) {
      *execution_order_check = 0;
      {
        auto h = initial_access<mydata>("a");
        create_work([=]{
          // make sure this task runs first
          EXPECT_EQ((*execution_order_check)++, 0);
          h.set_value(7);
          // make sure other task didn't start running yet
          EXPECT_EQ((*execution_order_check)++, 1);
        });
        // schedule a condition task
        bool ret = create_condition([=]{
          // make sure this task runs last
          EXPECT_EQ((*execution_order_check)++, 2);
          // make sure we see the correct initial value
          EXPECT_EQ(h.get_value(), 7);
          return (h.get_value() == 7);
        });
        // check the return value for correctness
        EXPECT_TRUE(ret);
      }
    }
  };
  DARMA_REGISTER_TOP_LEVEL_FUNCTOR(test_task);
  backend_init_finalize(argc_, argv_);
  // make sure condition task not still queued
  ASSERT_TRUE(execution_order_check.unique());
  // make sure condition task actually ran
  ASSERT_EQ(execution_order_check->load(), 3);
}

//////////////////////////////////////////////////////////////////////////////////

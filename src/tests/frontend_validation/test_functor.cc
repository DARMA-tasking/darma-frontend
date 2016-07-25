/*
//@HEADER
// ************************************************************************
//
//                        test_functor
//                           DARMA
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

#include "mock_backend.h"
#include "test_frontend.h"

#include <darma/interface/app/access_handle.h>
#include <darma/interface/app/create_work.h>
#include <darma/interface/app/initial_access.h>

using namespace darma_runtime;

////////////////////////////////////////////////////////////////////////////////

class TestFunctor
  : public TestFrontend
{
  protected:

    virtual void SetUp() {
      using namespace ::testing;

      setup_mock_runtime<::testing::NiceMock>();
      TestFrontend::SetUp();
      ON_CALL(*mock_runtime, get_running_task())
        .WillByDefault(Return(top_level_task.get()));
    }

    virtual void TearDown() {
      TestFrontend::TearDown();
    }

};

////////////////////////////////////////////////////////////////////////////////

struct SimpleFunctor {
  void
  operator()(
    int arg,
    AccessHandle<int> handle
  ) const {
    // Do nothing for now...
  }
};

////////////////////////////////////////////////////////////////////////////////

struct SimpleReadOnlyFunctor {
  void
  operator()(
    int arg,
    ReadAccessHandle<int> handle
  ) const {
    // Do nothing for now...
  }
};

////////////////////////////////////////////////////////////////////////////////

struct SimpleReadOnlyFunctorConvert {
  void
  operator()(
    int arg,
    int const& handle
  ) const {
    // Do nothing for now...
  }
};

////////////////////////////////////////////////////////////////////////////////

struct SimpleReadOnlyFunctorConvertValue {
  void
  operator()(
    int arg,
    int handle
  ) const {
    // Do nothing for now...
  }
};

////////////////////////////////////////////////////////////////////////////////

struct SimpleReadOnlyFunctorConvertLong {
  void
  operator()(
    int arg,
    long handle
  ) const {
    // Do nothing for now...
  }
};

////////////////////////////////////////////////////////////////////////////////

struct SimpleFunctorNonConstLvalue {
  void
  operator()(
    int& arg
  ) const {
    // Do nothing for now...
  }
};

////////////////////////////////////////////////////////////////////////////////

struct SimplerFunctor {
  void
  operator()() const {
    std::cout << "Hello World" << std::endl;
  }
};

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestFunctor, simpler) {
  using namespace ::testing;
  testing::internal::CaptureStdout();

  mock_runtime->save_tasks = true;

  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(_))
    .Times(1);

  create_work<SimplerFunctor>();

  run_all_tasks();

  ASSERT_EQ(testing::internal::GetCapturedStdout(),
    "Hello World\n"
  );
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestFunctor, simpler_named) {
  using namespace ::testing;
  testing::internal::CaptureStdout();
  using namespace darma_runtime::keyword_arguments_for_task_creation;

  mock_runtime->save_tasks = true;

  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(HasName(make_key("hello_task"))))
    .Times(1);

  create_work<SimplerFunctor>(name="hello_task");

  run_all_tasks();

  ASSERT_EQ(testing::internal::GetCapturedStdout(),
    "Hello World\n"
  );
}

//////////////////////////////////////////////////////////////////////////////

struct TestFunctorModCaptures
  : TestFunctor,
    ::testing::WithParamInterface<std::string>
{ };

TEST_P(TestFunctorModCaptures, Parametrized) {
  using namespace ::testing;
  using namespace darma_runtime;
  using namespace mock_backend;

  static_assert(std::is_convertible<meta::any_arg, AccessHandle<int>>::value, "any_arg not convertible!");

  mock_runtime->save_tasks = true;

  std::string test_type = GetParam();

  MockFlow f_initial, f_null, f_task_out;
  use_t* task_use = nullptr;

  expect_initial_access(f_initial, f_null, make_key("hello"));

  //--------------------
  // Expect mod capture:

  EXPECT_CALL(*mock_runtime, make_next_flow(&f_initial))
    .WillOnce(Return(&f_task_out));

  use_t::permissions_t expected_scheduling_permissions;
  if(test_type == "simple_handle") {
    expected_scheduling_permissions = use_t::Modify;
  }
  else {
    expected_scheduling_permissions = use_t::None;
  }

  EXPECT_CALL(*mock_runtime, register_use(IsUseWithFlows(
    &f_initial, &f_task_out,
    expected_scheduling_permissions,
    use_t::Modify
  ))).WillOnce(SaveArg<0>(&task_use));

  EXPECT_CALL(*mock_runtime, register_task_gmock_proxy(
    UseInGetDependencies(ByRef(task_use))
  ));

  EXPECT_CALL(*mock_runtime, establish_flow_alias(&f_task_out, &f_null));

  // End expect mod capture
  //--------------------

  {
    auto tmp = initial_access<int>("hello");
    if(test_type == "simple_handle") {
      create_work<SimpleFunctor>(15, tmp);
    }
    else if(test_type == "convert") {
      create_work<SimpleFunctorNonConstLvalue>(tmp);
    }
    else {
      FAIL() << "unknown test type: " << test_type;
    }
  }

  EXPECT_CALL(*mock_runtime, release_use(task_use));

  run_all_tasks();

}

INSTANTIATE_TEST_CASE_P(
  Parameterized,
  TestFunctorModCaptures,
  ::testing::Values(
    "simple_handle",
    "convert"
  )
);

////////////////////////////////////////////////////////////////////////////////

struct TestFunctorROCaptures
  : TestFunctor,
    ::testing::WithParamInterface<std::string>
{ };

TEST_P(TestFunctorROCaptures, Parameterized) {
  using namespace ::testing;
  using namespace mock_backend;

  mock_runtime->save_tasks = true;


  MockFlow fl_init, fl_null;
  use_t* task_use = nullptr;

  std::string test_type = GetParam();

  Sequence s1;

  expect_initial_access(fl_init, fl_null, make_key("hello"));

  //--------------------
  // Expect ro capture:

  use_t::permissions_t expected_scheduling_permissions;
  if(test_type == "convert" || test_type == "convert_value"
    || test_type == "convert_long" || test_type == "convert_string") {
    expected_scheduling_permissions = use_t::None;
  }
  else {
    expected_scheduling_permissions = use_t::Read;
  }

  int data = 0;

  EXPECT_CALL(*mock_runtime, register_use(
    IsUseWithFlows(
      &fl_init, &fl_init,
      expected_scheduling_permissions,
      use_t::Read
    )
  )).InSequence(s1).WillOnce(
    Invoke([&](auto* use) {
      use->get_data_pointer_reference() = (void*)(&data);
      task_use = use;
    })
  );


  EXPECT_CALL(*mock_runtime,
    register_task_gmock_proxy(UseInGetDependencies(ByRef(task_use)))
  ).InSequence(s1);

  EXPECT_CALL(*mock_runtime, establish_flow_alias(&fl_init, &fl_null))
    .InSequence(s1);

  // End expect ro capture
  //--------------------

  {
    if (test_type == "explicit_read") {
      auto tmp = initial_access<int>("hello");
      create_work<SimpleFunctor>(15, reads(tmp));
    }
    else if (test_type == "read_only_handle") {
      // Formal parameter is ReadOnlyAccessHandle<int>
      auto tmp = initial_access<int>("hello");
      create_work<SimpleReadOnlyFunctor>(15, tmp);
    }
    else if (test_type == "convert") {
      // Formal parameter is a const lvalue reference
      auto tmp = initial_access<int>("hello");
      create_work<SimpleReadOnlyFunctorConvert>(15, tmp);
    }
    else if (test_type == "convert_value") {
      // Formal parameter is by value
      auto tmp = initial_access<int>("hello");
      create_work<SimpleReadOnlyFunctorConvertValue>(15, tmp);
    }
    else if (test_type == "convert_long") {
      // Formal parameter is by value and is of type long int
      auto tmp = initial_access<int>("hello");
      create_work<SimpleReadOnlyFunctorConvertLong>(15, tmp);
    }
  }

  EXPECT_CALL(*mock_runtime, release_use(Eq(ByRef(task_use)))).InSequence(s1);

  run_all_tasks();

}

INSTANTIATE_TEST_CASE_P(
  Parameterized,
  TestFunctorROCaptures,
  ::testing::Values(
    "explicit_read",
    "read_only_handle",
    "convert",
    "convert_value",
    "convert_long"
  )
);

////////////////////////////////////////////////////////////////////////////////

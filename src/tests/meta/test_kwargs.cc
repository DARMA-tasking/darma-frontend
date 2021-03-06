/*
//@HEADER
// ************************************************************************
//
//                      test_kwargs.cc.cc
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


#include <darma/keyword_arguments/get_kwarg.h>
#include <darma/impl/meta/splat_tuple.h>
#include <darma/keyword_arguments/parse.h>
#include <darma/keyword_arguments/macros.h>
#include <darma/utility/static_assertions.h>

#include "blabbermouth.h"

typedef EnableCTorBlabbermouth<String, Copy, Move> BlabberMouth;
static MockBlabbermouthListener* listener;

using namespace darma;
using namespace darma::detail;
using namespace darma::meta;

////////////////////////////////////////////////////////////////////////////////

class TestKeywordArguments
  : public ::testing::Test
{
  protected:

    virtual void SetUp() {
      using namespace ::testing;
      listener_ptr = std::make_unique<StrictMock<MockBlabbermouthListener>>();
      BlabberMouth::listener = listener_ptr.get();
      listener = BlabberMouth::listener;
    }

    virtual void TearDown() {
      listener_ptr.reset();
    }

    std::unique_ptr<::testing::StrictMock<MockBlabbermouthListener>> listener_ptr;

};

////////////////////////////////////////////////////////////////////////////////
// old tests

void _test(BlabberMouth& val) { std::cout << val.data << std::endl; }
void _test(BlabberMouth&& val) { std::cout << val.data << std::endl; }

void _test_lvalue_only(BlabberMouth& val) { std::cout << val.data << std::endl; }
void _test_rvalue_only(BlabberMouth&& val) { std::cout << val.data << std::endl; }

template <typename... Args>
void test_lvalue_only(Args&&... args) {
  meta::splat_tuple(
    get_positional_arg_tuple(std::forward<Args>(args)...), _test_lvalue_only
  );
}

template <typename... Args>
void test_rvalue_only(Args&&... args) {
  meta::splat_tuple(
    get_positional_arg_tuple(std::forward<Args>(args)...), _test_rvalue_only
  );
}

TEST_F(TestKeywordArguments, rvalue_only) {
  EXPECT_CALL(*listener, string_ctor()).Times(1);
  testing::internal::CaptureStdout();

  test_rvalue_only(BlabberMouth("hello"));

  ASSERT_EQ(testing::internal::GetCapturedStdout(),
    "hello\n"
  );
}

TEST_F(TestKeywordArguments, lvalue_only) {
  EXPECT_CALL(*listener, string_ctor()).Times(1);
  testing::internal::CaptureStdout();

  BlabberMouth b("hello");
  test_lvalue_only(b);

  ASSERT_EQ(testing::internal::GetCapturedStdout(),
    "hello\n"
  );
}

////////////////////////////////////////////////////////////////////////////////

DeclareDarmaTypeTransparentKeyword(testing, test_kwarg_1);
DeclareDarmaTypeTransparentKeyword(testing, test_kwarg_2);
DeclareDarmaTypeTransparentKeyword(testing, test_kwarg_3);
DeclareDarmaTypeTransparentKeyword(testing, test_kwarg_4);

namespace kw = darma::keyword_tags_for_testing;

template <typename OverloadDescription, typename... Args>
void desc_should_fail(Args&&...) {
  static_assert(
    not OverloadDescription::var_helper_t::template is_valid_for_args<Args...>::value,
    "Expected description to be invalid for arguments"
  );
};

template <typename OverloadDescription, typename... Args>
void desc_should_pass(Args&&...) {
  static_assert(
    OverloadDescription::var_helper_t::template is_valid_for_args<Args...>::value,
    "Expected description to be valid for arguments"
  );
};

template <typename OverloadDescription, typename... Args>
auto desc_helper(Args&&... arg) {
  return typename OverloadDescription::var_helper_t::template _helper<Args...>{};
};

template <size_t i>
struct _forward_through_arg_impl {
  template <typename... Args>
  decltype(auto)
  operator()(Args&&... args) const {
    return std::get<i>(std::forward_as_tuple(std::forward<decltype(args)>(args)...));
  }
};

template <typename OverloadDescription, size_t i, typename... Args>
decltype(auto) invoke_arg(Args&&... args) {
  return OverloadDescription().invoke(
    std::forward_as_tuple(), std::forward_as_tuple(),
    _forward_through_arg_impl<i>{}, std::forward_as_tuple(std::forward<Args>(args)...)
  );
};

template <typename Parser, typename... Args>
void assert_valid(Args&&... args) {
  using _______________see_calling_context_on_next_line________________ = typename Parser::template static_assert_valid_invocation<Args...>;
};

template <typename... Args>
void test_function(Args&&... args) {

  using namespace darma::detail;
  using parser = kwarg_parser<
    overload_description<
      positional_or_keyword_argument<int, kw::test_kwarg_1>,
      positional_or_keyword_argument<double, kw::test_kwarg_2>,
      keyword_only_argument<double, kw::test_kwarg_3>
    >,
    overload_description<
      positional_only_argument<int>,
      positional_only_argument<int>
    >
  >;
  using _______________see_calling_context_on_next_line________________ = typename parser::template static_assert_valid_invocation<Args...>;
}

template <typename... Args>
void test_function_variadics(Args&&... args) {

  using namespace darma::detail;
  using parser = kwarg_parser<
    variadic_positional_overload_description<
      positional_or_keyword_argument<int, kw::test_kwarg_1>,
      positional_or_keyword_argument<double, kw::test_kwarg_2>,
      keyword_only_argument<double, kw::test_kwarg_3>
    >,
    overload_description<
      positional_only_argument<int>,
      positional_only_argument<int>
    >
  >;
  using _______________see_calling_context_on_next_line________________ = typename parser::template static_assert_valid_invocation<Args...>;
}

struct A { int value = 0; };
struct ConvToA {
  int value = 0;
  operator A() { A rv; rv.value = value; return rv; }
};
struct B { double value = 0.0; };

TEST_F(TestKeywordArguments, static_tests) {

  using namespace darma::detail;
  using namespace darma;
  using namespace darma::keyword_arguments_for_testing;

  using odesc0 = overload_description<
    positional_only_argument<int>
  >;
  desc_should_pass<odesc0>(5);
  desc_should_fail<odesc0>(test_kwarg_1=5);

  using odesc1 = overload_description<
    positional_or_keyword_argument<int, kw::test_kwarg_1>,
    positional_or_keyword_argument<double, kw::test_kwarg_2>,
    keyword_only_argument<double, kw::test_kwarg_3>
  >;

  desc_should_fail<odesc1>(27, test_kwarg_2=5.3, 3.14);
  desc_should_fail<odesc1>(5, 3.14, test_kwarg_2=27);
  desc_should_fail<odesc1>(5, test_kwarg_1=27, 3.14);
  desc_should_fail<odesc1>(27, 5.3, 3.14);

  desc_should_pass<odesc1>(27, 5.3, test_kwarg_3=3.14);
  desc_should_pass<odesc1>(test_kwarg_3=5.3, test_kwarg_2=3.14, test_kwarg_1=10);
  desc_should_pass<odesc1>(test_kwarg_3=5.3, test_kwarg_1=3, test_kwarg_2=3.14);
  desc_should_pass<odesc1>(42, test_kwarg_3=37.1, test_kwarg_2=3.14);
  desc_should_pass<odesc1>(test_kwarg_1=42, test_kwarg_2=37.1, test_kwarg_3=3.14);

  using odesc1_helper_1 = decltype(desc_helper<odesc1>(27, 5.3, 3.14));
  STATIC_ASSERT_VALUE_EQUAL(odesc1_helper_1::first_kwarg_given, 3);
  STATIC_ASSERT_VALUE_EQUAL(odesc1_helper_1::first_desc_given_as_kwarg, 3);
  STATIC_ASSERT_VALUE_EQUAL(odesc1_helper_1::first_kwarg_only_desc, 2);

  using odesc1_helper_2 = decltype(desc_helper<odesc1>(5, 3.14, test_kwarg_2=27));
  STATIC_ASSERT_VALUE_EQUAL(odesc1_helper_2::first_desc_given_as_kwarg, 1);

  STATIC_ASSERT_TYPE_EQ(
    decltype(invoke_arg<odesc1, 1>(test_kwarg_2=42, test_kwarg_1=37.1, test_kwarg_3=3.14)),
    int&&
  );
  STATIC_ASSERT_TYPE_EQ(
    decltype(invoke_arg<odesc1, 2>(test_kwarg_2=42, test_kwarg_1=37.1, test_kwarg_3=3.14)),
    double&&
  );
  STATIC_ASSERT_TYPE_EQ(
    decltype(invoke_arg<odesc1, 0>(test_kwarg_2=42, test_kwarg_1=37.1, test_kwarg_3=3.14)),
    double&&
  );

  using odesc2 = overload_description<
    positional_only_argument<A>,
    positional_or_keyword_argument<B, kw::test_kwarg_2>,
    keyword_only_argument<A, kw::test_kwarg_3>
  >;

  A a; ConvToA conv_a; B b;
  desc_should_pass<odesc2>(A(), b, test_kwarg_3=conv_a);
  desc_should_pass<odesc2>(conv_a, b, test_kwarg_3=conv_a);
  desc_should_pass<odesc2>(a, test_kwarg_3=conv_a, test_kwarg_2=b);

  desc_should_fail<odesc2>(b, b, test_kwarg_3=conv_a);
  desc_should_fail<odesc2>(a, test_kwarg_2=conv_a, test_kwarg_2=a);
  desc_should_fail<odesc2>(a, test_kwarg_3=A(), test_kwarg_2=conv_a);

  STATIC_ASSERT_TYPE_EQ(
    decltype(invoke_arg<odesc2, 0>(A(), b, test_kwarg_3=conv_a)), A&&
  );
  STATIC_ASSERT_TYPE_EQ(
    decltype(invoke_arg<odesc2, 1>(A(), b, test_kwarg_3=conv_a)), B&
  );
  STATIC_ASSERT_TYPE_EQ(
    decltype(invoke_arg<odesc2, 2>(A(), b, test_kwarg_3=conv_a)), ConvToA&
  );

  using odesc3 = overload_description<
    positional_only_argument<A&>,
    positional_only_argument<A&&>,
    positional_only_argument<A const&>
  >;
  desc_should_pass<odesc3>(a, A(), A());
  desc_should_fail<odesc3>(A(), A(), A());
  desc_should_pass<odesc3>(a, A(), A());
  desc_should_fail<odesc3>(a, a, A());
  desc_should_pass<odesc3>(a, A(), a);
  desc_should_pass<odesc3>(a, A(), A());

  using odesc4 = overload_description<
    keyword_only_argument<A&, kw::test_kwarg_1>,
    keyword_only_argument<A&&, kw::test_kwarg_2>,
    keyword_only_argument<A const&, kw::test_kwarg_3>
  >;
  desc_should_pass<odesc4>(test_kwarg_1=a, test_kwarg_2=A(), test_kwarg_3=A());
  desc_should_fail<odesc4>(test_kwarg_1=A(), test_kwarg_2=A(), test_kwarg_3=A());
  desc_should_pass<odesc4>(test_kwarg_1=a, test_kwarg_2=A(), test_kwarg_3=A());
  desc_should_fail<odesc4>(test_kwarg_1=a, test_kwarg_2=a, test_kwarg_3=A());
  desc_should_pass<odesc4>(test_kwarg_1=a, test_kwarg_2=A(), test_kwarg_3=a);
  desc_should_pass<odesc4>(test_kwarg_1=a, test_kwarg_2=A(), test_kwarg_3=A());

  using odesc5 = overload_description<
    positional_only_argument<A>,
    positional_or_keyword_argument<deduced_parameter, kw::test_kwarg_2>,
    keyword_only_argument<A, kw::test_kwarg_3>
  >;
  desc_should_pass<odesc5>(A(), 73, test_kwarg_3=conv_a);

  //============================================================================

  using parser = kwarg_parser<
    overload_description<
      positional_or_keyword_argument<int, kw::test_kwarg_1>,
      positional_or_keyword_argument<double, kw::test_kwarg_2>,
      keyword_only_argument<double, kw::test_kwarg_3>
    >,
    overload_description<
      positional_only_argument<int>,
      positional_only_argument<int>
    >
  >;

  assert_valid<parser>(5, 6);
  // should fail gracefully at compile time:
  //assert_valid<parser>("hello", 6);
  // should fail gracefully at compile time:
  //assert_valid<parser>(test_kwarg_1=5, test_kwarg_3=6);
  // should fail gracefully, useful as an example of what a user might see...
  // TODO the second fail reason here is wrong
  //test_function(test_kwarg_1=5, test_kwarg_3=6);
  // should fail gracefully, useful as an example of what a user might see...
  //test_function_variadics(test_kwarg_1=5, test_kwarg_3=6);

  //============================================================================

  // Adding on optional arguments...
  using odesc6 = overload_description<
    _positional<int>,
    _positional_or_keyword<int, kw::test_kwarg_1>,
    _keyword<int, kw::test_kwarg_2>,
    _optional<_keyword<int, kw::test_kwarg_3>>
  >;

  desc_should_pass<odesc6>(3, 4, test_kwarg_2=5);
  desc_should_pass<odesc6>(3, 4, test_kwarg_3=6, test_kwarg_2=5);
  desc_should_pass<odesc6>(3, test_kwarg_3=6, test_kwarg_2=5, test_kwarg_1=4);

  desc_should_fail<odesc6>(3, 4, test_kwarg_3=6);
  desc_should_fail<odesc6>(3, test_kwarg_3=6);
  desc_should_fail<odesc6>(3, test_kwarg_3=6, test_kwarg_1=7);
  desc_should_fail<odesc6>(3, 4, 5, 6, 7);
  desc_should_fail<odesc6>();
  desc_should_fail<odesc6>(test_kwarg_3=6);

  // only optional arguments...
  using odesc7 = overload_description<
    _optional<_keyword<int, kw::test_kwarg_2>>,
    _optional<_keyword<int, kw::test_kwarg_3>>
  >;
  desc_should_pass<odesc7>(test_kwarg_3=6, test_kwarg_2=5);
  desc_should_pass<odesc7>(test_kwarg_3=6);
  desc_should_pass<odesc7>(test_kwarg_2=6);
  desc_should_pass<odesc7>();

  using odesc7_helper_1 = decltype(desc_helper<odesc7>(42));
  STATIC_ASSERT_VALUE_EQUAL(odesc7_helper_1::n_variadic_given, 1);
  STATIC_ASSERT_VALUE_EQUAL(odesc7_helper_1::variadic_args_allowed, false);

  desc_should_fail<odesc7>(42);
  desc_should_fail<odesc7>(test_kwarg_1=6);
  desc_should_fail<odesc7>(test_kwarg_2="hello");

  // only positional and optional arguments...
  //using odesc8 = overload_description<
  //  _positional<int>,
  //  _optional<_keyword<int, kw::test_kwarg_2>>,
  //  _optional<_keyword<int, kw::test_kwarg_3>>
  //>;

}

////////////////////////////////////////////////////////////////////////////////

struct MyOverloads {
  explicit MyOverloads(int number) : overload_number(number) { }
  int overload_number;

  void operator()(int a, double b, double c = 5.0) {
    ASSERT_EQ(overload_number, 1);
    ASSERT_EQ(a, 3);
    ASSERT_EQ(b, 4.0);
    ASSERT_EQ(c, 5.0);
  }

  void operator()(A& v1, A const& v2, B& v3) {
    ASSERT_EQ(overload_number, 2);
    ASSERT_EQ(v1.value, 3);
    ASSERT_EQ(v2.value, 4);
    ASSERT_EQ(v3.value, 5.0);
  }

  template <typename T>
  void operator()(std::enable_if_t<std::is_integral<T>::value, A>& v1, T v2, B& v3) {
    ASSERT_EQ(overload_number, 3);
    ASSERT_EQ(v1.value, 3);
    ASSERT_EQ(v2, 4);
    ASSERT_EQ(v3.value, 5.0);
  }

  void operator()(double pi) {
    ASSERT_EQ(overload_number, 4);
    ASSERT_EQ(pi, (double)3.14);
  }

  void operator()(B v1) {
    ASSERT_EQ(overload_number, 5);
    ASSERT_EQ(v1.value, (double)3.14);
  }
};


TEST_F(TestKeywordArguments, overload_tests) {

  using namespace darma::detail;
  using namespace darma;
  using namespace darma::keyword_arguments_for_testing;

  using odesc1 = overload_description<
    positional_or_keyword_argument<int, kw::test_kwarg_1>,
    positional_or_keyword_argument<double, kw::test_kwarg_2>,
    keyword_only_argument<double, kw::test_kwarg_3>
  >;

  using odesc2 = overload_description<
    positional_only_argument<A&>,
    positional_or_keyword_argument<A const&, kw::test_kwarg_2>,
    positional_or_keyword_argument<B&, kw::test_kwarg_3>
  >;

  using odesc3 = overload_description<
    positional_only_argument<A&>,
    positional_or_keyword_argument<deduced_parameter, kw::test_kwarg_2>,
    positional_or_keyword_argument<B&, kw::test_kwarg_3>
  >;

  using parser = kwarg_parser<odesc1, odesc2, odesc3>;

  // Should be overload 1
  parser().invoke(MyOverloads(1), 3, 4, test_kwarg_3=5);

  A a; B b; ConvToA conv_a;
  a.value = 3;
  conv_a.value = 4;
  b.value = 5.0;

  // Should be overload 2
  parser().invoke(MyOverloads(2), a, test_kwarg_3=b, test_kwarg_2=conv_a);

  // Should be overload 3
  parser().invoke(MyOverloads(3), a, test_kwarg_3=b, test_kwarg_2=4);
}

////////////////////////////////////////////////////////////////////////////////


TEST_F(TestKeywordArguments, overload_with_default_generators_tests) {

  using namespace darma::detail;
  using namespace darma;
  using namespace darma::keyword_arguments_for_testing;

  using parser = kwarg_parser<
    overload_description<
      _optional<_keyword<double, kw::test_kwarg_1>>
    >
  >;
  parser().invoke(MyOverloads(4), test_kwarg_1=3.14);
  parser().with_default_generators(test_kwarg_1=[]{ return 3.14; }).invoke(MyOverloads(4));
  parser().with_default_generators(test_kwarg_1=[]{ return 6.28; }).invoke(MyOverloads(4), test_kwarg_1=3.14);
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(TestKeywordArguments, converter) {

  using namespace darma::detail;
  using namespace darma;
  using namespace darma::keyword_arguments_for_testing;

  using parser = kwarg_parser<
    overload_description<
      _optional<_keyword<converted_parameter, kw::test_kwarg_1>>
    >
  >;
  parser()
    .with_default_generators(test_kwarg_1=[]{ B rv; rv.value = 3.14; return rv; })
    .with_converters([](auto&& val) { B rv; rv.value = val; return rv; })
    .invoke(MyOverloads(5));
}

////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct is_B_impl : std::false_type { };

template <>
struct is_B_impl<B> : std::true_type { };

template <typename T>
using is_B = typename is_B_impl<std::decay_t<T>>::type;

TEST_F(TestKeywordArguments, metafunction_predicate) {

  using namespace darma::detail;
  using namespace darma;
  using namespace darma::keyword_arguments_for_testing;

  using parser = kwarg_parser<
    overload_description<
      _optional<_keyword<parameter_such_that<is_B>, kw::test_kwarg_1>>
    >
  >;

  B b;
  b.value = 3.14;

  parser()
    .with_default_generators(test_kwarg_1=[]{ B rv; rv.value = 6.28; return rv; })
    .parse_args(test_kwarg_1=b)
    .invoke(MyOverloads(5));
}

////////////////////////////////////////////////////////////////////////////////


TEST_F(TestKeywordArguments, invoke_lambda) {

  using namespace darma::detail;
  using namespace darma;
  using namespace darma::keyword_arguments_for_testing;

  using parser = kwarg_parser<
    overload_description<
      _optional<_keyword<converted_parameter, kw::test_kwarg_1>>
    >
  >;
  parser()
    .with_default_generators(test_kwarg_1=[]{ return 6.28; })
    .with_converters([](auto&& val) { B rv; rv.value = val; return rv; })
    .parse_args(test_kwarg_1=3.14)
    .invoke([](B val) {
      ASSERT_EQ(val.value, 3.14);
    });
  parser()
    .with_converters([](auto&& val) { B rv; rv.value = val; return rv; })
    .with_default_generators(test_kwarg_1=[]{ return 6.28; })
    .parse_args(test_kwarg_1=3.14)
    .invoke([](B val) {
      ASSERT_EQ(val.value, 3.14);
    });
}

////////////////////////////////////////////////////////////////////////////////


TEST_F(TestKeywordArguments, variadic_simple) {

  using namespace darma::detail;
  using namespace darma;
  using namespace darma::keyword_arguments_for_testing;

  using odesc1 = variadic_positional_overload_description<
    _optional_keyword<converted_parameter, kw::test_kwarg_1>
  >;

  using parser = kwarg_parser<odesc1>;

  desc_should_pass<odesc1>(test_kwarg_1=3.14);

  using odesc1_helper_1 = decltype(desc_helper<odesc1>(1, test_kwarg_1=3.14));

  STATIC_ASSERT_VALUE_EQUAL(not odesc1_helper_1::kwarg_given_before_last_positional, true);
  STATIC_ASSERT_VALUE_EQUAL(not odesc1_helper_1::too_many_positional, true);

  parser()
    .with_default_generators(test_kwarg_1=[]{ return 6.28; })
    .with_converters([](auto&& val) { B rv; rv.value = val; return rv; })
    .parse_args(1, 2, 3, 4, test_kwarg_1=3.14)
    .invoke([](B val, variadic_arguments_begin_tag, auto&&... args) {
      ASSERT_EQ(val.value, 3.14);
      auto test = [](int a, int b, int c, int d) {
        ASSERT_EQ(a, 1);
        ASSERT_EQ(b, 2);
        ASSERT_EQ(c, 3);
        ASSERT_EQ(d, 4);
      };
      test(args...);
    });
}

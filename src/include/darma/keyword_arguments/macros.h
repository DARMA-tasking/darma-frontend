/*
//@HEADER
// ************************************************************************
//
//                   keyword_arguments/macros.h
//                         darma
//              Copyright (C) 2015 Sandia Corporation
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

#include <darma/keyword_arguments/keyword_tag.h>
#include <darma/keyword_arguments/keyword_argument_name.h>

#include <type_traits>

#ifndef KEYWORD_ARGUMENTS_MACROS_H_
#define KEYWORD_ARGUMENTS_MACROS_H_

// TODO remove deprecated elements of this file!!!

////////////////////////////////////////////////////////////////////////////////

#define _darma_detail_remove_parens(...) __VA_ARGS__

#define _darma_declare_keyword_tag_typeless(argument_for, name)                                     \
namespace darma {                                                                           \
namespace keyword_tags_for_##argument_for {                                                         \
                                                                                                    \
class name : public detail::keyword_tag                                                             \
{                                                                                                   \
  public:                                                                                           \
    typedef std::true_type is_tag_t;                                                                \
    typedef _darma_detail_remove_parens (darma::detail::unknown_type) value_t;              \
};                                                                                                  \
                                                                                                    \
} /* end keyword_tags_for_<argument_for> */                                                         \
namespace keyword_arguments_for_##argument_for {                                                    \
                                                                                                    \
typedef detail::typeless_keyword_argument_name<                                                              \
  darma::keyword_tags_for_##argument_for::name                                              \
> name##_keyword_name_t;                                                                            \
                                                                                                    \
namespace {                                                                                         \
/* anonymous namespace so that static values are linked as one per translation unit */              \
static constexpr name##_keyword_name_t name;                                                        \
} /* end of anonymous namespace */                                                                  \
} /* end keyword_arguments_for_<argument_for> */                                                    \
} /* end namespace darma */

#define _darma_declare_keyword_tag_data(argument_for, name, argtype)                                \
namespace darma {                                                                           \
namespace detail {                                                                                  \
template<>                                                                                          \
struct tag_data<darma::keyword_tags_for_##argument_for::name>                               \
{                                                                                                   \
  typedef keyword_arguments_for_##argument_for::name##_keyword_name_t keyword_name_t;               \
  typedef _darma_detail_remove_parens argtype value_t;                                              \
  static constexpr bool has_default_value = false;                                                  \
  static constexpr bool has_type = not                                                              \
    std::is_same<value_t, darma::detail::unknown_type>::type::value;                        \
};                                                                                                  \
} /* end namespace detail */                                                                        \
} /* end namespace darma */

// This is the only way we should declare keyword arguments from now on
#define DeclareDarmaTypeTransparentKeyword(argument_for, name)                                      \
    _darma_declare_keyword_tag_typeless(argument_for, name)                                         \
    _darma_declare_keyword_tag_data(argument_for, name, (darma::detail::unknown_type))

////////////////////////////////////////////////////////////////////////////////


#define AliasDarmaKeyword(original_argument_for, name)                                              \
namespace {                                                                                         \
/* anonymous namespace for linking purposes */                                                      \
static constexpr const                                                                              \
::darma::keyword_arguments_for_##original_argument_for::name##_keyword_name_t&              \
name = ::darma::keyword_arguments_for_##original_argument_for::name;                        \
} /* end anonymous namespace */

#define AliasDarmaKeywordAs(original_argument_for, name, as_name)                                   \
namespace {                                                                                         \
/* anonymous namespace for linking purposes */                                                      \
static constexpr const                                                                              \
::darma::keyword_arguments_for_##original_argument_for::name##_keyword_name_t&              \
as_name = ::darma::keyword_arguments_for_##original_argument_for::name;                     \
} /* end anonymous namespace */

#define DeclareStandardDarmaKeywordArgumentAliases(original_argument_for, name)                     \
namespace darma {                                                                           \
namespace keyword_arguments {                                                                       \
AliasDarmaKeyword(original_argument_for, name);                                                     \
}                                                                                                   \
                                                                                                    \
namespace _keyword_arguments {                                                                      \
AliasDarmaKeywordAs(original_argument_for, name, DARMA_CONCAT_TOKEN_(_, name));                     \
}                                                                                                   \
                                                                                                    \
namespace keyword_arguments_ {                                                                      \
AliasDarmaKeywordAs(original_argument_for, name, DARMA_CONCAT_TOKEN_(name, _));                     \
}                                                                                                   \
                                                                                                    \
namespace _keyword_arguments_ {                                                                     \
AliasDarmaKeywordAs(original_argument_for, name,                                                    \
  DARMA_CONCAT_TOKEN_(DARMA_CONCAT_TOKEN_(_, name), _)                                              \
);                                                                                                  \
}                                                                                                   \
} /* end namespace darma */



#endif /* KEYWORD_ARGUMENTS_MACROS_H_ */

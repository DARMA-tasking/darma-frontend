/*
//@HEADER
// ************************************************************************
//
//                          darma_types.h
//                         dharma_new
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

#ifndef SRC_TESTS_FRONTEND_VALIDATION_DARMA_TYPES_H_
#define SRC_TESTS_FRONTEND_VALIDATION_DARMA_TYPES_H_

#include <limits> // std::numeric_limits
#include <cstdlib> // std::size_t
#include <string>
#include <ostream>

#include "test_frontend_fwd.h"
#include <darma/impl/feature_testing_macros.h>

namespace mock_backend {

struct MockResourcePack {
  MockResourcePack() : name("%##unnamed##%") { }
  MockResourcePack(std::string const& name) : name(name) { }
  std::string name;
  template <typename Archive>
  void serialize(Archive& ar) { ar | name; }
};

} // end namespace mock backend

namespace darma { namespace types {
using resource_pack_t = ::mock_backend::MockResourcePack;
}} // end namespace darma::types

namespace mock_backend {
class MockFlow {
  private:

    static std::size_t next_index;
    std::size_t index_;
    std::string name_;

  public:

    MockFlow()
      : index_(next_index++),
        name_("%##unnamed##%")
    { }

    MockFlow(const char* str)
      : index_(std::numeric_limits<size_t>::max()),
        name_(str)
    { }

    MockFlow(std::nullptr_t)
      : index_(std::numeric_limits<size_t>::max()),
        name_("%##unnamed##%")
    { }

    bool
    operator==(std::nullptr_t) const {
      return name_ == "%##unnamed##%"
        and index_ == std::numeric_limits<size_t>::max();
    }
    bool
    operator==(MockFlow const& other) const {
      if(index_ == std::numeric_limits<size_t>::max()) {
        if(name_ == "%##unnamed##%") return false;
        else {
          return name_ == other.name_;
        }
      }
      else {
        return index_ == other.index_;
      }
    }
    bool
    operator!=(MockFlow const& other) const {
      return not operator==(other);
    }
    bool
    operator!=(std::nullptr_t) const {
      return not operator==(nullptr);
    }

    friend bool
    operator==(std::nullptr_t, MockFlow const& fl) {
      return fl.operator==(nullptr);
    }
    friend bool
    operator!=(std::nullptr_t, MockFlow const& fl) {
      return fl.operator!=(nullptr);
    }

    friend std::ostream&
    operator<<(std::ostream& o, MockFlow const& f) {
      if(f.name_ != "%##unnamed##%") {
        o << "<flow named \"" << f.name_ << "\">";
      }
      else if(f.index_ != std::numeric_limits<size_t>::max()) {
        o << "<flow #" << f.index_ << ">";
      }
      else {
        o << "<nullptr flow>";
      }
      return o;
    }

};


#if _darma_has_feature(anti_flows)
// Intentionally copy-and-pasted so that MockFlow and MockAntiFlow are not
// related to each other, in case implementations want to do it this way
class MockAntiFlow {
  private:

    static std::size_t next_index;
    std::size_t index_;
    std::string name_;

  public:

    MockAntiFlow()
      : index_(next_index++),
        name_("%##unnamed##%")
    { }

    MockAntiFlow(const char* str)
      : index_(std::numeric_limits<size_t>::max()),
        name_(str)
    { }

    MockAntiFlow(MockAntiFlow* other)
      : index_(other ? other->index_ : std::numeric_limits<size_t>::max()),
        name_(other ? other->name_ : "%##unnamed##%")
    { }

    MockAntiFlow(std::nullptr_t)
      : index_(std::numeric_limits<size_t>::max()),
        name_("%##unnamed##%")
    { }

    bool
    operator==(MockAntiFlow const& other) const {
      if(index_ == std::numeric_limits<size_t>::max()) {
        if(name_ == "%##unnamed##%") return true;
        else {
          return name_ == other.name_;
        }
      }
      else {
        return index_ == other.index_;
      }
    }
    bool
    operator!=(MockAntiFlow const& other) const {
      return not operator==(other);
    }

    friend std::ostream&
    operator<<(std::ostream& o, MockAntiFlow const& f) {
      if(f.name_ != "%##unnamed##%") {
        o << "<anti-flow named \"" << f.name_ << "\">";
      }
      else if(f.index_ != std::numeric_limits<size_t>::max()) {
        o << "<anti-flow #" << f.index_ << ">";
      }
      else {
        o << "<nullptr anti-flow>";
      }
      return o;
    }
};
#endif // _darma_has_feature(anti_flows)
} // end namespace mock_backend

#include <darma/interface/defaults/pointers.h>

namespace darma { namespace types {
typedef ::mock_backend::MockFlow flow_t;
#if _darma_has_feature(anti_flows)
using anti_flow_t = ::mock_backend::MockAntiFlow;
#endif // _darma_has_feature(anti_flows)
}} // end namespace darma::types

namespace mock_backend {

struct MockTaskCollectionToken {
  MockTaskCollectionToken() : name("%##unnamed##%") { }
  MockTaskCollectionToken(const char* in_name) : name(in_name) { }
  MockTaskCollectionToken(MockTaskCollectionToken const& other) = default;
  MockTaskCollectionToken(MockTaskCollectionToken&& other) = default;
  MockTaskCollectionToken& operator=(MockTaskCollectionToken const& other) = default;
  MockTaskCollectionToken& operator=(MockTaskCollectionToken&& other) = default;
  std::string name;
  template <typename Archive>
  void serialize(Archive& ar) { ar | name; }
};

} // end namespace mock_backend


namespace darma { namespace types {
using task_collection_token_t = mock_backend::MockTaskCollectionToken;
}} // end namespace darma::types

namespace mock_backend {

struct MockRuntimeInstanceToken {
  MockRuntimeInstanceToken() : name("%##unnamed##%") { }
  MockRuntimeInstanceToken(const char* in_name) : name(in_name) { }
  MockRuntimeInstanceToken(MockRuntimeInstanceToken const& other) = default;
  MockRuntimeInstanceToken(MockRuntimeInstanceToken&& other) = default;
  MockRuntimeInstanceToken& operator=(MockRuntimeInstanceToken const& other) = default;
  MockRuntimeInstanceToken& operator=(MockRuntimeInstanceToken&& other) = default;
  std::string name;
  template <typename Archive>
  void serialize(Archive& ar) { ar | name; }
};

} // end namespace mock_backend

namespace darma { namespace types {
using runtime_instance_token_t = mock_backend::MockRuntimeInstanceToken;
}} // end namespace darma::types

#include <darma/impl/key/SSO_key_fwd.h>

namespace darma { namespace types {
  typedef darma::detail::SSOKey<> key_t;
}} // end namespace darma::types

#include <darma/impl/key/SSO_key.h>



#endif /* SRC_TESTS_FRONTEND_VALIDATION_DARMA_TYPES_H_ */

/*
//@HEADER
// ************************************************************************
//
//                          task.h
//                         darma_new
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

#ifndef DARMA_RUNTIME_TASK_H_
#define DARMA_RUNTIME_TASK_H_

#include <unordered_map>
#include <unordered_set>
#include <set>

#include <tinympl/greater.hpp>
#include <tinympl/int.hpp>
#include <tinympl/delay.hpp>
#include <tinympl/identity.hpp>
#include <tinympl/at.hpp>
#include <tinympl/erase.hpp>
#include <tinympl/bind.hpp>
#include <tinympl/logical_and.hpp>
#include <tinympl/vector.hpp>

#include <darma/interface/backend/types.h>
#include <darma/interface/backend/runtime.h>
#include <darma/interface/frontend/dependency_handle.h>
#include <darma/interface/frontend/task.h>

#include <darma/impl/util.h>
#include <darma/impl/runtime.h>
#include <darma/impl/handle_fwd.h>

namespace darma_runtime {

namespace detail {

// TODO decide between this and "is_key<>", which would not check the concept but would be self-declared
template <typename T>
struct meets_key_concept;
template <typename T>
struct meets_version_concept;

namespace template_tags {
template <typename T> struct input { typedef T type; };
template <typename T> struct output { typedef T type; };
template <typename T> struct in_out { typedef T type; };
}

namespace m = tinympl;
namespace mv = tinympl::variadic;
namespace pl = tinympl::placeholders;
namespace tt = template_tags;

template <typename... Types>
struct task_traits {
  private:
    static constexpr const bool _first_is_key = m::and_<
        m::greater<m::int_<sizeof...(Types)>, m::int_<0>>,
        m::delay<
          meets_key_concept,
          mv::at<0, Types...>
        >
      >::value;

  public:
    typedef typename std::conditional<
      _first_is_key,
      mv::at<0, Types...>,
      m::identity<types::key_t>
    >::type::type key_t;

  private:
    static constexpr const size_t _possible_version_index =
      std::conditional<_first_is_key, m::int_<1>, m::int_<0>>::type::value;
    static constexpr const bool _version_given = m::and_<
      m::greater<m::int_<sizeof...(Types)>, m::int_<_possible_version_index>>,
      m::delay<
        meets_version_concept,
        mv::at<_possible_version_index, Types...>
      >
    >::value;


  public:
    typedef typename std::conditional<
      _version_given,
      mv::at<_possible_version_index, Types...>,
      m::identity<types::version_t>
    >::type::type version_t;

  private:

    typedef typename m::erase<
      (size_t)0, (size_t)(_first_is_key + _version_given),
      m::vector<Types...>, m::vector
    >::type other_types;

  public:

};

}

namespace detail {

class RunnableBase {
 public:
   virtual void run() =0;
   virtual ~RunnableBase() { }
};

template <typename Callable>
struct Runnable : public RunnableBase
{
 public:
  explicit
  Runnable(Callable&& c)
    : run_this_(std::forward<Callable>(c))
  { }
  void run() override { run_this_(); }

 private:
  Callable run_this_;
};

class TaskBase : public abstract::backend::runtime_t::task_t
{
  protected:

    typedef abstract::backend::runtime_t::handle_t handle_t;
    typedef abstract::backend::runtime_t::key_t key_t;
    typedef abstract::backend::runtime_t::version_t version_t;
    typedef types::shared_ptr_template<handle_t> handle_ptr;
    typedef types::handle_container_template<handle_t*> get_deps_container_t;
    typedef std::unordered_set<handle_t*> needs_handle_container_t;

    get_deps_container_t dependencies_;

    needs_handle_container_t needs_read_deps_;
    needs_handle_container_t needs_write_deps_;
    std::vector<handle_ptr> all_deps_;

    key_t name_;

  public:

    template <typename DependencyHandleSharedPtr>
    void add_dependency(
      const DependencyHandleSharedPtr& dep,
      bool needs_read_data,
      bool needs_write_data
    ) {
      dependencies_.insert(dep.get());
      all_deps_.push_back(dep);
      assert(needs_read_data || needs_write_data);
      if(needs_read_data) needs_read_deps_.insert(dep.get());
      if(needs_write_data) needs_write_deps_.insert(dep.get());
    }

    template <typename AccessHandle>
    void do_capture(
      AccessHandle& captured,
      AccessHandle const& source_and_continuing
    );

    ////////////////////////////////////////////////////////////////////////////////
    // Implementation of abstract::frontend::Task

    const get_deps_container_t&
    get_dependencies() const override {
      return dependencies_;
    }

    bool
    needs_read_data(
      const handle_t* handle
    ) const override {
      // TODO figure out why we need a const cast here
      return needs_read_deps_.find(const_cast<handle_t*>(handle)) != needs_read_deps_.end();
    }

    bool
    needs_write_data(
      const handle_t* handle
    ) const override {
      // TODO figure out why we need a const cast here
      return needs_write_deps_.find(const_cast<handle_t*>(handle)) != needs_write_deps_.end();
    }

    const key_t&
    get_name() const override {
      return name_;
    }

    void
    set_name(const key_t& name) override {
      name_ = name;
    }

    bool
    is_migratable() const override {
      // Ignored for now:
      return false;
    }

    void run() override {
      assert(runnable_);
      for(auto& dep_ptr : this->all_deps_) {
        // Make sure the access handle does in fact have it
        assert(not dep_ptr.unique());
        dep_ptr.reset();
      }
      runnable_->run();
    }

    // end implementation of abstract::frontend::Task
    ////////////////////////////////////////////////////////////////////////////////

    void set_runnable(std::unique_ptr<RunnableBase>&& r) {
      runnable_ = std::move(r);
    }

    virtual ~TaskBase() noexcept { }

    TaskBase* current_create_work_context = nullptr;

    std::vector<std::function<void()>> registrations_to_run;
    std::vector<std::function<void()>> post_registration_ops;

  private:

    // Should this be a unique_ptr?
    std::unique_ptr<RunnableBase> runnable_;

};

class TopLevelTask
  : public TaskBase
{
  public:

    void run() override {
      // Abort, as specified.  This should never be called.
      abort();
    }

};

} // end namespace detail

} // end namespace darma_runtime


#include <darma/impl/task_capture_impl.h>


#endif /* DARMA_RUNTIME_TASK_H_ */

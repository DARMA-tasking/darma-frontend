/*
//@HEADER
// ************************************************************************
//
//                          mock_backend.cc
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

#include <iostream>

#include "mock_backend.h"

#include <abstract/backend/runtime.h>
#include <abstract/backend/data_block.h>

using namespace dharma_runtime;
using std::cout;
using std::endl;

class MockRuntime
  : public abstract::backend::runtime_t
{
  protected:

    typedef typename abstract::backend::runtime_t::task_unique_ptr task_unique_ptr;

  public:
    void register_task(task_unique_ptr&& task) override {
      cout << "registering task" << endl;
    }

    task_t* const
    get_running_task() const override {
      return top_level_task.get();
    }

    void
    register_handle(
      const handle_t* const handle
    ) override {
      cout << "registering handle" << endl;
    }

    void
    register_fetching_handle(
      handle_t* const handle,
      const key_t& user_version_tag,
      bool write_access_allowed
    ) override {
      cout << "registering fetching handle" << endl;
    }

    void
    release_read_only_usage(
      const handle_t* const handle
    ) override {
      cout << "releasing read-only usage" << endl;
    }

    void
    release_handle(
      const handle_t* const handle
    ) override {
      cout << "releasing handle" << endl;
    }

    void
    handle_done_with_version_depth(
      const handle_t* const handle
    ) override {
      cout << "handle done with version depth" << endl;
    }

    void
    publish_handle(
      const handle_t* const handle,
      const key_t& version_tag,
      const size_t n_fetchers = 1,
      bool is_final = false
    ) override {
      cout << "publishing handle" << endl;
    }


    void
    satisfy_handle(
      handle_t* const to_fill,
      bool needs_write_access = false
    ) override {
      cout << "satisfy handle" << endl;
    }

    void
    establish_containment_relationship(
      const handle_t* const inner_handle,
      const handle_t* const outer_handle,
      containment_manager_t const& manager
    ) override { }

    void
    establish_aliasing_relationship(
      const handle_t* const handle_a,
      const handle_t* const handle_b,
      aliasing_manager_t const& manager
    ) override { }

    void
    finalize() override {
      cout << "finalize" << endl;
    }

    types::unique_ptr_template<
      typename dharma_runtime::abstract::backend::runtime_t::task_t
    > top_level_task;

};


struct MockDataBlock : public abstract::backend::DataBlock
{
  void*
  get_data() override {
    return data_;
  }

  void
  acquire_data(void* const data) override { }

  void* data_;
};


void
dharma_runtime::abstract::backend::dharma_backend_initialize(
  int& argc, char**& argv,
  dharma_runtime::abstract::backend::runtime_t*& backend_runtime,
  types::unique_ptr_template<
    typename dharma_runtime::abstract::backend::runtime_t::task_t
  > top_level_task
) {
  auto* tmp_rt = new MockRuntime;
  tmp_rt->top_level_task = std::move(top_level_task);
  tmp_rt->top_level_task->set_name(
    dharma_runtime::make_key(DHARMA_BACKEND_SPMD_NAME_PREFIX, 0, 1)
  );
  backend_runtime = tmp_rt;
}


abstract::backend::runtime_t* dharma_runtime::detail::backend_runtime = nullptr;

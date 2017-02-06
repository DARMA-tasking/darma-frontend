/*
//@HEADER
// ************************************************************************
//
//                          task_capture_impl.h
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

#ifndef DARMA_TASK_CAPTURE_IMPL_H_H
#define DARMA_TASK_CAPTURE_IMPL_H_H

#include <darma/impl/task.h>
#include <darma/impl/handle.h>
#include <darma/impl/util/smart_pointers.h>

#include <darma/impl/flow_handling.h>

#include <thread>

#include "use.h"
#include "capture.h"

namespace darma_runtime {

namespace detail {

template <
  typename AccessHandleT1,
  typename AccessHandleT2
>
void
TaskBase::do_capture(
  AccessHandleT1& captured,
  AccessHandleT2 const& source_and_continuing
) {

  typedef AccessHandleT1 AccessHandleT;

  DARMA_ASSERT_MESSAGE(
    source_and_continuing.current_use_.get() != nullptr,
    "Can't capture handle after it was released or before it was initialized"
  );

  DARMA_ASSERT_MESSAGE(
    source_and_continuing.current_use_->use.scheduling_permissions_ != HandleUse::Permissions::None,
    "Can't do a capture of an AccessHandle with scheduling permissions of None"
  );

  bool check_aliasing = source_and_continuing.current_use_->use.use_->already_captured;

  source_and_continuing.captured_as_ |= default_capture_as_info;

  auto* backend_runtime = abstract::backend::get_backend_runtime();
  //std::cout << backend_runtime->get_execution_resource_count(0) << std::endl;

  auto& source = source_and_continuing;
  auto& continuing = source_and_continuing;

  if(check_aliasing) {
    if(allowed_aliasing and allowed_aliasing->aliasing_is_allowed_for(source, this)) {
      // Short-circuit rather than capturing twice...
      // TODO get the maximum permissions and pass those on
      return;
    }
    else {
      // Unallowed alias...
      DARMA_ASSERT_FAILURE(
        "Captured the same handle (with key = " << source_and_continuing.get_key()
        << ") via different variables without explicitly allowing aliasing in"
          " create_work call (using the keyword_arguments_for_task_creation::allow_aliasing"
          " keyword argument)."
      );
    }
  }

  bool ignored = (source.captured_as_ & AccessHandleBase::Ignored) != 0;

  if (not ignored) {

    ////////////////////////////////////////////////////////////////////////////////

    // Determine the capture type

    HandleUse::permissions_t requested_schedule_permissions, requested_immediate_permissions;

    // first check for any explicit permissions
    bool is_marked_read_capture = (source.captured_as_ & AccessHandleBase::ReadOnly) != 0;
    // Indicate that we've processed the ReadOnly bit by resetting it
    source.captured_as_ &= ~AccessHandleBase::ReadOnly;

    // And also schedule-only:
    bool is_marked_schedule_only = (source.captured_as_ & AccessHandleBase::ScheduleOnly) != 0;
    // Indicate that we've processed the ScheduleOnly bit by resetting it
    source.captured_as_ &= ~AccessHandleBase::ScheduleOnly;

    // And also schedule-only:
    bool is_marked_leaf = (source.captured_as_ & AccessHandleBase::Leaf) != 0;
    // Indicate that we've processed the Leaf bit by resetting it
    source.captured_as_ &= ~AccessHandleBase::Leaf;


    if (is_marked_read_capture) {
      requested_schedule_permissions = requested_immediate_permissions = HandleUse::Read;
    }
    else {
      // By default, use the strongest permissions we can schedule to
      switch (source.current_use_->use.scheduling_permissions_) {
        case HandleUse::Read: {
          requested_schedule_permissions = requested_immediate_permissions = HandleUse::Read;
          break;
        }
        case HandleUse::Modify: {
          requested_immediate_permissions = requested_schedule_permissions = HandleUse::Modify;
          break;
        }
        case HandleUse::None: {
          DARMA_ASSERT_UNREACHABLE_FAILURE(); // LCOV_EXCL_LINE
          break;
        }
        default: {
          DARMA_ASSERT_NOT_IMPLEMENTED(); // LCOV_EXCL_LINE
          break;
        }
      } // end switch on source permissions
    }

    // if it's marked schedule-only, set the requested immediate permissions to None
    if(is_marked_schedule_only) {
      requested_immediate_permissions = HandleUse::None;
    }

    // if it's marked as a leaf, set the requested scheduling permissions to None
    if(is_marked_leaf) {
      requested_schedule_permissions = HandleUse::None;
    }

    // Now make the captured use holder (and set up the continuing use holder,
    // if necessary)
    captured.current_use_ = detail::make_captured_use_holder(
      source.var_handle_,
      /* requested scheduling permissions */
      requested_schedule_permissions,
      /* requested immediate permissions */
      requested_immediate_permissions,
      source_and_continuing.current_use_
    );

    // Now add the dependency
    add_dependency(captured.current_use_->use);

    captured.var_handle_ = source.var_handle_;

  }
  else {
    // TODO remove this functionality, it's deprecated
    // ignored
    captured.current_use_ = nullptr;
  }

  // Assert that all of the captured_as info has been handled:
  assert(source.captured_as_ == AccessHandleBase::Normal);
  // And, just for good measure, that there aren't any flags set on captured
  assert(captured.captured_as_ == AccessHandleBase::Normal);

}


} // end namespace detail

} // end namespace darma_runtime

#endif //DARMA_TASK_CAPTURE_IMPL_H_H

/*
//@HEADER
// ************************************************************************
//
//                      create_work_while.h
//                         DARMA
//              Copyright (C) 2017 Sandia Corporation
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

#ifndef DARMA_IMPL_CREATE_WORK_WHILE_H
#define DARMA_IMPL_CREATE_WORK_WHILE_H

#include <tuple>
#include <type_traits>
#include <utility> // std::forward

#include <tinympl/vector.hpp>

#include <darma/impl/meta/detection.h>

#include <darma/impl/access_handle_base.h>

#include <darma/impl/util/not_a_type.h>

// TODO protect this with a preprocessor check or something?
//#include <experimental/optional>

#include "create_work_while_fwd.h"

// TODO Propagate task options and permissions downgrades

namespace darma_runtime {

namespace detail {

// TODO move these to their own file and protect it with preprocessor flags or something
//template <typename T>
//using optional = std::experimental::optional<T>;
//template <typename... Args>
//decltype(auto)
//make_optional(Args&&... args) {
//  return std::experimental::make_optional(std::forward<Args>(args)...);
//}

typedef enum WhileDoCaptureStage
{
  NotAWhileDoStage,
  OuterWhileCapture,
  OuterDoCapture,
  OuterNestedDoCapture,
  NestedDoCapture,
  NestedWhileCapture
} while_do_capture_stage_t;

struct WhileDoCaptureDescription
{
  AccessHandleBase const* source_and_continuing = nullptr;
  AccessHandleBase* captured = nullptr;
  std::shared_ptr<AccessHandleBase>* captured_copy; // should be a weak pointer
  HandleUse::permissions_t req_sched_perms = HandleUse::None;
  HandleUse::permissions_t req_immed_perms = HandleUse::None;
  //std::vector<std::shared_ptr<UseHolder>*> uses_to_set;
};

template <typename Callable, typename ArgsTuple, bool IsLambda, bool IsOuter>
struct CallableHolder;

//==============================================================================
// <editor-fold desc="CallableHolder"> {{{1

//------------------------------------------------------------------------------
// <editor-fold desc="CallableHolderBase"> {{{2

struct CallableHolderBase
{

  using capture_description_map_t = std::unordered_map<
    types::key_t, WhileDoCaptureDescription,
    typename key_traits<types::key_t>::hasher,
    typename key_traits<types::key_t>::key_equal
  >;
  using captured_handle_map_t = std::unordered_map<
    types::key_t, std::shared_ptr<AccessHandleBase>,
    typename key_traits<types::key_t>::hasher,
    typename key_traits<types::key_t>::key_equal
  >;

  HandleUse::permissions_t default_sched_perms;
  HandleUse::permissions_t default_immed_perms;

  capture_description_map_t capture_descs_;
  captured_handle_map_t implicit_captures_;
  captured_handle_map_t explicit_captures_;
  captured_handle_map_t old_explicit_captures_;

  CallableHolderBase(
    HandleUse::permissions_t default_sched_perms,
    HandleUse::permissions_t default_immed_perms
  ) : default_sched_perms(default_sched_perms),
      default_immed_perms(default_immed_perms) {}

  HandleUse::permissions_t
  get_requested_scheduling_permissions(types::key_t const& key) const
  {
    // TODO parse option args
    return default_sched_perms;
  }

  HandleUse::permissions_t
  get_requested_immediate_permissions(types::key_t const& key) const
  {
    // TODO parse option args
    return default_immed_perms;
  }

};

// </editor-fold> end CallableHolderBase }}}2
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// <editor-fold desc="CallableHolder, functor version"> {{{2

template <typename Functor, typename ArgsTuple, bool IsOuter>
struct CallableHolder<Functor, ArgsTuple, false, IsOuter>
  : CallableHolderBase,
    /* pretend to be a task so we can handle captures */
    TaskBase
{

  //------------------------------------------------------------------------------
  // <editor-fold desc="type aliases"> {{{2

  using nested_analogue_t = CallableHolder<Functor, ArgsTuple, false, false>;
  using outer_analogue_t = CallableHolder<Functor, ArgsTuple, false, true>;

  using args_tuple_t = ArgsTuple;

  // </editor-fold> end type aliases }}}2
  //------------------------------------------------------------------------------


  //------------------------------------------------------------------------------
  // <editor-fold desc="data members"> {{{2

  // args storage tuple
  std::unique_ptr<args_tuple_t> args_;
  // TODO inline storage with something like std::optional
  std::unique_ptr<args_tuple_t> args_tmp_ = nullptr;


  // </editor-fold> end data members }}}2
  //------------------------------------------------------------------------------

  template <
    typename Task
  >
  _not_a_lambda
  trigger_capture(Task& task)
  {
    // trigger a copy
    args_tmp_ = std::make_unique<args_tuple_t>(*args_);

    // delete the "copied out of" object ASAP
    // Note: this triggers a release/alias before a task is registered.
    // TODO Make sure this is compatible with all backend invariants (once we have them)
    args_ = nullptr;

    return _not_a_lambda{};
  }

  template <typename _ignored_SFINAE=void>
  void
  restore_callable(
    _not_a_lambda const&,
    std::enable_if_t<
      std::is_void<_ignored_SFINAE>::value // always true
        and IsOuter,
      detail::_not_a_type_numbered<0>
    > = {}
  )
  {
    //assert(false); // should never be called
    // make another copy for later use
    if(args_tmp_) { // basically, do the restore unless our parent is outer
      assert(not args_);
      args_ = std::make_unique<args_tuple_t>(std::move(*args_tmp_));
      args_tmp_ = nullptr; // delete the expired args_tmp ASAP
    }
  }

  template <typename _ignored_SFINAE=void>
  void
  restore_callable(
    _not_a_lambda const&,
    std::enable_if_t<
      std::is_void<_ignored_SFINAE>::value // always true
        and not IsOuter,
      detail::_not_a_type_numbered<1>
    > = {}
  )
  {
    // Not sure this is necessary; could just swap which variable we splat from
    if(args_tmp_) { // basically, do the restore unless our parent is outer
      assert(not args_);
      args_ = std::make_unique<args_tuple_t>(std::move(*args_tmp_));
      args_tmp_ = nullptr; // delete the expired args_tmp ASAP
    }
  }

  // Only called on the IsOuter case
  template <typename Task, typename ArgsFwdTuple>
  decltype(auto)
  _prepare_args(
    Task& task,
    ArgsFwdTuple&& args
  )
  {
    auto* parent_task = static_cast<darma_runtime::detail::TaskBase*>(
      abstract::backend::get_backend_context()->get_running_task()
    );
    // make ourselves the "Task" that handles the capture
    parent_task->current_create_work_context = &task;

    // This should trigger copy ctors of all AccessHandles
    // (and AccessHandleCollections, etc.) since they should be lvalues in the
    // outer enclosing scope
    return std::make_unique<args_tuple_t>(std::forward<ArgsFwdTuple>(args));
  }


  template <
    typename Task,
    typename ArgsFwdTuple,
    typename OptionArgsTuple
  >
  CallableHolder(
    Task& task,
    _not_a_lambda&&,
    ArgsFwdTuple&& args,
    OptionArgsTuple&&, // TODO parse option args
    HandleUse::permissions_t default_sched_perms,
    HandleUse::permissions_t default_immed_perms
  ) : CallableHolderBase(default_sched_perms, default_immed_perms),
      args_(_prepare_args(task, std::forward<ArgsFwdTuple>(args)))
  { }

  CallableHolder(CallableHolder&&) = default;
  CallableHolder(CallableHolder const&) = delete;

  template <typename _Ignored_SFINAE=void>
  CallableHolder(
    outer_analogue_t&& other,
    std::enable_if_t<
      std::is_void<_Ignored_SFINAE>::value
        and not IsOuter,
      _not_a_type
    > = {}
  ) : CallableHolderBase(std::move(other)),
      args_(std::move(other.args_)),
      args_tmp_(std::move(other.args_tmp_))
  { }

  template <size_t... Idxs>
  auto
  do_call_impl(
    std::integer_sequence<std::size_t, Idxs...>
  )
  {
    using functor_call_traits_ = functor_call_traits<
      Functor,
      std::tuple_element_t<Idxs, args_tuple_t>...
    >;
    return Functor{}(
      functor_call_traits_::template call_arg_traits<Idxs>::get_converted_arg(
        std::get<Idxs>(*args_)
      )...
    );
  }

  auto do_call()
  {
    return do_call_impl(
      std::make_index_sequence<std::tuple_size<args_tuple_t>::value>{}
    );
  }

};

// </editor-fold> end CallableHolder, functor version }}}2
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// <editor-fold desc="CallableHolder, lambda version"> {{{2

template <typename Lambda, bool IsOuter>
struct CallableHolder<Lambda, std::tuple<>, true, IsOuter>
  : CallableHolderBase
{

  using nested_analogue_t = CallableHolder<Lambda, std::tuple<>, true, false>;
  using outer_analogue_t = CallableHolder<Lambda, std::tuple<>, true, true>;

  using lambda_t = std::remove_reference_t<Lambda>; // force value semantics everywhere

  // TODO make this inline storage with something like std::optional
  std::unique_ptr<lambda_t> lambda_;

  template <typename Task>
  auto
  trigger_capture(Task& task)
  {
    // do the copy
    auto rv = std::make_unique<lambda_t>(*lambda_);

    // delete the "copied out of" object ASAP
    // Note: this triggers a release/alias before a task is registered.
    // TODO Make sure this is compatible with all backend invariants (once we have them)
    lambda_ = nullptr;

    // move into the return value
    return std::move(rv);
  }

  void
  restore_callable(std::unique_ptr<Lambda>& tmp_ptr)
  {
    // Now move it back
    // We have to do weird stuff here to make sure we invoke the move constructor
    // and not the move assignment operator (which may/should be deleted)
    lambda_ = std::make_unique<lambda_t>(std::move(*tmp_ptr));
    tmp_ptr =
      nullptr; // trigger destructor of (now-empty) Lambda in the unique ptr
  }

  template <typename Task, typename OptionArgsTuple>
  CallableHolder(
    Task& task,
    Lambda&& in_lambda,
    std::tuple<>&&,
    OptionArgsTuple&&, // TODO parse option args
    HandleUse::permissions_t default_sched_perms,
    HandleUse::permissions_t default_immed_perms
  ) : CallableHolderBase(default_sched_perms, default_immed_perms),
      lambda_(std::make_unique<lambda_t>(std::move(in_lambda))) {}

  CallableHolder(CallableHolder&&) = default;
  CallableHolder(CallableHolder const&) = delete;

  template <typename _Ignored_SFINAE=void>
  CallableHolder(
    outer_analogue_t&& other,
    std::enable_if_t<
      std::is_void<_Ignored_SFINAE>::value
        and not IsOuter,
      _not_a_type
    > = {}
  ) : CallableHolderBase(std::move(other)),
      lambda_(std::move(other.lambda_)) {}

  auto do_call()
  {
    return (*lambda_)();
  }

};

// </editor-fold> end CallableHolder, lambda version }}}2
//------------------------------------------------------------------------------

// </editor-fold> end CallableHolder }}}1
//==============================================================================


//==============================================================================
// <editor-fold desc="WhileDoTask"> {{{1

template <
  typename WhileCallable, typename WhileArgsTuple, bool WhileIsLambda,
  typename DoCallable, typename DoArgsTuple, bool DoIsLambda,
  bool IsDoWhilePhase /*=false*/, bool IsOuter/*=true*/
>
struct WhileDoTask: public TaskBase
{

  //------------------------------------------------------------------------------
  // <editor-fold desc="type aliases"> {{{2

  using while_holder_t = CallableHolder<
    WhileCallable,
    WhileArgsTuple,
    WhileIsLambda,
    IsOuter
  >;
  using do_holder_t = CallableHolder<
    DoCallable,
    DoArgsTuple,
    DoIsLambda,
    IsOuter
  >;

  template <typename PermanentHolder>
  using tmp_holder_t = decltype(
  std::declval<PermanentHolder>().trigger_capture(
    std::declval<WhileDoTask&>()
  )
  );
  using tmp_while_holder_t = tmp_holder_t<while_holder_t>;
  using tmp_do_holder_t = tmp_holder_t<do_holder_t>;

  // Needs to be a template to avoid infinite recursion (NOT ACTUALLY NECESSARY)
  template <bool InnerIsDoWhile>
  using nested_analogue = WhileDoTask<
    WhileCallable, WhileArgsTuple, WhileIsLambda,
    DoCallable, DoArgsTuple, DoIsLambda,
    InnerIsDoWhile, false
  >;

  // </editor-fold> end type aliases }}}2
  //------------------------------------------------------------------------------;


  //------------------------------------------------------------------------------
  // <editor-fold desc="Data members"> {{{2

  // NOTE THAT MEMBER ORDER MATTERS HERE FOR INITIALIZATION ORDER!!!

  while_do_capture_stage_t current_stage_;

  while_holder_t while_holder_;
  tmp_while_holder_t while_fxn_temp_holder_;

  do_holder_t do_holder_;
  tmp_do_holder_t do_fxn_temp_holder_;

  // </editor-fold> end Data members }}}2
  //------------------------------------------------------------------------------


  //------------------------------------------------------------------------------
  // <editor-fold desc="ctor helpers"> {{{2

  template <typename... Args>
  std::enable_if_t<
    sizeof...(Args) != 0 // always true
      and IsOuter // only used for the outer case
      and not WhileIsLambda,
    while_holder_t
  >
  _while_holder_ctor_helper(
    Args&& ... args
  )
  {
    // The capture actually happens in the holder ctor for the outer, non-lambda
    // case, so we need to set up the context before the ctor is called
    auto* parent_task =
      detail::safe_static_cast<darma_runtime::detail::TaskBase*>(
        abstract::backend::get_backend_context()->get_running_task()
      );
    parent_task->current_create_work_context = this;

    current_stage_ = OuterWhileCapture;
    is_double_copy_capture = false;

    return while_holder_t(
      std::forward<Args>(args)...
    );
  }

  template <typename... Args>
  std::enable_if_t<
    sizeof...(Args) != 0 // always true
      and IsOuter // only used for the outer case
      and WhileIsLambda,
    while_holder_t
  >
  _while_holder_ctor_helper(
    Args&& ... args
  )
  {
    // Set the current create work context anyway, so that we don't have to
    // do it in the do_holder helper
    auto* parent_task =
      detail::safe_static_cast<darma_runtime::detail::TaskBase*>(
        abstract::backend::get_backend_context()->get_running_task()
      );
    parent_task->current_create_work_context = this;

    return while_holder_t(
      std::forward<Args>(args)...
    );
  }

  template <typename _Ignored_SFINAE=void>
  tmp_while_holder_t
  _while_tmp_ctor_helper(
    std::enable_if_t<
      std::is_void<_Ignored_SFINAE>::value // always true
        and IsOuter // only used for the outer case
        and not WhileIsLambda,
      detail::_not_a_type_numbered<0>
    > = {}
  )
  {
    // Nothing to do here...
    return tmp_while_holder_t{};
  }

  template <typename _Ignored_SFINAE=void>
  tmp_while_holder_t
  _while_tmp_ctor_helper(
    std::enable_if_t<
      std::is_void<_Ignored_SFINAE>::value // always true
        and IsOuter // only used for the outer case
        and WhileIsLambda,
      detail::_not_a_type_numbered<1>
    > = {}
  )
  {
    // Do the capture in lambda mode (must happen in member "constructor"
    // so that the captures happen while first then do even if the while
    // is a lambda and the do is a functor
    current_stage_ = OuterWhileCapture;
    is_double_copy_capture = true;
    auto rv = while_holder_.trigger_capture(*this);
    is_double_copy_capture = false;
    current_stage_ = NotAWhileDoStage;

    return std::move(rv);
  }

  template <typename... Args>
  std::enable_if_t<
    sizeof...(Args) != 0 // always true
      and IsOuter // only used for the outer case
      and not DoIsLambda,
    do_holder_t
  >
  _do_holder_ctor_helper(
    Args&& ... args
  )
  {
    // The capture actually happens in the holder ctor for the outer, non-lambda
    // case, so we need to set up the context before the ctor is called
    // The current_create_work_context gets set in the while holder ctor helper
    // no matter what, so we don't need to do it here
    current_stage_ = OuterNestedDoCapture;
    is_double_copy_capture = false;

    return do_holder_t(
      std::forward<Args>(args)...
    );
  }

  template <typename... Args>
  std::enable_if_t<
    sizeof...(Args) != 0 // always true
      and IsOuter // only used for the outer case
      and DoIsLambda,
    do_holder_t
  >
  _do_holder_ctor_helper(
    Args&& ... args
  )
  {
    // Nothing to do in the Lambda case
    return do_holder_t(
      std::forward<Args>(args)...
    );
  }

  template <typename _Ignored_SFINAE=void>
  tmp_do_holder_t
  _do_tmp_ctor_helper(
    std::enable_if_t<
      std::is_void<_Ignored_SFINAE>::value // always true
        and IsOuter // only used for the outer case
        and not DoIsLambda,
      detail::_not_a_type_numbered<0>
    > = {}
  )
  {
    // Nothing to do here... (specifically, don't all trigger_capture())
    return tmp_do_holder_t{};
  }

  template <typename _Ignored_SFINAE=void>
  tmp_do_holder_t
  _do_tmp_ctor_helper(
    std::enable_if_t<
      std::is_void<_Ignored_SFINAE>::value // always true
        and IsOuter // only used for the outer case
        and DoIsLambda,
      detail::_not_a_type_numbered<1>
    > = {}
  )
  {
    current_stage_ = OuterNestedDoCapture;
    is_double_copy_capture = true;
    auto rv = do_holder_.trigger_capture(*this);
    is_double_copy_capture = false;
    current_stage_ = NotAWhileDoStage;

    return std::move(rv);
  }

  // </editor-fold> end ctor helpers }}}2
  //------------------------------------------------------------------------------


  //------------------------------------------------------------------------------
  // <editor-fold desc="Ctors"> {{{2

  // This is the outer constructor call from a create_work_while().do_()
  template <typename WhileDoHelper>
  WhileDoTask(
    WhileDoHelper&& helper,
    std::enable_if_t<
      not IsDoWhilePhase and IsOuter
        and not std::is_void<WhileDoHelper>::value, // should always be true
      int
    > = 0
  ) : while_holder_(
        _while_holder_ctor_helper(
          *this,
          std::move(helper.while_helper.while_lambda),
          std::move(helper.while_helper.args_fwd_tup),
          std::move(helper.while_helper.task_option_args_tup),
          HandleUse::Read, HandleUse::Read
        )
      ),
      while_fxn_temp_holder_(_while_tmp_ctor_helper()),
      do_holder_(
        _do_holder_ctor_helper(
          *this,
          std::move(helper.do_lambda),
          std::move(helper.args_fwd_tup),
          std::move(helper.task_option_args_tup),
          HandleUse::Modify, HandleUse::Modify
        )
      ),
      do_fxn_temp_holder_(_do_tmp_ctor_helper())
  {
    // Handle the while capture
    auto* parent_task =
      detail::safe_static_cast<darma_runtime::detail::TaskBase*>(
        abstract::backend::get_backend_context()->get_running_task()
      );
    // The current_create_work_context gets set in the while holder ctor helper
    // no matter what, so we don't need to do it here
    // (as if: `parent_task->current_create_work_context = this;` were here)

    // The while capture is done in the tmp_while_holder "ctor helper" so that
    // it happens before the do capture no matter what.  It's as if the following
    // code were run *before* the do holder ctor is run:
    // [-- begin "as-if" code:
    //   current_stage_ = OuterWhileCapture;
    //   is_double_copy_capture = true;
    //   while_fxn_temp_holder_ = while_holder_.trigger_capture(*this);
    //   is_double_copy_capture = false;
    //   current_stage_ = OuterNestedDoCapture;
    //   is_double_copy_capture = true;
    //   do_fxn_temp_holder_ = do_holder_.trigger_capture(*this);
    //   is_double_copy_capture = false;
    //   current_stage_ = NotAWhileDoStage;
    // --] end "as-if" code

    parent_task->current_create_work_context = nullptr;

    for (auto&& while_desc_pair : while_holder_.capture_descs_) {
      auto& while_desc = while_desc_pair.second;
      auto& key = while_desc_pair.first;

      // Now do the actual capture
      while_desc.captured->call_make_captured_use_holder(
        while_desc.source_and_continuing->var_handle_base_,
        while_desc.req_sched_perms,
        while_desc.req_immed_perms,
        *while_desc.source_and_continuing
      );
      // Not sure if I need this; note that it slices
      while_desc.captured_copy->get()->replace_use_holder_with(
        *while_desc.captured
      );
      add_dependency(*while_desc.captured->current_use_base_->use_base);
    }
    while_holder_.capture_descs_.clear();

    // Cleanup:  remove the alias guards
    // TODO make this work
    //for(auto* use_to_unmark : uses_to_unmark_already_captured) {
    //  use_to_unmark->already_captured = false;
    //}

  }

  // This is a recursive inner constructor, called for "do" mode
  template <
    typename InWhileHolderT,
    typename InDoHolderT
  >
  WhileDoTask(
    InWhileHolderT&& in_while_holder,
    InDoHolderT&& in_do_holder,
    tmp_while_holder_t&& in_tmp_while_holder,
    tmp_do_holder_t&& in_tmp_do_holder,
    std::enable_if_t<
      IsDoWhilePhase and not IsOuter
        and not std::is_void<InWhileHolderT>::value, // always true
      _not_a_type
    > = { }
  ) : while_holder_(std::move(in_while_holder)),
      do_holder_(std::move(in_do_holder)),
      while_fxn_temp_holder_(std::move(in_tmp_while_holder)),
      do_fxn_temp_holder_(std::move(in_tmp_do_holder))
  {
    // Register the do_descs from the outer task
    for (auto&& do_desc_pair : do_holder_.capture_descs_) {
      auto& do_desc = do_desc_pair.second;
      auto& key = do_desc_pair.first;

      // Now do the actual capture
      do_desc.captured->call_make_captured_use_holder(
        do_desc.source_and_continuing->var_handle_base_,
        do_desc.req_sched_perms,
        do_desc.req_immed_perms,
        *do_desc.source_and_continuing
      );
      do_desc.captured_copy->get()->replace_use_holder_with(
        *do_desc.captured
      );
      add_dependency(*do_desc.captured->current_use_base_->use_base);
    }
    do_holder_.capture_descs_.clear();

    // Delete the implicit and explicit from the while
    while_holder_.implicit_captures_.clear();
    while_holder_.old_explicit_captures_.clear();

    // The capture descriptions in while_holder_ should be correct at this point
    // except that they need to be corrected to also handle the schedule-only
    // captures of the do parts

    // Cleanup:  remove the alias guards
    for (auto* use_to_unmark : uses_to_unmark_already_captured) {
      use_to_unmark->already_captured = false;
    }
  }

  // This is a recursive inner constructor, called for "while" mode
  template <typename _SFINAE_only=void>
  WhileDoTask(
    while_holder_t&& in_while_holder,
    do_holder_t&& in_do_holder,
    tmp_while_holder_t&& in_tmp_while_holder,
    tmp_do_holder_t&& in_tmp_do_holder,
    std::enable_if_t<
      not IsDoWhilePhase and not IsOuter
        and std::is_void<_SFINAE_only>::value,
      int
    > = 0
  ) : while_holder_(std::move(in_while_holder)),
      do_holder_(std::move(in_do_holder)),
      while_fxn_temp_holder_(std::move(in_tmp_while_holder)),
      do_fxn_temp_holder_(std::move(in_tmp_do_holder))
  {
    // Register the do_descs from the outer task
    for (auto&& while_desc_pair : while_holder_.capture_descs_) {
      auto& while_desc = while_desc_pair.second;
      auto& key = while_desc_pair.first;

      // Now do the actual capture
      while_desc.captured->call_make_captured_use_holder(
        while_desc.captured->var_handle_base_,
        while_desc.req_sched_perms,
        while_desc.req_immed_perms,
        *while_desc.source_and_continuing
      );
      while_desc.captured_copy->get()->replace_use_holder_with(
        *while_desc.captured
      );
      add_dependency(*while_desc.captured->current_use_base_->use_base);
    }
    while_holder_.capture_descs_.clear();

    // Delete the implicit and explicit from the while
    do_holder_.implicit_captures_.clear();
    do_holder_.old_explicit_captures_.clear();

    // The capture descriptions in do_holder_ should be correct at this point
    // except that they need to be corrected to also handle the schedule-only
    // captures of the nested while parts

    // Cleanup:  remove the alias guards
    for (auto* use_to_unmark : uses_to_unmark_already_captured) {
      use_to_unmark->already_captured = false;
    }
  }

  // </editor-fold> end Ctors }}}2
  //------------------------------------------------------------------------------


  //------------------------------------------------------------------------------
  // <editor-fold desc="do_capture()"> {{{2

  void do_capture(
    AccessHandleBase& captured,
    AccessHandleBase const& source_and_continuing
  ) override
  {
    // TODO capture checks

    switch (current_stage_) {
      //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // <editor-fold desc="OuterWhileCapture"> {{{2

      case WhileDoCaptureStage::OuterWhileCapture: {

        auto& key = source_and_continuing.var_handle_base_->get_key();

        auto& while_desc = while_holder_.capture_descs_[key];

        while_desc.source_and_continuing = &source_and_continuing;
        while_desc.captured = &captured;

        while_desc.req_immed_perms =
          while_holder_.get_requested_immediate_permissions(key);
        while_desc.req_sched_perms =
          while_holder_.get_requested_scheduling_permissions(key);

        captured.release_current_use();

        // Make the AccessHandle for the explicit capture
        {
          auto insert_result = while_holder_.explicit_captures_.insert(
            std::make_pair(key, captured.copy(false))
          );
          assert(insert_result.second);
          while_desc.captured_copy = &(insert_result.first->second);
        } // delete insertion result

        // Make sure it can schedule itself
        if (while_desc.req_sched_perms < while_desc.req_immed_perms) {
          while_desc.req_sched_perms = while_desc.req_immed_perms;
        }

        break;
      }

        // </editor-fold> end OuterWhileCapture }}}2
        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // <editor-fold desc="NestedDoCapture"> {{{2

      case WhileDoCaptureStage::OuterNestedDoCapture:
      case WhileDoCaptureStage::NestedDoCapture: {
        auto& key = source_and_continuing.var_handle_base_->get_key();


        auto& while_desc = while_holder_.capture_descs_[key];
        auto& do_desc = do_holder_.capture_descs_[key];

        do_desc.req_immed_perms =
          do_holder_.get_requested_immediate_permissions(key);
        do_desc.req_sched_perms =
          do_holder_.get_requested_scheduling_permissions(key);
        do_desc.captured = &captured;

        // Make sure the callable we don't hold a duplicate of the outer
        // context handle, since it is held in the while part
        // This (should be?) okay since this will be replaced before another
        // capture that depends on it is triggered
        captured.release_current_use();
        //assert(captured.current_use_base_ == nullptr);

        // Make the AccessHandle for the explicit capture
        // (so that if the handle gets released in the middle, we still retain
        // scheduling permissions)
        {
          auto insert_result = do_holder_.explicit_captures_.insert(
            std::make_pair(key, captured.copy(false))
          );
          assert(insert_result.second);
          do_desc.captured_copy = &(insert_result.first->second);
        } // delete insertion result

        // Modify the (parent) do_desc so that it captures this if it doesn't already
        if (while_desc.captured == nullptr) {
          // create an implicit capture for the outer while
          assert(while_holder_.implicit_captures_.find(key)
            == while_holder_.implicit_captures_.end());
          {
            auto insert_result = while_holder_.implicit_captures_.insert(
              std::make_pair(key, source_and_continuing.copy(false))
            );
            assert(insert_result.second);
            while_desc.captured_copy = &(insert_result.first->second);
          } // delete insertion result
          while_desc.captured = while_desc.captured_copy->get();

          // This only works for the outer context case
          if (current_stage_ == WhileDoCaptureStage::OuterNestedDoCapture) {
            while_desc.source_and_continuing = &source_and_continuing;
          } else {
            // If it's implicit in the while, it has to be explicit in the do
            auto explicit_found = do_holder_.old_explicit_captures_.find(key);
            assert(explicit_found != do_holder_.old_explicit_captures_.end());
            while_desc.source_and_continuing = explicit_found->second.get();
          }

          while_desc.req_sched_perms = do_desc.req_immed_perms;

        } else {
          // Check whether or not the scheduling permissions need to be upgraded
          if (while_desc.req_sched_perms < do_desc.req_immed_perms) {
            while_desc.req_sched_perms = do_desc.req_immed_perms;
          }
        }

        // Make sure the while can schedule the things that the do schedules
        if (while_desc.req_sched_perms < do_desc.req_sched_perms) {
          while_desc.req_sched_perms = do_desc.req_sched_perms;
        }

        // Make sure it can schedule itself
        if (do_desc.req_sched_perms < do_desc.req_immed_perms) {
          do_desc.req_sched_perms = do_desc.req_immed_perms;
        }

        do_desc.source_and_continuing = while_desc.captured_copy->get();

        break;
      }

        // </editor-fold> end NestedDoCapture }}}2
        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // <editor-fold desc="NestedWhileCapture"> {{{2

      case WhileDoCaptureStage::NestedWhileCapture: {
        auto& key = source_and_continuing.var_handle_base_->get_key();
        auto& do_desc = do_holder_.capture_descs_[key];

        auto& while_desc = while_holder_.capture_descs_[key];
        while_desc.req_sched_perms =
          while_holder_.get_requested_scheduling_permissions(key);
        while_desc.req_immed_perms =
          while_holder_.get_requested_immediate_permissions(key);
        while_desc.captured = &captured;

        // Make the AccessHandle for the explicit capture
        // (so that if the handle gets released in the middle, we still retain
        // scheduling permissions)
        {
          auto insert_result = while_holder_.explicit_captures_.insert(
            std::make_pair(key,
              captured.copy(false)
            )
          );
          assert(insert_result.second);
          while_desc.captured_copy = &(insert_result.first->second);
        } // delete insertion result


        // Modify the (parent) do_desc so that it captures this if it doesn't already
        if (do_desc.captured == nullptr) {
          // create an implicit capture for the outer do
          assert(do_holder_.implicit_captures_.find(key)
            == do_holder_.implicit_captures_.end()
          );
          auto insert_result = do_holder_.implicit_captures_.insert(
            std::make_pair(key, captured.copy(false))
          );
          assert(insert_result.second);
          do_desc.captured_copy = &(insert_result.first->second);
          do_desc.captured = do_desc.captured_copy->get();
          do_desc.req_sched_perms = while_desc.req_immed_perms;
        } else {
          // Check whether or not the scheduling permissions need to be upgraded
          if (do_desc.req_sched_perms < while_desc.req_immed_perms) {
            do_desc.req_sched_perms = while_desc.req_immed_perms;
          }
        }

        // Make sure the do can schedule the things that the while schedules
        if (do_desc.req_sched_perms < while_desc.req_sched_perms) {
          do_desc.req_sched_perms = while_desc.req_sched_perms;
        }

        while_desc.source_and_continuing = do_desc.captured_copy->get();

        break;
      }

        // </editor-fold> end NestedWhileCapture }}}2
        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      case WhileDoCaptureStage::OuterDoCapture: {
        DARMA_ASSERT_NOT_IMPLEMENTED("Do-While loop mode");
        break;
      }
      case WhileDoCaptureStage::NotAWhileDoStage: {
        DARMA_ASSERT_UNREACHABLE_FAILURE(
          "Something went wrong with While-Do capture"
        );
        break;
      }
    }

  }

  // </editor-fold> end do_capture() }}}2
  //------------------------------------------------------------------------------


  //------------------------------------------------------------------------------
  // <editor-fold desc="run() and helpers"> {{{2

  void _do_run(
    std::true_type, /* running in do-while mode */
    std::false_type /* not outer */
  )
  {
    // Wait until the last possible moment to restore the lambda, so that pointers
    // used elsewhere don't get invalidated
    do_holder_.restore_callable(do_fxn_temp_holder_);

    do_holder_.do_call();

    // trigger a nested do block capture for the while to use,
    // since the descriptions in the while are already correct except that
    // they don't include the nested do scheduling permissions
    // The do_holder.capture_descs_ should be clear at this point, and
    // the capture here will set up the descriptions for the *next* do call
    // (if it ever gets made)
    assert(do_holder_.capture_descs_.empty());
    auto* parent_task = static_cast<darma_runtime::detail::TaskBase*>(
      abstract::backend::get_backend_context()->get_running_task()
    );
    // Setup to do the capture
    parent_task->current_create_work_context = this;
    current_stage_ = NestedDoCapture;
    is_double_copy_capture =
      false; // should be default, so we should be able to remove this
    std::swap(do_holder_.old_explicit_captures_, do_holder_.explicit_captures_);
    // Trigger the actual capture
    do_fxn_temp_holder_ = do_holder_.trigger_capture(*this);
    // Cleanup
    parent_task->current_create_work_context = nullptr;

    auto while_do_task = std::make_unique<
      nested_analogue<false /* i.e., while-do mode */>
    >(
      std::move(while_holder_), std::move(do_holder_),
      std::move(while_fxn_temp_holder_), std::move(do_fxn_temp_holder_)
    );

    abstract::backend::get_backend_runtime()->register_task(
      std::move(while_do_task)
    );
  }

  template <typename IsOuterType>
  void _do_run(
    std::false_type, /* not in do-while mode (i.e., in while-do mode) */
    IsOuterType /* is outer doesn't matter */
  )
  {
    // Wait until the last possible moment to restore the lambda, so that pointers
    // used elsewhere don't get invalidated
    while_holder_.restore_callable(while_fxn_temp_holder_);

    // Now do the actual call
    if (while_holder_.do_call()) {

      // trigger a nested while block capture for the do to use,
      // since the descriptions in the do are already correct except that
      // they don't include the nested while scheduling permissions
      // The while_holder.capture_descs_ should be clear at this point, and
      // the capture here will set up the descriptions for the *next* while call
      assert(while_holder_.capture_descs_.empty());
      auto* parent_task = static_cast<darma_runtime::detail::TaskBase*>(
        abstract::backend::get_backend_context()->get_running_task()
      );
      // Setup to do the capture
      parent_task->current_create_work_context = this;
      current_stage_ = NestedWhileCapture;
      is_double_copy_capture =
        false; // should be default, so we should be able to remove this
      std::swap(
        while_holder_.old_explicit_captures_, while_holder_.explicit_captures_
      );
      // Trigger the actual capture
      while_fxn_temp_holder_ = while_holder_.trigger_capture(*this);
      // Cleanup
      parent_task->current_create_work_context = nullptr;

      // Pass everything off to the next task
      auto do_while_task = std::make_unique<
        nested_analogue<true /* i.e., do-while mode */>
      >(
        std::move(while_holder_), std::move(do_holder_),
        std::move(while_fxn_temp_holder_), std::move(do_fxn_temp_holder_)
      );

      abstract::backend::get_backend_runtime()->register_task(
        std::move(do_while_task)
      );
    }
  }

  void run() override
  {
    _do_run(
      std::integral_constant<bool, IsDoWhilePhase>{},
      std::integral_constant<bool, IsOuter>{}
    );
  }

  // </editor-fold> end run() and helpers }}}2
  //------------------------------------------------------------------------------

};

// </editor-fold> end WhileDoTask }}}1
//==============================================================================


//==============================================================================
// <editor-fold desc="Task creation helpers in the form of proxy return objects"> {{{1

template <typename...>
struct _create_work_while_helper;
template <typename...>
struct _create_work_while_do_helper;

//------------------------------------------------------------------------------
// <editor-fold desc="create_work_while_do_helper, functor version"> {{{2

template <
  typename WhileHelper,
  typename Functor,
  typename LastArg,
  typename... Args
>
struct _create_work_while_do_helper<
  WhileHelper,
  Functor,
  tinympl::vector<Args...>,
  LastArg
>
{

  using functor_traits_t = functor_call_traits<Functor, Args..., LastArg>;
  using args_tuple_t = typename functor_traits_t::args_tuple_t;
  using args_fwd_tuple_t = std::tuple<Args&& ..., LastArg&&>;
  //using task_option_args_tuple_t = decltype(
  //  std::forward_as_tuple(std::declval<Args&&>()...)
  //);
  using task_option_args_tuple_t = std::tuple<>; // for now

  _not_a_lambda do_lambda;
  args_fwd_tuple_t args_fwd_tup;
  task_option_args_tuple_t task_option_args_tup;
  WhileHelper while_helper;

  static constexpr auto has_lambda_callable = false;

  _create_work_while_do_helper(_create_work_while_do_helper&&) = default;
  _create_work_while_do_helper(_create_work_while_do_helper const&) = delete;

  _create_work_while_do_helper(
    WhileHelper&& while_helper_in,
    Args&& ... args,
    LastArg&& last_arg
  ) : while_helper(std::move(while_helper_in)),
      args_fwd_tup(std::forward<Args>(args)...,
        std::forward<LastArg>(last_arg)
      ) {}

  ~_create_work_while_do_helper()
  {
    auto while_do_task = std::make_unique<
      WhileDoTask<
        typename WhileHelper::callable_t, typename WhileHelper::args_tuple_t,
        WhileHelper::has_lambda_callable,
        Functor, args_tuple_t, has_lambda_callable
      >
    >(
      std::move(*this)
    );

    abstract::backend::get_backend_runtime()->register_task(
      std::move(while_do_task)
    );
  }
};

// </editor-fold> end create_work_while_do_helper, functor version }}}2
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// <editor-fold desc="create_work_while_do_helper, lambda version"> {{{2

template <typename WhileHelper, typename Lambda, typename... Args>
struct _create_work_while_do_helper<
  WhileHelper,
  meta::nonesuch,
  tinympl::vector<Args...>,
  Lambda
>
{

  using callable_t = Lambda;
  using args_tuple_t = std::tuple<>;
  using args_fwd_tuple_t = std::tuple<>;
  using task_option_args_tuple_t = decltype(
  std::forward_as_tuple(std::declval<Args&&>()...)
  );

  callable_t do_lambda;
  args_fwd_tuple_t args_fwd_tup;
  task_option_args_tuple_t task_option_args_tup;
  WhileHelper while_helper;

  static constexpr auto has_lambda_callable = true;

  _create_work_while_do_helper(_create_work_while_do_helper&&) = default;
  _create_work_while_do_helper(_create_work_while_do_helper const&) = delete;

  _create_work_while_do_helper(
    WhileHelper&& while_helper_in,
    Args&& ... args,
    std::remove_reference_t<Lambda>&& f // force rvalue reference
  ) : do_lambda(std::move(f)),
      while_helper(std::move(while_helper_in)),
      task_option_args_tup(
        std::forward_as_tuple(std::forward<Args>(args)...)
      ) {}

  ~_create_work_while_do_helper()
  {

    auto while_do_task = std::make_unique<
      WhileDoTask<
        typename WhileHelper::callable_t, typename WhileHelper::args_tuple_t,
        WhileHelper::has_lambda_callable,
        callable_t, args_tuple_t, has_lambda_callable
      >
    >(
      std::move(*this)
    );

    abstract::backend::get_backend_runtime()->register_task(
      std::move(while_do_task)
    );
  }
};


// </editor-fold> end create_work_while_do_helper, lambda version }}}2
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// <editor-fold desc="create_work_while_helper, lambda version"> {{{2

template <typename Lambda, typename... Args>
struct _create_work_while_helper<
  meta::nonesuch,
  tinympl::vector<Args...>,
  Lambda
>
{

  using callable_t = Lambda;
  using args_tuple_t = std::tuple<>;
  using args_fwd_tuple_t = std::tuple<>;
  using task_option_args_tuple_t = decltype(
  std::forward_as_tuple(std::declval<Args&&>()...)
  );

  static constexpr auto has_lambda_callable = true;

  callable_t while_lambda;
  args_fwd_tuple_t args_fwd_tup;
  task_option_args_tuple_t task_option_args_tup;

  _create_work_while_helper(_create_work_while_helper&&) = default;
  _create_work_while_helper(_create_work_while_helper const&) = delete;

  _create_work_while_helper(
    Args&& ... args,
    std::remove_reference_t<Lambda>&& f // force rvalue reference<
  ) : while_lambda(std::move(f)),
      task_option_args_tup(
        std::forward_as_tuple(std::forward<Args>(args)...)
      ) {}

  template <typename DoFunctor=meta::nonesuch, typename... DoArgs>
  auto
  do_(DoArgs&& ... args)&&
  {
    return _create_work_while_do_helper<
      _create_work_while_helper, DoFunctor,
      typename tinympl::vector<DoArgs...>::pop_back::type,
      typename tinympl::vector<DoArgs...>::back::type
    >(
      std::move(*this),
      std::forward<DoArgs>(args)...
    );
  }

};

// </editor-fold> end create_work_while_helper, lambda version }}}2
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// <editor-fold desc="create_work_while_helper, functor version"> {{{2

template <typename Functor, typename LastArg, typename... Args>
struct _create_work_while_helper<
  Functor,
  tinympl::vector<Args...>,
  LastArg
>
{

  using callable_t = Functor;
  using functor_traits_t = functor_call_traits<Functor, Args..., LastArg>;
  using args_tuple_t = typename functor_traits_t::args_tuple_t;
  using args_fwd_tuple_t = std::tuple<Args&& ..., LastArg&&>;
  using task_option_args_tuple_t = std::tuple<>; // for now
  //using task_option_args_tuple_t = decltype(
  //  std::forward_as_tuple(std::declval<Args&&>()...)
  //);

  static constexpr auto has_lambda_callable = false;

  _not_a_lambda while_lambda;

  args_fwd_tuple_t args_fwd_tup;
  task_option_args_tuple_t task_option_args_tup;

  _create_work_while_helper(_create_work_while_helper&&) = default;
  _create_work_while_helper(_create_work_while_helper const&) = delete;

  _create_work_while_helper(
    Args&& ... args,
    LastArg&& last_arg
  ) : args_fwd_tup(
    std::forward<Args>(args)...,
    std::forward<LastArg>(last_arg)
  ) {}

  template <typename DoFunctor=meta::nonesuch, typename... DoArgs>
  auto
  do_(DoArgs&& ... args)&&
  {
    return _create_work_while_do_helper<
      _create_work_while_helper, DoFunctor,
      typename tinympl::vector<DoArgs...>::pop_back::type,
      typename tinympl::vector<DoArgs...>::back::type
    >(
      std::move(*this),
      std::forward<DoArgs>(args)...
    );
  }

};

// </editor-fold> end create_work_while_helper, functor version }}}2
//------------------------------------------------------------------------------

// </editor-fold> end Task creation helpers in the form of proxy return objects }}}1
//==============================================================================

} // end namespace detail


namespace experimental {

template <typename Functor=meta::nonesuch, typename... Args>
auto
create_work_while(Args&& ... args)
{
  return detail::_create_work_while_helper<
    Functor,
    typename tinympl::vector<Args...>::safe_pop_back::type,
    typename tinympl::vector<Args...>::template safe_back<meta::nonesuch>::type
  >(
    std::forward<Args>(args)...
  );
}

} // end namespace experimental


} // end namespace darma_runtime

#endif //DARMA_IMPL_CREATE_WORK_WHILE_H
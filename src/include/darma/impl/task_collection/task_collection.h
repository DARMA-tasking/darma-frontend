/*
//@HEADER
// ************************************************************************
//
//                      task_collection.h
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

#ifndef DARMA_IMPL_TASK_COLLECTION_TASK_COLLECTION_H
#define DARMA_IMPL_TASK_COLLECTION_TASK_COLLECTION_H

#include <darma/impl/feature_testing_macros.h>

#if _darma_has_feature(create_concurrent_work)
#include <darma/serialization/polymorphic/polymorphic_serialization_adapter.h>
#include <darma/impl/handle.h>
#include <darma/impl/task_collection/access_handle_collection.h>
#include <darma/interface/frontend/task_collection.h>
#include <darma/impl/capture.h>
#include <darma/impl/task/task.h>
#include <darma/impl/index_range/mapping_traits.h>
#include <darma/impl/index_range/index_range_traits.h>
#include "task_collection_fwd.h"

#include "task_collection_task.h"

#include "impl/argument_to_tc_storage.h"
#include "impl/tc_storage_to_task_storage.h"

namespace darma {

namespace detail {

//==============================================================================
// <editor-fold desc="TaskCollectionImpl">

template <
  typename Functor,
  typename IndexRangeT,
  typename... Args
>
struct TaskCollectionImpl
#if _darma_has_feature(task_migration)
  : serialization::PolymorphicSerializationAdapter<
      TaskCollectionImpl<Functor, IndexRangeT, Args...>,
      abstract::frontend::TaskCollection
    >
#else
  : abstract::frontend::TaskCollection
#endif //_darma_has_feature(task_migration)
{
  public:

    using index_range_t = IndexRangeT;
    using index_range_traits = indexing::index_range_traits<index_range_t>;

  protected:

    template <typename ArgsForwardedTuple, size_t... Spots>
    decltype(auto)
    _get_args_stored_impl(
      ArgsForwardedTuple&& args_fwd,
      std::index_sequence<Spots...>
    ) {
      return std::make_tuple(
        _task_collection_impl::_get_storage_arg_helper<
          decltype(std::get<Spots>(std::forward<ArgsForwardedTuple>(args_fwd))),
          // offset by 1 to incorporate the index parameter
          typename meta::callable_traits<Functor>::template param_n_traits<Spots+1>,
          IndexRangeT
        >{}(
          *this, std::get<Spots>(std::forward<ArgsForwardedTuple>(args_fwd))
        )...
      );
    }

    template <size_t... Spots>
    auto _make_task_impl(
      std::size_t index,
      std::index_sequence<Spots...> seq
    ) {
      return std::make_unique<
        TaskCollectionTaskImpl<
          Functor, typename index_range_traits::mapping_to_dense_type,
          typename _task_collection_impl::_get_task_stored_arg_helper<
            Functor, Args, Spots
          >::type...
        >
      >(
        index, index_range_traits::mapping_to_dense(collection_range_),
        *this, seq, args_stored_
      );
    }

    using args_tuple_t = std::tuple<Args...>;

    TaskCollectionImpl() = default;

  public:

    types::key_t name_ = detail::key_traits<types::key_t>::make_awaiting_backend_assignment_key();

    // Leave this member declaration order the same; construction of args_stored_
    // depends on collection_range_ being initialized already

    using dependencies_container_t = types::handle_container_template<
      abstract::frontend::DependencyUse*
    >;

    #if _darma_has_feature(task_collection_token)
    types::task_collection_token_t token_ = { };
    #endif // _darma_has_feature(task_collection_token)

    dependencies_container_t dependencies_;
    IndexRangeT collection_range_;
    args_tuple_t args_stored_;

    template <typename Archive>
    void compute_size(Archive& ar) const {
      ar | collection_range_;
      ar | args_stored_;
      ar | name_;
      // nothing to pack for dependencies.  They'll be handled later
    }

    template <typename Archive>
    void pack(Archive& ar) const {
      ar | collection_range_;
      ar | args_stored_;
      ar | name_;
      // nothing to pack for dependencies.  They'll be handled later
    }

    template <typename Archive>
    static void unpack(void* allocated, Archive& ar) {
      // Need to call ctor to initialize vtable
      auto* rv_ptr = new (allocated) TaskCollectionImpl();

      // TODO deal with range default constructibility?

      ar >> rv_ptr->collection_range_;

      // Some arguments might not be default constructible either...
      ar >> rv_ptr->args_stored_;

      // for dependencies_, just reconstruct directly; it was never packed
      new (&rv_ptr->dependencies_) dependencies_container_t();

      // Unpacking.
      // collection_range_ already unpacked in reconstruct
      // args_stored_ already unpacked in reconstruct
      ar >> rv_ptr->name_;

      // need to set up dependencies here...
      rv_ptr->_unpack_deps(std::index_sequence_for<Args...>{});
    }

  private:

    struct _do_unpack_deps {
      template <typename TaskCollectionT,
        typename MappedHandleCollectionT
      >
      std::enable_if_t<
        tinympl::is_instantiation_of<
          MappedHandleCollection,
          std::decay_t<MappedHandleCollectionT>
        >::value,
        int
      >
      operator()(TaskCollectionT& tc, MappedHandleCollectionT&& mcoll) const {
        tc.add_dependency(mcoll.collection.current_use_base_->use_base);
        return 0;
      }

      template <typename TaskCollectionT,
        typename AccessHandleT
      >
      std::enable_if_t<
        decayed_is_access_handle<AccessHandleT>::value,
        int
      >
      operator()(TaskCollectionT& tc, AccessHandleT&& ah) const {
        tc.add_dependency(ah.current_use_base_->use_base);
        return 0;
      }

      template <typename TaskCollectionT, typename T>
      std::enable_if_t<
        not tinympl::is_instantiation_of<
          MappedHandleCollection,
          std::decay_t<T>
        >::value
        and not decayed_is_access_handle<T>::value,
        int
      >
      operator()(TaskCollectionT& tc, T&& ah) const {
        // Nothing to do...
        return 0;
      }
    };

    template <size_t... Spots>
    void _unpack_deps(
      std::index_sequence<Spots...>
    ) {
      std::make_tuple(
        _do_unpack_deps()(
          *this,
          std::get<Spots>(args_stored_)
        )...
      ); // return value ignored
    }

  public:


    void add_dependency(abstract::frontend::DependencyUse* dep) {
      dependencies_.insert(dep);
    }

    ~TaskCollectionImpl() { }

    //==============================================================================
    // <editor-fold desc="Ctors"> {{{1

    template <typename IndexRangeDeduced, typename... ArgsForwarded>
    TaskCollectionImpl(
      IndexRangeDeduced&& collection_range,
      ArgsForwarded&& ... args_forwarded
    ) : collection_range_(std::forward<IndexRangeDeduced>(collection_range)),
#if _darma_has_feature(task_collection_token)
        token_(),
#endif // _darma_has_feature(task_collection_token)
        args_stored_(
          _get_args_stored_impl(
            std::forward_as_tuple(std::forward<ArgsForwarded>(args_forwarded)...),
            std::index_sequence_for<ArgsForwarded...>{}
          )
        )
    { }

    // </editor-fold> end Ctors }}}1
    //==============================================================================

    //==========================================================================
    // <editor-fold desc="TaskCollection concrete implementation"> {{{1

    size_t size() const override { return collection_range_.size(); }

    std::unique_ptr<types::task_collection_task_t>
    create_task_for_index(std::size_t index) override {
      return _make_task_impl(
        index, std::make_index_sequence<sizeof...(Args)>{}
      );
    }

    // This should really return something ternary like "known false, known true, or unknown"
    // TODO deprecated
    OptionalBoolean
    all_mappings_same_as(
      abstract::frontend::TaskCollection const* other
    ) const override {
      /* TODO */
      return OptionalBoolean::Unknown;
    }

    types::handle_container_template<abstract::frontend::DependencyUse*> const&
    get_dependencies() const override {
      return dependencies_;
    }

    types::key_t const&
    get_name() const override { return name_; }

    void
    set_name(types::key_t const& name) override { name_ = name; }

#if _darma_has_feature(task_collection_token)
    types::task_collection_token_t const&
    get_task_collection_token() const override {
      return token_;
    }

    void
    set_task_collection_token(types::task_collection_token_t const& token) override {
      token_ = token;
    }
#endif // _darma_has_feature(task_collection_token)

    // </editor-fold> end TaskCollection concrete implementation }}}1
    //==========================================================================

    template <typename, typename, typename, typename>
    friend struct _task_collection_impl::_get_storage_arg_helper;

    template <typename, typename, size_t, typename>
    friend struct _task_collection_impl::_get_task_stored_arg_helper;

};

template <typename Functor, typename IndexRangeT, typename... Args>
struct make_task_collection_impl_t {
  private:

    using arg_vector_t = tinympl::vector<Args...>;

    template <typename T> struct _helper;

    template <size_t... Idxs>
    struct _helper<std::index_sequence<Idxs...>> {
      using type = TaskCollectionImpl<
        Functor, IndexRangeT,
        typename detail::_task_collection_impl::_get_storage_arg_helper<
          tinympl::at_t<Idxs, arg_vector_t>,
          // offset by 1 to incorporate the index parameter
          typename meta::callable_traits<Functor>::template param_n_traits<Idxs+1>,
          IndexRangeT
        >::type...
      >;
    };

  public:

    using type = typename _helper<std::index_sequence_for<Args...>>::type;

};

// </editor-fold> end TaskCollectionImpl
//==============================================================================

} // end namespace detail

} // end namespace darma

#endif // _darma_has_feature(create_concurrent_work)
#endif //DARMA_IMPL_TASK_COLLECTION_TASK_COLLECTION_H

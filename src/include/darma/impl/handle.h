/*
//@HEADER
// ************************************************************************
//
//                          dependency.h
//                         darma_mockup
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

#ifndef DARMA_IMPL_HANDLE_H
#define DARMA_IMPL_HANDLE_H

#include <atomic>
#include <cassert>
#include <tuple>

#include <tinympl/variadic/find_if.hpp>
#include <tinympl/bind.hpp>
#include <tinympl/identity.hpp>
#include <tinympl/lambda.hpp>
#include <tinympl/filter.hpp>
#include <tinympl/logical_not.hpp>
#include <tinympl/copy_traits.hpp>

#include <darma_types.h>

#include <darma/interface/frontend/handle.h>
#include <darma/interface/backend/types.h>

#include <darma/impl/task_fwd.h>
#include <darma/impl/util.h>
#include <darma/impl/darma_assert.h>
#include <darma/impl/serialization/archive.h>
#include <darma/impl/serialization/policy_aware_archive.h>
#include <darma/impl/serialization/nonintrusive.h>
#include <darma/impl/serialization/traits.h>
#include <darma/impl/serialization/allocation.h>
#include <darma/impl/serialization/manager.h>

#include <darma/impl/keyword_arguments/keyword_arguments.h>
#include <darma/interface/app/keyword_arguments/n_readers.h>
#include <darma/interface/app/keyword_arguments/version.h>
#include <darma/impl/array/indexable.h>
#include <darma/impl/array/concept.h>



namespace darma_runtime {

////////////////////////////////////////////////////////////////////////////////
// <editor-fold desc="KeyedObject">

namespace detail {

template <typename key_type>
class KeyedObject
{
  public:

    typedef key_type key_t;

    KeyedObject() : key_() { }

    KeyedObject(const key_type& key) : key_(key) { }

    const key_t&
    get_key() const {
      return key_;
    }

    void
    set_key(const key_t& key) {
      key_ = key;
    }

  protected:
    key_t key_;
};

} // end namespace detail

// </editor-fold>
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// <editor-fold desc="DependencyHandleBase">

namespace detail {

// Tag type for handle migration unpack
struct handle_migration_unpack_t { };
static constexpr handle_migration_unpack_t handle_migration_unpack = {};

class VariableHandleBase
  : public KeyedObject<types::key_t>,
    public abstract::frontend::Handle {
  public:
    typedef types::key_t key_t;
    typedef KeyedObject<key_t> keyed_base_t;

    const key_t &
    get_key() const override {
      return this->KeyedObject<key_t>::get_key();
    }

    bool
    has_user_defined_key() const override {
      return not key_traits<types::key_t>::needs_backend_key(key_);
    }

    void
    set_key(key_t const& generated_key) override {
      return this->KeyedObject<key_t>::set_key(generated_key);
    }

    explicit VariableHandleBase(
      const key_t &key
    ) : keyed_base_t(key) { }

    VariableHandleBase() : keyed_base_t(key_t()) { }

    virtual ~VariableHandleBase() noexcept { }

};

} // end namespace detail

// </editor-fold>
//==============================================================================

//==============================================================================
// <editor-fold desc="VariableHandle">

namespace detail {

template <typename T>
class VariableHandle
  : public VariableHandleBase,
    public serialization::detail::SerializationManagerForType<T>,
    public ArrayConceptManagerForType<T, SimpleElementRange<T>>,
    public abstract::frontend::ArrayMovementManager

{
  protected:

    typedef VariableHandleBase base_t;

    using _ser_man_impl_t = serialization::detail::SerializationManagerForType<T>;

  public:

    typedef typename base_t::key_t key_t;

    ////////////////////////////////////////////////////////////
    // Constructors and Destructor <editor-fold desc="Constructors and Destructor">

    VariableHandle() : base_t() { }

    VariableHandle(
      const key_t &data_key
    ) : base_t(data_key) { }

    virtual ~VariableHandle() noexcept { }

    // end Constructors and Destructor </editor-fold>
    ////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////
    // <editor-fold desc="abstract::frontend::Handle implmentation">

    abstract::frontend::SerializationManager const*
    get_serialization_manager() const override {
      return this;
    }

    abstract::frontend::ArrayMovementManager const*
    get_array_movement_manager() const override {
      return this;
    }

    abstract::frontend::ArrayConceptManager const*
    get_array_concept_manager() const override {
      return this;
    }

    // </editor-fold>
    ////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////
    // ArrayMovementManager implementation <editor-fold desc="ArrayMovementManager implementation">

    size_t
    get_packed_size(
      void const* obj,
      size_t offset, size_t n_elem,
      abstract::backend::SerializationPolicy* ser_pol
    ) const override {
      using serialization::detail::DependencyHandle_attorneys::ArchiveAccess;
      auto ar = _ser_man_impl_t::_get_best_compatible_archive(
        tinympl::identity<typename _ser_man_impl_t::best_compatible_archive_t>(),
        ser_pol
      );
      ArchiveAccess::start_sizing(ar);
      IndexingTraits<T>::get_packed_size(
        *static_cast<T const*>(obj), ar, offset, n_elem
      );
      return ArchiveAccess::get_size(ar);
    }

    void
    pack_elements(
      void const* obj, void* buffer,
      size_t offset, size_t n_elem,
      abstract::backend::SerializationPolicy* ser_pol
    ) const override
    {
      using serialization::detail::DependencyHandle_attorneys::ArchiveAccess;
      auto ar = _ser_man_impl_t::_get_best_compatible_archive(
        tinympl::identity<typename _ser_man_impl_t::best_compatible_archive_t>(),
        ser_pol
      );
      ArchiveAccess::start_packing_with_buffer(
        ar, buffer
      );
      IndexingTraits<T>::pack_elements(
        *static_cast<T const*>(obj), ar,
        offset, n_elem
      );
    }

    void
    unpack_elements(
      void* obj, void const* buffer,
      size_t offset, size_t n_elem,
      abstract::backend::SerializationPolicy* ser_pol
    ) const override
    {
      using serialization::detail::DependencyHandle_attorneys::ArchiveAccess;
      auto ar = _ser_man_impl_t::_get_best_compatible_archive(
        tinympl::identity<typename _ser_man_impl_t::best_compatible_archive_t>(),
        ser_pol
      );
      ArchiveAccess::start_unpacking_with_buffer(
        ar, buffer
      );
      IndexingTraits<T>::unpack_elements(
        *static_cast<T*>(obj), ar,
        offset, n_elem
      );
    }

    // end ArrayMovementManager implementation </editor-fold>
    ////////////////////////////////////////////////////////////

};

} // end namespace detail

// </editor-fold>
//==============================================================================

//==============================================================================
// <editor-fold desc="assorted attorneys">

namespace detail {

namespace create_work_attorneys {

class for_AccessHandle;

} // end namespace create_work_attorneys

namespace access_attorneys {

class for_AccessHandle;

} // end namespace access_attorneys

namespace analogous_access_handle_attorneys {

struct AccessHandleAccess {
  template <typename AccessHandleT>
  static auto&
  captured_as(AccessHandleT& ah) {
    return ah.captured_as_;
  }
  template <typename AccessHandleT>
  static auto&
  dep_handle(AccessHandleT& ah) {
    return ah.dep_handle_;
  }
  template <typename AccessHandleT>
  static auto&
  read_only_holder(AccessHandleT& ah) {
    return ah.read_only_holder_;
  }
  template <typename AccessHandleT>
  static auto&
  state(AccessHandleT& ah) {
    return ah.state_;
  }
};

} // end namespace analogous_access_handle_attorneys

} // end namespace detail

// </editor-fold>
//==============================================================================

//==============================================================================
// <editor-fold desc="AccessHandleBase">

namespace detail {

class AccessHandleBase {
  public:

    typedef enum CaptureOp {
      ro_capture,
      mod_capture
    } capture_op_t;

    // TODO figure out if this as efficient as a bitfield (it's definitely more readable)
    typedef enum CapturedAsInfo {
      Normal = 0,
      Ignored = 1,
      ReadOnly = 2,
      // Future use:
      ScheduleOnly = 4,
      Leaf = 8
    } captured_as_info_t;

  protected:
    using task_t = detail::TaskBase;

    mutable unsigned captured_as_ = CapturedAsInfo::Normal;
    task_t* capturing_task = nullptr;

};

} // end namespace detail

// </editor-fold>
//==============================================================================

//==============================================================================
// <editor-fold desc="access_handle_traits and helpers">

namespace detail {

typedef enum AccessHandlePermissions {
  NotGiven=-1,
  None=0, Read=1, Modify=2
} access_handle_permissions_t;

// (Not really true, needs more explanation): Min permissions refers to as a parameter, max permissions refers to as a call argument or lvalue
// (or as a parameter for determining whether a capture is read-only).  All are only the known compile-time
// bounds; if no restrictions are given at compile time, all will be AccessHandlePermissions::NotGiven
// TODO more full documentation
template <
  access_handle_permissions_t MinSchedulePermissions = NotGiven,
  access_handle_permissions_t MinImmediatePermissions = NotGiven,
  access_handle_permissions_t MaxSchedulePermissions = NotGiven,
  access_handle_permissions_t MaxImmediatePermissions = NotGiven
>
struct access_handle_traits {

  static constexpr auto min_schedule_permissions = MinSchedulePermissions;
  static constexpr auto min_schedule_permissions_given = MinSchedulePermissions != NotGiven;
  static constexpr auto min_immediate_permissions = MinImmediatePermissions;
  static constexpr auto min_immediate_permissions_given = MinImmediatePermissions != NotGiven;
  static constexpr auto max_schedule_permissions = MaxSchedulePermissions;
  static constexpr auto max_schedule_permissions_given = MaxSchedulePermissions != NotGiven;
  static constexpr auto max_immediate_permissions = MaxImmediatePermissions;
  static constexpr auto max_immediate_permissions_given = MaxImmediatePermissions != NotGiven;

  template <access_handle_permissions_t new_min_schedule_permissions>
  struct with_min_schedule_permissions {
    typedef access_handle_traits<
      new_min_schedule_permissions,
      MinImmediatePermissions,
      MaxSchedulePermissions,
      MaxImmediatePermissions
    > type;
  };

  template <access_handle_permissions_t new_max_schedule_permissions>
  struct with_max_schedule_permissions {
    typedef access_handle_traits<
      MinSchedulePermissions,
      MinImmediatePermissions,
      new_max_schedule_permissions,
      MaxImmediatePermissions
    > type;
  };

  template <access_handle_permissions_t new_min_immediate_permissions>
  struct with_min_immediate_permissions {
    typedef access_handle_traits<
      MinSchedulePermissions,
      new_min_immediate_permissions,
      MaxSchedulePermissions,
      MaxImmediatePermissions
    > type;
  };

  template <access_handle_permissions_t new_max_immediate_permissions>
  struct with_max_immediate_permissions {
    typedef access_handle_traits<
      MinSchedulePermissions,
      MinImmediatePermissions,
      MaxSchedulePermissions,
      new_max_immediate_permissions
    > type;
  };

};


//------------------------------------------------------------
// <editor-fold desc="make_access_traits and associated 'keyword' template arguments">

template <access_handle_permissions_t permissions>
struct min_schedule_permissions {
  static constexpr auto value = permissions;
};

template <access_handle_permissions_t permissions>
struct max_schedule_permissions {
  static constexpr auto value = permissions;
};

template <access_handle_permissions_t permissions>
struct min_immediate_permissions {
  static constexpr auto value = permissions;
};

template <access_handle_permissions_t permissions>
struct max_immediate_permissions {
  static constexpr auto value = permissions;
};

namespace _impl {

template <typename traits, typename... modifiers>
struct _make_access_handle_traits;

template <typename traits, access_handle_permissions_t permissions, typename... modifiers>
struct _make_access_handle_traits<traits, min_schedule_permissions<permissions>, modifiers...> {
  using type = typename _make_access_handle_traits<
    typename traits::template with_min_schedule_permissions<permissions>::type,
    modifiers...
  >::type;
};

template <typename traits, access_handle_permissions_t permissions, typename... modifiers>
struct _make_access_handle_traits<traits, max_schedule_permissions<permissions>, modifiers...> {
  using type = typename _make_access_handle_traits<
    typename traits::template with_max_schedule_permissions<permissions>::type,
    modifiers...
  >::type;
};

template <typename traits, access_handle_permissions_t permissions, typename... modifiers>
struct _make_access_handle_traits<traits, min_immediate_permissions<permissions>, modifiers...> {
  using type = typename _make_access_handle_traits<
    typename traits::template with_min_immediate_permissions<permissions>::type,
    modifiers...
  >::type;
};

template <typename traits, access_handle_permissions_t permissions, typename... modifiers>
struct _make_access_handle_traits<traits, max_immediate_permissions<permissions>, modifiers...> {
  using type = typename _make_access_handle_traits<
    typename traits::template with_max_immediate_permissions<permissions>::type,
    modifiers...
  >::type;
};

template <typename traits>
struct _make_access_handle_traits<traits> {
  using type = traits;
};

} // end namespace _impl

template <typename... modifiers>
struct make_access_handle_traits {
  using type = typename
    _impl::_make_access_handle_traits<access_handle_traits<>, modifiers...>::type;
};

template <typename... modifiers>
using make_access_handle_traits_t = typename make_access_handle_traits<modifiers...>::type;

// </editor-fold>
//------------------------------------------------------------

} // end namespace detail

// </editor-fold>
//==============================================================================

// Forward declaration of AccessHandle
template <
  typename T = void,
  typename traits = detail::access_handle_traits<>
>
class AccessHandle;

//==============================================================================
// <editor-fold desc="is_access_handle">

namespace detail {

template <typename T, typename Enable=void>
struct is_access_handle
  : std::false_type { };

// TODO decide if this is a good or bad idea
template <typename T>
struct is_access_handle<T,
  std::enable_if_t<not std::is_same<
    T, std::remove_cv_t<std::remove_reference_t<T>>
  >::value>
> : is_access_handle<std::remove_cv_t<std::remove_reference_t<T>>> { };

template <typename... Args>
struct is_access_handle<AccessHandle<Args...>, void>
  : std::true_type { };

namespace _impl {

template <typename T>
using _value_type_archetype = typename T::value_type;

} // end namespace _impl

template <typename T, typename Otherwise=meta::nonesuch>
using value_type_if_access_handle = std::conditional<
  is_access_handle<T>::value,
  meta::detected_t<_impl::_value_type_archetype, std::decay_t<T>>,
  Otherwise
>;

template <typename T, typename Otherwise=meta::nonesuch>
using value_type_if_access_handle_t = typename value_type_if_access_handle<T, Otherwise>::type;

} // end namespace detail

// </editor-fold>
//==============================================================================


} // end namespace darma_runtime


#endif /* DARMA_IMPL_HANDLE_H_ */

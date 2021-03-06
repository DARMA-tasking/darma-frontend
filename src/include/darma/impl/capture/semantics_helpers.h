/*
//@HEADER
// ************************************************************************
//
//                      semantics_helpers.h
//                         DARMA
//              Copyright (C) 2017 Sandia Corporation
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

#ifndef DARMAFRONTEND_SEMANTICS_HELPERS_H
#define DARMAFRONTEND_SEMANTICS_HELPERS_H

#include <tinympl/find.hpp>

#include <darma_types.h>

#include <darma/impl/handle_use_base.h>
#include <darma/utility/macros.h>

#include <darma/impl/capture/semantics_macros.h>


namespace darma {
namespace detail {
namespace capture_semantics {

struct CaptureCaseInput {
  frontend::permissions_t source_scheduling;
  frontend::permissions_t source_immediate;
  frontend::permissions_t captured_scheduling;
  frontend::permissions_t captured_immediate;
  frontend::coherence_mode_t coherence_mode;
};

struct CaptureCaseInputHash {
  std::size_t operator()(CaptureCaseInput const& c) const {
    using underlying_permissions_t = std::underlying_type_t<frontend::permissions_t>;
    using underlying_coherence_mode_t = std::underlying_type_t<frontend::coherence_mode_t>;
    auto rv = std::hash<underlying_permissions_t>()((underlying_permissions_t)c.source_scheduling);
    darma::detail::hash_combine(rv, (underlying_permissions_t)c.source_immediate);
    darma::detail::hash_combine(rv, (underlying_permissions_t)c.captured_scheduling);
    darma::detail::hash_combine(rv, (underlying_permissions_t)c.captured_immediate);
    darma::detail::hash_combine(rv, (underlying_coherence_mode_t)c.coherence_mode);
    return rv;
  }
};

struct CaptureCaseInputEqual {
  bool operator()(CaptureCaseInput const& a, CaptureCaseInput const& b) const {
    return a.source_immediate == b.source_immediate
      and a.captured_immediate == b.captured_immediate
      and a.source_scheduling == b.source_scheduling
      and a.captured_scheduling == b.captured_scheduling
      and a.coherence_mode == b.coherence_mode;
  }
};

struct CaptureCaseOutput {
  typedef FlowRelationshipImpl (*captured_relationship_generator)(types::flow_t*, types::flow_t*);
  typedef FlowRelationshipImpl (*continuation_relationship_generator)(
    types::flow_t*, types::flow_t*, types::flow_t*, types::flow_t*
  );
  typedef FlowRelationshipImpl (*captured_anti_relationship_generator)(
    types::flow_t*, types::flow_t*, types::anti_flow_t*, types::anti_flow_t*
  );
  typedef FlowRelationshipImpl (*continuation_anti_relationship_generator)(
    types::flow_t*, types::flow_t*, types::flow_t*, types::flow_t*,
    types::anti_flow_t*, types::anti_flow_t*, types::anti_flow_t*, types::anti_flow_t*
  );

  frontend::permissions_t continuation_scheduling;
  frontend::permissions_t continuation_immediate;
  captured_relationship_generator captured_in_flow_relationship;
  captured_relationship_generator captured_out_flow_relationship;
  captured_anti_relationship_generator captured_anti_in_flow_relationship;
  captured_anti_relationship_generator captured_anti_out_flow_relationship;
  continuation_relationship_generator continuation_in_flow_relationship;
  continuation_relationship_generator continuation_out_flow_relationship;
  continuation_anti_relationship_generator continuation_anti_in_flow_relationship;
  continuation_anti_relationship_generator continuation_anti_out_flow_relationship;
  bool needs_new_continuation_use;
  bool could_be_alias;
};

using capture_case_table_t = std::unordered_map<
  CaptureCaseInput,
  CaptureCaseOutput,
  CaptureCaseInputHash,
  CaptureCaseInputEqual
>;

template <typename _force_same_across_compilation_units=void>
capture_case_table_t&
get_capture_case_table() {
  static capture_case_table_t static_table = { };
  return static_table;
}

template <typename CaptureCaseT>
struct CaptureCaseRegistrar {
  size_t index;
  CaptureCaseRegistrar() {
    constexpr auto input = CaptureCaseInput{
      CaptureCaseT::source_scheduling(),
      CaptureCaseT::source_immediate(),
      CaptureCaseT::captured_scheduling(),
      CaptureCaseT::captured_immediate(),
      CaptureCaseT::coherence_mode()
    };
    auto& table = capture_semantics::get_capture_case_table();
    auto found = table.find(input);
    assert(found == table.end() || !"Duplicate capture case");
    index = table.size();
    table[input] = CaptureCaseOutput{
      CaptureCaseT::continuation_scheduling(),
      CaptureCaseT::continuation_immediate(),
      &CaptureCaseT::captured_in_flow_relationship,
      &CaptureCaseT::captured_out_flow_relationship,
      &CaptureCaseT::captured_anti_in_flow_relationship,
      &CaptureCaseT::captured_anti_out_flow_relationship,
      &CaptureCaseT::continuation_in_flow_relationship,
      &CaptureCaseT::continuation_out_flow_relationship,
      &CaptureCaseT::continuation_anti_in_flow_relationship,
      &CaptureCaseT::continuation_anti_out_flow_relationship,
      CaptureCaseT::needs_new_continuation_use(),
      CaptureCaseT::could_be_alias()
    };
  }
};

template <typename CaptureCaseT>
struct CaptureCaseRegistrarWrapper {
  static CaptureCaseRegistrar<CaptureCaseT> registrar;
};

template <typename CaptureCaseT>
CaptureCaseRegistrar<CaptureCaseT> CaptureCaseRegistrarWrapper<CaptureCaseT>::registrar = { };

template <typename CaptureCaseT>
size_t
register_capture_case() {
  return CaptureCaseRegistrarWrapper<CaptureCaseT>::registrar.index;
}


inline void _capture_case_not_implemented(
  CaptureCaseInput const& case_in
) {
  DARMA_ASSERT_NOT_IMPLEMENTED(
    "Capture case with source permissions { "
      << permissions_to_string(case_in.source_scheduling)
      << ", "
      << permissions_to_string(case_in.source_immediate)
      << " }, captured permissions { "
      << permissions_to_string(case_in.captured_scheduling)
      << ", "
      << permissions_to_string(case_in.captured_immediate)
      << " }, and coherence mode "
      << coherence_mode_to_string(case_in.coherence_mode)
      << ".  Number of known cases is " << get_capture_case_table().size()
      << "."
  );
}

/*******************************************************************************
 * @internal
 * @brief Basic class template that gets specialized for known Use-Flow capture
 * cases.
 *
 * Specializations of this template (e.g., via the `_darma_CAPTURE_CASE()`
 * macro) contain all of the information about captured and continuing
 * permissions and flows for a given input of source and captured (i.e.,
 * requested) Permissions and CoherenceMode.
 *
 * @note The parameters to methods in this class have the same names as the ones
 * in the `_darma_CAPTURE_CASE()` macro, so they are the names that should be
 * used as argument to the `FlowRelationship` creation functions (in namespace
 * `darma::detail::flow_relationships`, which is `using`'d in the bodies
 * of all the macro functions for brevity).
 *
 * @sa semantics.h
 *
 * @tparam SourceSchedulingIn Source scheduling permissions, accessible via the
 *   `source_scheduling` static member
 * @tparam SourceImmediateIn Source immediate permissions, accessible via the
 *   `source_immediate` static member
 * @tparam CapturedSchedulingIn Captured (i.e., "requested") scheduling
 *   permissions, accessible via the `captured_scheduling` static member
 * @tparam CapturedImmediateIn Captured immediate permissions, accessible via
 *   the `captured_scheduling` static member
 * @tparam CoherenceModeIn `frontend::CoherenceMode` that the capture case is
 *   handling, accessible via the `coherence_mode` static member
 * @tparam Enable (for SFINAE use only)
 ******************************************************************************/
template <
  frontend::permissions_t SourceSchedulingIn, frontend::permissions_t SourceImmediateIn,
  /* -> { */
  frontend::permissions_t CapturedSchedulingIn, frontend::permissions_t CapturedImmediateIn,
  /* } */
  frontend::coherence_mode_t CoherenceModeIn,
  typename Enable=void
>
struct CaptureCase {
  public:

    // These are static methods rather than data members because their return
    // values might be bound to (rvalue) references in perfect forwarding
    // contexts, and if they were data members I would have to instantiate them,
    // which is a pain.

//    static inline constexpr auto source_scheduling() { return SourceSchedulingIn; }
//    static inline constexpr auto source_immediate() { return SourceImmediateIn; }
//    static inline constexpr auto captured_scheduling() { return CapturedSchedulingIn; }
//    static inline constexpr auto captured_immediate() { return CapturedImmediateIn; }
//    static inline constexpr auto continuation_scheduling() { return frontend::Permissions::_invalid; }
//    static inline constexpr auto continuation_immediate() { return frontend::Permissions::_invalid; }
//    static inline constexpr auto coherence_mode() { return CoherenceModeIn; }
//    static inline constexpr auto could_be_alias() { return false; }
//    static inline constexpr auto needs_new_continuation_use() { return false; }
//    static inline constexpr auto is_valid_capture_case() { return false; }
//
//  private:
//    static auto _case_not_implemented() {
//      capture_semantics::_capture_case_not_implemented(
//        CaptureCaseInput{
//          SourceSchedulingIn, SourceImmediateIn,
//          CapturedSchedulingIn, CapturedImmediateIn,
//          CoherenceModeIn
//        }
//      );
//      return FlowRelationshipImpl(); // unreachable, but it makes auto work
//    }
//
//  public:
//    /***************************************************************************
//     *
//     * @param source_in
//     * @param source_out
//     * @return
//     **************************************************************************/
//    static auto captured_in_flow_relationship(
//      types::flow_t* source_in,
//      types::flow_t* source_out
//    ) { return _case_not_implemented(); }
//
//    /***************************************************************************
//     *
//     * @param source_in
//     * @param source_out
//     * @return
//     **************************************************************************/
//    static auto captured_out_flow_relationship(
//      types::flow_t* source_in,
//      types::flow_t* source_out
//    ) { return _case_not_implemented(); }
//
//    /***************************************************************************
//     *
//     * @param source_in
//     * @param source_out
//     * @param source_anti_in
//     * @param source_anti_out
//     * @return
//     **************************************************************************/
//    static auto captured_anti_in_flow_relationship(
//      types::flow_t* source_in,
//      types::flow_t* source_out,
//      types::anti_flow_t* source_anti_in,
//      types::anti_flow_t* source_anti_out
//    ) { return _case_not_implemented(); }
//
//    /***************************************************************************
//     *
//     * @param source_in
//     * @param source_out
//     * @param source_anti_in
//     * @param source_anti_out
//     * @return
//     **************************************************************************/
//    static auto captured_anti_out_flow_relationship(
//      types::flow_t* source_in,
//      types::flow_t* source_out,
//      types::anti_flow_t* source_anti_in,
//      types::anti_flow_t* source_anti_out
//    ) { return _case_not_implemented(); }
//
//    /***************************************************************************
//     *
//     * @param source_in
//     * @param source_out
//     * @param captured_in
//     * @param captured_out
//     * @return
//     **************************************************************************/
//    static auto continuation_in_flow_relationship(
//      types::flow_t* source_in,
//      types::flow_t* source_out,
//      types::flow_t* captured_in,
//      types::flow_t* captured_out
//    ) { return _case_not_implemented(); }
//
//    /***************************************************************************
//     *
//     * @param source_in
//     * @param source_out
//     * @param captured_in
//     * @param captured_out
//     * @return
//     **************************************************************************/
//    static auto continuation_out_flow_relationship(
//      types::flow_t* source_in,
//      types::flow_t* source_out,
//      types::flow_t* captured_in,
//      types::flow_t* captured_out
//    ) { return _case_not_implemented(); }
//
//    /***************************************************************************
//     *
//     * @param source_in
//     * @param source_out
//     * @param captured_in
//     * @param captured_out
//     * @param source_anti_in
//     * @param source_anti_out
//     * @param captured_anti_in
//     * @param captured_anti_out
//     * @return
//     **************************************************************************/
//    static auto continuation_anti_in_flow_relationship(
//      types::flow_t* source_in,
//      types::flow_t* source_out,
//      types::flow_t* captured_in,
//      types::flow_t* captured_out,
//      types::anti_flow_t* source_anti_in,
//      types::anti_flow_t* source_anti_out,
//      types::anti_flow_t* captured_anti_in,
//      types::anti_flow_t* captured_anti_out
//    ) { return _case_not_implemented(); }
//
//    /***************************************************************************
//     *
//     * @param source_in
//     * @param source_out
//     * @param captured_in
//     * @param captured_out
//     * @param source_anti_in
//     * @param source_anti_out
//     * @param captured_anti_in
//     * @param captured_anti_out
//     * @return
//     **************************************************************************/
//    static auto continuation_anti_out_flow_relationship(
//      types::flow_t* source_in,
//      types::flow_t* source_out,
//      types::flow_t* captured_in,
//      types::flow_t* captured_out,
//      types::anti_flow_t* source_anti_in,
//      types::anti_flow_t* source_anti_out,
//      types::anti_flow_t* captured_anti_in,
//      types::anti_flow_t* captured_anti_out
//    ) { return _case_not_implemented(); }
    static size_t _make_index_to_force_registration() {
      return 0; // Don't register an unimplemented capture case.
    }
};




//==============================================================================
// <editor-fold desc="Implementation details"> {{{1

/**
 * @internal
 * @addtogroup InternalDetail
 * @{
 */
namespace _impl {

//==============================================================================
// <editor-fold desc="Metafunctions for use with SFINAE in CaptureCase pattern matching"> {{{1

template <
  frontend::permissions_t... Permissions
>
struct _permissions_case_mfn {
  template <frontend::permissions_t Perm>
  struct apply : tinympl::bool_<
    tinympl::find<
      tinympl::vector<std::integral_constant<frontend::permissions_t, Permissions>...>,
      std::integral_constant<frontend::permissions_t, Perm>
    >::type::value != sizeof...(Permissions)
  > { };
};

template <
  frontend::coherence_mode_t... Modes
>
struct _coherence_mode_case_mfn {
  template <frontend::coherence_mode_t Mode>
  struct apply : tinympl::bool_<
    tinympl::find<
      tinympl::vector<std::integral_constant<frontend::coherence_mode_t, Modes>...>,
      std::integral_constant<frontend::coherence_mode_t, Mode>
    >::type::value != sizeof...(Modes)
  > { };
};

// </editor-fold> end Metafunctions for use with SFINAE in CaptureCase pattern matching }}}1
//==============================================================================

//==============================================================================
// <editor-fold desc="CaptureCase dynamic-to-static transformation via case statements"> {{{1

template <
  frontend::CoherenceMode coherence_mode,
  frontend::Permissions source_scheduling,
  frontend::Permissions source_immediate,
  frontend::Permissions captured_scheduling,
  typename GenericCallable
>
inline auto
_with_captured_immediate(
  GenericCallable&& callable,
  frontend::Permissions captured_immediate
) {
#define _darma_permissions_case(perm) \
  case frontend::Permissions::perm: \
    return std::forward<GenericCallable>(callable)( \
      CaptureCase<source_scheduling, source_immediate, captured_scheduling, \
        frontend::Permissions::perm, coherence_mode \
      >{ } \
    )

  switch(captured_immediate) {
    _darma_permissions_case(Modify);
    _darma_permissions_case(Read);
    _darma_permissions_case(None);
    _darma_permissions_case(Write);
    default: {
      DARMA_ASSERT_UNREACHABLE_FAILURE(
        "Captured scheduling permissions " << permissions_to_string(captured_scheduling)
      );
    }
  }
#undef _darma_permissions_case
}

template <
  frontend::CoherenceMode coherence_mode,
  frontend::Permissions source_scheduling,
  frontend::Permissions source_immediate,
  typename GenericCallable
>
inline auto
_with_captured_scheduling(
  GenericCallable&& callable,
  frontend::Permissions captured_scheduling,
  frontend::Permissions captured_immediate
) {
#define _darma_permissions_case(perm) \
  case frontend::Permissions::perm: \
    return _with_captured_immediate<coherence_mode, source_scheduling, source_immediate, \
      frontend::Permissions::perm \
    >( \
      std::forward<GenericCallable>(callable), \
      captured_immediate \
    )

  switch(captured_scheduling) {
    _darma_permissions_case(Modify);
    _darma_permissions_case(Read);
    _darma_permissions_case(None);
    _darma_permissions_case(Write);
    default: {
      DARMA_ASSERT_UNREACHABLE_FAILURE(
        "Captured scheduling permissions " << permissions_to_string(captured_scheduling)
      );
    }
  }
#undef _darma_permissions_case
}

template <
  frontend::CoherenceMode coherence_mode,
  frontend::Permissions source_scheduling,
  typename GenericCallable
>
inline auto
_with_source_immediate(
  GenericCallable&& callable,
  frontend::Permissions source_immediate,
  frontend::Permissions captured_scheduling,
  frontend::Permissions captured_immediate
) {
#define _darma_permissions_case(perm) \
  case frontend::Permissions::perm: \
    return _with_captured_scheduling<coherence_mode, source_scheduling, \
      frontend::Permissions::perm \
    >( \
      std::forward<GenericCallable>(callable), \
      captured_scheduling, captured_immediate \
    )

  switch(source_immediate) {
    _darma_permissions_case(Modify);
    _darma_permissions_case(Read);
    _darma_permissions_case(None);
    _darma_permissions_case(Write);
    default: {
      DARMA_ASSERT_UNREACHABLE_FAILURE(
        "Source immediate permissions " << permissions_to_string(source_immediate)
      );
    }
  }
#undef _darma_permissions_case
}

template <
  frontend::CoherenceMode coherence_mode,
  typename GenericCallable
>
auto _with_source_scheduling(
  GenericCallable&& callable,
  frontend::Permissions source_scheduling,
  frontend::Permissions source_immediate,
  frontend::Permissions captured_scheduling,
  frontend::Permissions captured_immediate
) {
#define _darma_permissions_case(perm) \
  case frontend::Permissions::perm: \
    return _with_source_immediate<coherence_mode, frontend::Permissions::perm>( \
      std::forward<GenericCallable>(callable), \
      source_immediate, captured_scheduling, captured_immediate \
    )

  switch(source_scheduling) {
    _darma_permissions_case(Modify);
    _darma_permissions_case(Read);
    _darma_permissions_case(None);
    _darma_permissions_case(Write);
    default: {
      DARMA_ASSERT_UNREACHABLE_FAILURE(
        "Source scheduling permissions " << permissions_to_string(source_scheduling)
      );
    }
  }
#undef _darma_permissions_case
}

template <
  typename GenericCallable
>
inline auto
_dynamic_to_static_capture_case_transformation(
  GenericCallable&& callable,
  frontend::CoherenceMode coherence_mode,
  frontend::Permissions source_scheduling,
  frontend::Permissions source_immediate,
  frontend::Permissions captured_scheduling,
  frontend::Permissions captured_immediate
) {
#define _darma_coherence_mode_case(mode) \
  case frontend::CoherenceMode::mode: \
    return _with_source_scheduling<frontend::CoherenceMode::mode>( \
      std::forward<GenericCallable>(callable), \
      source_scheduling, source_immediate, captured_scheduling, captured_immediate \
    )

  switch(coherence_mode) {
    _darma_coherence_mode_case(Sequential);
    _darma_coherence_mode_case(Commutative);
    _darma_coherence_mode_case(Reduce);
    _darma_coherence_mode_case(Relaxed);
    default: {
      DARMA_ASSERT_UNREACHABLE_FAILURE(
        "Coherence mode " << coherence_mode_to_string(coherence_mode)
      );
    }
  }
#undef _darma_coherence_mode_case
}

// </editor-fold> end CaptureCase dynamic-to-static transformation via case statements }}}1
//==============================================================================

} // end namespace _impl
/** @} */

// </editor-fold> end Implementation details }}}1
//==============================================================================

/*******************************************************************************
 * @internal
 * @brief Call a generic callback with the proper specialization of `CaptureCase`
 *
 * This essentially applies a "dynamic-to-static" transformation on the
 * arguments to get the correct specialization of `CaptureCase` so that the
 * `callback` can extract the semantic information for the `Permissions` and
 * `CoherenceMode` given as arguments.
 *
 *
 * The implementations proceeds via a series of calls to templated functions
 * that use switch statements to promote the arguments to template parameters.
 *
 ******************************************************************************/
template <
  typename GenericUnaryCallable
>
auto apply_with_capture_case(
  frontend::Permissions source_scheduling,
  frontend::Permissions source_immediate,
  frontend::Permissions captured_scheduling,
  frontend::Permissions captured_immediate,
  frontend::CoherenceMode coherence_mode,
  GenericUnaryCallable&& callback
) {
  return _impl::_dynamic_to_static_capture_case_transformation(
    std::forward<GenericUnaryCallable>(callback),
    coherence_mode, source_scheduling, source_immediate,
    captured_scheduling, captured_immediate
  );
}

} // end namespace capture_semantics
} // end namespace detail
} // end namespace darma

#endif //DARMAFRONTEND_SEMANTICS_HELPERS_H

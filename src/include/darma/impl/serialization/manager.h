/*
//@HEADER
// ************************************************************************
//
//                      manager.h
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

#ifndef DARMA_IMPL_SERIALIZATION_MANAGER_H
#define DARMA_IMPL_SERIALIZATION_MANAGER_H

#include <darma/interface/frontend/serialization_manager.h>
#include <darma/serialization/simple_handler.h>

namespace darma {
namespace serialization {
namespace detail {

static constexpr struct serialization_manager_default_construct_tag_t { }
  serialization_manager_default_construct_tag { };

template <typename T>
class SerializationManagerForType
  : public abstract::frontend::SerializationManager
{
  public:

    size_t
    get_metadata_size() const override {
      return sizeof(T);
    }

  protected:

    // just use simple archive and handler for now
    using serialization_handler_t = SimpleSerializationHandler<std::allocator<T>>;

  public:

    size_t
    get_packed_data_size(
      const void *const object_data,
      abstract::backend::SerializationPolicy* /* ignored */
    ) const override {
      auto ar = serialization_handler_t::make_sizing_archive();
      // call the customization point, allow ADL
      darma_compute_size(*static_cast<T const*>(object_data), ar);
      return serialization_handler_t::get_size(ar);
    }

    void
    pack_data(
      const void *const object_data,
      void *const serialization_buffer,
      abstract::backend::SerializationPolicy* /* ignored */
    ) const override {
      auto ar = serialization_handler_t::make_packing_archive(
        // Capacity unknown, but it doesn't matter
        NonOwningSerializationBuffer(
          serialization_buffer, std::numeric_limits<size_t>::max()
        )
      );
      // call the customization point, allow ADL
      darma_pack(*static_cast<T const*>(object_data), ar);
    }

    void
    unpack_data(
      void *const object_dest,
      const void *const serialized_data,
      abstract::backend::SerializationPolicy* /* unused */
    ) const override {
      auto ar = serialization_handler_t::make_unpacking_archive(
        // Capacity unknown, but it doesn't matter
        ConstNonOwningSerializationBuffer(
          serialized_data, std::numeric_limits<size_t>::max()
        )
      );
      // call the customization point, allow ADL
      darma_unpack(
        darma::serialization::allocated_buffer_for<T>(object_dest), ar
      );
    }

  private:

    template <typename U>
    using _has_tagged_default_construct_archetype =
      decltype( U{ serialization_manager_default_construct_tag } );

    template <typename U>
    using _has_tagged_default_construct = tinympl::bool_<
      tinympl::is_detected<
        _has_tagged_default_construct_archetype, U
      >::value
    >;

    void _default_construct_impl(void* allocated, std::true_type) const {
      new (allocated) T(serialization_manager_default_construct_tag);
    }

    void _default_construct_impl(void* allocated, std::false_type) const {
      new (allocated) T();
    }

  public:

    void
    default_construct(void* allocated) const override {
      _default_construct_impl(allocated, _has_tagged_default_construct<T>{});
    }

    void
    destroy(void* constructed_object) const override {
      // TODO allocator awareness?
      ((T*)constructed_object)->~T();
    }
};

} // end namespace detail
} // end namespace serialization
} // end namespace darma

#endif //DARMA_IMPL_SERIALIZATION_MANAGER_H

/*
//@HEADER
// ************************************************************************
//
//                          key.h
//                         dharma_mockup
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

#ifndef NEW_KEY_H_
#define NEW_KEY_H_

#include <cassert>
#include <tuple>
#include <cstring> // std::memcmp

#include "util.h"
#include "key_fwd.h"

// TODO Key isn't officially part of the frontend right now.  It should be moved to something like src/abstract/defaults or something

#ifndef DHARMA_DEBUG_KEY_TYPES
#  define DHARMA_DEBUG_KEY_TYPES 1
#endif

#if DHARMA_DEBUG_KEY_TYPES
#  include <typeindex>
#endif

namespace dharma_runtime {

namespace detail {

class key_part {
  public:

    explicit
    key_part(
      void* part_data
#if DHARMA_DEBUG_KEY_TYPES
      , size_t data_size
#endif
    ) : data(part_data)
#if DHARMA_DEBUG_KEY_TYPES
      , expected_size_(data_size)
#endif
    { }


    template <typename T>
    T& as() {
#if DHARMA_DEBUG_KEY_TYPES
      assert(sizeof(T) == expected_size_);
#endif
      return *(T*)data;
    }

  private:
    // an unowned pointer to the start of the data for this part
    void* data;
#if DHARMA_DEBUG_KEY_TYPES
    //type_info type;
    size_t expected_size_;
#endif
};

class key_impl_base
{
  public:

    virtual const key_part
    get_part(constexpr unsigned spot) const =0;

    virtual const void*
    get_start() const =0;

    virtual size_t
    get_size() const =0;

    virtual size_t
    get_n_parts() const =0;

    virtual ~key_impl_base() { }

#if DHARMA_DEBUG_KEY_TYPES
    virtual bool
    parts_equal(const key_impl_base* other) =0;

    virtual std::type_index
    get_type_index() const =0;
#endif

};

template <typename... Types>
class key_impl
{
  public:

    // TODO validate that Types... are bitwise copyable

    key_impl(
      constexpr const variadic_constructor_arg_t,
      Types&&... parts
    ) : parts_(std::forward<Types>(parts)...)
#if DHARMA_DEBUG_KEY_TYPES
      , type_idx(typeid(key_impl))
#endif
    { }

    explicit key_impl(
      std::tuple<Types...>&& tup
    ) : parts_(std::forward<std::tuple<Types...>>(tup))
#if DHARMA_DEBUG_KEY_TYPES
      , type_idx(typeid(key_impl))
#endif
    { }

    const key_part
    get_part(constexpr unsigned spot) const {
      return key_part((void*)&std::get<spot>(parts_));
    }

    const void*
    get_start() const {
      return &parts_;
    }

    size_t
    get_size() const {
      return sizeof(tuple_t);
    }

    size_t
    get_n_parts() const {
      return sizeof...(Types);
    }

#if DHARMA_DEBUG_KEY_TYPES
    bool
    parts_equal(const key_impl_base* other_in) {
      key_impl* other = dynamic_cast<key_impl*>(other_in);
      if(other) {
        return parts_ == other->parts_;
      }
      else {
        return false;
      }
    }

    std::type_index
    get_type_index() const {
      return type_idx;
    }
#endif

  private:

    typedef std::tuple<Types...> tuple_t;

    std::tuple<Types...> parts_;

#if DHARMA_DEBUG_KEY_TYPES
    std::type_index type_idx;
#endif

};

struct key_hash;
struct key_equal;

} // end namespace detail

class Key {
  private:

    typedef std::shared_ptr<detail::key_impl_base> _impl_ptr_t;
    typedef typename detail::smart_ptr_traits<std::shared_ptr>::maker<_impl_ptr_t> _impl_ptr_maker_t;

  public:

    typedef detail::key_hash hasher;
    typedef detail::key_equal equal;

    template <typename... Types>
    Key(
      constexpr const detail::variadic_constructor_arg_t,
      Types&&... data
    ) : k_impl_(_impl_ptr_maker_t()(
          std::forward<Types>(data)...
        ))
    { }

    template <typename... Types>
    explicit Key(
      std::tuple<Types...>&& data
    ) : k_impl_(_impl_ptr_maker_t()(
          std::forward<std::tuple<Types...>>(data)
        ))
    { }

    template <unsigned Spot>
    const detail::key_part
    component() const {
      return k_impl_->get_part(Spot);
    }

  private:

    _impl_ptr_t k_impl_;

    friend struct detail::key_hash;
    friend struct detail::key_equal;
};

typedef Key default_key_t;

template <typename... Types>
Key
make_key(Types&&... data)
{
  return Key(
    detail::variadic_constructor_arg,
    std::forward<Types>(data)...
  );
}

template <typename... Types>
Key
make_key_from_tuple(std::tuple<Types...>&& data)
{
  return Key(
    std::forward<std::tuple<Types...>>(data)
  );
}

namespace detail {

struct key_hash {
  size_t
  operator()(const Key& key) const {
    // TODO optimize this w.r.t. virtual method invocations
    typedef char align_t;
    align_t* data = (align_t*)key.k_impl_->get_start();
    size_t size = key.k_impl_->get_size() / sizeof(align_t);
    assert(key.k_impl_->get_size() % sizeof(align_t) == 0);
    if(size == 0) return 0;
    size_t rv = std::hash<align_t>()(data[0]);
    for(int i = 1; i < size; ++i) {
      hash_combine(rv, data[i]);
    }
    return rv;
  }
};

struct key_equal {
  bool
  operator()(const Key& lhs, const Key& rhs) const {
    // Shortcut: if the shared pointers point to the same thing, they must be equal
    if(lhs.k_impl_.get() == rhs.k_impl_.get()) return true;
    // TODO optimize this w.r.t. virtual method invocations
    void* lhs_data = lhs.k_impl_->get_start();
    void* rhs_data = rhs.k_impl_->get_start();
    size_t lhs_size = lhs.k_impl_->get_size();
    size_t rhs_size = rhs.k_impl_->get_size();
    if(lhs_size != rhs_size) return false;
    if(std::memcmp(lhs_data, rhs_data, lhs_size) != 0) return false;
#if DHARMA_DEBUG_KEY_TYPES
    assert(lhs.k_impl_->get_type_index() == rhs.k_impl_->get_type_index());
    assert(lhs.k_impl_->parts_equal(rhs.k_impl_.get()));
#endif
    return true;
  }
};

} // end namespace detail

inline bool
operator==(const Key& a, const Key& b)
{
  return detail::key_equal()(a, b);
}

} // end namespace dharma_runtime




#endif /* NEW_KEY_H_ */

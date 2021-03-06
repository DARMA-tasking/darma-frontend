/*
//@HEADER
// ************************************************************************
//
//                       bytes_convert.h
//                         darma
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

#ifndef DARMA_BYTES_CONVERT_H
#define DARMA_BYTES_CONVERT_H

#include <darma/key/key_fwd.h>
#include <darma/key/bytes_type_metadata.h>

#include <darma/utility/darma_assert.h>

#include <tinympl/detection.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <cassert>
#include <cstring>
#include <sstream>

namespace darma {

namespace detail {


////////////////////////////////////////////////////////////////////////////////
// <editor-fold desc="bytes_convert and its specializations">

template <typename T>
using has_char_traits_archetype = std::char_traits<T>;

template <typename T>
using has_char_traits = tinympl::is_detected<has_char_traits_archetype, T>;

//----------------------------------------
// <editor-fold desc="std::basic_string">

template <typename CharT, typename Traits, typename Allocator>
struct bytes_convert<std::basic_string<CharT, Traits, Allocator>> {
  static constexpr bool size_known_statically = false;
  typedef std::basic_string<CharT, Traits, Allocator> string_t;

  inline void
  get_bytes_type_metadata(bytes_type_metadata* md, string_t const& val) const {
    assert(val.size() <= string_like_type_metadata::max_string_size);
    auto* md_string = reinterpret_cast<string_like_type_metadata*>(md);
    md_string->_always_true = 1;
    md_string->size = (uint8_t)val.size();
  }

  inline uint32_t
  get_extension_byte_value() const {
    assert(false); // should never be called
    return 0;
  }

  inline size_t
  get_size(string_t const& val) const {
    return val.size() * sizeof(CharT);
  }

  inline constexpr void
  operator()(string_t const &val, void *dest, const size_t n_bytes, const size_t offset) const {
    const size_t size = get_size(val);
    ::memcpy(dest, val.data() + offset, n_bytes);
  }

  inline string_t
  get_value(bytes_type_metadata const* md, void const* data, size_t size) const {
    DARMA_ASSERT_EQUAL(
      (int)reinterpret_cast<const string_like_type_metadata*>(md)->size, (int)(size / sizeof(CharT))
    );
    return string_t((CharT*)data, size / sizeof(CharT));
  }
};

template <typename CharT, typename Traits, typename Allocator>
struct bytes_convert<const std::basic_string<CharT, Traits, Allocator>>
  : bytes_convert<std::basic_string<CharT, Traits, Allocator>>
{ };
template <typename CharT, typename Traits, typename Allocator>
struct bytes_convert<const volatile std::basic_string<CharT, Traits, Allocator>>
  : bytes_convert<std::basic_string<CharT, Traits, Allocator>>
{ };
template <typename CharT, typename Traits, typename Allocator>
struct bytes_convert<volatile std::basic_string<CharT, Traits, Allocator>>
  : bytes_convert<std::basic_string<CharT, Traits, Allocator>>
{ };

// </editor-fold>
//----------------------------------------

//----------------------------------------
// <editor-fold desc="generic version, using stream operator">

template <typename T, typename Enable>
struct bytes_convert;

// </editor-fold>
//----------------------------------------

//----------------------------------------
// <editor-fold desc="integral types">

template <typename T>
struct bytes_convert<T,
  // I think is_integral returns false for enums anyway, but just in case
  std::enable_if_t<std::is_integral<std::remove_cv_t<T>>::value
    and not std::is_enum<std::remove_cv_t<T>>::value>
> {
  private:
    uint8_t get_min_bytes_exponent(T const& val) const {
      uint8_t N = 0;
      // 8*(2**N) = 2**(N+3) = 1<<(N+3)
      uintmax_t current_max = uintmax_t(1) << uintmax_t(1 << (N+3));
      while(
        val >= current_max
        and N <= int_like_type_metadata::max_int_size_exponent
      ) {
        assert(current_max > 0); // overflow if this fails
        current_max = uintmax_t(1) << uintmax_t(1 << (N+3));
        ++N;
      }
      return N;
    }

  public:

    inline void
    get_bytes_type_metadata(bytes_type_metadata* md, T const& val) const {
      auto* md_int = reinterpret_cast<int_like_type_metadata*>(md);
      md_int->_always_false = false;
      md_int->_always_true = true;
      md_int->is_enumerated = false;
      md_int->is_negative = val < 0;
      md_int->int_size_exponent = get_min_bytes_exponent(val);
    }

    inline uint32_t
    get_extension_byte_value() const {
      assert(false); // should never be called
      return 0;
    }

    inline size_t get_size(T const& val) const {
      return 1ull << get_min_bytes_exponent(val);
    }

    inline constexpr void
    operator()(T const &val, void *dest, const size_t n_bytes, const size_t offset) const {
      using signed_T = std::make_signed_t<T>;
      switch(get_min_bytes_exponent(val)) {
        case 0: {
          uint8_t v = (uint8_t)(std::abs(signed_T(val)));
          ::memcpy(dest, (char*)(&v) + offset, n_bytes);
          break;
        }
        case 1: {
          uint16_t v = (uint16_t)(std::abs(signed_T(val)));
          ::memcpy(dest, (char*)(&v) + offset, n_bytes);
          break;
        }
        case 2: {
          uint32_t v = (uint32_t)(std::abs(signed_T(val)));
          ::memcpy(dest, (char*)(&v) + offset, n_bytes);
          break;
        }
        case 3: {
          uint64_t v = (uint64_t)(std::abs(signed_T(val)));
          ::memcpy(dest, (char*)(&v) + offset, n_bytes);
          break;
        }
        default : {
          assert(false); // not implemented
          static_assert(sizeof(uint64_t) == sizeof(uintmax_t), "not enough it sizes handled");
          break;
        }
      }
    }

    inline T
    get_value(bytes_type_metadata const* md, void const* data, size_t size) const {
      const auto* md_int = reinterpret_cast<const int_like_type_metadata*>(md);
      DARMA_ASSERT_MESSAGE(md_int->_always_true, "Bad conversion to integral type from data stored as non-integer");
      DARMA_ASSERT_MESSAGE(not md_int->_always_false, "Bad conversion to integral type from data stored as non-integer");
      assert(std::is_signed<T>::value or not md_int->is_negative);
      T rv;
      assert(size == 1 << md_int->int_size_exponent);
      // TODO overflow checking (i.e., can T fit the value)
      switch(md_int->int_size_exponent) {
        case 0: {
          rv = *(uint8_t*)data;
          break;
        }
        case 1: {
          rv = *(uint16_t*)data;
          break;
        }
        case 2: {
          rv = *(uint32_t*)data;
          break;
        }
        case 3: {
          rv = *(uint64_t*)data;
          break;
        }
        default : {
          assert(false); // not implemented
          static_assert(sizeof(uint64_t) == sizeof(uintmax_t), "not enough it sizes handled");
          break;
        }
      }
      if(md_int->is_negative) rv = -rv;
      return rv;
    }
};


template <typename T>
struct bytes_convert<T,
  std::enable_if_t<std::is_enum<std::remove_cv_t<T>>::value>
>
{
  inline void
  get_bytes_type_metadata(bytes_type_metadata* md, T const& val) const {
    auto* md_enum = reinterpret_cast<enum_like_type_metadata*>(md);
    md_enum->_always_false = false;
    md_enum->_always_true_1 = true;
    md_enum->_always_true_2 = true;
    size_t index = get_registered_enum_index(val);
    md_enum->enum_category = index < enum_like_type_metadata::use_extension_byte_for_category ?
                             (uint8_t)index : enum_like_type_metadata::use_extension_byte_for_category;
  }

  inline uint32_t
  get_extension_byte_value() const {
    size_t index = get_registered_enum_index(T());
    return index < enum_like_type_metadata::use_extension_byte_for_category ? 0 : (uint32_t)index;
  }

  inline size_t get_size(T const& val) const {
    return sizeof(T);
  }

  inline constexpr void
  operator()(T const &val, void *dest, const size_t n_bytes, const size_t offset) const {
    memcpy(dest, (char*)&val + offset, n_bytes);
  }

  inline T
  get_value(bytes_type_metadata const* md, void const* data, size_t size) const {
    size_t correct_index = get_registered_enum_index(T());
    const auto* md_enum = reinterpret_cast<enum_like_type_metadata const*>(md);
    DARMA_ASSERT_MESSAGE(md_enum->_always_true_1, "Bad conversion to enumerated type from data stored as non-enum");
    DARMA_ASSERT_MESSAGE(md_enum->_always_true_2, "Bad conversion to enumerated type from data stored as non-enum");
    DARMA_ASSERT_MESSAGE(not md_enum->_always_false, "Bad conversion to enumerated type from data stored as non-enum");
    size_t stored_index = md_enum->enum_category == enum_like_type_metadata::use_extension_byte_for_category ?
                          category_extension_bytes_value(static_cast<const void*>(md)) : md_enum->enum_category;
    // TODO more readable error message.
    // This indicates that the value is being retrieved as an enum type that it wasn't stored as
    DARMA_ASSERT_EQUAL(correct_index, stored_index);
    DARMA_ASSERT_EQUAL(size, sizeof(T));
    return *static_cast<T*>(const_cast<void*>(data));
  }

};

template <typename T>
struct bytes_convert<T,
  std::enable_if_t<std::is_floating_point<std::remove_cv_t<T>>::value>
>
{
  inline void
  get_bytes_type_metadata(bytes_type_metadata* md, T const& val) const {
    auto* md_float = reinterpret_cast<float_like_type_metadata*>(md);
    md_float->_always_true = true;
    md_float->_always_false_1 = false;
    md_float->_always_false_2 = false;
  }

  inline uint32_t
  get_extension_byte_value() const {
    assert(false); // should never be called
    return 0;
  }

  inline size_t get_size(T const& val) const {
    return sizeof(double);
  }

  inline constexpr void
  operator()(T const &val, void *dest, const size_t n_bytes, const size_t offset) const {
    double val_d = val;
    memcpy(dest, (char*)&val_d + offset, n_bytes);
  }

  inline T
  get_value(bytes_type_metadata const* md, void const* data, size_t size) const {
    size_t correct_index = get_registered_enum_index(T());
    const auto* md_float = reinterpret_cast<float_like_type_metadata const*>(md);
    DARMA_ASSERT_MESSAGE(md_float->_always_true, "Bad conversion to floating point type from data stored as non-floating point");
    DARMA_ASSERT_MESSAGE(not md_float->_always_false_1, "Bad conversion to floating point type from data stored as non-floating point");
    DARMA_ASSERT_MESSAGE(not md_float->_always_false_2, "Bad conversion to floating point type from data stored as non-floating point");
    return (T)*static_cast<double*>(const_cast<void*>(data));
  }

};


// </editor-fold>
//----------------------------------------

//----------------------------------------
// <editor-fold desc="fundamental types (not implemented any more, was too general)">

//template <typename T>
//struct bytes_convert<T, std::enable_if_t<std::is_fundamental<T>::value>> {
//  static constexpr bool size_known_statically = true;
//  static constexpr bool can_reinterpret_cast = true;
//  static constexpr size_t size = sizeof(T);
//  inline constexpr size_t get_size(T const&) const { return size; }
//  inline constexpr void
//  operator()(T const &val, void *dest, const size_t n_bytes, const size_t offset) const {
//    ::memcpy(dest, (char*)(&val) + offset, n_bytes);
//  }
//};

// </editor-fold>
//----------------------------------------

//----------------------------------------
// <editor-fold desc="Arrays of fundamental types (other than characters) (not implemented any more)">

//template <typename T, size_t N>
//struct bytes_convert<T[N], std::enable_if_t<
//  // Char strings should be treated specially (i.e., ignoring the trailing '\0')
//  std::is_fundamental<T>::value and not has_char_traits<std::remove_cv_t<T>>::value
//>> {
//  static constexpr bool size_known_statically = true;
//  static constexpr bool can_reinterpret_cast = true;
//  static constexpr size_t size = sizeof(T) * N;
//  inline constexpr size_t get_size(T const val[N]) const { return size; }
//  inline constexpr void
//  operator()(T const val[N], void* dest, const size_t n_bytes, const size_t offset) const {
//    ::memcpy(dest, val + offset, n_bytes);
//  }
//};

// </editor-fold>
//----------------------------------------

//----------------------------------------
// <editor-fold desc="Arrays of character types, pointers to character types">

// e.g. const char* and other c-style strings with variable length.
template <typename CharT>
struct bytes_convert<CharT*,
 std::enable_if_t<has_char_traits<std::remove_cv_t<CharT>>::value>
> {
  static_assert(sizeof(CharT) == 1, "wide characters not yet implemeneted");
  inline void
  get_bytes_type_metadata(bytes_type_metadata* md, CharT* val) const {
    assert(get_size(val) <= string_like_type_metadata::max_string_size);
    auto* md_string = reinterpret_cast<string_like_type_metadata*>(md);
    md_string->_always_true = 1;
    md_string->size = (uint8_t)get_size(val) * (uint8_t)sizeof(CharT);
  }

  inline uint32_t
  get_extension_byte_value() const {
    assert(false); // should never be called
    return 0;
  }

  static constexpr bool size_known_statically = false;
  typedef std::char_traits<std::remove_cv_t<CharT>> traits_t;
  inline size_t
  get_size(CharT* val) const {
    return traits_t::length(val);
  }

  inline constexpr void
  operator()(CharT* val, void* dest, const size_t n_bytes, const size_t offset) const {
    const size_t size = get_size(val);
    ::memcpy(dest, ((char*)val) + offset, n_bytes);
  }
  // Can't be converted back
};

template <typename CharT, size_t N>
struct bytes_convert<CharT[N],
  std::enable_if_t<
    has_char_traits<std::remove_cv_t<CharT>>::value
    and not std::is_const<CharT>::value
  >
> {
  static_assert(sizeof(CharT) == 1, "wide characters not yet implemeneted");
  static_assert(N < std::numeric_limits<uint8_t>::max(),
    "Too many characters in string given to key"
  );
  inline void
  get_bytes_type_metadata(bytes_type_metadata* md, CharT const* val) const {
    assert(get_size(val) <= string_like_type_metadata::max_string_size);
    auto* md_string = reinterpret_cast<string_like_type_metadata*>(md);
    md_string->_always_true = 1;
    md_string->size = (uint8_t)get_size(val) * (uint8_t)sizeof(CharT);
  }

  inline uint32_t
  get_extension_byte_value() const {
    assert(false); // should never be called
    return 0;
  }

  static constexpr bool size_known_statically = false;
  typedef std::char_traits<std::remove_cv_t<CharT>> traits_t;
  inline size_t
  get_size(CharT const val[N]) const {
    return traits_t::length(val);
  }

  inline constexpr void
  operator()(CharT const val[N], void* dest, const size_t n_bytes, const size_t offset) const {
    const size_t size = get_size(val);
    ::memcpy(dest, ((char*)val) + offset, n_bytes);
  }
  // Can't be converted back
};

template <typename CharT, size_t N>
struct bytes_convert<const CharT[N],
  std::enable_if_t<has_char_traits<std::remove_cv_t<CharT>>::value>
> {
  static_assert(sizeof(CharT) == 1, "wide characters not yet implemeneted");
  static_assert(N > 0, "don't know what to do with a zero-sized char array");
  static_assert(N < std::numeric_limits<uint8_t>::max(),
    "Too many characters in string given to key"
  );
  inline void
  get_bytes_type_metadata(bytes_type_metadata* md, CharT const* val) const {
    assert(get_size(val) <= string_like_type_metadata::max_string_size);
    auto* md_string = reinterpret_cast<string_like_type_metadata*>(md);
    md_string->_always_true = 1;
    md_string->size = (uint8_t)(N-1);
  }

  inline uint32_t
  get_extension_byte_value() const {
    assert(false); // should never be called
    return 0;
  }

  static constexpr bool size_known_statically = true;
  static constexpr auto static_size = (N-1) * sizeof(CharT);
  typedef std::char_traits<std::remove_cv_t<CharT>> traits_t;
  inline size_t
  get_size(CharT const val[N]) const {
    assert(traits_t::length(val) == static_size);
    return static_size;
  }

  inline constexpr void
  operator()(CharT const val[N], void* dest, const size_t n_bytes, const size_t offset) const {
    const size_t size = get_size(val);
    ::memcpy(dest, ((char*)val) + offset, n_bytes);
  }
  // Can't be converted back
};

// </editor-fold>
//----------------------------------------


// TODO floating-point types
// TODO enums

// </editor-fold>
////////////////////////////////////////////////////////////////////////////////

namespace _impl {
template<typename T>
using convertible_to_bytes_archetype = decltype(bytes_convert<T>()(
  std::declval<T>(), std::declval<void *>(), size_t(0), size_t(0)
));
} // end namespace _impl
template <typename T>
using convertible_to_bytes = tinympl::is_detected<_impl::convertible_to_bytes_archetype, T>;

namespace _impl {
template<typename T>
using convertible_from_bytes_archetype = decltype(std::declval<bytes_convert<T>>().get_value(
  std::declval<void const *>(), size_t(0)
));
} // end namespace _impl
template <typename T>
using convertible_from_bytes = tinympl::is_detected_convertible<T, _impl::convertible_from_bytes_archetype, T>;

namespace _impl {
template<typename T>
using can_reinterpret_cast_bytes_archetype =
  std::integral_constant<bool, bytes_convert<T>::can_reinterpret_cast>;
} // end namespace _impl
template <typename T>
using can_reinterpret_cast_bytes =
  tinympl::detected_or<std::false_type, _impl::can_reinterpret_cast_bytes_archetype, T>;

namespace _impl {
template<typename T>
using bytes_size_known_statically_archetype =
std::integral_constant<bool, bytes_convert<T>::size_known_statically>;
} // end namespace _impl
template <typename T>
using bytes_size_known_statically =
  tinympl::detected_or<std::false_type, _impl::bytes_size_known_statically_archetype, T>;



} // end namespace detail

} // end namespace darma

#endif //DARMA_BYTES_CONVERT_H

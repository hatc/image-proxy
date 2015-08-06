// formatidiv.hpp
#pragma once

#ifndef __CPCL_FORMATIDIV_HPP
#define __CPCL_FORMATIDIV_HPP

#include <cpcl/basic.h>

#ifdef _MSC_VER
#include <type_traits>
#else
#include <tr1/type_traits>
#endif

namespace cpcl {

struct formatidiv_signed_tag {};
struct formatidiv_unsigned_tag {};
struct formatidiv_unsigned_hex_tag {};
//struct formatidiv_signed_hex_tag {};
struct formatidiv_unsigned_bin_tag {};

template<size_t Base, bool ValueTypeIsUnsigned>
struct formatidiv_traits {
  typedef formatidiv_signed_tag tag;
};
template<size_t Base>
struct formatidiv_traits < Base, true > {
  typedef formatidiv_unsigned_tag tag;
};
template<>
struct formatidiv_traits < 16, true > {
  typedef formatidiv_unsigned_hex_tag tag;
};
//template<>
//struct formatidiv_traits<16, false> {
//	typedef formatidiv_signed_hex_tag tag;
//};
template<>
struct formatidiv_traits < 2, true > {
  typedef formatidiv_unsigned_bin_tag tag;
};

template<class ValueType, size_t Base>
inline char* formatidiv(formatidiv_signed_tag*, char *s, ValueType v, size_t width, char place) {
  bool sign = false;
  if (v < 0) {
    v *= -1;
    if ('0' == place) {
      *s++ = '-';
      --width;
    }
    else
      sign = true;
  }
  for (char *h = s - 1 + width; h >= s; --h)
    if (v != 0) { // (v > 0)
    *h = "0123456789ABCDEF"[v % Base];
    v /= Base;
    }
    else {
      if (sign) {
        *h = '-';
        sign = false;
      }
      else
        *h = place;
    }
    return s + width;
}
template<class ValueType, size_t Base>
inline char* formatidiv(formatidiv_unsigned_tag*, char *s, ValueType v, size_t width, char place) {
  for (char *h = s - 1 + width; h >= s; --h)
    if (v != 0) {
    *h = "0123456789ABCDEF"[v % Base];
    v /= Base;
    }
    else
      *h = place;
  return s + width;
}
template<class ValueType, size_t Base>
inline char* formatidiv(formatidiv_unsigned_hex_tag*, char *s, ValueType v, size_t width, char place) {
  for (char *h = s - 1 + width; h >= s; --h)
    if (v != 0) {
    *h = "0123456789ABCDEF"[v & 0xF];
    v >>= 4;
    }
    else
      *h = place;
  return s + width;
}
//template<class ValueType, size_t Base>
//inline char* formatidiv(formatidiv_signed_hex_tag*, char *s, ValueType v, size_t width, char place) {
//	typedef boost::detail::make_unsigned<ValueType>::type ValueUnsignedType;
//	return formatidiv<ValueUnsignedType, 16>((formatidiv_unsigned_hex_tag *)0, s, static_cast<ValueUnsignedType>(v), width, place);
//}
template<class ValueType, size_t Base>
inline char* formatidiv(formatidiv_unsigned_bin_tag*, char *s, ValueType v, size_t width, char place) {
  for (char *h = s - 1 + width; h >= s; --h)
    if (v != 0) {
    *h = "01"[v & 1];
    v >>= 1;
    }
    else
      *h = place;
  return s + width;
}

template<size_t Base, class ValueType>
inline char* formatidiv(char *s, ValueType v, size_t width, char place = '0') {
  typedef char BaseInvalid[Base < 17 ? 1 : -1];
  typedef char ValueTypeInvalid[std::tr1::is_integral<ValueType>::value && !(std::tr1::is_same<ValueType, bool>::value || std::tr1::is_same<ValueType, wchar_t>::value) ? 1 : -1];

  return formatidiv<ValueType, Base>((typename formatidiv_traits<Base, std::tr1::is_unsigned<ValueType>::value>::tag *)0, s, v, width, place);
}

/* sar(arithmetic right shift) vs shr(logical right shift)... */
template<>
inline char* formatidiv<16, char>(char *s, char v, size_t width, char place) {
  return formatidiv<unsigned char, 16>((formatidiv_unsigned_hex_tag *)0, s, static_cast<unsigned char>(v), width, place);
}
//template<>
//inline char* formatidiv<16, wchar_t>(char *s, wchar_t v, size_t width, char place) {
//	return formatidiv<unsigned wchar_t, 16>((formatidiv_hex_tag *)0, s, static_cast<unsigned wchar_t>(v), width, place);
//}
template<>
inline char* formatidiv<16, short>(char *s, short v, size_t width, char place) {
  return formatidiv<unsigned short, 16>((formatidiv_unsigned_hex_tag *)0, s, static_cast<unsigned short>(v), width, place);
}
template<>
inline char* formatidiv<16, int>(char *s, int v, size_t width, char place) {
  return formatidiv<unsigned int, 16>((formatidiv_unsigned_hex_tag *)0, s, static_cast<unsigned int>(v), width, place);
}
template<>
inline char* formatidiv<16, long>(char *s, long v, size_t width, char place) {
  return formatidiv<unsigned long, 16>((formatidiv_unsigned_hex_tag *)0, s, static_cast<unsigned long>(v), width, place);
}
template<>
inline char* formatidiv<16, int64>(char *s, int64 v, size_t width, char place) {
  return formatidiv<uint64, 16>((formatidiv_unsigned_hex_tag *)0, s, static_cast<uint64>(v), width, place);
}

template<>
inline char* formatidiv<2, char>(char *s, char v, size_t width, char place) {
  return formatidiv<unsigned char, 2>((formatidiv_unsigned_bin_tag *)0, s, static_cast<unsigned char>(v), width, place);
}
template<>
inline char* formatidiv<2, short>(char *s, short v, size_t width, char place) {
  return formatidiv<unsigned short, 2>((formatidiv_unsigned_bin_tag *)0, s, static_cast<unsigned short>(v), width, place);
}
template<>
inline char* formatidiv<2, int>(char *s, int v, size_t width, char place) {
  return formatidiv<unsigned int, 2>((formatidiv_unsigned_bin_tag *)0, s, static_cast<unsigned int>(v), width, place);
}
template<>
inline char* formatidiv<2, long>(char *s, long v, size_t width, char place) {
  return formatidiv<unsigned long, 2>((formatidiv_unsigned_bin_tag *)0, s, static_cast<unsigned long>(v), width, place);
}
template<>
inline char* formatidiv<2, int64>(char *s, int64 v, size_t width, char place) {
  return formatidiv<uint64, 2>((formatidiv_unsigned_bin_tag *)0, s, static_cast<uint64>(v), width, place);
}

} // namespace cpcl

#endif // __CPCL_FORMATIDIV_HPP

// string_util_posix.hpp
#pragma once

#ifndef __CPCL_STRING_UTIL_POSIX_HPP
#define __CPCL_STRING_UTIL_POSIX_HPP

#include <cstring> // size_t
#include <cstdarg>
#include <cstdio>
#include <wchar.h> // vswprintf

namespace cpcl {

namespace string_format_impl_private {
inline int vsnprintf(char *r, size_t r_size, char const *format, va_list arguments) {
  return ::vsnprintf(r, r_size, format, arguments);
}
inline int vsnprintf(wchar_t *r, size_t r_size, wchar_t const *format, va_list arguments) {
  return ::vswprintf(r, r_size, format, arguments);
}
} // namespace string_format_impl_private

template<class Char>
inline size_t StringFormatImpl(Char *r, size_t r_size, Char const *format, va_list arguments, bool *truncated) {
  if (!(!!r && r_size > 0))
    return 0;
  
  bool r_truncated(false);
  int n = string_format_impl_private::vsnprintf(r, r_size, format, arguments);
  if (n < 0 || n >= (int)r_size) {
    r_truncated = true;
    if (n > 0) {
      --r_size;
    } else {
      r_size = 0;
      r[r_size] = 0;
    }
  } else
    r_size = static_cast<size_t>(n);
  if (truncated)
    *truncated = r_truncated;
  return r_size;
}

} // namespace cpcl

#endif // __CPCL_STRING_UTIL_POSIX_HPP

// string_util_win.hpp
#pragma once

#ifndef __CPCL_STRING_UTIL_WIN_HPP
#define __CPCL_STRING_UTIL_WIN_HPP

#include <cstdarg>

namespace cpcl {

namespace string_format_impl_private {

inline int vsnprintf(char *r, size_t r_size, char const *format, va_list arguments) {
  return _vsnprintf_s(r, r_size, _TRUNCATE, format, arguments);
}
inline int vsnprintf(wchar_t *r, size_t r_size, wchar_t const *format, va_list arguments) {
  return _vsnwprintf_s(r, r_size, _TRUNCATE, format, arguments);
}

} // namespace string_format_impl_private

template<class Char>
inline size_t StringFormatImpl(Char *r, size_t r_size, Char const *format, va_list arguments, bool *truncated) {
  if (!((r) && r_size > 0))
    return 0;

  bool r_truncated(false);
  int n = string_format_impl_private::vsnprintf(r, r_size, format, arguments);
  if (n < 0) {
    r_truncated = true;
    r[r_size - 1] = 0;
  }

  if (truncated)
    *truncated = r_truncated;
  return (r_truncated) ? (r_size - 1) : static_cast<size_t>(n);
}

} // namespace cpcl

#endif // __CPCL_STRING_UTIL_WIN_HPP

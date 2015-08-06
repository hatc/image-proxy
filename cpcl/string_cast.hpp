// string_cast.hpp
#pragma once

#ifndef __CPCL_STRING_CAST_HPP
#define __CPCL_STRING_CAST_HPP

#include <math.h> // HUGE_VAL
#include <stdlib.h> // strtol, strtod
#include <wchar.h> // wcstol, wcstod

#include <boost/lexical_cast.hpp>

#include <cpcl/string_piece.hpp>

namespace cpcl {

template<class T, class CharType>
inline bool TryConvert(BasicStringPiece<CharType, std::char_traits<CharType> > const &s, T *r) {
  try {
    T tmp = boost::lexical_cast<T>(s.as_string());
    if (r)
      *r = tmp;
  } catch(boost::bad_lexical_cast const&) {
    return false;
  }
  return true;
}

template<>
inline bool TryConvert<long, char>(StringPiece const &s, long *r) {
  std::string s_ = s.as_string();
  char const *str = s_.c_str();
  char *tail;
  long const n = ::strtol(str, &tail, 10);

  if ((n == LONG_MAX) || (n == LONG_MIN) || ((n == 0) && (tail == str)))
    return false;
  if (r)
    *r = n;
  return true;
}
template<>
inline bool TryConvert<unsigned long, char>(StringPiece const &s, unsigned long *r) {
  std::string s_ = s.as_string();
  char const *str = s_.c_str();
  char *tail;
  unsigned long const n = ::strtoul(str, &tail, 10);

  if ((n == ULONG_MAX) || ((n == 0) && (tail == str)))
    return false;
  long const tmp = ::strtoul(str, NULL, 10);
  if (tmp < 0)
    return false;
  if (r)
    *r = n;
  return true;
}
template<>
inline bool TryConvert<int, char>(StringPiece const &s, int *r) {
  long tmp;
  if (TryConvert(s, &tmp)) {
    if (r)
      *r = static_cast<int>(tmp);
    return true;
  }
  return false;
}
template<>
inline bool TryConvert<unsigned int, char>(StringPiece const &s, unsigned int *r) {
  unsigned long tmp;
  if (TryConvert(s, &tmp)) {
    if (r)
      *r = static_cast<unsigned int>(tmp);
    return true;
  }
  return false;
}
template<>
inline bool TryConvert<double, char>(StringPiece const &s, double *r) {
  std::string s_ = s.as_string();
  char const *str = s_.c_str();
  char *tail;
  double const n = ::strtod(str, &tail);

  if ((n == -HUGE_VAL) || (n == +HUGE_VAL) || ((n == 0) && (tail == str)))
    return false;
  if (r)
    *r = n;
  return true;
}

template<>
inline bool TryConvert<long, wchar_t>(WStringPiece const &s, long *r) {
  std::wstring s_ = s.as_string();
  wchar_t const *str = s_.c_str();
  wchar_t *tail;
  long const n = ::wcstol(str, &tail, 10);

  if ((n == LONG_MAX) || (n == LONG_MIN) || ((n == 0) && (tail == str)))
    return false;
  if (r)
    *r = n;
  return true;
}
template<>
inline bool TryConvert<unsigned long, wchar_t>(WStringPiece const &s, unsigned long *r) {
  std::wstring s_ = s.as_string();
  wchar_t const *str = s_.c_str();
  wchar_t *tail;
  unsigned long const n = ::wcstoul(str, &tail, 10);

  if ((n == ULONG_MAX) || ((n == 0) && (tail == str)))
    return false;
  long const tmp = ::wcstol(str, NULL, 10);
  if (tmp < 0)
    return false;
  if (r)
    *r = n;
  return true;
}
template<>
inline bool TryConvert<int, wchar_t>(WStringPiece const &s, int *r) {
  long tmp;
  if (TryConvert(s, &tmp)) {
    if (r)
      *r = static_cast<int>(tmp);
    return true;
  }
  return false;
}
template<>
inline bool TryConvert<unsigned int, wchar_t>(WStringPiece const &s, unsigned int *r) {
  unsigned long tmp;
  if (TryConvert(s, &tmp)) {
    if (r)
      *r = static_cast<unsigned int>(tmp);
    return true;
  }
  return false;
}
template<>
inline bool TryConvert<double, wchar_t>(WStringPiece const &s, double *r) {
  std::wstring s_ = s.as_string();
  wchar_t const *str = s_.c_str();
  wchar_t *tail;
  double const n = ::wcstod(str, &tail);

  if ((n == -HUGE_VAL) || (n == +HUGE_VAL) || ((n == 0) && (tail == str)))
    return false;
  if (r)
    *r = n;
  return true;
}

} // namespace cpcl

#endif // __CPCL_STRING_CAST_HPP

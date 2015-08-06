// string_util.hpp
#pragma once

#ifndef __CPCL_STRING_UTIL_HPP
#define __CPCL_STRING_UTIL_HPP

#include <algorithm> // std::min

#ifdef _MSC_VER
#include <cpcl/string_util_win.hpp>
#else
#include <cpcl/string_util_posix.hpp>
#endif

#include <cpcl/string_piece.hpp>

namespace cpcl {
/* copy string
 * returns number of character copied, exclude NULL character                               
 * StringCopy always append NULL character to output string, if it's size > 0 */
template<class Char>
inline size_t StringCopy(Char *r, size_t r_size, BasicStringPiece<Char> const &s) {
  if (!(!!r && r_size > 0))
    return 0;
  
  r_size = (std::min)(r_size - 1, s.size());
  std::char_traits<Char>::copy(r, s.data(), r_size);
  r[r_size] = 0;
  return r_size;
}
template<class Char>
inline size_t StringCopy(Char *r, size_t r_size, Char const *s) {
  return StringCopy(r, r_size, BasicStringPiece<Char>(s));
}
template<class Char, size_t N>
inline size_t StringCopyFromLiteral(Char *r, size_t r_size, Char const (&s)[N]) {
  return StringCopy(r, r_size, BasicStringPiece<Char>(s, N - 1));
}

template<class Char, size_t K>
inline size_t StringCopy(Char (&r)[K], BasicStringPiece<Char> const &s) {
  return StringCopy(r, K, s);
}
template<class Char, size_t K>
inline size_t StringCopy(Char (&r)[K], Char const *s) {
  return StringCopy(r, K, BasicStringPiece<Char>(s));
}
template<class Char, size_t N, size_t K>
inline size_t StringCopyFromLiteral(Char (&r)[K], Char const (&s)[N]) {
  return StringCopy(r, K, BasicStringPiece<Char>(s, N - 1));
}

template<class Char>
inline size_t StringFormat(Char *r, size_t r_size, Char const *format, ...) {
  va_list arguments;
  va_start(arguments, format);
  size_t n = StringFormatImpl(r, r_size, format, arguments, 0);
  va_end(arguments);
  return n;
}
template<class Char, size_t N>
inline size_t StringFormat(Char(&r)[N], Char const *format, ...) {
  va_list arguments;
  va_start(arguments, format);
  size_t n = StringFormatImpl(r, N, format, arguments, 0);
  va_end(arguments);
  return n;
}

template<class Char/*, size_t FormatBufSize = 0x100*/>
inline std::basic_string<Char>& StringFormat(std::basic_string<Char> *r, Char const *format, ...) {
  size_t const FormatBufSize = 0x100;
  Char buf[FormatBufSize];
  va_list arguments;
  va_start(arguments, format);
  /* basic_string::resize: _Ptr = _Mybase::_Alval.allocate(_Newres + 1);
     i.e. resize(N) actual allocate N + 1
     r->resize(vsnprintf(char[1], 1, format, va_copy(arguments)));
     Char *buf = &(*r->begin()); */
  r->assign(buf, StringFormatImpl(buf, FormatBufSize, format, arguments, 0));
  va_end(arguments);
  return *r;
}
template<class Char/*, size_t FormatBufSize = 0x100*/>
inline std::basic_string<Char> StringFormat(Char const *format, ...) {
  size_t const FormatBufSize = 0x100;
  Char buf[FormatBufSize];
  va_list arguments;
  va_start(arguments, format);
  std::basic_string<Char> r(buf, StringFormatImpl(buf, FormatBufSize, format, arguments, 0));
  va_end(arguments);
  return r;
}

// bool || size_t StringFind(String where, String what)

template<class Char>
inline Char ToLowerASCII(Char c) {
  return ('A' <= c && c <= 'Z') ? c + ('a' - 'A') : c;
}

template<class Char>
inline bool StringEqualsIgnoreCaseASCII(Char const *a, Char const *b) {
  if (!a || !b)
    return a == b;

  bool r(ToLowerASCII(*a) == ToLowerASCII(*b));
  while ((*a & *b) != 0 && r)
    r = ToLowerASCII(*++a) == ToLowerASCII(*++b);
  return r;
}

inline bool StringEqualsIgnoreCaseASCII(cpcl::WStringPiece const &a, cpcl::WStringPiece const &b) {
  if (a.size() != b.size())
    return false;

  for (wchar_t const *ia = a.begin(), *ib = b.begin(), *tail = a.end(); ia != tail; ++ia, ++ib)
    if (ToLowerASCII(*ia) != ToLowerASCII(*ib))
      return false;
  return true;
}

extern unsigned char const charsASCII[];
inline bool StringEqualsIgnoreCaseASCII(char const *a, char const *b) {
  if (!a || !b)
    return a == b;

  unsigned ac = charsASCII[(unsigned char)*a], bc = charsASCII[(unsigned char)*b];
  bool r = (ac == bc);
  while (((ac & bc) != 0) && (r)) {
    ac = charsASCII[(unsigned char)*++a]; bc = charsASCII[(unsigned char)*++b];
    r = (ac == bc);
  }
  return r;
}
inline bool StringEqualsIgnoreCaseASCII(cpcl::StringPiece const &a, cpcl::StringPiece const &b) {
  if (a.size() != b.size())
    return false;

  for (char const *ia = a.begin(), *ib = b.begin(), *tail = a.end(); ia != tail;)
    if ((unsigned)charsASCII[(unsigned char)*ia++] != (unsigned)charsASCII[(unsigned char)*ib++])
      return false;
  return true;
}
/* compare release asm for code:
if (!StringEqualsIgnoreCaseASCII("abcd", "ABCD"))
throw 0;
if (!StringEqualsIgnoreCaseASCII(StringPiece("abcd"), StringPiece("ABCD")))
throw 0;
char const* is better optimized(~1.5) than StringPiece const& */

namespace string_append_format_impl_private {
template<class Char>
inline void StringAppendFormatImpl(std::basic_string<Char> &r, BasicStringPiece<Char> const &s, Char const *format, va_list arguments) {
  size_t const FormatBufSize = 0x100;
  Char buf[FormatBufSize];
  
  size_t n = StringFormatImpl(buf, FormatBufSize, format, arguments, 0);
  r.reserve(n + s.size()); // actually alloc ((n + s.size()) ~ 3/2) + 1, heap alloc
  r.assign(s.data(), s.size());
  r.append(buf, n);
}
}

/* append formatted text to string \a s
 * two overloads provided for use implicit cast from char const* and std::basic_string to BasicStringPiece */
inline std::string StringAppendFormat(StringPiece const &s, char const *format, ...) {
  std::string r;

  va_list arguments;
  va_start(arguments, format);
  string_append_format_impl_private::StringAppendFormatImpl(r, s, format, arguments);
  va_end(arguments);

  return r;
}
inline std::wstring StringAppendFormat(WStringPiece const &s, wchar_t const *format, ...) {
  std::wstring r;

  va_list arguments;
  va_start(arguments, format);
  string_append_format_impl_private::StringAppendFormatImpl(r, s, format, arguments);
  va_end(arguments);

  return r;
}
/* another option is use explicit cast:
StringAppendFormat(StringPiece(), format, ...);
or overload template
template<class Char>
inline std::basic_string<Char> StringAppendFormat(Char const *s, Char const *format, ...) {
  size_t const FormatBufSize = 0x100;
  Char buf[FormatBufSize];
  
  va_list arguments;
  va_start(arguments, format);
  size_t n = cpcl::StringFormatImpl(buf, FormatBufSize, format, arguments, 0);
  va_end(arguments);

  size_t s_size = std::char_traits<Char>::length(s);
  std::basic_string<Char> r;
  r.reserve(n + s_size); // actually alloc ((n + s.size()) ~ 3/2) + 1, heap alloc
  r.assign(s, s_size);
  r.append(buf, n);
  return r;
  }

template<class Char>
inline std::basic_string<Char> StringAppendFormat(std::basic_string<Char> const &s, Char const *format, ...) {
  size_t const FormatBufSize = 0x100;
  Char buf[FormatBufSize];

  va_list arguments;
  va_start(arguments, format);
  size_t n = cpcl::StringFormatImpl(buf, FormatBufSize, format, arguments, 0);
  va_end(arguments);

  std::basic_string<Char> r;
  r.reserve(n + s.size()); // actually alloc ((n + s.size()) ~ 3/2) + 1, heap alloc
  r.assign(s);
  r.append(buf, n);
  return r;
  }

but function template overloading is funny - for example template specializations don't overloads
i.e. compiler select base template first and then search for most specialized version of it */

} // namespace cpcl

#endif // __CPCL_STRING_UTIL_HPP

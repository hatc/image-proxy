// trace_helpers.hpp
#pragma once

#ifndef __CPCL_TRACE_HELPERS_HPP
#define __CPCL_TRACE_HELPERS_HPP

#include <cpcl/string_piece.hpp>
#include <cpcl/timer.h>

namespace cpcl {

struct A {};

template<class Char, size_t N>
struct RandNameCharacters {
  static Char const* value() { Char InvalidFunction[Char(0) ? -1 : -1]; }
};
template<>
struct RandNameCharacters<char, 0x10> {
  static char const* value() { return "0123456789ABCDEF"; }
};
template<>
struct RandNameCharacters<wchar_t, 0x10> {
  static wchar_t const* value() { return L"0123456789ABCDEF"; }
};

// 0123456789ABCDEF

template<class CharType>
inline CharType* RandName(CharType *buf, size_t const len, cpcl::BasicStringPiece<CharType, std::char_traits<CharType> > const &head, cpcl::BasicStringPiece<CharType, std::char_traits<CharType> > const &tail) {
 if (len < head.size() + tail.size() + 1)
  return *buf = 0,buf;
 
 CharType *s = buf;
 for (size_t i = 0; i < head.size(); ++i, ++s)
  *s = head[i];
 
 unsigned int seed = TimedSeed();
 for (CharType const * const s_ = s + len - head.size() - tail.size() - 1; s != s_; ++s) {
  seed = seed * 214013L + 2531011L;
  *s = RandNameCharacters<CharType, 0x10>::value()[(unsigned int)(((seed >> 16) & 0x7fff) / 0x800)]; // i.e. (max(rand) == 0x7fff / 0x800) < 0x10
 }
 
 for (size_t i = 0; i < tail.size(); ++i, ++s)
  *s = tail[i];
 *s = 0;
 return buf;
}
template<class CharType, size_t N>
inline CharType* RandName(CharType (&buf)[N], cpcl::BasicStringPiece<CharType, std::char_traits<CharType> > const &head, cpcl::BasicStringPiece<CharType, std::char_traits<CharType> > const &tail) {
 return RandName(buf, N, head, tail);
}

} // namespace cpcl

#endif // __CPCL_TRACE_HELPERS_HPP

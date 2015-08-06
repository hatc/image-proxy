// string_util.h
#pragma once

#ifndef __CPCL_STRING_UTIL_H
#define __CPCL_STRING_UTIL_H

#include <string.h> // memcpy
#include <algorithm> // std::min
#include <cpcl/string_piece.hpp>

namespace cpcl {

extern unsigned char const charsASCII[];
inline bool striASCIIequal(char const *a, char const *b) {
  unsigned ac = charsASCII[(unsigned char)*a], bc = charsASCII[(unsigned char)*b];
  bool r = (ac == bc);
  while (((ac & bc) != 0) && (r)) {
    ac = charsASCII[(unsigned char)*++a]; bc = charsASCII[(unsigned char)*++b];
    r = (ac == bc);
  }
  return r;
}
inline bool striASCIIequal(cpcl::StringPiece const &a_, cpcl::StringPiece const &b_) {
  size_t const size = a_.size();
  if (size != b_.size())
    return false;

  char const *a = a_.data(), *b = b_.data();
  bool r(true);
  for (size_t i = 0; r && (i < size); ++i) {
    r = (charsASCII[(unsigned char)a[i]] == charsASCII[(unsigned char)b[i]]);
  }
  return r;
}
inline bool striASCIIequal(wchar_t const *a, wchar_t const *b) {
  wchar_t ac, bc;
  bool r;
  do {
    ac = *a++; bc = *b++;
    if ((ac >= L'A') && (ac <= L'Z'))
      ac = (L'a' + (ac - L'A'));
    if ((bc >= L'A') && (bc <= L'Z'))
      bc = (L'a' + (bc - L'A'));

    r = (ac == bc);
  } while (((ac & bc) != 0) && (r));
  return r;
}
inline bool striASCIIequal(cpcl::WStringPiece const &a_, cpcl::WStringPiece const &b_) {
  size_t const size = a_.size();
  if (size != b_.size())
    return false;

  wchar_t const *a = a_.data(), *b = b_.data();
  bool r(true);
  for (size_t i = 0; r && (i < size); ++i) {
    wchar_t ac = a[i], bc = b[i];
    if ((a[i] >= L'A') && (a[i] <= L'Z'))
      ac = (L'a' + (a[i] - L'A'));
    if ((b[i] >= L'A') && (b[i] <= L'Z'))
      bc = (L'a' + (b[i] - L'A'));

    r = (ac == bc);
  }
  return r;
}

extern int const CP1251_UTF16_TAB[256];
inline int CP1251_UTF16(unsigned char c) {
  return CP1251_UTF16_TAB[c];
}

// kov_serg
extern int const UTF16_CP1251_TAB_0[28];
extern unsigned char const UTF16_CP1251_TAB_1[];
extern unsigned char const UTF16_CP1251_TAB_2[];
inline unsigned char UTF16_CP1251(int u) {
  if (u < 0x007F) return (unsigned char)u;
  if (u < 0x2000) {
    if (u >= 0x00A0 && u <= 0x00BB) {
      return UTF16_CP1251_TAB_0[u - 0x00A0] ? (unsigned char)u : 0;
    }
    if (u <= 0x45F) {
      if (u >= 0x0401) {
        return (unsigned char)UTF16_CP1251_TAB_1[u - 0x0401];
      }
    }
    else {
      if (u == 0x0490) return (unsigned char)0xA5;
      if (u == 0x0491) return (unsigned char)0xB4;
    }
  }
  else {
    if (u < 0x2116) {
      if (u >= 0x2013 && u <= 0x203A) {
        return (unsigned char)UTF16_CP1251_TAB_2[u - 0x2013];
      }
      if (u == 0x20AC) return (unsigned char)0x88;
      if (u == 0x20DE) return (unsigned char)0x98; // officially undefined. replased by box symbol
    }
    else {
      if (u == 0x2116) return (unsigned char)0xB9;
      if (u == 0x2122) return (unsigned char)0x99;
    }
  }
  return 0;
}
// kov_serg

inline unsigned char* ConvertUTF16_CP1251(wchar_t const *s, size_t s_len, unsigned char *r, size_t r_len) {
  for (wchar_t const * const tail = s + (std::min)(s_len, r_len); s != tail; ++s, ++r)
    if ((*r = UTF16_CP1251(*s)) == 0)
      *r = '?';
  return r;
}
inline unsigned char* ConvertUTF16_CP1251(WStringPiece const &s, unsigned char *r) {
  return ConvertUTF16_CP1251(s.data(), s.size(), r, s.size());
}
inline std::string/*RVO*/ ConvertUTF16_CP1251(WStringPiece const &s) {
  std::string r; r.resize(s.size());
  /*char *s_ = const_cast<char*>(r.c_str());
  for (wchar_t const *i = s.data(), * const tail = s.data() + s.size(); i != tail; ++i, ++s_)
  *s_ = uc_to_cp1251(*i);*/
  ConvertUTF16_CP1251(s.data(), s.size(), (unsigned char*)r.c_str(), s.size());
  return r;
}
inline bool TryConvertUTF16_CP1251(WStringPiece const &s, std::string *r) {
  std::string output; output.resize(s.size());
  unsigned char *j = (unsigned char*)output.c_str();
  wchar_t const *i = s.data();
  for (wchar_t const * const tail = s.data() + s.size(); i != tail; ++i, ++j) {
    unsigned char const c = UTF16_CP1251(*i);
    if (c == 0)
      return false;
    *j = c;
  }
  if (r)
    (*r).swap(output);
  return true;
}

inline wchar_t* ConvertCP1251_UTF16(unsigned char const *s, size_t s_len, wchar_t *r, size_t r_len) {
  for (unsigned char const * const tail = s + (std::min)(s_len, r_len); s != tail; ++s, ++r)
    *r = (wchar_t)CP1251_UTF16(*s);
  return r;
}
inline wchar_t* ConvertCP1251_UTF16(StringPiece const &s, wchar_t *r) {
  return ConvertCP1251_UTF16((unsigned char const*)s.data(), s.size(), r, s.size());
}
inline std::wstring ConvertCP1251_UTF16(StringPiece const &s) {
  std::wstring r; r.resize(s.size());
  ConvertCP1251_UTF16((unsigned char const*)s.data(), s.size(), (wchar_t*)r.c_str(), s.size());
  return r;
}

//template<class InputCharType, class OutputCharType>
//inline OutputCharType* Convert(InputCharType const *input, size_t input_len,
//															OutputCharType *output, size_t output_len,
//	/* require function address - compiler unable to inline conversion function */ OutputCharType (&f)(InputCharType)) {
//	size_t const len = min(input_len, output_len);
//	for (InputCharType const * const tail = input + len; input != tail; ++input, ++output)
//		*output = f(*input);
//	return output;
//}

inline int uc_to_utf(int w, unsigned char *p) {
  int r, n;
  if (w & 0x80000000) return 0;
  if (w & 0xFC000000) n = 6; else
    if (w & 0xFFE00000) n = 5; else
      if (w & 0xFFFF0000) n = 4; else
        if (w & 0xFFFFF800) n = 3; else
          if (w & 0xFFFFFF80) n = 2; else { p[0] = char(w); return 1; }
  r = n; while (n > 1) { p[--n] = (char)((w & 0x3F) | 0x80); w >>= 6; }
  p[0] = (char)((255 << (8 - r)) | w); return r;
}
// kov_serg

inline int utf_to_uc(unsigned char const *p, int *w, int lim) {
  if (lim < 1) return 0;
  int z = (unsigned char)p[0];
  if ((z & 0x80) == 0) { if (w) *w = z; return 1; }
  int n = 0, q = 0x80; do { n++; q >>= 1; } while (z&q);
  if (n < 2 || lim < n) return 0;     // invalid or no room
  int r = z&(q - 1); if (r == 0) return 0; // prohibited coding 
  int len = n; while (n > 1) {
    z = *++p; if ((z & 0xC0) != 0x80) return 0; // invalid coding
    r = (r << 6) | (z & 0x3F); n--;
  }
  if (w) *w = r; return len;
}
// kov_serg

// -1-can not be utf, 0-could be utf, but no utf symbols, 1-valid utf and utf symbols found
inline int could_be_utf(unsigned char const *s, int len) {
  int res = -1, i = 0;
  while (i < len) {
    int w, k = utf_to_uc(s + i, &w, len - i);
    if (k == 0 || w > 65535) return -1;
    if (k > 1) res = 0;
    i += k;
  }
  return ++res;
}
// kov_serg

bool TryConvertUTF8_UTF16(StringPiece const &s, std::wstring *r);
/* returns:
 r_size <= r_len, if success
 required r_size > r_len, if success and more storage needed, r contain data
 -1, if error, r probaly contain data*/
int TryConvertUTF8_UTF16(StringPiece const &s, wchar_t *r, size_t r_len);
std::string ConvertUTF16_UTF8(WStringPiece const &s);
inline unsigned char* ConvertUTF16_UTF8(wchar_t const *s, size_t s_len, unsigned char *r, size_t r_len) {
  unsigned char buf[0x10];
  for (wchar_t const * const tail = s + s_len; s != tail; ++s) {
    int k_ = uc_to_utf(*s, buf);
    size_t k = (k_ < 0) ? 0 : (size_t)k_;
    if (r_len >= k) {
      memcpy(r, buf, k);
      r += k;
      r_len -= k;
    }
    else
      break;
  }
  return r;
}
/* returns:
 r_size <= r_len, if success
 required r_size > r_len, if success and more storage needed, r contain data
 -1, if error, r probaly contain data*/
int TryConvertUTF16_UTF8(wchar_t const *s, size_t s_len, unsigned char *r, size_t r_len);

} // namespace cpcl

#endif // __CPCL_STRING_UTIL_H

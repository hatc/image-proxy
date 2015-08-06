#include "basic.h"
#include <math.h>

#include "string_cast.hpp"

#include <cassert>
#include <iostream>

template<class T, class CharType>
inline void assert_convert(cpcl::BasicStringPiece<CharType, std::char_traits<CharType> > const &s, bool convert, T value = T()) {
  T r;
  assert(/*cpcl:: ADL))*/TryConvert(s, &r) == convert);
  if (convert) {
    assert(r == value);
  }
}

inline bool nearly_equal(double a, double b) {
  static double const epsilon = 0.0001;
  return (fabs(a - b) < epsilon);
}
template<class T, class CharType>
inline void assert_convert_d(cpcl::BasicStringPiece<CharType, std::char_traits<CharType> > const &s, bool convert, T value = T()) {
  T r;
  assert(TryConvert(s, &r) == convert);
  if (convert) {
    assert(nearly_equal(r, value));
  }
}

void test_convert() {
  {
   assert_convert<unsigned long>(cpcl::WStringPiece(L""), false);
   assert_convert<unsigned long>(cpcl::WStringPiece(L"0"), true, 0);
   assert_convert<unsigned long>(cpcl::WStringPiece(L"1"), true, 1);
   assert_convert<unsigned long>(cpcl::WStringPiece(L"+1"), true, 1);
   assert_convert<unsigned long>(cpcl::WStringPiece(L"-1"), false);
   assert_convert<unsigned long>(cpcl::WStringPiece(L"127"), true, 127);
   assert_convert<unsigned long>(cpcl::WStringPiece(L"-127"), false);
   assert_convert<unsigned long>(cpcl::WStringPiece(L"2147483646"), true, 2147483646);
   assert_convert<unsigned long>(cpcl::WStringPiece(L"2147483647"), true, 2147483647);
   assert_convert<unsigned long>(cpcl::WStringPiece(L"2147483648"), true, 2147483648);
   assert_convert<unsigned long>(cpcl::WStringPiece(L"2147483649"), true, 2147483649);
   assert_convert<unsigned long>(cpcl::WStringPiece(L"-2147483649"), false);
   assert_convert<unsigned long>(cpcl::WStringPiece(L"-2147483648"), false);
   assert_convert<unsigned long>(cpcl::WStringPiece(L"-2147483647"), false);
   assert_convert<unsigned long>(cpcl::WStringPiece(L"-2147483646"), false);
 }
 {
   assert_convert<long>(cpcl::WStringPiece(L""), false);
   assert_convert<long>(cpcl::WStringPiece(L"0"), true, 0);
   assert_convert<long>(cpcl::WStringPiece(L"1"), true, 1);
   assert_convert<long>(cpcl::WStringPiece(L"+1"), true, 1);
   assert_convert<long>(cpcl::WStringPiece(L"-1"), true, -1);
   assert_convert<long>(cpcl::WStringPiece(L"127"), true, 127);
   assert_convert<long>(cpcl::WStringPiece(L"-127"), true, -127);
   assert_convert<long>(cpcl::WStringPiece(L"2147483646"), true, 2147483646);
   assert_convert<long>(cpcl::WStringPiece(L"2147483647"), false);
   assert_convert<long>(cpcl::WStringPiece(L"2147483648"), false);
   assert_convert<long>(cpcl::WStringPiece(L"2147483649"), false);
   assert_convert<long>(cpcl::WStringPiece(L"-2147483649"), false);
   assert_convert<long>(cpcl::WStringPiece(L"-2147483648"), false);
   assert_convert<long>(cpcl::WStringPiece(L"-2147483647"), true, -2147483647);
   assert_convert<long>(cpcl::WStringPiece(L"-2147483646"), true, -2147483646);
 }

 {
   char buf[32]; double v = 123.458934;
   _snprintf_s(buf, _TRUNCATE, "%e", v);
   assert_convert_d<double>(cpcl::StringPiece(buf), true, v);
   _snprintf_s(buf, _TRUNCATE, "%E", v);
   assert_convert_d<double>(cpcl::StringPiece(buf), true, v);
   _snprintf_s(buf, _TRUNCATE, "%f", v);
   assert_convert_d<double>(cpcl::StringPiece(buf), true, v);
   _snprintf_s(buf, _TRUNCATE, "%g", v);
   assert_convert_d<double>(cpcl::StringPiece(buf), true, v);
   _snprintf_s(buf, _TRUNCATE, "%G", v);
   assert_convert_d<double>(cpcl::StringPiece(buf), true, v);
 }
 {
   wchar_t buf[32]; double v = -62.5423;
   _snwprintf_s(buf, _TRUNCATE, L"%e", v);
   assert_convert_d<double>(cpcl::WStringPiece(buf), true, v);
   _snwprintf_s(buf, _TRUNCATE, L"%E", v);
   assert_convert_d<double>(cpcl::WStringPiece(buf), true, v);
   _snwprintf_s(buf, _TRUNCATE, L"%f", v);
   assert_convert_d<double>(cpcl::WStringPiece(buf), true, v);
   _snwprintf_s(buf, _TRUNCATE, L"%g", v);
   assert_convert_d<double>(cpcl::WStringPiece(buf), true, v);
   _snwprintf_s(buf, _TRUNCATE, L"%G", v);
   assert_convert_d<double>(cpcl::WStringPiece(buf), true, v);
 }

 {
   unsigned long r;
   if (cpcl::TryConvert(cpcl::StringPiece("abc"), &r))
     std::cout << r << std::endl;
   if (cpcl::TryConvert(cpcl::StringPiece("    100"), &r))
     std::cout << r << std::endl;
   if (cpcl::TryConvert(cpcl::StringPiece("    900"), &r))
     std::cout << r << std::endl;
   if (cpcl::TryConvert(cpcl::StringPiece("    900    "), &r))
     std::cout << r << std::endl;
   if (cpcl::TryConvert(cpcl::StringPiece("  -  900    "), &r))
     std::cout << r << std::endl;
 }
  std::cout << "\n";
  {
    double r;
    if (cpcl::TryConvert(cpcl::StringPiece("abc"), &r))
      std::cout << r << std::endl;
    if (cpcl::TryConvert(cpcl::StringPiece(" "), &r))
      std::cout << r << std::endl;
    if (cpcl::TryConvert(cpcl::StringPiece("0"), &r))
      std::cout << r << std::endl;
    if (cpcl::TryConvert(cpcl::StringPiece("1.384593"), &r))
      std::cout << r << std::endl;
    if (cpcl::TryConvert(cpcl::StringPiece("-1.384593"), &r))
      std::cout << r << std::endl;
  }
}

#include <cassert>

#include <sstream>
#include <iostream>

#include "timer.h"
#include "string_util.h"
#include "formatidiv.hpp"

using namespace cpcl;

template<size_t Base, class ValueType>
struct test_formatidiv_impl {
  ValueType v;
  size_t width;
  
  operator bool() {
    std::stringstream s; s.width(width);
    s << v;

    char buf[0x20];
    *formatidiv<Base>(buf, v, width, ' ') = '\0';

    if (!striASCIIequal(s.str().c_str(), buf)) {
      std::cout << "error: \n" << s.str() << std::endl << buf << std::endl;
      return false;
    }
    return true;
  }

  test_formatidiv_impl(ValueType v_, size_t width_) : v(v_), width(width_) {}
};
template<class ValueType>
struct test_formatidiv_impl<16, ValueType> {
  ValueType v;
  size_t width;
  
  operator bool() {
    std::stringstream s; s.width(width);
    s << std::hex << v;

    char buf[0x20];
    *formatidiv<16>(buf, v, width, ' ') = '\0';

    if (!striASCIIequal(s.str().c_str(), buf)) {
      std::cout << "error: \n" << s.str() << std::endl << buf << std::endl;
      return false;
    }
    return true;
  }

  test_formatidiv_impl(ValueType v_, size_t width_) : v(v_), width(width_) {}
};

template<size_t Base, class ValueType>
inline test_formatidiv_impl<Base, ValueType> make_test_formatidiv(ValueType v, size_t width) {
  return test_formatidiv_impl<Base, ValueType>(v, width);
}

//void test_formatidiv() {
//	assert(make_test_formatidiv<16>(static_cast<const int>(-1), 8));
//	assert(make_test_formatidiv<16>(static_cast<int>(-1), 8));
//	assert(make_test_formatidiv<10>(static_cast<int>(1), 8));
//	assert(make_test_formatidiv<10>(static_cast<int>(-1), 8));
//	assert(make_test_formatidiv<16>(static_cast<int>(1), 8));
//	// assert(make_test_formatidiv<16>(static_cast<int>(-1), 4)); - formatidiv<16> produce correct value 'FFFF'
//	assert(make_test_formatidiv<10>(static_cast<short>(-1), 4));
//	assert(make_test_formatidiv<10>(static_cast<long>(-123), 8));
//	assert(make_test_formatidiv<10>(static_cast<const unsigned int>(-1), 16));
//	assert(make_test_formatidiv<10>(static_cast<unsigned int>(-1), 16));
//	assert(make_test_formatidiv<16>(static_cast<const unsigned int>(-1), 8));
//	assert(make_test_formatidiv<16>(static_cast<unsigned int>(-1), 8));
//	assert(make_test_formatidiv<10>(static_cast<unsigned int>(1), 8));
//	assert(make_test_formatidiv<16>(static_cast<unsigned __int64>(-1), 16));
//	// assert(make_test_formatidiv<16>(static_cast<char>(-1), 4)); - formatidiv<16> produce correct value 'FF'
//	assert(make_test_formatidiv<16>(static_cast<short>(-1), 4));
//	assert(make_test_formatidiv<16>(static_cast<__int64>(-1), 16));
//}

double bench_formatidiv_idiv_dec(int v) {
  char buf[0x10];
  timer t;
  for (size_t i = 0; i < 0x1000; ++i) {
    *formatidiv<10>(buf, v, 4, '0') = '\0';
  }
  double r = t.elapsed();
  std::cout.width(30);
  std::cout << "bench_formatidiv_idiv_dec" << " : " << buf << std::endl;
  return r;
}
double bench_snprintf_idiv_dec(int v) {
  char buf[0x10];
  timer t;
  for (size_t i = 0; i < 0x1000; ++i) {
    _snprintf_s(buf, _TRUNCATE, "%04d", v);
  }
  double r = t.elapsed();
  std::cout.width(30);
  std::cout << "bench_snprintf_idiv_dec" << " : " << buf << std::endl;
  return r;
}
double bench_formatidiv_hex(int v) {
  char buf[0x10];
  timer t;
  for (size_t i = 0; i < 0x1000; ++i) {
    *formatidiv<16>(buf, v, 8) = '\0';
  }
  double r = t.elapsed();
  std::cout.width(30);
  std::cout << "bench_formatidiv_hex" << " : " << buf << std::endl;
  return r;
}
double bench_snprintf_hex(int v) {
  char buf[0x10];
  timer t;
  for (size_t i = 0; i < 0x1000; ++i) {
    _snprintf_s(buf, _TRUNCATE, "%08X", v);
  }
  double r = t.elapsed();
  std::cout.width(30);
  std::cout << "bench_snprintf_hex" << " : " << buf << std::endl;
  return r;
}
double bench_formatidiv_idiv_oct(int v) {
  char buf[0x10];
  timer t;
  for (size_t i = 0; i < 0x1000; ++i) {
    *formatidiv<8>(buf, (unsigned int)v, 8) = '\0';
  }
  double r = t.elapsed();
  std::cout.width(30);
  std::cout << "bench_formatidiv_idiv_oct" << " : " << buf << std::endl;
  return r;
}
double bench_snprintf_idiv_oct(int v) {
  char buf[0x10];
  timer t;
  for (size_t i = 0; i < 0x1000; ++i) {
    _snprintf_s(buf, _TRUNCATE, "%08o", v);
  }
  double r = t.elapsed();
  std::cout.width(30);
  std::cout << "bench_snprintf_idiv_oct" << " : " << buf << std::endl;
  return r;
}

void test_formatidiv_() {
  int v = -100;
  double r_0[9], r_1[9];
  {
    r_0[0] = bench_formatidiv_idiv_dec(v);
    r_1[0] = bench_snprintf_idiv_dec(v);
  }
 {
   v = 1234;
   r_1[1] = bench_snprintf_idiv_dec(v);
   r_0[1] = bench_formatidiv_idiv_dec(v);
 }
 {
   v = -61;
   r_0[2] = bench_formatidiv_idiv_dec(v);
   r_1[2] = bench_snprintf_idiv_dec(v);
 }
 {
   v = -161;
   r_0[3] = bench_formatidiv_idiv_oct(v);
   r_1[3] = bench_snprintf_idiv_oct(v);
 }
  double v_snprintf_s = (r_1[0] + r_1[1] + r_1[2] + r_1[3]) / 4;
  double v_formati = (r_0[0] + r_0[1] + r_0[2] + r_0[3]) / 4;

  std::cout.width(20); std::cout << "snprintf_s" << " : " << std::fixed << v_snprintf_s << std::endl;
  std::cout.width(20); std::cout << "formati" << " : " << std::fixed << v_formati << std::endl;
  /*
  1550
  58
  */
}

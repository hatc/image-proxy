// formatted_exception.hpp
#pragma once

#include <stdio.h>
#include <memory.h>

#include <cstdarg>
#include <exception>

#include <cpcl/string_util.hpp> // StringFormatImpl

template<class T>
class formatted_exception : public std::exception {
protected:
  char s[0x100];
public:
  formatted_exception(char const *s_ = NULL) {
    if (s_) {
      size_t len = strlen(s_);
      if (len >= sizeof(s))
        len = sizeof(s) - 1;
      memcpy(s, s_, len);
      s[len] = 0;
    } else
      *s = 0;
  }
#if defined(_MSC_VER)
  virtual char const* what() const {
#else
  virtual char const* what() const throw() {
#endif
    return s;
  }
  
  static void throw_formatted(T e, char const *format, ...) {
    va_list arguments;
    va_start(arguments, format);
    cpcl::StringFormatImpl(e.s, sizeof(e.s), format, arguments, 0);
    va_end(arguments);
    
    throw e;
  }
};

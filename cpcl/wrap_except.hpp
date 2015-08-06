// wrap_except.hpp
#pragma once

#include <cstdarg>

#include <boost/function.hpp>

enum SEH_EXCEPTION_TYPE {
  SEH_NONE = 0,
  SEH_STACK_OVERFLOW,
  SEH_ACCESS_VIOLATION,
  SEH_EXCEPTION
};

int WrapExcept(boost::function<void()> *f);

class seh_exception {
  char s[0x100];
public:
  seh_exception(char const *s_ = NULL) {
    if (s_) {
      size_t len = strlen(s_);
      if (len >= sizeof(s))
        len = sizeof(s) - 1;
      memcpy(s, s_, len);
      s[len] = 0;
    }
    else
      *s = 0;
  }

  char const* what() const {
    return s;
  }

  static void throw_formatted(char const *format, ...) {
    seh_exception e;

    va_list args;
    va_start(args, format);
    if (_vsnprintf_s(e.s, _TRUNCATE, format, args) < 0)
      e.s[sizeof(e.s) - 1] = 0;
    va_end(args);

    throw e;
  }
};

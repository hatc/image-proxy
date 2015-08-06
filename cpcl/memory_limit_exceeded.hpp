// memory_limit_exceeded.hpp
#pragma once

#include <cpcl/formatted_exception.hpp>

class memory_limit_exceeded : public formatted_exception<memory_limit_exceeded> {
public:
  size_t memory_limit;
  
  memory_limit_exceeded(size_t memory_limit_) : memory_limit(memory_limit_)
  {}
#if defined(_MSC_VER)
  virtual char const* what() const {
#else
  virtual char const* what() const throw() {
#endif
    char const *s = formatted_exception<memory_limit_exceeded>::what();
    if (*s)
      return s;
    else
      return "memory_limit_exceeded";
  }
};


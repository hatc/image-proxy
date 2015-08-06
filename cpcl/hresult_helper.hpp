// hresult_helper.hpp
#pragma once

#include <cpcl/formatted_exception.hpp>

class hresult_exception : public formatted_exception < hresult_exception > {
public:
  long ecode;

  hresult_exception(long ecode_) : ecode(ecode_) {}
  virtual char const* what() const {
    char const *s = formatted_exception<hresult_exception>::what();
    if (*s)
      return s;
    else
      return "hresult_exception";
  }
};

#ifdef THROW_FAILED
#undef THROW_FAILED
#endif
#ifdef THROW_FAILED_
#undef THROW_FAILED_
#endif
#ifdef THROW_FAILED_LOCAL_NAME
#undef THROW_FAILED_LOCAL_NAME
#endif

#define THROW_FAILED_LOCAL_NAME(n) hr##n
#define THROW_FAILED_(Expr, n) long THROW_FAILED_LOCAL_NAME(n) = (Expr);\
  if (THROW_FAILED_LOCAL_NAME(n) < 0)\
  hresult_exception::throw_formatted(hresult_exception(THROW_FAILED_LOCAL_NAME(n)), #Expr " fails with code: 0x%08X", THROW_FAILED_LOCAL_NAME(n))
#define THROW_FAILED(Expr) THROW_FAILED_(Expr, __COUNTER__)

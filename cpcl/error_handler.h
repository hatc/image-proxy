// error_handler.h
#pragma once

#ifndef __CPCL_ERROR_HANDLER_H
#define __CPCL_ERROR_HANDLER_H

#include <stddef.h>

namespace cpcl {

#ifdef _MSC_VER
typedef void (__stdcall *ErrorHandlerPtr)(char const *s, size_t s_length);
#else
typedef void (*ErrorHandlerPtr)(char const *s, size_t s_length);
#endif

void SetErrorHandler(ErrorHandlerPtr v);

void ErrorHandler(char const *s, size_t s_length);
template<size_t N>
inline void ErrorHandler(char const (&s)[N]) {
  size_t s_length(N);
  if (s[s_length - 1] == '\0')
    --s_length;
  ErrorHandler(s, s_length);
}

} // namespace cpcl

#endif // __CPCL_ERROR_HANDLER_H

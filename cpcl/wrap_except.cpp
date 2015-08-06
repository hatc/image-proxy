#include "wrap_except.hpp"

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <excpt.h>

static int WrapExceptSF(boost::function<void()> *f) {
  __try {
    (*f)();
    return SEH_NONE;
  } __except (GetExceptionCode() == EXCEPTION_STACK_OVERFLOW ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
    return SEH_STACK_OVERFLOW;
  }
}

static int WrapExceptAV(boost::function<void()> *f) {
  __try {
    return WrapExceptSF(f);
  } __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
    return SEH_ACCESS_VIOLATION;
  }
}

int WrapExcept(boost::function<void()> *f) {
  __try {
    return WrapExceptAV(f);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return SEH_EXCEPTION;
  }
}

#include "basic.h"

#include <iostream>

#include "error_handler.h"

namespace cpcl {

#ifdef _MSC_VER
static void __stdcall _DefaultErrorHandler(char const *s, size_t s_length) {
  std::cout << "ErrorHandler :: \n";
  if (s_length > 2 && s[s_length - 2] == '\r' && s[s_length - 1] == '\n')
    s_length -= 2;
  std::cout.write(s, static_cast<std::streamsize>(s_length));
  std::cout << std::endl;
}
//static void _DefaultErrorHandler(char const *s, unsigned int s_length) {
//	LPSTR s_ = (LPSTR)::HeapAlloc(::GetProcessHeap(), 0, s_length + 1);
//	if (s_) {
//		memcpy(s_, s, s_length);
//		s_[s_length] = '\0';
//		::MessageBoxA(NULL, s_, NULL, MB_ICONERROR | MB_SERVICE_NOTIFICATION);
//		::HeapFree(::GetProcessHeap(), 0, s_);
//	}
//}
#else
static void _DefaultErrorHandler(char const *s, size_t s_length) {
  std::cerr << "ErrorHandler :: \n";
  if (s_length > 1 && s[s_length - 1] == '\n')
    s_length -= 1;
  std::cerr.write(s, static_cast<std::streamsize>(s_length));
  std::cerr << std::endl;
}
#endif

static ErrorHandlerPtr ERROR_HANDLER = _DefaultErrorHandler;

void SetErrorHandler(ErrorHandlerPtr v) {
  ERROR_HANDLER = v;
}

void ErrorHandler(char const *s, size_t s_length) {
  if (ERROR_HANDLER)
    ERROR_HANDLER(s, s_length);
}

} // namespace cpcl

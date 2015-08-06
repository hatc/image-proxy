#include "basic.h"

#include <stdexcept>

#include "dumbassert.h"
#include "trace.h"

#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOGDICAPMASKS
#define NOMETAFILE
#define NOMINMAX
#define NORASTEROPS
#define NOSCROLL
#define NOSOUND
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOMENUS
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define NOSHOWWINDOW
#define NOCLIPBOARD
#define NOCOLOR
#define NOSYSMETRICS
#define NOTEXTMETRIC
#define NOCTLMGR
#define NODRAWTEXT
#define NOMB
#define NOWH
#define NOWINOFFSETS
#define NOCOMM
#define NOKANJI
#define NOCRYPT
#define NOSERVICE
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX
#define NOIME
#include <windows.h>

namespace cpcl {

static void __stdcall _DefaultAssertHandler(char const *s, char const *file, unsigned int line) {
  cpcl::Trace(CPCL_TRACE_LEVEL_ERROR, "%s at %s:%d epic fail, process terminates", s, file, line);
  
  HANDLE hProcess = ::GetCurrentProcess();
  ::TerminateProcess(hProcess, (UINT)-1);
  ::WaitForSingleObject(hProcess, INFINITE);
}

static AssertHandlerPtr ASSERT_HANDLER = _DefaultAssertHandler;

void SetAssertHandler(AssertHandlerPtr v) {
  ASSERT_HANDLER = v;
}

void dumbassert(char const *s, char const *file, unsigned int line) {
  if (ASSERT_HANDLER)
    ASSERT_HANDLER(s, file, line);
  else
    throw std::runtime_error(s);
}

} // namespace cpcl

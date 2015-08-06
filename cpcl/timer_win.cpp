#include "basic.h"

#include "timer.h"

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

timer::timer() : counts_per_second(0) {
  LARGE_INTEGER v;
  if (::QueryPerformanceFrequency(&v))
    counts_per_second = v.QuadPart;
  if (counts_per_second)
    restart();
  else
    counts_per_second = 1; /* prevent fp division by zero - If the installed hardware does not support a high-resolution performance counter */
}

void timer::restart() { // postcondition: elapsed()==0
  LARGE_INTEGER v;
  ::QueryPerformanceCounter(&v);
  start_time = v.QuadPart;
}

double timer::elapsed() const { // return elapsed time in seconds
  LARGE_INTEGER v;
  ::QueryPerformanceCounter(&v);
  return double(v.QuadPart - start_time) / counts_per_second;
}

unsigned int TimedSeed() {
  LARGE_INTEGER v;
  if (::QueryPerformanceCounter(&v) == 0)
    v.LowPart = ::GetTickCount();
  return v.LowPart;
}

} // namespace cpcl

#include "named_os_mutex.hpp"
#include "trace.h"

#include <windows.h>

namespace cpcl {
  
NamedMutexOS::NamedMutexOS(wchar_t const *name) : abadoned(false), mutex_os(NULL), ecode(0) {
  mutex_os = ::CreateMutexW(NULL, FALSE, name);
  if (!mutex_os) {
    ecode = ::GetLastError();
    if (ecode == ERROR_ACCESS_DENIED) {
      mutex_os = ::OpenMutexW(SYNCHRONIZE, FALSE, name);
      if (!mutex_os)
        ecode = ::GetLastError();
    }
  }
}

NamedMutexOS::~NamedMutexOS() {
  if (mutex_os)
    ::CloseHandle(mutex_os);
}

bool NamedMutexOS::timed_lock_(unsigned long milliseconds) {
  if (!mutex_os) {
    ErrorSystem(ecode, "NamedMutexOS(): CreateMutexW/OpenMutexW fails:");
    throw boost::thread_resource_error(ecode);
  }

  unsigned long const r = ::WaitForSingleObject(mutex_os, milliseconds);
  if (r == WAIT_OBJECT_0) {
    return true;
  }
  else if (r == WAIT_ABANDONED) {
    return (abadoned = true);
  }
  else if (r != WAIT_TIMEOUT) {
    throw boost::thread_resource_error(::GetLastError());
  }

  return false;
}

void NamedMutexOS::unlock() {
  if (!mutex_os) {
    ErrorSystem(ecode, "NamedMutexOS(): CreateMutexW/OpenMutexW fails:");
    throw boost::thread_resource_error(ecode);
  }

  ::ReleaseMutex(mutex_os);
}

}  // namespace cpcl

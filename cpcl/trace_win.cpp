#include "basic.h"

#include <stdarg.h>
#include <stdio.h>

#ifdef CPCL_USE_BOOST_THREAD_SLEEP
#include <boost/thread/thread.hpp>
#endif
#include <boost/thread/mutex.hpp>

#include <windows.h>

#include "trace.h"
#include "formatidiv.hpp"
#include "string_util.h"
#include "error_handler.h"

namespace cpcl {

#if defined(_DEBUG) || defined(DEBUG)
int TRACE_LEVEL = CPCL_TRACE_LEVEL_ERROR | CPCL_TRACE_LEVEL_WARNING | CPCL_TRACE_LEVEL_DEBUG;
#else
int TRACE_LEVEL = CPCL_TRACE_LEVEL_ERROR | CPCL_TRACE_LEVEL_WARNING;
#endif
wchar_t const *TRACE_FILE_PATH = (wchar_t const *)0;

void SetTraceFilePath(WStringPiece const &v) {
  if (TRACE_FILE_PATH)
    delete [] TRACE_FILE_PATH;
  
  size_t len = v.size();
  if (len) {
    wchar_t *s = new wchar_t[len + 1];
    wcscpy_s(s, len + 1, v.data());
    TRACE_FILE_PATH = s;
  } else
    TRACE_FILE_PATH = 0;
}

static boost::timed_mutex TRACE_FILE_MUTEX;
//static NamedMutexOS TRACE_FILE_MUTEX(L"Local\\Trace2394mcw942q3829485043m");
static inline void WriteString(CPCL_TRACE_LEVEL l, char const *s, size_t size) {
  DWORD error_code(ERROR_SUCCESS);
  StringPiece error;
  if (TRACE_FILE_PATH) {
    // boost::unique_lock<NamedMutexOS> lock(TRACE_FILE_MUTEX, boost::try_to_lock);
    // no reason to use MutexOS, coz shared resource(i.e. hFile) anyway lost on TerminateThread
    // so, we could detect dead lock with MutexOS thanks to WAIT_ABANDONED_0, but it does a little help...
    boost::unique_lock<boost::timed_mutex> lock(TRACE_FILE_MUTEX, boost::try_to_lock);
    if ((!lock) && (!lock.timed_lock(boost::posix_time::seconds(1)))) {
      error = StringPiece("TRACE_FILE_MUTEX locked");
    } else {
      HANDLE hFile = ::CreateFileW(TRACE_FILE_PATH, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
        (LPSECURITY_ATTRIBUTES)NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
      if (INVALID_HANDLE_VALUE == hFile) {
        error_code = ::GetLastError();
        if (ERROR_SHARING_VIOLATION == error_code) {
#ifdef CPCL_USE_BOOST_THREAD_SLEEP
          boost::this_thread::sleep(boost::posix_time::milliseconds(0x100)); /* let's take a nap ^_^ */
#else
          ::SleepEx(0x100, TRUE); /* let's take a nap ^_^ */
#endif
          hFile = ::CreateFileW(TRACE_FILE_PATH, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
            (LPSECURITY_ATTRIBUTES)NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
          error_code = ERROR_SUCCESS;
        }
      }
      if (INVALID_HANDLE_VALUE == hFile) {
        if (ERROR_SUCCESS == error_code)
          error_code = ::GetLastError();
        error = StringPieceFromLiteral("unable to open log file for writing:");
      } else {
        /* two IO function call with acquired shared resource - hFile
        if thread has terminated with exception at this IO operations, hFile still open - log locked
        so probaly a good idea add scope_guard(CloseHandle, hFile) */
        LONG unused(0);
        /* If lpDistanceToMoveHigh is NULL and the new file position does not fit in a 32-bit value, 
        the function fails and returns INVALID_SET_FILE_POINTER. */
        ::SetFilePointer(hFile, 0, &unused, FILE_END);
        DWORD bytes_written(0);
        BOOL r = ::WriteFile(hFile, s, size, &bytes_written, NULL);
        if ((FALSE == r) || (bytes_written < size)) {
          error_code = ::GetLastError();
          error = StringPieceFromLiteral("cannot write to log file:");
        }
        ::CloseHandle(hFile);
      }
    }
  } else if (l >= CPCL_TRACE_LEVEL_ERROR)
    error = StringPieceFromLiteral("TRACE_FILE_PATH is not set");
  
  if (!error.empty()) {
    std::string error_buf;
    if (TRACE_FILE_PATH) {
      size_t len = wcslen(TRACE_FILE_PATH);
      error_buf.assign(len + 1, '\n');
      ConvertUTF16_CP1251(WStringPiece(TRACE_FILE_PATH, len), (unsigned char*)(error_buf.c_str() + 1));
    }
    if (error_code != ERROR_SUCCESS) {
      void *text(0);
      DWORD n = ::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, error_code,
        MAKELANGID(LANG_ENGLISH/*LANG_NEUTRAL*/, SUBLANG_ENGLISH_US/*SUBLANG_NEUTRAL*/),
        (LPSTR)&text, 0, NULL);
      if ((n > 0) && (text)) {
        error_buf += '\n';
        error_buf.append((char*)text, n);
        ::LocalFree((HLOCAL)text);
      }
    }
    if (!error_buf.empty()) {
      error_buf.insert(0, error.data(), error.size());
      error = StringPiece(error_buf);
    }
    
    if (TRACE_FILE_PATH) /* StringPiece("TRACE_FILE_PATH is not set") should never traced ))) */
      ErrorHandler(error.data(), error.size());
    ErrorHandler(s, size);
  }
}

static inline char const* TraceLevelNames(CPCL_TRACE_LEVEL v) {
  if (CPCL_TRACE_LEVEL_INFO == v)
    return "[notice] ";
  else if (CPCL_TRACE_LEVEL_DEBUG == v)
    return "[debug]  ";
  else if (CPCL_TRACE_LEVEL_WARNING == v)
    return "[warning]";
  else if (CPCL_TRACE_LEVEL_ERROR == v)
    return "[error]  ";
  else
    return "  [^_^]  ";
}
static size_t const HEADER_LENGTH = 45;
static inline char* FormatHeader(char *s_, CPCL_TRACE_LEVEL l, SYSTEMTIME t, DWORD processId, DWORD threadId) {
  char *s = formatidiv<10>(s_, t.wYear, 4);
  *s = '/';
  s = formatidiv<10>(s + 1, t.wMonth, 2);
  *s = '/';
  s = formatidiv<10>(s + 1, t.wDay, 2);
  *s = ' ';
  s = formatidiv<10>(s + 1, t.wHour, 2);
  *s = ':';
  s = formatidiv<10>(s + 1, t.wMinute, 2);
  *s = ':';
  s = formatidiv<10>(s + 1, t.wSecond, 2);
  *s = '.';
  s = formatidiv<10>(s + 1, t.wMilliseconds, 3);
  *s = ' ';
  memcpy(s + 1, TraceLevelNames(l), arraysize("  [^_^]  ") - 1); s += arraysize("  [^_^]  ");
  *s = ' ';
  s = formatidiv<10>(s + 1, processId, 4);
  *s = '#';
  s = formatidiv<10>(s + 1, threadId, 4);
  *s = ':';
  *++s = ' ';
  // assert(HEADER_LENGTH == (s - s_ + 1));
  return s;
}
void __cdecl Trace(CPCL_TRACE_LEVEL trace_level, char const *format, ...) {
  if ((trace_level & TRACE_LEVEL) == 0)
    return;

  char buf[0x200 + HEADER_LENGTH];

  va_list args;
  va_start(args, format);
  int n = _vsnprintf_s(buf + HEADER_LENGTH, arraysize(buf) - HEADER_LENGTH - 2, _TRUNCATE, format, args);
  if (n < 0) {
    n = arraysize(buf) - 2;
    if ('\0' == buf[n - 1])
      --n;
  }
  else
    n += HEADER_LENGTH;
  size_t size = static_cast<size_t>(n + 2);
  buf[size - 2] = '\r';
  buf[size - 1] = '\n';

  SYSTEMTIME t;
  ::GetLocalTime(&t);
  DWORD processId = ::GetCurrentProcessId();
  DWORD threadId = ::GetCurrentThreadId();
  FormatHeader(buf, trace_level, t, processId, threadId);

  WriteString(trace_level, buf, size);
}

void TraceString(CPCL_TRACE_LEVEL trace_level, StringPiece const &s) {
  if ((trace_level & TRACE_LEVEL) == 0)
    return;

  char buf[0x200 + HEADER_LENGTH];
  size_t size = min(s.size(), arraysize(buf) - HEADER_LENGTH - 2);

  SYSTEMTIME t;
  ::GetLocalTime(&t);
  DWORD processId = ::GetCurrentProcessId();
  DWORD threadId = ::GetCurrentThreadId();
  FormatHeader(buf, trace_level, t, processId, threadId);

  memcpy(buf + HEADER_LENGTH, s.data(), size);

  buf[HEADER_LENGTH + size] = '\r';
  buf[HEADER_LENGTH + size + 1] = '\n';

  WriteString(trace_level, buf, size + HEADER_LENGTH + 2);
}

void __cdecl ErrorSystem(unsigned long error_code, char const *format, ...) {
  if ((CPCL_TRACE_LEVEL_ERROR & TRACE_LEVEL) == 0)
    return;

  char buf[0x300 + 2 * HEADER_LENGTH];

  va_list args;
  va_start(args, format);
  int n = _vsnprintf_s(buf + HEADER_LENGTH, arraysize(buf) - 2 * (HEADER_LENGTH + 2) - 8, _TRUNCATE, format, args);
  if (n < 0) {
    n = arraysize(buf) - HEADER_LENGTH - 12;
    if ('\0' == buf[n - 1])
      --n;
  }
  else
    n += HEADER_LENGTH;
  size_t size = static_cast<size_t>(n + 2);
  buf[size - 2] = '\r';
  buf[size - 1] = '\n';

  typedef char HEADER_LENGTH_SIZE_INVALID[bool(HEADER_LENGTH > 14) ? 1 : -1];
  memset(buf + size, ' ', HEADER_LENGTH - 14);
  {
    char *s = buf + size + HEADER_LENGTH - 14;
    *s++ = '('; *s++ = '0'; *s++ = 'x';
    s = formatidiv<16>(s, error_code, 8);
    *s++ = ')'; *s++ = ':'; *s++ = ' ';
  }
  size += HEADER_LENGTH;

  //throw 0; // check n && offset - "HEADER UserMessage\r\n''.join(' ' for _ in xrange(HEADER_LENGTH)) SystemMessage\r\n"
  DWORD n_ = ::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL, error_code,
    MAKELANGID(LANG_ENGLISH/*LANG_NEUTRAL*/, SUBLANG_ENGLISH_US/*SUBLANG_NEUTRAL*/),
    buf + size, arraysize(buf) - size - 2, NULL);
  if (0 == n_) {
    if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER)
      ErrorHandler("ErrorSystem(): something was definitely wrong...");
    formatidiv<16>(buf + size, error_code, 8);
    n_ = 8;
  }
  else if ((n_ > 2)
    && ('\r' == *(buf + size + n_ - 2))
    && ('\n' == *(buf + size + n_ - 1)))
    n_ -= 2;
  size += n_ + 2;
  buf[size - 2] = '\r';
  buf[size - 1] = '\n';

  SYSTEMTIME t;
  ::GetLocalTime(&t);
  DWORD processId = ::GetCurrentProcessId();
  DWORD threadId = ::GetCurrentThreadId();
  FormatHeader(buf, CPCL_TRACE_LEVEL_ERROR, t, processId, threadId);

  WriteString(CPCL_TRACE_LEVEL_ERROR, buf, size);
}

std::string GetSystemMessage(unsigned long error_code) {
  std::string r;
  void *text(0);
  ::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL, error_code,
    MAKELANGID(LANG_ENGLISH/*LANG_NEUTRAL*/, SUBLANG_ENGLISH_US/*SUBLANG_NEUTRAL*/),
    (LPSTR)&text, 0, NULL);
  if (text) {
    r = (LPCSTR)text;
    ::LocalFree((HLOCAL)text);
  }

  return r;
}

} // namespace cpcl

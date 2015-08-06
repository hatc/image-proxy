#include "basic.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h> // gettimeofday
#include <time.h> // localtime_r
#include <fcntl.h>

#include <pwd.h>
#include <grp.h>

#include <unistd.h> // getpid
#include <sys/syscall.h>

#include <string.h> // strerror_r
#include <errno.h>

#include <boost/thread/thread.hpp>

#include "formatidiv.hpp"
#include "trace.h"
#include "string_util.hpp" // StringCopy, StringFormatImpl
#include "error_handler.h"

namespace cpcl {

#if defined(_DEBUG) || defined(DEBUG)
int TRACE_LEVEL = CPCL_TRACE_LEVEL_ERROR | CPCL_TRACE_LEVEL_WARNING | CPCL_TRACE_LEVEL_DEBUG;
#else
int TRACE_LEVEL = CPCL_TRACE_LEVEL_ERROR | CPCL_TRACE_LEVEL_WARNING;
#endif
FilePathChar const *TRACE_FILE_PATH = (FilePathChar const *)0;

void SetTraceFilePath(BasicStringPiece<FilePathChar> const &v) {
  if (TRACE_FILE_PATH)
    delete [] TRACE_FILE_PATH;
  
  size_t len = v.size();
  if (len) {
    FilePathChar *s = new FilePathChar[len + 1];
    StringCopy(s, len + 1, v); // wcscpy_s(s, len + 1, v.data());
    TRACE_FILE_PATH = s;
  }
  else
    TRACE_FILE_PATH = 0;
}

static boost::timed_mutex TRACE_FILE_MUTEX;
inline void WriteString(CPCL_TRACE_LEVEL l, char const *s, size_t size) {
  StringPiece error;
  int error_code(0);
  if (TRACE_FILE_PATH) {
    boost::unique_lock<boost::timed_mutex> lock(TRACE_FILE_MUTEX, boost::try_to_lock);
    if (!lock && !lock.timed_lock(boost::posix_time::seconds(2)))
      error = StringPieceFromLiteral("TRACE_FILE_MUTEX locked");

    if (error.empty()) {
      int fd = ::open(TRACE_FILE_PATH, O_CREAT | O_APPEND | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
      if (fd != -1) {
        int bytes_written = ::write(fd, s, size);
        if (-1 == bytes_written) { // || bytes_written < (int)size)
          error_code = errno;
          error = StringPieceFromLiteral("cannot write to log file: ");
        }
        ::close(fd);
      }
      else {
        error_code = errno;
        error = StringPieceFromLiteral("unable to open log file for writing: ");
      }
    }
  }
  else if (l >= CPCL_TRACE_LEVEL_ERROR)
    error = StringPieceFromLiteral("TRACE_FILE_PATH is not set");

  if (!error.empty()) {
    std::string error_buf;
    if (error_code != 0) {
      error_buf.assign(error.data(), error.size());
      error_buf.append(GetSystemMessage(error_code));
      error = StringPiece(error_buf);
    }

    ErrorHandler(error.data(), error.size());
    ErrorHandler(s, size);
  }
}

static inline char const* TraceLevelNames(CPCL_TRACE_LEVEL v) {
  switch (v) {
  case CPCL_TRACE_LEVEL_INFO:
    return "[notice] ";
  case CPCL_TRACE_LEVEL_DEBUG:
    return "[debug]  ";
  case CPCL_TRACE_LEVEL_WARNING:
    return "[warning]";
  case CPCL_TRACE_LEVEL_ERROR:
    return "[error]  ";
  }
  return "  [^_^]  ";
}
static size_t const HEADER_LENGTH = 45;
static inline char* FormatHeaderImpl(char *s_, CPCL_TRACE_LEVEL l, struct tm *t, unsigned int useconds, uint32 process_id, uint32 thread_id) {
  char *s = formatidiv<10>(s_, t->tm_year + 1900, 4);
  *s = '/';
  s = formatidiv<10>(s + 1, t->tm_mon + 1, 2);
  *s = '/';
  s = formatidiv<10>(s + 1, t->tm_mday, 2);
  *s = ' ';
  s = formatidiv<10>(s + 1, t->tm_hour, 2);
  *s = ':';
  s = formatidiv<10>(s + 1, t->tm_min, 2);
  *s = ':';
  s = formatidiv<10>(s + 1, t->tm_sec, 2);
  *s = '.';
  unsigned int milliseconds = useconds / 1000;
  if (useconds % 1000 > 500)
    ++milliseconds;
  s = formatidiv<10>(s + 1, milliseconds, 3);
  *s = ' ';
  memcpy(s + 1, TraceLevelNames(l), arraysize("  [^_^]  ") - 1); s += arraysize("  [^_^]  ");
  *s = ' ';
  s = formatidiv<10>(s + 1, process_id, 4);
  *s = '#';
  s = formatidiv<10>(s + 1, thread_id, 4);
  *s = ':';
  *++s = ' ';
  // assert(HEADER_LENGTH == (s - s_ + 1));
  return s;
}
static inline char* FormatHeader(char *s, CPCL_TRACE_LEVEL l) {
  struct timeval tv = { 0, 0 };
  ::gettimeofday(&tv, NULL);
  struct tm t;
  ::localtime_r(&tv.tv_sec, &t);

  uint32 process_id = static_cast<uint32>(::getpid());
  uint32 thread_id = static_cast<unsigned int>(::syscall(SYS_gettid));

  return FormatHeaderImpl(s, l, &t, static_cast<unsigned int>(tv.tv_usec), process_id, thread_id);
}

void Trace(CPCL_TRACE_LEVEL trace_level, char const *format, ...) {
  if ((TRACE_LEVEL & trace_level) == 0)
    return;
  
  char buf[0x200 + HEADER_LENGTH];
  va_list arguments;
  va_start(arguments, format);
  size_t n = StringFormatImpl(buf + HEADER_LENGTH, arraysize(buf) - HEADER_LENGTH - 1, format, arguments, 0);
  va_end(arguments);

  n += HEADER_LENGTH + 1;
  buf[n - 1] = '\n';

  FormatHeader(buf, trace_level);
  WriteString(trace_level, buf, n);
}

void TraceString(CPCL_TRACE_LEVEL trace_level, StringPiece const &s) {
  if ((trace_level & TRACE_LEVEL) == 0)
    return;

  char buf[0x200 + HEADER_LENGTH];
  size_t n = StringCopy(buf + HEADER_LENGTH, arraysize(buf) - HEADER_LENGTH - 1, s);

  n += HEADER_LENGTH + 1;
  buf[n - 1] = '\n';

  FormatHeader(buf, trace_level);
  WriteString(trace_level, buf, n);
}

static StringPiece FOR_STR = StringPieceFromLiteral(" for ");
static inline size_t CurrentProcessUserGroup(char *s, size_t n) {
  char buf[0x100];
  
  size_t offset(0);
  struct passwd user_info_storage, *user_info;
  if (!!::getpwuid_r(::getuid(), &user_info_storage, buf, arraysize(buf), &user_info))
    offset += StringCopyFromLiteral(s, n, "getpwuid_r failed");
  else
    offset += StringCopy(s, n, user_info->pw_name);

  if (offset < n)
    s[offset++] = ':';
  s += offset;
  n -= offset;

  struct group group_info_storage, *group_info;
  if (!!::getgrgid_r(getgid(), &group_info_storage, buf, arraysize(buf), &group_info))
    offset += StringCopyFromLiteral(s, n, "getgrgid_r failed");
  else
    offset += StringCopy(s, n, group_info->gr_name);
  return offset;
}

void ErrorSystem(int error_code, char const *format, ...) {
  if ((CPCL_TRACE_LEVEL_ERROR & TRACE_LEVEL) == 0)
    return;

  char buf[0x300 + HEADER_LENGTH];
  va_list arguments;
  va_start(arguments, format);
  size_t n = StringFormatImpl(buf + HEADER_LENGTH, arraysize(buf) - HEADER_LENGTH, format, arguments, 0);
  va_end(arguments);

  n += HEADER_LENGTH;

  StringPiece s = ::strerror_r(error_code, buf + n, arraysize(buf) - n - 1);
  /*The GNU-specific strerror_r() returns a pointer to a string containing the error message.
  This may be either a pointer to a string that the function stores in buf,
  or a pointer to some (immutable) static string (in which case buf is unused)*/
  if (s.data() != buf + n)
    n += StringCopy(buf + n, arraysize(buf) - n - 1, s);
  else
    n += s.size();

  if (EACCES == error_code && (n - 1) < arraysize(buf)) {
    n += StringCopy(buf + n, arraysize(buf) - n - 1, FOR_STR);
    n += CurrentProcessUserGroup(buf + n, arraysize(buf) - n - 1);
  }
  buf[++n - 1] = '\n';

  FormatHeader(buf, CPCL_TRACE_LEVEL_ERROR);
  WriteString(CPCL_TRACE_LEVEL_ERROR, buf, n);
}

std::string GetSystemMessage(int error_code) {
  char buf[0x100];
  size_t n(0);
  StringPiece s = ::strerror_r(error_code, buf, arraysize(buf));
  /* The GNU-specific strerror_r() returns a pointer to a string containing the error message.
  This may be either a pointer to a string that the function stores in buf,
  or a pointer to some (immutable) static string (in which case buf is unused) */
  if (s.data() != buf + n)
    n += StringCopy(buf, arraysize(buf), s);
  else
    n += s.size();

  if (EACCES == error_code && n < arraysize(buf)) {
    n += StringCopy(buf + n, arraysize(buf) - n, FOR_STR);
    n += CurrentProcessUserGroup(buf + n, arraysize(buf) - n);
  }
  return std::string(buf, n);
}

} // namespace cpcl

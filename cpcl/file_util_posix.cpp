#include "basic.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h> // PATH_MAX
#include <cstdlib> // getenv
#include <errno.h>

#include "file_util.h"
#include "trace.h"

namespace cpcl {

bool GetTemporaryDirectory(std::string *r) {
  StringPiece s = ::getenv("TMPDIR");
  if (s.empty())
    s = StringPieceFromLiteral("/tmp");
  else
    s = s.trim_tail(StringPieceFromLiteral("/")); // remove trailing slash
  
  if (r)
    r->assign(s.data(), s.size());
  return true;
}

bool GetModuleFilePath(void*, std::string *r) {
  char buf[PATH_MAX];
  ssize_t const n = ::readlink("/proc/self/exe", buf, arraysize(buf));
  if (n == -1) {
    ErrorSystem(errno, "GetModuleFilePath(): readlink fails: ");
    return false;
  }
  if (r) {
    StringPiece s = DirName(StringPiece(buf, static_cast<size_t>(n)));
    r->assign(s.data(), s.size());
  }
  return true;
}

bool ExistFilePath(char const *file_path, bool *is_directory) {
  struct stat info;
  if (::stat(file_path, &info) == -1) {
    if (errno != ENOENT)
      ErrorSystem(errno, "ExistFilePath(): stat('%s') fails: ", file_path);
    return false;
  }

  if (is_directory)
    *is_directory = !!(info.st_mode & S_IFDIR);
  return true;
}

bool DeleteFilePath(char const *file_path) {
  bool is_directory;
  if (!ExistFilePath(file_path, &is_directory))
    return false;
  if (is_directory) {
    Error(StringPieceFromLiteral("DeleteFilePath(): recursive remove not implemented"));
    return false;
  }

  if (::unlink(file_path) == -1) {
#ifdef _BSD_SOURCE
    ::usleep(0x1000); /* yaaaaaaaawn ^_^ */
#else
    ::sleep(1); /* yaaaaaaaaaaaaaaaaaaaaaaaaawn ^_^ */
#endif
    if (::unlink(file_path) == -1) {
      ErrorSystem(errno, "ExistFilePath(): unlink('%s') fails: ", file_path);
      return false;
    }
  }
  return true;
}

} // namespace cpcl

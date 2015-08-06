#include "basic.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <string.h> // strerror_r
#include <errno.h>

#include <memory> // std::auto_ptr

#include "file_util.h" // GetTemporaryDirectory
#include "trace.h"
#include "trace_helpers.hpp" // RandName
#include "file_stream.h"

namespace cpcl {

struct FileStream::Deleter {
  std::string file_path;
  
  Deleter(std::string const &file_path_) : file_path(file_path_)
  {}
  ~Deleter() {
    if (::unlink(file_path.c_str()) == -1) {
      Trace(CPCL_TRACE_LEVEL_WARNING,
        "FileStream::~FileStream(): unlink('%s') fails: %s",
        file_path.c_str(), GetSystemMessage(errno).c_str());
    }
  }
};

uint32 FileStream::Read(void *data, uint32 size) {
  ssize_t r = ::read(fd, data, size);
  state = (r != -1);
  if (!state) {
    ErrorSystem(errno, "FileStream::Read(): read fails: ");
    r = 0;
  }
  return static_cast<uint32>(r);
}

uint32 FileStream::Write(void const *data, uint32 size) {
  ssize_t r = ::write(fd, data, size);
  state = (r != -1);
  if (!state) {
    ErrorSystem(errno, "FileStream::Write(): write fails: ");
    r = 0;
  }
  return static_cast<uint32>(r);
}

bool FileStream::Seek(int64 move_to, uint32 move_method, int64 *position) {
  int64 r = ::lseek64(fd, move_to, static_cast<int>(move_method));
  // state = (r != (off_t)-1); - Seek method not change IOStream state, because its returns result
  if ((int64)-1 == r) {
    ErrorSystem(errno, "FileStream::Seek(): lseek64(move_to = %lld, move_method = %u) fails: ", move_to, move_method);
    return false;
  }

  if (position)
    *position = r;
  return true;
}

int64 FileStream::Tell() {
  int64 r;
  state = Seek(0, SEEK_CUR, &r);
  if (!state)
    r = (int64)-1;
  return r;
}

int64 FileStream::Size() {
  int64 r;
  int64 offset = Tell();
  if (!state)
    return (int64)-1;
  state = Seek(0, SEEK_END, &r);
  if (!state)
    return (int64)-1;
  state = Seek(offset, SEEK_SET, 0);
  return r;
}

/* static inline bool ReadLink(char const *file_path, char *r, size_t r_size) {
  ssize_t n = ::readlink(file_path, r, r_size - 1);
  if (n < 0)
    return false;
  if ((size_t)n == (r_size - 1)) {
    r[r_size - 1] = 0;
    stat(r, );
    if (ENOENT == errno)
      ;
  }
}

IOStream* FileStream::Clone() {
  FileStream *r;

  char link_buf[0x20];
  char file_path_buf[0x100];
  StringFormat(link_buf, "/proc/self/fd/%d", fd);
  ssize_t n = ::readlink(link_buf, file_path_buf, arraysize(file_path_buf));
  if (n < 0)
    ErrorSystem(errno, "FileStream::Clone(): readlink('%s') fails: ", link_buf); */

IOStream* FileStream::Clone() {
  /*can't use dup:
  They refer to the same open file description (see open(2)) and thus share file offset and file status flags;
  for example, if the file offset is modified by using lseek(2) on one of the descriptors, the offset is also changed for the other.*/
  /*
  You can use readlink on /proc/self/fd/NNN where NNN is the file descriptor.
  This will give you the name of the file as it was when it was opened — however,
  if the file was moved or deleted since then, it may no longer be accurate (although Linux can track renames in some cases).
  To verify, stat the filename given and fstat the fd you have, and make sure st_dev and st_ino are the same.

  Of course, not all file descriptors refer to files, and for those you'll see some odd text strings, such as pipe:[1538488].
  Since all of the real filenames will be absolute paths, you can determine which these are easily enough.
  Further, as others have noted, files can have multiple hardlinks pointing to them - this will only report the one it was opened with.
  If you want to find all names for a given file, you'll just have to traverse the entire filesystem.*/
  FileStream *r;
  /*int fd = ::open(file_path.c_str(), O_RDWR, 0);
  if (-1 == fd) {
  ErrorSystem(errno, "FileStream::Clone('%s'): open fails: ", file_path.c_str());
  r = (FileStream*)0;
  } else {
  std::auto_ptr<FileStream> r_(new FileStream(fd, unlink_on_close)); // ctor may fail due to low memory, descriptor leaked
  r_->file_path = file_path;

  r_->Seek(Tell(), SEEK_SET, (int64*)0);

  r = r_.release();
  } */
  if (!FileStream::FileStreamCreate(file_path.c_str(), O_RDONLY, 0, "Clone", &r)) {
    r = (FileStream*)0;
  } else {
    std::auto_ptr<FileStream> tmp(r); // r->file_path = file_path may throw, so keep r in auto_ptr
    r->deleter = deleter;
    r->file_path = file_path;
    r->Seek(Tell(), SEEK_SET, (int64*)0);
    tmp.release();
  }
  return r;
}

FileStream::FileStream(int fd_) : fd(fd_), state(true)
{}
FileStream::~FileStream() {
  ::close(fd);
}

bool FileStream::FileStreamCreate(char const *file_path,
  int flags, uint32 mode, char const *method_name, FileStream **v) {
  int fd = ::open(file_path, flags, static_cast<mode_t>(mode));
  if (-1 == fd) {
    ErrorSystem(errno, "FileStream::%s('%s'): open fails: ", method_name, file_path);
    return false;
  }
  std::auto_ptr<FileStream> r(new FileStream(fd)); // ctor may fail due to low memory, descriptor leaked
  r->file_path = file_path;

  if (v)
    *v = r.release();
  return true;
}

bool FileStream::Create(char const *file_path, FileStream **r) {
  return FileStream::FileStreamCreate(file_path,
    O_CREAT | O_EXCL | O_RDWR,
    S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH,
    "Create", r);
}

bool FileStream::Read(char const *file_path, FileStream **r) {
  return FileStream::FileStreamCreate(file_path,
    O_RDONLY,
    0,
    "Read", r);
}

bool FileStream::ReadWrite(char const *file_path, FileStream **r) {
  return FileStream::FileStreamCreate(file_path,
    O_RDWR,
    0,
    "ReadWrite", r);

  /* int fd = ::open(file_path, O_RDWR, 0);
  if (-1 == fd) {
    ErrorSystem(errno, "FileStream::ReadWrite('%s'): open fails: ", file_path);
    return false;
  }
  if (v)
    *v = new FileStream(fd); // ctor may fail due to low memory, descriptor leaked
  return true; */
}

static StringPiece const HEAD = StringPieceFromLiteral("^_^");
static StringPiece const TAIL = StringPieceFromLiteral(".tmp");
bool FileStream::CreateTemporary(FileStream **r) {
  std::string file_path;
  if (!GetTemporaryDirectory(&file_path))
    return false;
  size_t const len = 0x20 + HEAD.size() + TAIL.size() + 1;
  file_path.append(len, os_path_traits<char>::Delimiter());
  char *buf = const_cast<char*>(file_path.c_str() + file_path.size() - len + 1);
  for (size_t i = 0; i < 0x100; ++i) {
    RandName(buf, len, HEAD, TAIL);
    
    int fd = ::open(file_path.c_str(),
      O_CREAT | O_EXCL | O_RDWR,
      S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (-1 == fd) {
      if (errno != EEXIST) {
        ErrorSystem(errno,
          "FileStream::CreateTemporary('%s'): open fails: ",
          file_path.c_str());
        return false;
      }
    } else {
      std::auto_ptr<FileStream> tmp(new FileStream(fd)); // ctor may fail due to low memory, descriptor leaked
      tmp->deleter.reset(new FileStream::Deleter(file_path)); // std::basic_string ctor may fail due to low memory, temporary file not deleted, i.e. leaked
      tmp->file_path.swap(file_path); // file_path used by FileStream dtor, but basic_string<> no-throw if allocators same
      if (r)
        *r = tmp.release();
      return true;
    }
  }
  Trace(CPCL_TRACE_LEVEL_ERROR,
    "FileStream::CreateTemporary('%s'): too many temporary files, consider remake pseudo random name generator",
    file_path.c_str());
  return false;
}

} // namespace cpcl

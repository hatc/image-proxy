#include <memory> // std::auto_ptr<>

#include "file_stream.h"
#include "trace_helpers.hpp"
#include "string_util.h"
#include "file_util.h"
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

/*FileStream
Read():
 precondition: size > 0, file pointer at EOF
 postcondition: state = true, Read() return 0

 precondition: size > 0, file pointer beyond EOF
 postcondition: state = true, Read() return 0

Write():
 precondition: file opened with read-only access
 postcondition: state = false, Write() return 0

 precondition: size > 0, file pointer beyond EOF
 postcondition: state = true, Write() return number of bytes written, intervening bytes(between EOF and file pointer before Write() called) in file uninitialized

Seek():
 precondition: file pointer valid, move_to beyond EOF
 postcondition: state = true, file pointer beyond EOF, return true

 precondition: file pointer valid, move_to before begin of file, i.e. move_to < 0 && SEEK_SET
 postcondition: state = true, file pointer unchanged, return false
*/

namespace cpcl {

struct FileStream::Deleter {
  std::wstring file_path;
  
  Deleter(std::wstring const &file_path_) : file_path(file_path_)
  {}
  ~Deleter() {
    if (::DeleteFileW(file_path.c_str()) == FALSE) {
      ::SleepEx(0x100, TRUE); /* yaaaaaaaawn ^_^ */
      if (::DeleteFileW(file_path.c_str()) == FALSE) {
        Trace(CPCL_TRACE_LEVEL_WARNING,
          "FileStream::~FileStream(): DeleteFileW('%s') fails: %s",
          ConvertUTF16_CP1251(file_path).c_str(), GetSystemMessage(::GetLastError()).c_str());
      }
    }
  }
};

FileStream::FileStream() : state(false), hFile(INVALID_HANDLE_VALUE)
{}
FileStream::~FileStream() {
  if (hFile != INVALID_HANDLE_VALUE)
    ::CloseHandle(hFile);
}

static unsigned long const CHUNK_SIZE_MAX = (1 << 22);
bool FileStream::ReadPart(void *data, unsigned long size, unsigned long &processed_size) {
  if (size > CHUNK_SIZE_MAX)
    size = CHUNK_SIZE_MAX;
  DWORD processed_size_ = 0;
  bool r = (::ReadFile(hFile, data, size, &processed_size_, NULL) != FALSE);
  if (!r) {
    ErrorSystem(::GetLastError(), "FileStream::ReadPart('%s'): ReadFile fails:", ConvertUTF16_CP1251(FilePath()).c_str());
  }
  processed_size = (unsigned long)processed_size_;
  return r;
}
bool FileStream::WritePart(void const *data, unsigned long size, unsigned long &processed_size) {
  if (size > CHUNK_SIZE_MAX)
    size = CHUNK_SIZE_MAX;
  DWORD processed_size_ = 0;
  bool r = (::WriteFile(hFile, data, size, &processed_size_, NULL) != FALSE);
  if (!r) {
    ErrorSystem(::GetLastError(), "FileStream::WritePart('%s'): WriteFile fails:", ConvertUTF16_CP1251(FilePath()).c_str());
  }
  processed_size = (unsigned long)processed_size_;
  return r;
}

uint32 FileStream::Read(void *data, uint32 size_) {
  unsigned long r(0), size(size_);
  do {
    unsigned long processed_size = 0;
    state = ReadPart(data, size, processed_size);
    r += processed_size;
    if (!state || (0 == processed_size))
      break;
    data = (void *)((unsigned char *)data + processed_size);
    size -= processed_size;
  } while (size > 0);
  return static_cast<uint32>(r);
}

uint32 FileStream::Write(void const *data, uint32 size_) {
  unsigned long r(0), size(size_);
  do {
    unsigned long processed_size = 0;
    state = WritePart(data, size, processed_size);
    r += processed_size;
    if (!state || (0 == processed_size))
      break;
    data = (void const *)((unsigned char const *)data + processed_size);
    size -= processed_size;
  } while (size > 0);
  return static_cast<uint32>(r);
}

bool FileStream::Seek(int64 move_to, uint32 move_method, int64 *position) {
  LARGE_INTEGER v;
  v.QuadPart = move_to;
  v.LowPart = ::SetFilePointer(hFile, v.LowPart, &v.HighPart, move_method);
  if (INVALID_SET_FILE_POINTER == v.LowPart) {
    unsigned long const error_code = ::GetLastError();
    if (error_code != NO_ERROR) {
      ErrorSystem(error_code, "FileStream::Seek('%s'): SetFilePointer(move_to = %u, move_method = %d) fails:",
        ConvertUTF16_CP1251(FilePath()).c_str(), (unsigned int)move_to, move_method);
      return false;
    }
  }

  if (position)
    *position = v.QuadPart;
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
  LARGE_INTEGER v;
  state = (::GetFileSizeEx(hFile, &v) != FALSE);
  if (!state) {
    ErrorSystem(::GetLastError(), "FileStream::Size('%s'): GetFileSizeEx fails:", ConvertUTF16_CP1251(FilePath()).c_str());
    v.QuadPart = -1;
  }
  return v.QuadPart;
}

IOStream* FileStream::Clone() {
  FileStream *r;
  if (!FileStream::FileStreamCreate(FilePath(),
    GENERIC_READ,
    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, &r)) {
    r = NULL;
    ErrorSystem(::GetLastError(),
      "FileStream::Clone('%s'): CreateFileW fails:",
      ConvertUTF16_CP1251(FilePath()).c_str());
  }
  else {
    r->Seek(Tell(), FILE_BEGIN, NULL);
    r->deleter = deleter;
  }
  return r;
}

static WStringPiece const prefix = WStringPieceFromLiteral(L"\\\\?\\");
static inline void ExpandPath(std::wstring *v) {
  if (!!v && !WStringPiece(NormalizePath(*v)).starts_with(prefix))
    v->insert(0, prefix.data(), prefix.size());
}
static inline std::wstring ExpandPath(cpcl::WStringPiece const &v) {
  std::wstring r;
  if (!v.starts_with(prefix))
    r.reserve(v.size() + prefix.size() + 1);
  r.assign(v.data(), v.size());
  ExpandPath(&r);
  return r;
}

cpcl::WStringPiece FileStream::FilePath() const {
  cpcl::WStringPiece r = file_path;
  if (r.starts_with(prefix) && (r.size() > prefix.size()))
    r = cpcl::WStringPiece(r.data() + prefix.size(), r.size() - prefix.size());
  return r;
}

bool FileStream::FileStreamCreate(cpcl::WStringPiece const &path,
  unsigned long access_mode, unsigned long share_mode,
  unsigned long disposition, unsigned long attributes,
  FileStream **v) {
  std::auto_ptr<FileStream> r(new FileStream());
  /* std::wstring path_ = FileStream::ExpandPath(path);
  in Release with RVO: only one ctor call */
  std::wstring path_ = ExpandPath(path);
  HANDLE const hFile = ::CreateFileW(path_.c_str(), access_mode, share_mode,
    (LPSECURITY_ATTRIBUTES)NULL, disposition, attributes, (HANDLE)NULL);
  if (hFile == INVALID_HANDLE_VALUE)
    return false;

  r->hFile = hFile;
  r->file_path.swap(path_);
  r->state = true;
  if (v)
    *v = r.release();
  return true;
}

bool FileStream::Create(FilePathChar const *path, FileStream **r) {
  unsigned long const access_mode = GENERIC_READ | GENERIC_WRITE,
    share_mode = FILE_SHARE_READ,
    disposition = CREATE_NEW,
    attributes = FILE_ATTRIBUTE_NORMAL;
  if (!FileStream::FileStreamCreate(path,
    access_mode, share_mode, disposition, attributes, r)) {

    ErrorSystem(::GetLastError(), "FileStream::Create('%s'): CreateFileW fails:", ConvertUTF16_CP1251(path).c_str());
    return false;
  }
  return true;
}

bool FileStream::Read(FilePathChar const *path, FileStream **r) {
  unsigned long const access_mode = GENERIC_READ,
    share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
    disposition = OPEN_EXISTING,
    attributes = FILE_ATTRIBUTE_NORMAL;
  if (!FileStream::FileStreamCreate(path,
    access_mode, share_mode, disposition, attributes, r)) {
    ErrorSystem(::GetLastError(), "FileStream::Read('%s'): CreateFileW fails:", ConvertUTF16_CP1251(path).c_str());
    return false;
  }
  return true;
}

bool FileStream::ReadWrite(FilePathChar const *path, FileStream **r) {
  unsigned long const access_mode = GENERIC_READ | GENERIC_WRITE,
    share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
    disposition = OPEN_EXISTING,
    attributes = FILE_ATTRIBUTE_NORMAL;
  if (!FileStream::FileStreamCreate(path,
    access_mode, share_mode, disposition, attributes, r)) {
    ErrorSystem(::GetLastError(), "FileStream::ReadWrite('%s'): CreateFileW fails:", ConvertUTF16_CP1251(path).c_str());
    return false;
  }
  return true;
}

static WStringPiece const HEAD = WStringPieceFromLiteral(L"^_^");
static WStringPiece const TAIL = WStringPieceFromLiteral(L".tmp");
bool FileStream::CreateTemporary(FileStream **r) {
  unsigned long const access_mode = GENERIC_READ | GENERIC_WRITE,
    share_mode = FILE_SHARE_READ | FILE_SHARE_DELETE,
    disposition = CREATE_NEW,
    // attributes = FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE;
    attributes = FILE_ATTRIBUTE_TEMPORARY;

  std::wstring path;
  if (!GetTemporaryDirectory(&path))
    return false;
  ExpandPath(&path);
  size_t const len = 0x20 + HEAD.size() + TAIL.size() + 1;
  path.append(len, L'\\');
  wchar_t *buf = const_cast<wchar_t*>(path.c_str() + path.size() - len + 1);
  for (size_t i = 0; i < 0x100; ++i) {
    RandName(buf, len, HEAD, TAIL);
    if (FileStream::FileStreamCreate(path,
      access_mode, share_mode, disposition, attributes, r)) {
      (*r)->deleter.reset(new FileStream::Deleter((*r)->file_path));
      return true;
    }

    unsigned long const error_code = ::GetLastError();
    if (error_code != ERROR_FILE_EXISTS) {
      ErrorSystem(error_code, "FileStream::CreateTemporary('%s'): CreateFileW fails:", ConvertUTF16_CP1251(path).c_str());
      return false;
    }
  }
  Trace(CPCL_TRACE_LEVEL_ERROR, "FileStream::CreateTemporary('%s'): too many temporary files, consider remake pseudo random name generator",
    ConvertUTF16_CP1251(path).c_str());
  return false;
}

} // namespace cpcl

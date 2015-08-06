#include "string_util.h" // cpcl::ConvertUTF16_CP1251
#include "file_iterator.h"
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

// use ScopedPtr with placement delete for store Handle - can't use for store, need complete type for instanation - FileIterator::Handle incomplete
struct FileIterator::Handle {
  HANDLE hFindFile;
  WIN32_FIND_DATAW find_data;
  
  explicit Handle(HANDLE hFindFile_) : hFindFile(hFindFile_)
  {}
  ~Handle() {
    if (hFindFile != INVALID_HANDLE_VALUE) {
      ::FindClose(hFindFile);
      hFindFile = INVALID_HANDLE_VALUE;
    }
  }
};

//#ifdef SearchPath // windows.h define SearchPath as SearchPathW
//#undef SearchPath
//#endif
/* full file path i.e. C:\\some\\dir\\file interpreted as directory name
 * search file path i.e. C:\\some\\dir\\*.dll returned as is
 * examples:
 * L"c:\\some\\dir" -> L"c:\\some\\dir\\*.*"
 * L"c:\\some\\dir\\" -> L"c:\\some\\dir\\*.*"
 * L"c:\\some\\dir\\file", L"c:\\some\\dir\\file\\*.*"
 * L"c:\\some\\dir\\*.dll", L"c:\\some\\dir\\*.dll" */
static inline std::wstring& SearchPath(std::wstring &storage, WStringPiece const &search_path) {
  WStringPiece file_name = FileName(search_path), dir_name;
  if (!file_name.empty()) {
    if (!!::wmemchr(file_name.data(), L'*', file_name.size()))
      storage.assign(search_path.data(), search_path.size());
    else
      dir_name = search_path;
  }
  else
    dir_name = DirName(search_path);

  if (!dir_name.empty()) {
    WStringPiece search_mask = WStringPieceFromLiteral(L"\\*.*");
    storage.reserve(dir_name.size() + search_mask.size());
    storage.assign(dir_name.data(), dir_name.size());
    storage.append(search_mask.data(), search_mask.size());
  }

  return storage;
}

static inline std::string& SearchPathPosix(std::string &storage, StringPiece search_path) {
  search_path = search_path.trim_tail(StringPieceFromLiteral("/"));
  storage.reserve(search_path.size() + 1);
  storage.assign(search_path.data(), search_path.size());
  storage.append(1, '/');
  return storage;
}

static inline WStringPiece WStringPieceFromFindData(WIN32_FIND_DATAW const &find_data) {
  size_t const file_name_size = ::wcsnlen(find_data.cFileName, arraysize(find_data.cFileName));
  if (!!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
    /* if ((file_name_size == 1 && find_data.cFileName[0] == L'.')
     || (file_name_size == 2 && find_data.cFileName[0] == L'.' && find_data.cFileName[1] == L'.')) */
    // file_name_size can't be zero
    if (find_data.cFileName[0] == L'.' && (file_name_size == 1 || (find_data.cFileName[1] == L'.' && file_name_size == 2)))
      return cpcl::WStringPiece();
  }
  return cpcl::WStringPiece(find_data.cFileName, file_name_size);
}

FileIterator::FileIterator(WStringPiece const &search_path) : handle(0), dir_path_size(0) {
  COMPILE_ASSERT(sizeof(FileIterator::Handle) <= sizeof(handle_storage), invalid_handle_storage);

  WIN32_FIND_DATAW find_data;
  HANDLE hFindFile = ::FindFirstFileW(SearchPath(file_path, search_path).c_str(), &find_data);
  if (INVALID_HANDLE_VALUE == hFindFile) {
    unsigned long error_code = ::GetLastError();
    if ((error_code != ERROR_FILE_NOT_FOUND) && (error_code != ERROR_PATH_NOT_FOUND))
      ErrorSystem(error_code, "FileIterator('%s'): FindFirstFileW fails:", cpcl::ConvertUTF16_CP1251(file_path).c_str());
  }
  else {
    handle = new (handle_storage)Handle(hFindFile);
    handle->find_data = find_data;
  }
}

FileIterator::~FileIterator() {
  Dispose();
}

void FileIterator::Dispose() {
  if (!!handle) {
    handle->~Handle();
    handle = 0;
  }
}

bool FileIterator::Next(FileInfo *r) {
  if (!handle)
    return false;

  cpcl::WStringPiece file_name = WStringPieceFromFindData(handle->find_data);
  if (!file_name.empty()) {
    SetFilePath(file_name);
    if (!!r) {
      // fill result with look-ahead value
      r->file_path = file_path;
      r->attributes = FileInfo::ATTRIBUTE_INVALID;
      if (!!(handle->find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        r->attributes |= FileInfo::ATTRIBUTE_DIRECTORY;
      if (!!(handle->find_data.dwFileAttributes & FILE_ATTRIBUTE_NORMAL))
        r->attributes |= FileInfo::ATTRIBUTE_NORMAL;
    }
  }

  if (::FindNextFileW(handle->hFindFile, &handle->find_data) == 0) {
    unsigned long error_code = ::GetLastError();
    if (error_code != ERROR_NO_MORE_FILES)
      ErrorSystem(error_code, "FileIterator::Next(): FindNextFileW fails:");

    Dispose();
  }

  return (file_name.empty()) ? Next(r) : true;
}

} // namespace cpcl

template<size_t N, size_t K>
inline void test_searchpath(wchar_t const (&s_)[N], wchar_t const (&check_)[K]) {
  cpcl::WStringPiece s = cpcl::WStringPieceFromLiteral(s_);
  cpcl::WStringPiece check = cpcl::WStringPieceFromLiteral(check_);
  std::wstring storage;
  if (check != cpcl::SearchPath(storage, s))
    std::wcout << "'" << storage << "' != '" << check << "'\n";
}
//{
//	test_searchpath(L"c:\\some\\dir", L"c:\\some\\dir\\*.*");
//	test_searchpath(L"c:\\some\\dir\\", L"c:\\some\\dir\\*.*");
//	test_searchpath(L"c:\\some\\dir\\file", L"c:\\some\\dir\\file\\*.*");
//	test_searchpath(L"c:\\some\\dir\\*.dll", L"c:\\some\\dir\\*.dll");
//	test_searchpath(L"c:\\some\\dir\\*.*\\", L"c:\\some\\dir\\*.*\\*.*");
//}

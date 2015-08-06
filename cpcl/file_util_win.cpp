#include "basic.h"

#include <boost/scoped_array.hpp>
#ifdef CPCL_USE_BOOST_THREAD_SLEEP
#include <boost/thread/thread.hpp>
#endif

#include "file_util.h"
#include "string_util.h"
#include "trace_helpers.hpp"
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
#include <shellapi.h> // SHFileOperation

namespace cpcl {

bool GetTemporaryDirectory(std::wstring *r) {
  wchar_t static_buf[1024];
  boost::scoped_array<wchar_t> dynamic_buf;
  wchar_t *buf = static_buf; /* === &static_buf[0] === &(*static_buf) ;))) */
  DWORD n = ::GetTempPathW(arraysize(static_buf), buf);
  if (n > arraysize(static_buf)) {
    dynamic_buf.reset(new wchar_t[n]);
    buf = dynamic_buf.get();

    n = ::GetTempPathW(n, buf);
  }
  if (0 == n) {
    ErrorSystem(::GetLastError(), "%s: GetTempPathW fails:", "GetTemporaryDirectory()");
    /*if (r)
     *r = L"c:\\temp";*/
    return false;
  }
  if (r) {
    if (L'\\' == buf[n - 1])
      --n;
    (*r).assign(buf, n);
  }
  return true;
}

bool GetModuleFilePath(void *hmodule, std::wstring *r) {
  wchar_t static_buf[1024];
  boost::scoped_array<wchar_t> dynamic_buf;
  wchar_t *buf = static_buf;
  DWORD size = arraysize(static_buf);
  DWORD n = ::GetModuleFileNameW((HMODULE)hmodule, buf, size);
  /*If the function succeeds, then returns path length not including the terminating null character.

  If the buffer is too small to hold the module name
  Windows XP: The string is truncated to nSize characters and is not null-terminated. The function returns nSize. ERROR_INSUFFICIENT_BUFFER == GetLastError()
  Vista +   : The string is truncated to nSize characters including the terminating null character. The function returns nSize. ERROR_SUCCESS == GetLastError()*/
  while (n == size) {
    size *= 2;
    dynamic_buf.reset(new wchar_t[size]);
    buf = dynamic_buf.get();

    n = ::GetModuleFileNameW((HMODULE)hmodule, buf, size);
  }
  if (0 == n) {
    ErrorSystem(::GetLastError(), "%s: GetModuleFileNameW fails:", "GetModuleFilePath()");
    return false;
  }
  /*The string returned will use the same format that was specified when the module was loaded.
  Therefore, the path can be a long or short file name, and can use the prefix "\\?\".

  use GetFullPathName ?*/
  for (; n > 1; --n)
    if (L'\\' != buf[n - 1])
      break;
  for (; n > 0; --n)
    if (L'\\' == buf[n - 1])
      break;

  if (r) {
    if (n > 1 && (L'\\' == buf[n - 1]))
      --n;
    (*r).assign(buf, n);
  }
  return true;
}

bool ExistFilePath(wchar_t const *file_path, bool *is_directory) {
  WIN32_FIND_DATAW find_data;
  HANDLE hFindFile = ::FindFirstFileW(file_path, &find_data);
  if (INVALID_HANDLE_VALUE == hFindFile) {
    unsigned long const error_code = ::GetLastError();
    if (error_code != ERROR_FILE_NOT_FOUND) {
      /*boost::scoped_array<unsigned char> path_cp1251;
      ErrorSystem(error_code, "ExistFilePath('%s'): FindFirstFileW fails:", UTF16_CP1251_(path, path_cp1251));*/
      ErrorSystem(error_code, "ExistFilePath('%s'): FindFirstFileW fails:", ConvertUTF16_CP1251(file_path).c_str());
    }
    return false;
  }
  if (is_directory)
    *is_directory = ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
  ::FindClose(hFindFile);
  return true;
}

inline bool RemoveReadOnlyAttribute(WStringPiece const &path) {
  DWORD attrs = ::GetFileAttributesW(path.data());
  if (INVALID_FILE_ATTRIBUTES == attrs) {
    /*boost::scoped_array<unsigned char> path_cp1251;
    ErrorSystem(::GetLastError(), "RemoveReadOnlyAttribute('%s'): GetFileAttributesW fails:", UTF16_CP1251_(path, path_cp1251));*/
    ErrorSystem(::GetLastError(), "RemoveReadOnlyAttribute('%s'): GetFileAttributesW fails:", ConvertUTF16_CP1251(path).c_str());
    return false;
  }

  if (attrs & FILE_ATTRIBUTE_READONLY) {
    attrs ^= FILE_ATTRIBUTE_READONLY;
    if (::SetFileAttributes(path.data(), attrs) == FALSE) {
      /*boost::scoped_array<unsigned char> path_cp1251;
      ErrorSystem(::GetLastError(), "RemoveReadOnlyAttribute('%s'): SetFileAttributes fails:", UTF16_CP1251_(path, path_cp1251));*/
      ErrorSystem(::GetLastError(), "RemoveReadOnlyAttribute('%s'): SetFileAttributes fails:", ConvertUTF16_CP1251(path).c_str());
      return false;
    }
  }
  return true;
}

bool DeleteFilePath(wchar_t/*WStringPiece const&*/ const *file_path) {
  bool is_directory;
  if (!ExistFilePath(file_path, &is_directory))
    return false;
  WStringPiece path(file_path); // file_path_ is CString, so it's ok to pass path.data() as LPCWSTR
  /*****
  To delete a read-only file, first you must remove the read-only attribute.
  To delete or rename a file, you must have either delete permission on the file, or delete child permission in the parent directory.
  To recursively delete the files in a directory, use the SHFileOperation function.
  To remove an empty directory, use the RemoveDirectory function.
  *****/
  if (is_directory) {
    SHFILEOPSTRUCTW sh_operations; memset(&sh_operations, 0, sizeof(sh_operations));
    size_t size = path.size() + 5;
    if (*(path.end() - 1) != L'\\')
      ++size;
    wchar_t *buf = (wchar_t*)::LocalAlloc(LMEM_ZEROINIT, size * sizeof(wchar_t));
    if (!buf) {
      /*boost::scoped_array<unsigned char> path_cp1251;
      ErrorSystem(::GetLastError(), "DeleteFilePath('%s'): LocalAlloc(%d) fails:",
      UTF16_CP1251_(path, path_cp1251), size * sizeof(wchar_t));*/
      ErrorSystem(::GetLastError(), "DeleteFilePath('%s'): LocalAlloc(%d) fails:",
        ConvertUTF16_CP1251(path).c_str(), size * sizeof(wchar_t));
      return false;
    }

    wmemcpy(buf, path.begin(), path.size());
    buf[size - 2] = L'*';
    buf[size - 3] = L'.';
    buf[size - 4] = L'*';
    buf[size - 5] = L'\\';

    sh_operations.pFrom = buf;
    sh_operations.wFunc = FO_DELETE;
    sh_operations.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR;

    DWORD dw = ::GetLastError();
    /* Do not use GetLastError with the return values of this function. */
    bool res = ::SHFileOperationW(&sh_operations) == 0; // use IFileOperation
    //***** shOperations.hwnd == 0 => ERROR_INVALID_HANDLE *****//
    ::SetLastError(dw);

    ::LocalFree((HLOCAL)buf);
    if (res) {
      if (::RemoveDirectoryW(path.data()) != FALSE) // RemoveDirectoryW work for both cases: (path || path + '\\')
        return true;
      else {
        /*boost::scoped_array<unsigned char> path_cp1251;
        ErrorSystem(::GetLastError(), "DeleteFilePath('%s'): RemoveDirectoryW fails:", UTF16_CP1251_(path, path_cp1251));*/
        ErrorSystem(::GetLastError(), "DeleteFilePath('%s'): RemoveDirectoryW fails:", ConvertUTF16_CP1251(path).c_str());
      }
    }
    else {
      // Trace(CPCL_TRACE_LEVEL_ERROR, "DeleteFilePath('%s'): SHFileOperationW fails:", ConvertUTF16_CP1251(path).c_str());
      ErrorSystem(::GetLastError(), "DeleteFilePath('%s'): SHFileOperationW fails:", ConvertUTF16_CP1251(path).c_str());
    }
  }
  else {
    // replace witn inline deleteFile() { if ::DeleteFileW() == FALSE; return ::DeleteFileW() != FALSE } - file can be unaccesible due function call but then become again online
    if (RemoveReadOnlyAttribute(path)) {
      if (::DeleteFileW(path.data()) != FALSE)
        return true;
#ifdef CPCL_USE_BOOST_THREAD_SLEEP
      boost::this_thread::sleep(boost::posix_time::milliseconds(0x100)); /* yaaaaaaaawn ^_^ */
#else
      ::SleepEx(0x100, TRUE); /* yaaaaaaaawn ^_^ */
#endif
      if (::DeleteFileW(path.data()) != FALSE)
        return true;

      ErrorSystem(::GetLastError(), "DeleteFilePath('%s'): DeleteFileW fails:", ConvertUTF16_CP1251(path).c_str());
    }
  }

  return false;
}

#if 0

size_t AvailableDiskSpaceMib(WStringPiece const &path) {
  ULARGE_INTEGER freeBytesAvailable;
  if (::GetDiskFreeSpaceExW(path.data(), &freeBytesAvailable, NULL, NULL) == FALSE) {
    ErrorSystem(::GetLastError(), "AvailableDiskSpaceMib('%s'): GetDiskFreeSpaceExW fails:", ConvertUTF16_CP1251(path).c_str());
    return 0;
  }
  return (size_t)(freeBytesAvailable.QuadPart >> 20);
}

#endif

} // namespace cpcl

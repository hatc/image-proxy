// file_util.h
#pragma once

#ifndef __CPCL_FILE_UTIL_H
#define __CPCL_FILE_UTIL_H

#include <cpcl/file_util.hpp>

namespace cpcl {

template<class CharType>
inline CharType* NormalizePath(CharType* path, size_t path_size = 0) { // return pointer at new end of string
  typedef os_path_traits<CharType> OSPathTraits;
  CharType const delimiter = OSPathTraits::Delimiter();
  CharType const invalid_delimiter = (delimiter == (CharType)'/') ? (CharType)'\\' : (CharType)'/';
  
  if (!path_size)
    path_size = std::char_traits<CharType>::length(path);
  
  for (size_t i = 0; i < path_size; ++i)
    if (invalid_delimiter == path[i])
      path[i] = delimiter;
  path += path_size - 1;
  while (delimiter == *path)
    --path;
  return path;
}
template<class CharType>
inline std::basic_string<CharType>& NormalizePath(std::basic_string<CharType>& path) {
  CharType* path_ = const_cast<CharType*>(path.c_str());
  CharType* path_delimiter = NormalizePath(path_, path.size());
  path.resize(path_delimiter - path_ + 1);
  return path;
}

inline std::basic_string<FilePathChar> Join(BasicStringPiece<FilePathChar> v, BasicStringPiece<FilePathChar> s) {
  typedef os_path_traits<FilePathChar> OSPathTraits;
  if (!v.empty()) { // BasicStringPiece::trim_tail(Char)
    FilePathChar const *head = v.data() - 1;
    FilePathChar const *i = head + v.size();
    for (; i != head; --i) {
      if (*i != OSPathTraits::Delimiter())
        break;
    }
    v = BasicStringPiece<FilePathChar>(v.data(), i - head);
  }
  if (!s.empty()) { // BasicStringPiece::trim_head(Char)
    FilePathChar const *i = s.data();
    FilePathChar const *tail = i + s.size();
    for (; i != tail; ++i) {
      if (*i != OSPathTraits::Delimiter())
        break;
    }
    s = BasicStringPiece<FilePathChar>(i, tail - i);
  }

  std::basic_string<FilePathChar> r;
  if (v.empty())
    return s.as_string();
  else if (s.empty())
    return v.as_string();
  r.reserve(v.size() + s.size() + 1);
  r.assign(v.data(), v.size());
  r.append(1, OSPathTraits::Delimiter());
  r.append(s.data(), s.size());
  return r;
}

/* on windows returns full file path to executable if hmodule == 0
 * or returns full file path to module(normally dll) with hmodule == HMODULE
 * on linux returns full file path to executable(/proc/self/exe), hmodule value ignored */
bool GetModuleFilePath(void *hmodule, std::basic_string<FilePathChar> *r);

bool GetTemporaryDirectory(std::basic_string<FilePathChar> *r);

bool ExistFilePath(FilePathChar const *file_path, bool *is_directory);
inline bool ExistFilePath(std::basic_string<FilePathChar> const &file_path, bool *is_directory) {
  return ExistFilePath(file_path.c_str(), is_directory);
}
// if apriori known that StringPiece points to C string, just use ExistFilePath(FilePathChar const *, ...) overload with StringPiece.data()

bool DeleteFilePath(FilePathChar const *file_path);
inline bool DeleteFilePath(std::basic_string<FilePathChar> const &file_path) {
  return DeleteFilePath(file_path.c_str());
}

/* bool GetPathComponents(WStringPiece const &path,
  std::wstring *basename, std::wstring *name, std::wstring *ext);
template<class CharType>
bool GetPathComponents(StringPiece<CharType> const &path,
  std::wstring *basename, std::wstring *name, std::wstring *ext) {
  NormalizePath;
  for (;;) {
    ...
  }
}
size_t AvailableDiskSpaceMib(WStringPiece const &path); ??? */

} // namespace cpcl

#endif // __CPCL_FILE_UTIL_H

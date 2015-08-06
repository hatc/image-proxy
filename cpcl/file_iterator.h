// file_iterator.h
#pragma once

#ifndef __CPCL_FILE_ITERATOR_H
#define __CPCL_FILE_ITERATOR_H

#include <cpcl/file_util.hpp>

// on linux convert /some/dir -> /some/dir/, opendir supports both variant
namespace cpcl {

class FileIterator {
  struct Handle;
  Handle *handle;
  // unsigned char handle_storage[sizeof(void*)]; // auto_ptr<Handle> handle;
  // or just use void* and explicit cast in Next() ?
  unsigned char handle_storage[0x280]; // FilterIterator under windows need to store WIN32_FIND_DATAW look-ahead value

  std::basic_string<FilePathChar> file_path;
  size_t dir_path_size; // include trailing slash

  std::basic_string<FilePathChar>& SetFilePath(BasicStringPiece<FilePathChar> file_name) {
    if (!dir_path_size) // first enrty, file_path contain search_pattern
      // [c:\\some\\dir\\, c:\\some\\dir\\*.*, c:\\some\\dir\\*.dll] -> DirName() + 1
      dir_path_size = DirName(BasicStringPiece<FilePathChar>(file_path)).size() + 1; // directory path size include trailing slash

    file_path.assign(file_path.c_str(), dir_path_size);
    file_path.append(file_name.data(), file_name.size());
    // using replace i.e.
    // file_path.replace(dir_path_size, file_path.size() - dir_path_size, file_name.data(), file_name.size())
    // seems as more native, but it's not))))
    // std::basic_string::replace (especially libstdc++) do much more memory allocation/deallocation

    return file_path;
  }

  void Dispose();

  DISALLOW_COPY_AND_ASSIGN(FileIterator);
public:
  struct FileInfo {
    enum /*FILE_*/ATTRIBUTE {
      /*FILE_*/ATTRIBUTE_INVALID = 0,
      /*FILE_*/ATTRIBUTE_DIRECTORY = 1,
      /*FILE_*/ATTRIBUTE_NORMAL = 1 << 1
    };
    int attributes;
    bool Normal() const { return !!(attributes & ATTRIBUTE_NORMAL); }
    bool Directory() const { return !!(attributes & ATTRIBUTE_DIRECTORY); }
    // Time atime, mtime, ctime;
    BasicStringPiece<FilePathChar> file_path;

    FileInfo() : attributes(ATTRIBUTE_INVALID)
    {}
  };

  FileIterator(BasicStringPiece<FilePathChar> const &search_path);
  ~FileIterator();

  bool Next(FileInfo *r);

  template<class FileInfoImpl>
  bool Next(FileInfoImpl *r) {
    FileInfo file_info;
    if (!Next(&file_info))
      return false;

    if (r) {
      r->file_path = this->file_path; // this->file_path is a std::string, used as greedy buffer, therefore FileInfoImpl::file_path can be StringPiece or std::string
      r->attributes = file_info.attributes;
    }
    return true;
  }
};

} // namespace cpcl

#endif // __CPCL_FILE_ITERATOR_H

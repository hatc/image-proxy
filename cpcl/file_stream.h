// file_stream.h
#pragma once

#ifndef __CPCL_FILE_STREAM_H
#define __CPCL_FILE_STREAM_H

#include <boost/shared_ptr.hpp>

#include <cpcl/string_piece.hpp>
#include <cpcl/io_stream.h>

namespace cpcl {

class FileStream : public IOStream {
  struct Deleter;
  boost::shared_ptr<Deleter> deleter;
#if defined(_MSC_VER)
  void *hFile;
  
  bool ReadPart(void *data, unsigned long size, unsigned long &processed_size); 
  bool WritePart(void const *data, unsigned long size, unsigned long &processed_size);
  cpcl::WStringPiece FilePath() const;
#else
  int fd;
#endif
  bool state;
  std::basic_string<FilePathChar> file_path;
  
  DISALLOW_COPY_AND_ASSIGN(FileStream);
#if defined(_MSC_VER)
  FileStream();
  
  static bool FileStreamCreate(WStringPiece const &file_path,
    unsigned long access_mode, unsigned long share_mode,
    unsigned long disposition, unsigned long attributes,
    FileStream **v);
#else
  explicit FileStream(int fd_);
  
  static bool FileStreamCreate(FilePathChar const *file_path,
    int flags, uint32 mode, char const *method_name, FileStream **v);
#endif
public:
  virtual ~FileStream();
  
  virtual bool operator!() { return !state; }
  virtual IOStream* Clone();
  virtual uint32 Read(void *data, uint32 size);
  virtual uint32 Write(void const *data, uint32 size);
  virtual bool Seek(int64 move_to, uint32 move_method, int64 *position);
  virtual int64 Tell();
  virtual int64 Size();

  static bool Create(FilePathChar const *file_path, FileStream **r);
  static bool CreateTemporary(FileStream **r);
  static bool Read(FilePathChar const *file_path, FileStream **r);
  static bool ReadWrite(FilePathChar const *file_path, FileStream **r);
};

} // namespace cpcl

#endif // __CPCL_FILE_STREAM_H

// memory_stream.h
#pragma once

#ifndef __CPCL_MEMORY_STREAM_H
#define __CPCL_MEMORY_STREAM_H

#include <stddef.h>
#include <cpcl/io_stream.h>

namespace cpcl {

class MemoryStream : public IOStream {
  bool state;
public:
  unsigned char *p;
  int32 p_size;
  int32 p_offset;
  bool throw_eof_write;

  void Assign(unsigned char *p_, size_t p_size_);
  unsigned char ReadByte();
  void WriteByte(unsigned char v);

  MemoryStream();
  MemoryStream(unsigned char *p_, size_t p_size_, bool throw_eof_write_ = false);
  virtual ~MemoryStream();

  virtual bool operator!() const { return !state; }
  virtual IOStream* Clone();
  virtual uint32 CopyTo(IOStream *output, uint32 size);
  virtual uint32 Read(void *data, uint32 size);
  virtual uint32 Write(void const *data, uint32 size);
  virtual bool Seek(int64 move_to, uint32 move_method, int64 *position);
  virtual int64 Tell();
  virtual int64 Size();
};

} // namespace cpcl

#endif // __CPCL_MEMORY_STREAM_H

// dynamic_memory_stream.h
#pragma once

#ifndef __CPCL_DYNAMIC_MEMORY_STREAM_H
#define __CPCL_DYNAMIC_MEMORY_STREAM_H

#include <stddef.h>
#include <cpcl/io_stream.h>

#include <boost/shared_ptr.hpp>

namespace cpcl {

class DynamicMemoryStream : public IOStream {
  struct MemoryStorage;
  
  bool state;
  boost::shared_ptr<MemoryStorage> memory_storage;
public:
  size_t p_size;
  size_t p_offset;
  
  /* unsigned char ReadByte();
  void WriteByte(unsigned char v); */
  void Clear() {
    p_size = p_offset = 0; state = true;
  }

  explicit DynamicMemoryStream(size_t block_size = 0x1000);
  virtual ~DynamicMemoryStream();

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

#endif // __CPCL_DYNAMIC_MEMORY_STREAM_H

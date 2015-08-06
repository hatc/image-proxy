// io_stream.h
#pragma once

#ifndef __CPCL_IO_STREAM_H
#define __CPCL_IO_STREAM_H

#include <cpcl/basic.h>

namespace cpcl {

class IOStream {
public:
  virtual ~IOStream();

  virtual bool operator!() { return false; }
  virtual IOStream* Clone() = 0; // returns a new stream object with its own seek pointer that references the same bytes as the original stream
  virtual uint32 CopyTo(IOStream *output, uint32 size); // return number of bytes written to output
  virtual uint32 Read(void *data, uint32 size) = 0;
  virtual uint32 Write(void const *data, uint32 size) = 0;
  virtual bool Seek(int64 move_to, uint32 move_method, int64 *position) = 0;
  virtual int64 Tell() = 0;
  virtual int64 Size() = 0;
};

} // namespace cpcl

#endif // __CPCL_IO_STREAM_H

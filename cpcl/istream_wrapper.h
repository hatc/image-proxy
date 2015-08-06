// istream_wrapper.h
#pragma once

#ifndef __CPCL_ISTREAM_WRAPPER_H
#define __CPCL_ISTREAM_WRAPPER_H

#include <cpcl/io_stream.h>

struct IStream;

namespace cpcl {

class IStreamWrapper : public IOStream {
  IStream *stream;
  
  IStreamWrapper();
public:
  long ecode;
  
  IStreamWrapper(IStream *v);
  IStreamWrapper(const IStreamWrapper &r);
  operator IStream*() const;
  IStream* operator=(IStream *v);
  IStreamWrapper& operator=(const IStreamWrapper &r);
  virtual ~IStreamWrapper();

  virtual operator bool() const;
  virtual IOStream* Clone();
  virtual unsigned long Read(void *data, unsigned long size);
  virtual unsigned long Write(void const *data, unsigned long size);
  virtual bool Seek(__int64 move_to, unsigned long move_method, __int64 *position);
  virtual __int64 Tell();
  virtual __int64 Size();
};

} // namespace cpcl

#endif // __CPCL_ISTREAM_WRAPPER_H

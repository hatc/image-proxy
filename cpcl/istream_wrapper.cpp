#include "istream_wrapper.h"

#define WIN32_LEAN_AND_MEAN
#define INC_OLE2
#include <Objidl.h>

#include "com_ptr.hpp"

namespace cpcl {

IStreamWrapper::IStreamWrapper(IStream *v) : ecode(S_OK) {
  if (v)
    v->AddRef();
  stream = v;
}
IStreamWrapper::IStreamWrapper(const IStreamWrapper &r) : ecode(r.ecode) {
  if (r.stream)
    r.stream->AddRef();
  stream = r.stream;
}
IStreamWrapper::operator IStream*() const {
  return stream;
}
IStream* IStreamWrapper::operator=(IStream *v) {
  if (v)
    v->AddRef();
  if (stream)
    stream->Release();
  stream = v;
  ecode = S_OK;
  return stream;
}
/*IStream* IStreamWrapper::operator=(const IStreamWrapper &r) {
 return (*this = r.stream);
 }*/IStreamWrapper& IStreamWrapper::operator=(const IStreamWrapper &r) {
  *this = r.stream; // i.e. call IStream* IStreamWrapper::operator=(IStream *v) {}
  return *this;
}
IStreamWrapper::~IStreamWrapper() {
  if (stream)
    stream->Release();
}

inline bool Succeeded(long v) { return v >= 0; }
IStreamWrapper::operator bool() const {
  return Succeeded(ecode);
}

IOStream* IStreamWrapper::Clone() {
  IStreamWrapper *r(NULL);
  ComPtr<IStream> stream_;
  if (Succeeded(ecode = stream->Clone(stream_.GetAddressOf()))) {
    // i.e. ecode = stream->Clone(stream_.GetAddressOf()); if (*this) {
    r = new IStreamWrapper(stream_);
    stream_.Detach();
  }

  return r;
}

unsigned long IStreamWrapper::Read(void *data, unsigned long size) {
  ULONG processed(0);
  ecode = stream->Read(data, static_cast<ULONG>(size), &processed);
  return static_cast<unsigned long>(processed);
}

unsigned long IStreamWrapper::Write(void const *data, unsigned long size) {
  ULONG processed(0);
  ecode = stream->Write(data, static_cast<ULONG>(size), &processed);
  return static_cast<unsigned long>(processed);
}

#ifndef INT64_MAX
static __int64 const INT64_MAX = (~(unsigned __int64)0) >> 1;
#endif
bool IStreamWrapper::Seek(__int64 move_to, unsigned long move_method, __int64 *position) {
  LARGE_INTEGER offset; offset.QuadPart = move_to;
  ULARGE_INTEGER position_;
  if (Succeeded(ecode = stream->Seek(offset, static_cast<DWORD>(move_method), &position_))) {
    if (position)
      *position = static_cast<__int64>(position_.QuadPart & INT64_MAX);
    return true;
  }
  return false;
}

__int64 IStreamWrapper::Tell() {
  LARGE_INTEGER move_to; move_to.QuadPart = 0;
  ULARGE_INTEGER position;
  return (Succeeded(ecode = stream->Seek(move_to, STREAM_SEEK_CUR, &position))) ? static_cast<__int64>(position.QuadPart & INT64_MAX) : -1;
}

__int64 IStreamWrapper::Size() {
  STATSTG statstg = { 0 };
  return (Succeeded(ecode = stream->Stat(&statstg, STATFLAG_NONAME))) ? static_cast<__int64>(statstg.cbSize.QuadPart & INT64_MAX) : -1;
}

} // namespace cpcl


// istream_impl.h
#pragma once

#ifndef __CPCL_ISTREAM_IMPL_H
#define __CPCL_ISTREAM_IMPL_H

#include <cpcl/io_stream.h>
#include <cpcl/internal_com.h>

#include <Objidl.h>

namespace cpcl {

class IStreamImpl : public CUnknownImp, public IStream {
  IOStream *stream;
  bool owner;
  
  IStreamImpl();
  IStreamImpl(IStreamImpl const&);
  void operator=(IStreamImpl const&);
public:
  IStreamImpl(IOStream *stream_, bool owner_ = false);
  virtual ~IStreamImpl();

  STDMETHOD(Read)(/* [length_is][size_is][out] */ void* pv,
    /* [in] */ ULONG cb,
    /* [out] */ ULONG* pcbRead);
  STDMETHOD(Write)(/* [size_is][in] */ const void* pv,
    /* [in] */ ULONG cb,
    /* [out] */ ULONG* pcbWritten);
  STDMETHOD(Seek)(/* [in] */ LARGE_INTEGER dlibMove,
    /* [in] */ DWORD dwOrigin,
    /* [out] */ ULARGE_INTEGER* plibNewPosition);
  STDMETHOD(SetSize)(/* [in] */ ULARGE_INTEGER libNewSize);
  STDMETHOD(CopyTo)(/* [unique][in] */ IStream *pstm,
    /* [in] */ ULARGE_INTEGER cb,
    /* [out] */ ULARGE_INTEGER* pcbRead,
    /* [out] */ ULARGE_INTEGER* pcbWritten);
  STDMETHOD(Commit)(/* [in] */ DWORD /*grfCommitFlags*/);
  STDMETHOD(Revert)(void);
  STDMETHOD(LockRegion)(/* [in] */ ULARGE_INTEGER /*libOffset*/,
    /* [in] */ ULARGE_INTEGER /*cb*/,
    /* [in] */ DWORD /*dwLockType*/);
  STDMETHOD(UnlockRegion)(/* [in] */ ULARGE_INTEGER /*libOffset*/,
    /* [in] */ ULARGE_INTEGER /*cb*/,
    /* [in] */ DWORD /*dwLockType*/);
  STDMETHOD(Stat)(/* [out] */ STATSTG* pstatstg,
    /* [in] */ DWORD grfStatFlag);
  STDMETHOD(Clone)(/* [out] */ IStream** ppstm);

  CPCL_UNKNOWN_IMP1_MT(IStream)
};

} // namespace cpcl

#endif // __CPCL_ISTREAM_IMPL_H

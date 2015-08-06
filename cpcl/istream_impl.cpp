#include "basic.h"
#include "targetver.h"

#include <stdio.h> // _snwprintf_s

#define WIN32_LEAN_AND_MEAN
#define INC_OLE2
#include "com_ptr.hpp"
#include "istream_impl.h"

namespace cpcl {

IStreamImpl::IStreamImpl(IOStream *stream_, bool owner_) : stream(stream_), owner(owner_)
{}
IStreamImpl::~IStreamImpl() {
  if (owner)
    delete stream;
}

STDMETHODIMP IStreamImpl::Read(void* pv, ULONG cb, ULONG* pcbRead) {
  unsigned long const processed = stream->Read(pv, cb);
  if (pcbRead)
    *pcbRead = processed;
  if (!(*stream))
    return E_FAIL;
  return (processed == cb) ? S_OK : S_FALSE;
}

STDMETHODIMP IStreamImpl::Write(const void* pv, ULONG cb, ULONG* pcbWritten) {
  unsigned long const processed = stream->Write(pv, cb);
  if (pcbWritten)
    *pcbWritten = processed;
  if (!(*stream))
    return E_FAIL;
  return (processed == cb) ? S_OK : S_FALSE;
}

STDMETHODIMP IStreamImpl::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition) {
  if (dwOrigin > 2)
    return STG_E_INVALIDFUNCTION; // switch(dwOrigin) { case STREAM_SEEK_SET: ... break; case STREAM_SEEK_CUR: ... break; case STREAM_SEEK_END: ... break; default: return STG_E_INVALIDFUNCTION; }

  __int64 position;
  if (!stream->Seek(dlibMove.QuadPart, dwOrigin, &position))
    return E_FAIL;
  if (plibNewPosition)
    (*plibNewPosition).QuadPart = position;
  return S_OK;
}

STDMETHODIMP IStreamImpl::SetSize(ULARGE_INTEGER libNewSize) {
  if (((unsigned __int64)stream->Tell()) > libNewSize.QuadPart)
    return STG_E_INVALIDFUNCTION;
  else
    return STG_E_MEDIUMFULL;
}

STDMETHODIMP IStreamImpl::CopyTo(IStream *pstm, // A pointer to the destination stream. The stream pointed to by pstm can be a new stream or a clone of the source stream.
  ULARGE_INTEGER cb, // The number of bytes to copy from the source stream.
  ULARGE_INTEGER* pcbRead, // A pointer to the location where this method writes the actual number of bytes read from the source. You can set this pointer to NULL. In this case, this method does not provide the actual number of bytes read.
  ULARGE_INTEGER* pcbWritten) { // A pointer to the location where this method writes the actual number of bytes written to the destination. You can set this pointer to NULL. In this case, this method does not provide the actual number of bytes written.
  /* This method is equivalent to reading cb bytes into memory using ISequentialStream::Read and then immediately
  writing them to the destination stream using ISequentialStream::Write, although IStream::CopyTo will be more efficient.
  The destination stream can be a clone of the source stream created by calling the IStream::Clone method. */
  HRESULT hr(S_OK);
  unsigned char buf[0x1000];
  unsigned __int64 to_read = cb.QuadPart;
  unsigned long read = min((unsigned long)to_read, sizeof(buf));
  unsigned long readed, written;
  unsigned __int64 total_read(0), total_written(0);
  while ((readed = stream->Read(buf, read)) > 0) {
    total_read += readed;

    if ((hr = pstm->Write(buf, readed, &written)) != S_OK)
      break;
    total_written += written;

    if ((to_read -= readed) == 0)
      break;
    read = min((unsigned long)to_read, sizeof(buf));
  }
  if (pcbWritten)
    pcbWritten->QuadPart = total_written;
  if (pcbRead)
    pcbRead->QuadPart = total_read;

  return hr;
}

STDMETHODIMP IStreamImpl::Commit(DWORD /*grfCommitFlags*/) { return S_OK; }

STDMETHODIMP IStreamImpl::Revert(void) { return S_OK; }

STDMETHODIMP IStreamImpl::LockRegion(/* [in] */ ULARGE_INTEGER /*libOffset*/,
  /* [in] */ ULARGE_INTEGER /*cb*/,
  /* [in] */ DWORD /*dwLockType*/) {
  return STG_E_INVALIDFUNCTION;
}

STDMETHODIMP IStreamImpl::UnlockRegion(/* [in] */ ULARGE_INTEGER /*libOffset*/,
  /* [in] */ ULARGE_INTEGER /*cb*/,
  /* [in] */ DWORD /*dwLockType*/) {
  return STG_E_INVALIDFUNCTION;
}

STDMETHODIMP IStreamImpl::Stat(/* [out] */ STATSTG* pstatstg,
  /* [in] */ DWORD grfStatFlag) {
  if (!pstatstg)
    return STG_E_INVALIDPOINTER;
  if (grfStatFlag > 1)
    return STG_E_INVALIDFLAG;

  memset(pstatstg, 0, sizeof(STATSTG));
  if (STATFLAG_DEFAULT == grfStatFlag) {
    wchar_t s[0x100];
    int l_ = _snwprintf_s(s, _TRUNCATE, L"iostream at %08X", stream);
    size_t l = (l_ >= 0) ? (((size_t)l_) + 1) : sizeof(s) / sizeof(s[0]);

    pstatstg->pwcsName = (wchar_t*)::CoTaskMemAlloc(l * sizeof(wchar_t));
    memcpy(pstatstg->pwcsName, s, l * sizeof(wchar_t));
  }
  pstatstg->type = STGTY_STREAM;
  pstatstg->cbSize.QuadPart = stream->Size();
  pstatstg->clsid = CLSID_NULL;
  pstatstg->grfLocksSupported = 0;/* LOCK_WRITE; */

  return S_OK;
}

STDMETHODIMP IStreamImpl::Clone(/* [out] */ IStream** ppstm) {
  if (!ppstm)
    return STG_E_INVALIDPOINTER;

  IOStream *stream_ = stream->Clone();
  if (!stream_)
    return STG_E_INSUFFICIENTMEMORY;

  ComPtr<IStreamImpl> stream_ptr(new IStreamImpl(stream_, true));
  *ppstm = stream_ptr.Detach();
  return S_OK;
}

} // namespace cpcl

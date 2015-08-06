#include "basic.h"

#include "string_util.h"
#include "file_util.h"
#include "file_istream.h"
#include "trace.h"

namespace cpcl {

FileIStream::FileIStream() : stream_impl(NULL)
{}
FileIStream::~FileIStream() {
  delete stream_impl;
}

STDMETHODIMP FileIStream::Read(void* pv, ULONG cb, ULONG* pcbRead) {
  unsigned long const processed = stream_impl->Read(pv, cb);
  if (pcbRead)
    *pcbRead = processed;
  if (!(*stream_impl))
    return E_FAIL;
  return (processed == cb) ? S_OK : S_FALSE;
}

STDMETHODIMP FileIStream::Write(const void* pv, ULONG cb, ULONG* pcbWritten) {
  unsigned long const processed = stream_impl->Write(pv, cb);
  if (pcbWritten)
    *pcbWritten = processed;
  if (!(*stream_impl))
    return E_FAIL;
  return (processed == cb) ? S_OK : S_FALSE;
}

STDMETHODIMP FileIStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition) {
  if (dwOrigin > 2)
    return STG_E_INVALIDFUNCTION; // switch(dwOrigin) { case STREAM_SEEK_SET: ... break; case STREAM_SEEK_CUR: ... break; case STREAM_SEEK_END: ... break; default: return STG_E_INVALIDFUNCTION; }

  __int64 position;
  if (!stream_impl->Seek(dlibMove.QuadPart, dwOrigin, &position))
    return E_FAIL;
  if (plibNewPosition)
    (*plibNewPosition).QuadPart = position;
  return S_OK;
}

#ifndef INT64_MAX
static __int64 const INT64_MAX = (~(unsigned __int64)0) >> 1;
#endif
STDMETHODIMP FileIStream::SetSize(ULARGE_INTEGER libNewSize) {
  __int64 position, move_to(libNewSize.QuadPart & INT64_MAX);
  if (!stream_impl->Seek(move_to, STREAM_SEEK_SET, &position))
    return STG_E_INVALIDFUNCTION;
  if (position != move_to)
    return E_FAIL;

  if (::SetEndOfFile(stream_impl->hFile) == 0) {
    ErrorSystem(::GetLastError(), "FileIStream::SetSize('%s', %I64u): SetEndOfFile fails:",
      ConvertUTF16_CP1251(stream_impl->Path()).c_str(), move_to);
    return STG_E_MEDIUMFULL;
  }
  return S_OK;
}

STDMETHODIMP FileIStream::CopyTo(IStream *pstm, // A pointer to the destination stream. The stream pointed to by pstm can be a new stream or a clone of the source stream.
  ULARGE_INTEGER cb, // The number of bytes to copy from the source stream.
  ULARGE_INTEGER* pcbRead, // A pointer to the location where this method writes the actual number of bytes read from the source. You can set this pointer to NULL. In this case, this method does not provide the actual number of bytes read.
  ULARGE_INTEGER* pcbWritten) { // A pointer to the location where this method writes the actual number of bytes written to the destination. You can set this pointer to NULL. In this case, this method does not provide the actual number of bytes written.
  /*
  This method is equivalent to reading cb bytes into memory using ISequentialStream::Read and then immediately
  writing them to the destination stream using ISequentialStream::Write, although IStream::CopyTo will be more efficient.
  The destination stream can be a clone of the source stream created by calling the IStream::Clone method.
  */
  HRESULT hr(S_OK);
  unsigned char buf[0x1000];
  unsigned __int64 to_read = cb.QuadPart;
  unsigned long read = min((unsigned long)to_read, sizeof(buf));
  unsigned long readed, written;
  unsigned __int64 total_read(0), total_written(0);
  while ((readed = stream_impl->Read(buf, read)) > 0) {
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

STDMETHODIMP FileIStream::Commit(DWORD /*grfCommitFlags*/) { return S_OK; }

STDMETHODIMP FileIStream::Revert(void) { return S_OK; }

STDMETHODIMP FileIStream::LockRegion(/* [in] */ ULARGE_INTEGER /*libOffset*/,
  /* [in] */ ULARGE_INTEGER /*cb*/,
  /* [in] */ DWORD /*dwLockType*/) {
  return STG_E_INVALIDFUNCTION;
}

STDMETHODIMP FileIStream::UnlockRegion(/* [in] */ ULARGE_INTEGER /*libOffset*/,
  /* [in] */ ULARGE_INTEGER /*cb*/,
  /* [in] */ DWORD /*dwLockType*/) {
  return STG_E_INVALIDFUNCTION;
}

STDMETHODIMP FileIStream::Stat(/* [out] */ STATSTG* pstatstg,
  /* [in] */ DWORD grfStatFlag) {
  if (!pstatstg)
    return STG_E_INVALIDPOINTER;

  memset(pstatstg, 0, sizeof(STATSTG));
  if (STATFLAG_DEFAULT == grfStatFlag) {
    WStringPiece path = stream_impl->Path();
    if (!path.empty()) {
      pstatstg->pwcsName = (wchar_t*)::CoTaskMemAlloc((path.size() + 1) * sizeof(wchar_t));
      memcpy(pstatstg->pwcsName, path.data(), path.size() * sizeof(wchar_t));
      *(pstatstg->pwcsName + path.size()) = 0;
    }
  }
  pstatstg->type = STGTY_STREAM;
  pstatstg->cbSize.QuadPart = stream_impl->Size();
  pstatstg->clsid = CLSID_NULL;
  pstatstg->grfLocksSupported = 0;/* LOCK_WRITE; */

  return S_OK;
}

STDMETHODIMP FileIStream::Clone(/* [out] */ IStream** ppstm) {
  if (!ppstm)
    return STG_E_INVALIDPOINTER;
  // file open with FILE_ATTRIBUTE_TEMPORARY flag - all data can be not flushed to disk
  //::FlushFileBuffers(handle_);

  /* if clone_(shared / access)_mode == 0
  return STG_E_INSUFFICIENTMEMORY The stream was not cloned due to a lack of memory. */

  ComPtr<FileIStream> stream(new FileIStream());
  if (!FileStream::FileStreamCreate(stream_impl->Path(), clone_access_mode, clone_share_mode,
    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, &stream->stream_impl))
    return E_FAIL;
  //HANDLE handle = ::CreateFileW(path.c_str(),
  //	cloneAccessMode_,
  //	cloneShareMode_,
  //	(LPSECURITY_ATTRIBUTES)NULL,
  //	OPEN_EXISTING, // When opening an existing file(dwCreationDisposition set to OPEN_EXISTING)
  //	FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL); // all FILE_ATTRIBUTE_* specified by dwFlagsAndAttributes ignored
  //if (INVALID_HANDLE_VALUE == handle)
  //	return E_FAIL;

  stream->clone_access_mode = clone_access_mode;
  stream->clone_share_mode = clone_share_mode;
  stream->stream_impl->Seek(stream_impl->Tell(), STREAM_SEEK_SET, NULL);

  *ppstm = stream.Detach();
  return S_OK;
}

bool FileIStream::Create(cpcl::WStringPiece const &path, FileIStream **v) {
  ComPtr<FileIStream> r(new FileIStream());
  if (!FileStream::Create(path, &r->stream_impl))
    return false;

  r->clone_access_mode = GENERIC_READ;
  r->clone_share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;
  if (v)
    *v = r.Detach();
  return true;
}

bool FileIStream::Read(cpcl::WStringPiece const &path, FileIStream **v) {
  ComPtr<FileIStream> r(new FileIStream());
  if (!FileStream::Read(path, &r->stream_impl))
    return false;

  r->clone_access_mode = GENERIC_READ;
  r->clone_share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
  if (v)
    *v = r.Detach();
  return true;
}

bool FileIStream::ReadWrite(cpcl::WStringPiece const &path, FileIStream **v) {
  ComPtr<FileIStream> r(new FileIStream());
  if (!FileStream::ReadWrite(path, &r->stream_impl))
    return false;

  r->clone_access_mode = GENERIC_READ | GENERIC_WRITE;
  r->clone_share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
  if (v)
    *v = r.Detach();
  return true;
}

bool FileIStream::CreateTemporary(FileIStream **v) {
  ComPtr<FileIStream> r(new FileIStream());
  if (!FileStream::CreateTemporary(&r->stream_impl))
    return false;

  r->clone_access_mode = GENERIC_READ;
  r->clone_share_mode = FILE_SHARE_READ | FILE_SHARE_DELETE;
  if (v)
    *v = r.Detach();
  return true;
}

} // namespace cpcl

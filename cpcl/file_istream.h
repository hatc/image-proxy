// file_istream.h
#pragma once

#ifndef __CPCL_FILE_ISTREAM_H
#define __CPCL_FILE_ISTREAM_H

#include <cpcl/string_piece.hpp>
#include <cpcl/file_stream.h>
#include <cpcl/internal_com.h>

#include <windows.h>
#include <Objidl.h>

namespace cpcl {

class FileIStream : public IStream, public CUnknownImp {
  DWORD clone_access_mode; 
  DWORD clone_share_mode;

  FileIStream();
  FileIStream(FileIStream const&);
  void operator=(FileIStream const&);
public:
  FileStream *stream_impl;

  CPCL_UNKNOWN_IMP1_MT(IStream)

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

  virtual ~FileIStream();

  // WStringPiece Path() const; // return WStringPiece(path.c_str() + 4, path.size() - 4);

  static bool Create(WStringPiece const &path, FileIStream **v); // NormalizePath(FileStream.path); FileStream.path.insert(0, L"\\\\?\\");
  static bool CreateTemporary(FileIStream **v); // use CreateGUID as random name
  static bool Read(WStringPiece const &path, FileIStream **v);
  static bool ReadWrite(WStringPiece const &path, FileIStream **v);
};

} // namespace cpcl

#endif // __CPCL_FILE_ISTREAM_H

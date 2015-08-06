// internal_com.h
#pragma once

#ifndef __CPCL_INTERNAL_COM_H
#define __CPCL_INTERNAL_COM_H

#include <cpcl/com_ptr.hpp>

#ifndef RINOK
#define RINOK(x) { HRESULT __result_ = (x); if (__result_ != S_OK) return __result_; }
#endif

#define CPCL_DECLARE_INTERFACE(guid) struct __declspec(uuid(guid)) __declspec(novtable)

class CUnknownImp
{
public:
  ULONG __m_RefCount;
  CUnknownImp(): __m_RefCount(0) {}
};

// why { if (iid == __uuidof(i)) ... } work ???
//
//#include <Guiddef.h>
//...
//#ifdef __cplusplus
//__inline int operator==(REFGUID guidOne, REFGUID guidOther)
//{
//    return IsEqualGUID(guidOne,guidOther);
//}
//
//__inline int operator!=(REFGUID guidOne, REFGUID guidOther)
//{
//    return !(guidOne == guidOther);
//}
//#endif

#define CPCL_QUERYINTERFACE_BEGIN STDMETHOD(QueryInterface) \
    (REFGUID iid, void **outObject) {

#define CPCL_QUERYINTERFACE_ENTRY(i) if (iid == __uuidof(i)) \
    { *outObject = (void *)(i *)this; AddRef(); return S_OK; }

#define CPCL_QUERYINTERFACE_ENTRY_UNKNOWN(i) if (iid == __uuidof(IUnknown)) \
    { *outObject = (void *)(IUnknown *)(i *)this; AddRef(); return S_OK; }

#define CPCL_QUERYINTERFACE_BEGIN2(i) CPCL_QUERYINTERFACE_BEGIN \
    CPCL_QUERYINTERFACE_ENTRY_UNKNOWN(i) \
    CPCL_QUERYINTERFACE_ENTRY(i)

#define CPCL_QUERYINTERFACE_END return E_NOINTERFACE; }

#define CPCL_ADDREF_RELEASE_MT \
STDMETHOD_(ULONG, AddRef)() { InterlockedIncrement((LONG *)&__m_RefCount); return __m_RefCount; } \
STDMETHOD_(ULONG, Release)() { InterlockedDecrement((LONG *)&__m_RefCount); if (__m_RefCount != 0) \
  return __m_RefCount; delete this; return 0; }

#define CPCL_UNKNOWN_IMP_SPEC_MT(i) \
  CPCL_QUERYINTERFACE_BEGIN \
  CPCL_QUERYINTERFACE_ENTRY(IUnknown) \
  i \
  CPCL_QUERYINTERFACE_END \
  CPCL_ADDREF_RELEASE_MT

#define CPCL_UNKNOWN_IMP_SPEC_MT2(i1, i) \
  CPCL_QUERYINTERFACE_BEGIN \
  if (iid == __uuidof(IUnknown)) \
      { *outObject = (void *)(IUnknown *)(i1 *)this; AddRef(); return S_OK; }  i \
  CPCL_QUERYINTERFACE_END \
  CPCL_ADDREF_RELEASE_MT

#define CPCL_UNKNOWN_IMP_MT CPCL_UNKNOWN_IMP_SPEC_MT(;)

#define CPCL_UNKNOWN_IMP1_MT(i) CPCL_UNKNOWN_IMP_SPEC_MT2( \
  i, \
  CPCL_QUERYINTERFACE_ENTRY(i) \
  )

#define CPCL_UNKNOWN_IMP2_MT(i1, i2) CPCL_UNKNOWN_IMP_SPEC_MT2( \
  i1, \
  CPCL_QUERYINTERFACE_ENTRY(i1) \
  CPCL_QUERYINTERFACE_ENTRY(i2) \
  )

#define CPCL_UNKNOWN_IMP3_MT(i1, i2, i3) CPCL_UNKNOWN_IMP_SPEC_MT2( \
  i1, \
  CPCL_QUERYINTERFACE_ENTRY(i1) \
  CPCL_QUERYINTERFACE_ENTRY(i2) \
  CPCL_QUERYINTERFACE_ENTRY(i3) \
  )

#endif // __CPCL_INTERNAL_COM_H

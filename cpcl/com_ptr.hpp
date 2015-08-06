// com_ptr.h
#pragma once

#ifndef __CPCL_COM_PTR_H
#define __CPCL_COM_PTR_H

namespace cpcl {

template <class T>
class ComPtr {
  T* p;
public:
  ComPtr() : p((T*)0) {}
  ComPtr(T* p_) { if ((p = p_) != NULL) p->AddRef(); }
  ComPtr(ComPtr<T> const &r) { if ((p = r.p) != NULL) p->AddRef(); }
  ~ComPtr() { if (p) p->Release(); }
  void Release() { if (p) { p->Release(); p = NULL; } }
  operator T*() const { return (T*)p; }
  /* C++98/03 prohibited elements of STL containers from overloading their address-of operator.
  This is what classes like CComPtr do, so helper classes like CAdapt were required to shield the STL from such overloads
  CopyConstructible required for store in stl containers
  http://www.rsdn.ru/forum/cpp/2773658.flat.aspx */
  T** ReleaseAndGetAddressOf() { Release(); return &p; }
  T** GetAddressOf() { /* ASSERT(p == NULL); */ return &p; }
  T* operator->() const { return p; }
  T* operator=(T *p_) {
    if (p_)
      p_->AddRef();
    if (p)
      p->Release();
    return (p = p_);
  }
  T* operator=(ComPtr<T> const &r) { return (*this = r.p); }
  /* ComPtr<T>& operator=(ComPtr<T> const &r) { *this = r.p; return (*this); } */
  bool operator!() const { return (p == NULL); }

  void Attach(T *p_) { Release(); p = p_; }
  T* Detach() {
    T *p_ = p;
    p = NULL;
    return p_;
  }
};

} // namespace cpcl

#endif // __CPCL_COM_PTR_H

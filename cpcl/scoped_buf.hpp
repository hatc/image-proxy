// scoped_buf.hpp - `greedy` buffer for storage within the current scope
#pragma once

#ifndef __CPCL_SCOPED_BUF_HPP
#define __CPCL_SCOPED_BUF_HPP

#include <stddef.h>
#include <algorithm> // std::swap
#include <cpcl/dumbassert.h>

namespace cpcl {

template<class T, size_t Count>
struct ScopedBuf {
  template<class U, size_t N> friend struct ScopedBuf;

  ScopedBuf() : p(NULL), p_owner(true), p_size(0), p_capacity(Count)
  {}
  ~ScopedBuf() {
    Release();
  }
  
  T* Alloc(size_t v) {
    if (p_capacity < v) {
      Release();
      if (v > Count) {
        p = new T[v];
        p_capacity = v;
      }
    }
    p_size = v;
    return Data();
  }
  void Attach(T *p_, size_t p_size_) {
    Release();
    p = p_;
    p_capacity = p_size = p_size_;
    p_owner = false;
  }
  void Release() {
    if ((p) && p_owner) {
      delete[] p;
      //p_capacity = Count;
    }
    p = NULL;
    p_capacity = Count;
    p_size = 0;
    p_owner = true;
  }
  T* Data() {
    DUMBASS_CHECK_EXPLANATION(p_size, "ScopedBuf::Data(): buffer is empty");
    return (p) ? p : buf;
  }
  size_t Size() const { return p_size; }

  template<size_t N>
  void swap(ScopedBuf<T, N> &r) {
    /* unsigned char tmp[N < Count ? N : Count];
    memcpy(tmp, buf, sizeof(tmp));
    memcpy(buf, r.buf, sizeof(tmp));
    memcpy(r.buf, tmp, sizeof(tmp)); */
    DUMBASS_CHECK_EXPLANATION((p || !p_size) && (r.p || !r.p_size), "can't swap static buffer");
    std::swap(p, r.p);
    std::swap(p_owner, r.p_owner);
    std::swap(p_size, r.p_size);
    std::swap(p_capacity, r.p_capacity);
  }

private:
  T buf[Count];
  T *p;
  bool p_owner;
  size_t p_size;
  size_t p_capacity;

  ScopedBuf(ScopedBuf<T, Count> const&);
  void operator=(ScopedBuf<T, Count> const&);
};

template<class T>
struct ScopedBuf<T, 0> {
  template<class U, size_t N> friend struct ScopedBuf;

  ScopedBuf() : p(NULL), p_owner(true), p_size(0), p_capacity(0)
  {}
  ~ScopedBuf() {
    Release();
  }

  T* Alloc(size_t v) {
    if (p_capacity < v) {
      Release();
      p = new T[v];
      p_capacity = v;
    }
    p_size = v;
    return Data();
  }
  void Attach(T *p_, size_t p_size_) {
    Release();
    p = p_;
    p_capacity = p_size = p_size_;
    p_owner = false;
  }
  void Release() {
    if ((p) && p_owner) {
      delete[] p;
    }
    p = NULL;
    p_capacity = p_size = 0;
    p_owner = true;
  }
  T* Data() const {
    DUMBASS_CHECK_EXPLANATION(p_size, "ScopedBuf::Data(): buffer is empty");
    return p;
  }
  size_t Size() const { return p_size; }

  template<size_t N>
  void swap(ScopedBuf<T, N> &r) {
    DUMBASS_CHECK_EXPLANATION(r.p || !r.p_size, "can't swap static buffer"); // if (!r.p && r.p_size) assert(False)
    std::swap(p, r.p);
    std::swap(p_owner, r.p_owner);
    std::swap(p_size, r.p_size);
    std::swap(p_capacity, r.p_capacity);
  }

private:
  T *p;
  bool p_owner;
  size_t p_size;
  size_t p_capacity;

  ScopedBuf(ScopedBuf<T, 0> const&);
  void operator=(ScopedBuf<T, 0> const&);
};

template<class T, size_t N, size_t K>
inline void swap(ScopedBuf<T, N> &l, ScopedBuf<T, K> &r) {
  l.swap(r);
}

} // namespace cpcl

/* swap() not seems as valid method for simple, non-copyable, greedy buffer
so std::swap(buf0, buf1) also not valid expression
for internal use swap(buf0, buf1) may be acceptable, if known exactly what is going on

namespace std {
template<class T, size_t N, size_t K>
inline void swap(cpcl::ScopedBuf<T, N> &l, cpcl::ScopedBuf<T, K> &r) {
  l.swap(r);
}
} */

#endif // __CPCL_SCOPED_BUF_HPP

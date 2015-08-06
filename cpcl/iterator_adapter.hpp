// iterator_adapter.hpp - template wrappers to create STL compatible iterators
#pragma once

#ifndef __CPCL_ITERATOR_ADAPTER_HPP
#define __CPCL_ITERATOR_ADAPTER_HPP

#include <iterator>
#include <stdexcept> // std::runtime_error

namespace cpcl {

// Iterator : no requirements
// Value    : DefaultConstructible, Assignable
template<class Iterator, class Value = typename Iterator::value_type>
struct InputIteratorAdapter : std::iterator < std::input_iterator_tag, Value > {
  typedef InputIteratorAdapter<Iterator, Value> Self;
  Iterator *it;
  Value value; // look-ahead value
  
  InputIteratorAdapter() : it((Iterator*)0) //, value() - for primitive types may left uninitialized because operator*() check it; for user types default ctor is called in any case
  {}
  InputIteratorAdapter(Iterator *it) : it(it) {
    GetValue();
  }

  Value operator*() const {
    if (!it)
      throw std::runtime_error("InputIteratorAdapter is not dereferencable");
    return value;
  }

  bool operator==(Self const &r) const {
    return it == r.it;
  }
  bool operator!=(Self const &r) const {
    return !(*this == r);
  }

  Self& operator++() { // preincrement
    if (!it)
      throw std::runtime_error("InputIteratorAdapter is not incrementable");
    GetValue();
    return *this;
  }

  Self /*void*/ operator++(int) { // postincrement
    Self tmp = *this; // Iterator pointer && value copy
    ++*this;
    return tmp;
  }
private:
  void GetValue() {
    if (!it->Next(&value))
      it = 0;
  }
};

// Iterator : CopyConstructible(assume lightweight copy constructor), DefaultConstructible not required
// Value    : DefaultConstructible, Assignable
template<class Iterator, class Value = typename Iterator::value_type>
struct InputIteratorCopyAdapter : std::iterator < std::input_iterator_tag, Value > {
  typedef InputIteratorCopyAdapter<Iterator, Value> Self;
  unsigned char it_storage[sizeof(Iterator)];
  Iterator *it;
  Value value; // look-ahead value

  InputIteratorCopyAdapter() : it(0)
  {}
  InputIteratorCopyAdapter(Iterator const &r) {
    it = new (it_storage)Iterator(r); // Iterator -> TrivialIterator -> Assignable -> CopyConstructible
    GetValue();
  }
  ~InputIteratorCopyAdapter() {
    Release();
  }

  InputIteratorCopyAdapter(Self const &r) : it(0) {
    if (r.it) {
      it = new (it_storage)Iterator(*r.it);
      value = r.value;
    }
  }
  Self& operator=(Self const &r) {
    if (r.it != it) {
      Release();
      if (r.it) {
        it = new (it_storage)Iterator(*r.it);
        value = r.value;
      }
    }
    return *this;
  }

  Value operator*() const {
    if (!it)
      throw std::runtime_error("InputIteratorCopyAdapter is not dereferencable");
    return value;
  }

  bool operator==(Self const &r) const {
    return it == r.it;
  }
  bool operator!=(Self const &r) const {
    return !(*this == r);
  }

  Self& operator++() { // preincrement
    if (!it)
      throw std::runtime_error("InputIteratorCopyAdapter is not incrementable");
    GetValue();
    return *this;
  }

  Self /*void*/ operator++(int) { // postincrement
    Self tmp = *this; // Iterator object complete copy && value copy
    ++*this;
    return tmp;
  }

private:
  void GetValue() {
    if (!it->Next(&value))
      Release();
  }

  void Release() {
    if (it) {
      it->~Iterator();
      it = 0;
    }
  }
};

//template<class ValueType>
//struct ForwardIteratorAdapter : std::iterator<std::forward_iterator_tag, ValueType> {
//	if enumerator is a model of CopyConstructible.
//};

} // namespace cpcl

#endif // __CPCL_ITERATOR_ADAPTER_HPP

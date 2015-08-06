// split_iterator.hpp
#pragma once

#ifndef __CPCL_SPLIT_ITERATOR_HPP
#define __CPCL_SPLIT_ITERATOR_HPP

#include <iterator>
#include <cpcl/string_piece.hpp>

namespace cpcl {

// "|" -> ["", ""]
// "text" -> ["text"]
// "few|strings|" -> ["few", "strings", ""]
// "|||" -> ["", "", "", ""]
// { '|', '|', '|', '|' } {size() - 1} -> ["", "", "", ""]
// { 't', 'e', 'x', 't' } {size() - 1} -> ["tex"]
// "few|strings||" -> ["few", "strings", "", ""]
// "few|strings" {size() - 1} -> ["few", "string"]
template<class CharType>
struct BasicSplitIterator : public std::iterator<std::forward_iterator_tag, BasicStringPiece<CharType, std::char_traits<CharType> > > {
  typedef BasicStringPiece<CharType, std::char_traits<CharType> > StringPieceType;
  typedef BasicSplitIterator<CharType> self_type;
  
  CharType const *head, *tail, *last;
  CharType split;
  BasicSplitIterator() : head(NULL), tail(NULL), last(NULL) {}
  BasicSplitIterator(StringPieceType const &v, CharType split_)
    : head(v.data()), tail(v.data()), last(v.data() + v.size()), split(split_) {
    Init(v.size());
  }
  BasicSplitIterator(CharType const *data, size_t size, CharType split_)
    : head(data), tail(data), last(data + size), split(split_) {
    Init(size);
  }

  self_type& operator++() {
    if (tail != last) {
      for (head = ++tail; tail != last; ++tail) {
        if (*tail == split)
          break;
      }
    }
    else {
      head = tail = last = NULL;
    }
    return *this;
  }
  self_type operator++(int) {
    self_type r(*this);
    ++*this;
    return r;
  }

  value_type operator*() const {
    return StringPieceType((tail == head) ? NULL : head, tail - head);
    //return StringPieceType(head, tail - head);
  }
  bool operator==(self_type const &r) const {
    return (r.head == head)
      && (r.tail == tail)
      && (r.last == last);
  }
  bool operator!=(self_type const &r) const {
    return !(r == *this);
  }
private:
  void Init(size_t size) {
    if (size) {
      for (; tail != last; ++tail) {
        if (*tail == split)
          break;
      }
    }
    else {
      head = tail = last = NULL;
    }
  }
};

typedef BasicSplitIterator < char >
StringSplitIterator;
typedef BasicSplitIterator < wchar_t >
WStringSplitIterator;

} // namespace cpcl

#endif // __CPCL_SPLIT_ITERATOR_HPP

// csv_reader.hpp - reader and field iterator for CSV file format as described by rfc4180
#pragma once

#ifndef __CPCL_CSV_READER_HPP
#define __CPCL_CSV_READER_HPP

#include <string.h> // memchr, memmove, memcpy

#include <cpcl/basic.h>
#include <cpcl/string_piece.hpp>
#include <cpcl/scoped_buf.hpp>
#include <cpcl/io_stream.h>
#include <cpcl/iterator_adapter.hpp>

namespace cpcl {

inline size_t CountChar(unsigned char *head, unsigned char *tail, unsigned char c) {
  size_t r(0);
  for (; head != tail; ++r, ++head) {
    head = static_cast<unsigned char*>(memchr(head, c, tail - head));
    if (!head)
      break;
  }
  return r;
}

inline unsigned char* ReplaceDoubleQuotes(unsigned char *head, unsigned char *tail) {
  if (head == tail)
    return tail;
  for (unsigned char *i = tail - 1; head != i; ++head) {
    head = static_cast<unsigned char*>(memchr(head, '"', i - head));
    if (!head)
      break;

    if ('"' == *(head + 1)) { // safe, because i = tail - 1
      memmove(head, head + 1, i - head); // i - head == tail - 1 - head == tail - (head + 1)
      tail = i;
      if (head == --i)
        break;
    }
  }
  return tail;
}

template<size_t N>
struct CsvReader {
  struct FieldIteratorImpl {
    // typedef cpcl::StringPiece value_type;
    unsigned char *head, *tail;
    bool replace_double_quotes;
    bool empty_field; // !@#!@#@#$@!@!@#!@$!#@

    FieldIteratorImpl()
      : head(0), tail(0), replace_double_quotes(false),
      empty_field(false) // !@#!$!!!!!!!!!!!!!!!$!
    {}
    FieldIteratorImpl(unsigned char *head_, unsigned char *tail_, bool replace_double_quotes_)
      : head(head_), tail(tail_), replace_double_quotes(replace_double_quotes_),
      empty_field(head == tail || ',' == *(tail - 1)) // trait empty lines as row with one empty field
      // empty_field(head != tail && ',' == *(tail - 1)) // !@#!$!!!!!!!!!!!!!!!$!
    {}

    StringPiece Trim(unsigned char *head, unsigned char *tail) {
      if (head != tail) {
        unsigned char *i = static_cast<unsigned char*>(memchr(head, '"', tail - head));
        if (!!i) {
          head = i;
          if ((head + 1) != tail)
            ++head;
          for (i = tail - 1; i != head; --i)
            if ('"' == *i)
              break;
          tail = i;

          // replace double quotes only if [head, tail) contain one '"' - i.e. if (!!i)
          if (replace_double_quotes)
            tail = ReplaceDoubleQuotes(head, tail);
        }
      }
      return StringPiece(reinterpret_cast<char*>(head), tail - head);
    }

    bool Next(StringPiece *field) {
      size_t quote_chars(0);
      for (unsigned char *i = head; i != tail;) {
        unsigned char *c = static_cast<unsigned char*>(memchr(i, ',', tail - i));
        if (!c) {
          if (field)
            *field = Trim(head, tail);
          head = tail;
          return true;
        }
        quote_chars += CountChar(i, c, '"');
        if ((quote_chars & 1) == 0) {
          if (field)
            *field = Trim(head, c);
          head = c + 1;
          return true;
        }
        i = c + 1;
      }
      if (empty_field) { // !@!@!#!!!!!!!!!!!!!!#@!#@#
        empty_field = false;
        if (field)
          *field = StringPiece();
        return true;
      }
      return false;
    }
  };
  // typedef IteratorAdapter<FieldIteratorImpl> FieldIterator;
  typedef InputIteratorCopyAdapter<typename CsvReader<N>::FieldIteratorImpl, StringPiece> FieldIterator;
  // typedef typename CsvReader<N>::FieldIteratorImpl FieldIterator;

  // static size_t const N = 0x100;
  IOStream *input;
  ScopedBuf<unsigned char, N> line_buf;
  unsigned char *head, *tail;
  bool replace_double_quotes;
  size_t skip, quote_chars;

  CsvReader(IOStream *input_, bool replace_double_quotes_)
    : input(input_), head(0), tail(0), replace_double_quotes(replace_double_quotes_), skip(0), quote_chars(0) {
    line_buf.Alloc(N);
    FillBuf();
  }

  // bool Next(cpcl::StringPiece *line) {
  bool Next(FieldIterator *field_it) {
    unsigned char *eol(0);
    quote_chars = 0;
    if (head != tail)
      // if line perfect fit in line_buf, i.e. '\n' at tail - 1, then head == tail, because head = eol + 1
      eol = Find(head, tail); // memchr(head, '\n', tail - head);

    while (!eol && !!input) {
      // size_t n = FillBuf();
      // Find(head + skip, n);
      FillBuf();
      eol = Find(head + skip, tail);
      // [!!! gotcha] eol = Find(head + skip, FillBuf()); // FillBuf() change head && skip, so if first argument for Find evaluted first and FillBuf change head && skip got AV
    }

    // head == tail if and only if eof
    if (head == tail)
      return false;

    if (!eol)
      eol = tail;
    if (field_it)
      *field_it = FieldIt(eol);
    head = eol; // head = eol + 1;
    if (head != tail)
      ++head;
    return true;
  }

private:
  size_t FillBuf() {
    if (!input)
      return 0;

    skip = tail - head;
    // if line perfect fit in line_buf, i.e. '\n' at tail - 1, then head == tail, because head = eol + 1
    // else need resize line_buf and copy tail
    if (skip) { // head != tail
      if (head != line_buf.Data()) {
        memmove(line_buf.Data(), head, skip);
      }
      else {
        ScopedBuf<unsigned char, 0> tmp;
        COMPILE_ASSERT(N > 1, malovato_budet);
        memcpy(tmp.Alloc((((line_buf.Size() * 3) >> 1) + N - 1) & ~(N - 1)), head, skip); // ((line_buf.Size() * 3) >> 1) == (line_buf.Size() * 3) / 2 == line_buf.Size() * 1.5
        line_buf.Release();
        swap(line_buf, tmp); // line_buf.swap(tmp);
      }
    }
    head = line_buf.Data();
    size_t n = input->Read(head + skip, line_buf.Size() - skip);
    tail = head + n + skip;
    if (n != (line_buf.Size() - skip))
      input = (IOStream*)0;
    return n;
  }

  FieldIterator FieldIt(unsigned char *eol) const {
    /*eol != tail - if eol == tail, no line feed('\n') found, so no need for '\r' trim*/
    if (eol != head && eol != tail && '\r' == *(eol - 1))
      --eol;
    // return cpcl::StringPiece(reinterpret_cast<char*>(head), n);
    return FieldIterator(CsvReader<N>::FieldIteratorImpl(head, eol, replace_double_quotes));
    // return FieldIterator(head, eol, replace_double_quotes);
  }

  unsigned char* Find(unsigned char *head, unsigned char *tail) {
    // some clever vendors implement memchr(..., size_t length) as { while (--length >= 0) { ... } }, so we need check for zero length
    for (; head != tail;) {
      unsigned char *eol = static_cast<unsigned char*>(memchr(head, '\n', tail - head));
      // if ((CountCharQuote(head, (!!eol) ? eol : tail, quote_chars) & 1) == 0 && !!eol)
      if (!eol) {
        quote_chars += CountChar(head, tail, '"'); // because buffer != line, so after we read some more data we must know how many quote already in line_buf
        break;
      }
      quote_chars += CountChar(head, eol, '"');
      if ((quote_chars & 1) == 0)
        return eol;
      head = eol + 1;
    }
    return static_cast<unsigned char*>(0);
  }
};

} // namespace cpcl

#endif // __CPCL_CSV_READER_HPP

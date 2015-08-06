// http_parse.hpp
#pragma once

#ifndef __HTTP_PARSE_HPP
#define __HTTP_PARSE_HPP

#include <cpcl/basic.h>

#include "http_parser.h"

#include <boost/shared_ptr.hpp>

#include <cpcl/string_util.hpp>
#include <cpcl/io_stream.h>

namespace net {

struct HttpParser {
  // -> shared buf[0x1000] && use PrintBuf
  struct Header {
    char field_buf[0x100];
    cpcl::StringPiece field;
    void AppendField(char const *data, size_t len) {
      field = Append(field_buf, field, cpcl::StringPiece(data, len));
    }

    char value_buf[0x200];
    cpcl::StringPiece value;
    void AppendValue(char const *data, size_t len) {
      value = Append(value_buf, value, cpcl::StringPiece(data, len));
    }
  private:
    template<size_t N>
    static cpcl::StringPiece Append(char (&buf)[N], cpcl::StringPiece s, cpcl::StringPiece const &data) {
      char *head = buf + s.size();
      size_t n = N - s.size();
      if (n)
        n = cpcl::StringCopy(head, n, data);
      return cpcl::StringPiece(buf, n + s.size());
    }
  } headers[0x10];
  Header *head, *tail, *header_it;
  std::string url;
  int status_code;
  boost::shared_ptr<cpcl::IOStream> content;
  bool headers_complete, message_complete;
  
  char const* HttpMethodStr() {
    return http_method_str(static_cast<http_method>(parser.method));
  }
  http_method HttpMethod() {
    return static_cast<http_method>(parser.method);
  }
  
  explicit HttpParser(bool is_request);
  
  void ClearHeaders() {
    head = headers; tail = headers + arraysize(headers);
    for (Header *it = headers; it != tail; ++it) {
      it->field.clear();
      it->value.clear();
    }
    header_it = head;
  }
  bool GetHeader(cpcl::StringPiece const &field, cpcl::StringPiece *value) {
    Header *r = (Header*)0;
    for (Header *it = head; it != tail; ++it) {
      if (cpcl::StringEqualsIgnoreCaseASCII(it->field, field)) {
        r = it;
        break;
      }
    }
    if (!!r && !!value)
      *value = r->value;
    return !!r;
  }
  void Reset(bool is_request);
  bool Parse(char const *data, size_t len);
private:
  http_parser_settings settings;
  http_parser parser;
};

} // namespace net

#endif // __HTTP_PARSE_HPP

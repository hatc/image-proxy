#include "http_parse.hpp"

#include <cpcl/trace.h>

namespace net {

static int on_message_begin(http_parser *parser) {
  HttpParser *p = (HttpParser*)parser->data;
  p->header_it = p->headers;
  return 0;
}

static int on_headers_complete(http_parser *parser) {
  HttpParser *p = (HttpParser*)parser->data;
  p->head = p->headers;
  p->tail = p->header_it;
  p->headers_complete = true;
  p->status_code = parser->status_code;
  return 0;
}

static int on_message_complete(http_parser *parser) {
  HttpParser *p = (HttpParser*)parser->data;
  p->message_complete = true;
  return 0;
}

static int on_url(http_parser *parser, char const *s, size_t len) {
  HttpParser *p = (HttpParser*)parser->data;
  p->url.append(s, len);
  return 0;
}

static int on_header_field(http_parser *parser, char const *s, size_t len) {
  HttpParser *p = (HttpParser*)parser->data;
  if (p->head != p->header_it)
    p->head = p->header_it;
  if (p->head != p->tail)
    p->head->AppendField(s, len);
  return 0;
}

static int on_header_value(http_parser *parser, char const *s, size_t len) {
  HttpParser *p = (HttpParser*)parser->data;
  if (p->head != p->tail) {
    if (p->head == p->header_it)
      p->header_it++;
    p->head->AppendValue(s, len);
  }
  return 0;
}

static int on_body(http_parser *parser, char const *s, size_t len) {
  HttpParser *p = (HttpParser*)parser->data;
  if (!p->content) {
    cpcl::Error(cpcl::StringPieceFromLiteral("http_parser::on_body: HttpParser.content not set"));
    return 1;
  }
  p->content->Write(s, len);
  return 0;
}

HttpParser::HttpParser(bool is_request)
  : head(headers), tail(headers + arraysize(headers)), header_it(head),
  status_code(-1), content(content), headers_complete(false), message_complete(false) {
  memset(&settings, 0, sizeof(settings));
  settings.on_message_begin = on_message_begin;
  settings.on_url = on_url;
  settings.on_header_field = on_header_field;
  settings.on_header_value = on_header_value;
  settings.on_headers_complete = on_headers_complete;
  settings.on_body = on_body;
  settings.on_message_complete = on_message_complete;

  http_parser_init(&parser, is_request ? HTTP_REQUEST : HTTP_RESPONSE);
  parser.data = this;
}

void HttpParser::Reset(bool is_request) {
  status_code = -1; headers_complete = message_complete = false;
  url.clear();
  ClearHeaders();
  http_parser_init(&parser, is_request ? HTTP_REQUEST : HTTP_RESPONSE);
}

bool HttpParser::Parse(char const *data, size_t len) {
  if (http_parser_execute(&parser, &settings, data, len) != len) {
    cpcl::Trace(CPCL_TRACE_LEVEL_ERROR, "http_parser error: %s (%s)",
      http_errno_description(HTTP_PARSER_ERRNO(&parser)), http_errno_name(HTTP_PARSER_ERRNO(&parser)));
    return false;
  }
  return true;
}

} // namespace net

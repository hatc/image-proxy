// libxml_util.h
#pragma once

#ifndef __CPCL_LIBXML_UTIL_H
#define __CPCL_LIBXML_UTIL_H

#include <memory>

#include <cpcl/scoped_buf.hpp>
#include <cpcl/string_piece.hpp>
#include <cpcl/io_stream.h>

namespace cpcl {

struct LibXmlStuff;

class XmlDocument {
  ScopedBuf<wchar_t, 0x100> conv_buf;
  std::auto_ptr<LibXmlStuff> libxml_stuff;

  XmlDocument();
public:
  ~XmlDocument();

  bool TrimResults;

  bool XPath(StringPiece const &v, std::wstring *r);

  static wchar_t const TrimChars[];

  static bool Create(IOStream *input, XmlDocument **r);
};

class XmlReader {
  std::auto_ptr<LibXmlStuff> libxml_stuff;
  bool end_element;

  XmlReader();
  XmlReader(XmlReader const &v);
  void operator=(XmlReader const &v);
public:
  ~XmlReader();

  bool Read();
  bool IsEndElement();
  //bool Reset();
  int Depth() const;

  bool HasAttributes();
  bool MoveToFirstAttribute();
  bool MoveToNextAttribute();

  StringPiece Name_UTF8();
  StringPiece Value_UTF8();

  static bool Create(IOStream *input, XmlReader **r);
};

class XmlWriter {
  ScopedBuf<unsigned char, 0x100> conv_buf;
  std::auto_ptr<LibXmlStuff> libxml_stuff;

  XmlWriter();
  XmlWriter(XmlWriter const &v);
  void operator=(XmlWriter const &v);
public:
  ~XmlWriter();

  bool StartDocument(StringPiece encoding = StringPiece());
  bool StartElement(StringPiece const &name_utf8);
  bool WriteAttribute(StringPiece const &name_utf8, StringPiece const &v_utf8);
  bool WriteAttribute(StringPiece const &name_utf8, WStringPiece const &v);
  bool WriteAttribute(StringPiece const &name_utf8, double v);
  bool WriteAttribute(StringPiece const &name_utf8, unsigned int v);
  bool WriteAttribute(StringPiece const &name_utf8, unsigned long v);
  bool WriteAttribute(StringPiece const &name_utf8, int v);
  bool WriteString(StringPiece const &v_utf8);
  bool WriteString(WStringPiece const &v);
  bool EndElement();

  bool FlushWriter();
  bool SetEncoding(char const *encoding); /* needed for xml fragment writing */

  static bool Create(IOStream *output/*writer not own output stream, but output stream must be valid until XmlWriter not deleted*/, XmlWriter **r);
};

void LibXmlInit();
void LibXmlCleanup();

} // namespace cpcl

#endif // __CPCL_LIBXML_UTIL_H

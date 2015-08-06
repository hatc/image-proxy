// jpeg_compressor_stuff.h
#pragma once

#include <cpcl/basic.h>

#include <stdio.h>
#include <jpeglib.h>
#include <setjmp.h>

#include <boost/shared_ptr.hpp>

#include <cpcl/formatted_exception.hpp>
#include <cpcl/io_stream.h>

class jpeg_exception : public formatted_exception<jpeg_exception> {
public:
  jpeg_exception(char const *s = NULL) : formatted_exception<jpeg_exception>(s)
  {}

#if defined(_MSC_VER)
  virtual char const* what() const {
#else
  virtual char const* what() const throw() {
#endif
    char const *s = formatted_exception<jpeg_exception>::what();
    return (*s) ? s : "jpeg_exception";
  }
};

struct JpegOutputManager : jpeg_destination_mgr {
  explicit JpegOutputManager(boost::shared_ptr<cpcl::IOStream> out);
  
  boost::shared_ptr<cpcl::IOStream> out;
  // data output buffer - used by jpeg compressor
  unsigned char buffer[0x1000];
};

struct JpegErrorManager : jpeg_error_mgr {
  /* setjmp in Init && each call of SweepScanline
  // All objects need to be instantiated before this setjmp call
  // so that they will be cleaned up properly if an error occurs.
  if (setjmp(jpeg_stuff.jerr.jexit)) {
    throw jpeg_exception("JpegPage::Render(): error");
  } */
  jmp_buf jexit;
};

struct JpegStuff {
  jpeg_compress_struct cinfo;
  JpegErrorManager jerr;
  
  JpegStuff();
  ~JpegStuff();
};

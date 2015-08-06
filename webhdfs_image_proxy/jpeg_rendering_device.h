// jpeg_rendering_device.h
#pragma once

#include "jpeg_compressor_stuff.h"

#include <boost/shared_ptr.hpp>

#include <cpcl/io_stream.h>
#include <cpcl/scoped_buf.hpp>
#include <plcl/rendering_device.h>

class JpegRenderingDevice : public plcl::RenderingDevice {
  unsigned int width, height;
  int input_components;
  bool initialized, write_scanline, flip;
  cpcl::ScopedBuf<unsigned char, 0> scanline_buf;
  JpegStuff jpeg_stuff;
  JpegOutputManager jpeg_output_manager;

  bool Init();

  DISALLOW_COPY_AND_ASSIGN(JpegRenderingDevice);
public:
  explicit JpegRenderingDevice(boost::shared_ptr<cpcl::IOStream> out);
  virtual ~JpegRenderingDevice();

  virtual void Pixfmt(unsigned int v);
  virtual bool SetViewport(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2);
  virtual void SweepScanline(unsigned int y, unsigned char **scanline);
  virtual void Render();
};

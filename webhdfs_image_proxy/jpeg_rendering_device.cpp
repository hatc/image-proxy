#include <cpcl/basic.h>

#include <boost/thread/locks.hpp>

#include <cpcl/dumbassert.h>
#include <cpcl/trace.h>

#include "jpeg_rendering_device.h"

JpegRenderingDevice::JpegRenderingDevice(boost::shared_ptr<cpcl::IOStream> out)
  : RenderingDevice(PLCL_PIXEL_FORMAT_GRAY_8 | PLCL_PIXEL_FORMAT_BGR_24, PLCL_PIXEL_FORMAT_BGR_24),
  width(0), height(0), input_components(3), initialized(false), write_scanline(false), flip(false),
  jpeg_output_manager(out)
{}
JpegRenderingDevice::~JpegRenderingDevice()
{}

bool JpegRenderingDevice::Init() {
  if (initialized)
    return true;
  if (width < 1 || height < 1)
    return false;
  
  j_compress_ptr cinfo = &jpeg_stuff.cinfo;
  // Step 2: specify data destination
  cinfo->dest = &jpeg_output_manager;
  
  // Step 3: set parameters for compression
  cinfo->image_width = width;
  cinfo->image_height = height;
  
  cinfo->input_components = input_components;
  if (1 == input_components)
    cinfo->in_color_space = JCS_GRAYSCALE;
  else
    cinfo->in_color_space = JCS_RGB;
  
  jpeg_set_defaults(cinfo);
  // These three values are not used by the JPEG code, merely copied
  // into the JFIF APP0 marker.
  // density_unit can be 0 for unknown, 1 for dots/inch, or 2 for dots/cm.
  // Note that the pixel aspect ratio is defined by
  // X_density/Y_density even when density_unit=0.
  cinfo->density_unit = 1;
  cinfo->X_density = 300;
  cinfo->Y_density = 300;
  
  // set subsampling options if required
  if (JCS_RGB == cinfo->in_color_space) {
    // 4:4:4 (1x1 1x1 1x1) - CrH 100% - CbH 100% - CrV 100% - CbV 100%
    // the resolution of chrominance information (Cb & Cr) is preserved
    // at the same rate as the luminance (Y) information
    cinfo->comp_info[0].h_samp_factor = 1;	// Y
    cinfo->comp_info[0].v_samp_factor = 1;
    cinfo->comp_info[1].h_samp_factor = 1;	// Cb
    cinfo->comp_info[1].v_samp_factor = 1;
    cinfo->comp_info[2].h_samp_factor = 1;	// Cr
    cinfo->comp_info[2].v_samp_factor = 1;
  }
  
  // Step 4: set quality
  jpeg_set_quality(cinfo, 80, TRUE/* limit to baseline-JPEG values */);
  
  // Step 5: Start compressor
  jpeg_start_compress(cinfo, TRUE);
  
  return (initialized = true);
}

void JpegRenderingDevice::Pixfmt(unsigned int v) {
  if (pixel_format != v) {
    if ((supported_pixel_formats & v) != 0) {
      if (PLCL_PIXEL_FORMAT_BGR_24 == v)
        input_components = 3;
      else
        input_components = 1;
      pixel_format = v;
    }
  }
}

bool JpegRenderingDevice::SetViewport(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2) {
  if (initialized)
    return false;
  
  x2 -= x1; y2 -= y1;
  if (x2 < 1 || y2 < 1)
    return false;
  width = x2; height = y2;
  return true;
}

void JpegRenderingDevice::SweepScanline(unsigned int y, unsigned char **scanline) {
  bool check_flip(!initialized);
  if (!Init()) {
    cpcl::Error(cpcl::StringPieceFromLiteral("JpegRenderingDevice::SweepScanline(): initialization failed"));
    return;
  }
  j_compress_ptr cinfo = &jpeg_stuff.cinfo;
  
  int const stride_ = plcl::RenderingData::Stride(pixel_format, width);
  DUMBASS_CHECK(stride_ > 0);
  size_t const stride = static_cast<size_t>(stride_);
  if (check_flip && cinfo->next_scanline != y) {
    // image flipped, store at buffer and then write at Render
    flip = true;
    scanline_buf.Alloc(height * stride);
  }
  if (flip) {
    if (scanline)
      *scanline = scanline_buf.Data() + y * stride;
    return;
  }
  
  if (scanline_buf.Size() != stride)
    scanline_buf.Alloc(stride);
  if (scanline)
    *scanline = scanline_buf.Data();
  
  if (write_scanline) {
    // All objects need to be instantiated before this setjmp call so that
    // they will be cleaned up properly if an error occurs.
    if (setjmp(jpeg_stuff.jerr.jexit))
      throw jpeg_exception("JpegPage::SweepScanline(): error");

    // Step 6: while (scan lines remain to be written)
    if (cinfo->next_scanline < cinfo->image_height && y < height) {
      if (jpeg_write_scanlines(cinfo, scanline, 1) == 0)
        throw jpeg_exception("JpegRenderingDevice::SweepScanline(): I/O suspension");
    }
  }
  write_scanline = true;
}

void JpegRenderingDevice::Render() {
  if (!Init()) {
    cpcl::Error(cpcl::StringPieceFromLiteral("JpegRenderingDevice::Render(): initialization failed"));
    return;
  }
  
  // All objects need to be instantiated before this setjmp call so that
  // they will be cleaned up properly if an error occurs.
  if (setjmp(jpeg_stuff.jerr.jexit))
    throw jpeg_exception("JpegPage::Render(): error");
  
  j_compress_ptr cinfo = &jpeg_stuff.cinfo;
  unsigned char *scanline = scanline_buf.Data();
  if (flip) {
    DUMBASS_CHECK(0 == cinfo->next_scanline);
    int const stride_ = plcl::RenderingData::Stride(pixel_format, width);
    DUMBASS_CHECK(stride_ > 0);
    size_t const stride = static_cast<size_t>(stride_);
    for (unsigned int y = 0; y < height; ++y, scanline += stride) {
      if (jpeg_write_scanlines(cinfo, &scanline, 1) == 0)
        throw jpeg_exception("JpegRenderingDevice::Render(): I/O suspension");
    }
  } else {
    if (cinfo->next_scanline < cinfo->image_height)
      jpeg_write_scanlines(cinfo, &scanline, 1);
	}
  
  // Step 7: Finish compression
  jpeg_finish_compress(cinfo);
  
  initialized = false;
  write_scanline = false;
  flip = false;
}

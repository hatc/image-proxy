#include "basic.h"

#include "io_stream.h"

#ifndef min
#define min(a, b) (a < b) ? a : b
#endif

namespace cpcl {

IOStream::~IOStream()
{}

uint32 IOStream::CopyTo(IOStream *output, uint32 size) {
  unsigned char buf[0x1000];
  uint32 to_read = min(size, (uint32)sizeof(buf));
  uint32 readed, written, total_written(0);
  while ((readed = Read(buf, to_read)) > 0) {
    written = output->Write(buf, readed);
    total_written += written;
    if (written != readed)
      break;

    if ((size -= readed) == 0)
      break;
    to_read = min(size, (uint32)sizeof(buf));
  }
  return total_written;
}

} // namespace cpcl

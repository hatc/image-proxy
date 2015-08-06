#include "basic.h"

#include <string.h> // memcpy
#include <stdio.h> // SEEK_SET SEEK_CUR SEEK_END
#include <algorithm> // std::min
#include <vector>

#include <boost/make_shared.hpp>

#include "dynamic_memory_stream.h"
#include "trace.h"

namespace cpcl {

struct DynamicMemoryStream::MemoryStorage {
  size_t Read(size_t offset, unsigned char *data, size_t size) {
    size_t readed(0), pad(offset % N);
    for (BlocksIt it = Block(offset), tail = blocks.end(); it != tail; ++it) {
      size_t const n = std::min(N - pad, size);
      memcpy(data + readed, *it + pad, n);
      readed += n;
      size -= n;
      if (!size)
        break;
      pad = 0;
    }
    return readed;
  }
  /* size_t Write(size_t offset, unsigned char const *data, size_t size) {
    if (blocks.empty()) {
      if (!!offset) // offset after EOF
        return 0;
      unsigned char *block = new unsigned char[N];
      blocks.push_back(block);
    }
    size_t written(0), pad(offset % N);
    for (BlocksIt it = Block(offset), tail = blocks.end(); it != tail; ++it) {
      size_t const n = std::min(N - pad, size);
      memcpy(*it + pad, data + written, n);
      written += n;
      size -= n;
      if (!size)
        break;
      pad = 0;
    }
    if (!written && !!size && (offset != blocks.size() * N)) // offset after EOF
      return 0;
    while (!!size) {
      unsigned char *block = new unsigned char[N];
      blocks.push_back(block);
      size_t const n = std::min(N, size);
      memcpy(block, data + written, n);
      written += n;
      size -= n;
    }
    return written;
  } */
  size_t Write(size_t offset, unsigned char const *data, size_t size) {
    if (blocks.size() * N == offset) {
      unsigned char *block = new unsigned char[N];
      blocks.push_back(block);
    }
    size_t written(0), pad(offset % N);
    for (BlocksIt it = Block(offset), tail = blocks.end(); it != tail; ++it) {
      size_t const n = std::min(N - pad, size);
      memcpy(*it + pad, data + written, n);
      written += n;
      size -= n;
      if (!size)
        break;
      pad = 0;
    }
    if (!written && !!size) // offset after EOF
      return 0;
    while (!!size) {
      unsigned char *block = new unsigned char[N];
      blocks.push_back(block);
      size_t const n = std::min(N, size);
      memcpy(block, data + written, n);
      written += n;
      size -= n;
    }
    return written;
  }
  
  MemoryStorage(size_t N = 4 * 1024) : N(N)
  {}
  ~MemoryStorage() {
    for (BlocksIt it = blocks.begin(), tail = blocks.end(); it != tail; ++it)
      delete [] *it;
    blocks.clear();
  }
// private:
  typedef std::vector<unsigned char*> Blocks;
  typedef Blocks::iterator BlocksIt;
  size_t const N;
  Blocks blocks;
  
  BlocksIt Block(size_t offset) {
    size_t const idx = offset / N;
    if (idx < blocks.size())
      return blocks.begin() + idx;
    else
      return blocks.end();
  }
private:
  DISALLOW_COPY_AND_ASSIGN(MemoryStorage);
};

DynamicMemoryStream::DynamicMemoryStream(size_t block_size)
  : state(true), memory_storage(boost::make_shared<DynamicMemoryStream::MemoryStorage>(block_size)), p_size(0), p_offset(0)
{}
DynamicMemoryStream::~DynamicMemoryStream()
{}

IOStream* DynamicMemoryStream::Clone() {
  return new DynamicMemoryStream(*this);
}
uint32 DynamicMemoryStream::CopyTo(IOStream *output, uint32 size) {
  uint32 written(0); size_t pad(p_offset % memory_storage->N);
  for (DynamicMemoryStream::MemoryStorage::BlocksIt it = memory_storage->Block(p_offset), tail = memory_storage->blocks.end(); it != tail; ++it) {
    size_t const n = std::min(memory_storage->N - pad, (size_t)size);
    uint32 written_ = output->Write(*it + pad, (uint32)n);
    written += written_;
    if (written_ != (uint32)n)
      break;
    // written += n;
    size -= n;
    if (!size)
      break;
    pad = 0;
  }
  return written;
}

uint32 DynamicMemoryStream::Read(void *data, uint32 size) {
  uint32 r(0);
  size = std::min(size, uint32(p_size - p_offset));
  if (size > 0) {
    r = memory_storage->Read(p_offset, (unsigned char*)data, size);
    p_offset += r;
    state = true;
  }
  return r;
}

uint32 DynamicMemoryStream::Write(void const *data, uint32 size) {
  if (p_size != p_offset) {
    cpcl::Warning(cpcl::StringPieceFromLiteral("DynamicMemoryStream::Write(): only sequential write access supported"));
    state = false;
    return 0;
  }
  uint32 r(size);
  if (size > 0) {
    r = memory_storage->Write(p_offset, (unsigned char const*)data, size);
    p_size = p_offset = r + p_size;
    state = true;
  }
  return r;
}

bool DynamicMemoryStream::Seek(int64 move_to, uint32 move_method, int64 *position) {
  if (SEEK_CUR == move_method)
    move_to = (int64)p_offset + move_to;
  else if (SEEK_END == move_method)
    move_to = (int64)p_size + move_to;
  else if (SEEK_SET != move_method) {
    Error(StringPieceFromLiteral("DynamicMemoryStream::Seek(): invalid move_method"));
    return false;
  }

  if (move_to < 0) {
    Error(StringPieceFromLiteral("DynamicMemoryStream::Seek(): move_to before the beginning of the memory chunk"));
    return false;
  }
  else if (move_to > (int64)p_size) {
    Error(StringPieceFromLiteral("DynamicMemoryStream::Seek(): move_to beyond the memory chunk"));
    return false;
  }
  p_offset = (size_t)move_to;
  if (position)
    *position = (int64)p_offset;
  return true;
}

int64 DynamicMemoryStream::Tell() {
  return (int64)p_offset;
}

int64 DynamicMemoryStream::Size() {
  return (int64)p_size;
}

} // namespace cpcl

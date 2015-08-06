#include <cpcl/basic.h>

#include <boost/thread/locks.hpp>

#include <cpcl/trace.h>
#include <cpcl/dynamic_memory_stream.h>

#include "image_cache.h"

ImageCache::ImageCache(size_t items_cap) : items_cap(items_cap)
{}
ImageCache::~ImageCache()
{}

ImageCache::ItemHit ImageCache::Get(std::string const &k) {
  Value item;
  bool hit(false);
  {
    scoped_lock lock(mutex, boost::try_to_lock);
    if (!lock && !lock.timed_lock(boost::posix_time::seconds(1))) {
      cpcl::Warning(cpcl::StringPieceFromLiteral("ImageCache::Get(): can't obtain exclusive ownership for the current thread"));
    } else {
      MapIterator i = map.find(k);
      if (i != map.end()) {
        item = (*i).second;
        hit = true;
      }
    }
  }
  if (!item)
    item.reset(new cpcl::DynamicMemoryStream());
  return std::make_pair(item, hit);
}

void ImageCache::Put(std::string const &k, boost::shared_ptr<cpcl::IOStream> v) {
  scoped_lock lock(mutex, boost::try_to_lock);
  if (!lock && !lock.timed_lock(boost::posix_time::seconds(1))) {
    cpcl::Warning(cpcl::StringPieceFromLiteral("ImageCache::Put(): can't obtain exclusive ownership for the current thread"));
    return;
  }
  
  std::pair<MapIterator, bool> it = map.insert(Map::value_type(k, v));
  if (it.second)
    Push(it.first);
  else
    it.first->second = v;
}

void ImageCache::Push(MapIterator v) {
  queue.push(v);
  if (queue.size() > items_cap)
    queue.pop();
}

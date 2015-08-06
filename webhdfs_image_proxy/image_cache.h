// image_cache.h
#pragma once

#ifndef __IMAGE_CACHE_H
#define __IMAGE_CACHE_H

#include <map>
#include <queue>
#include <string>

#include <boost/thread/mutex.hpp>

#include <cpcl/io_stream.h>

class ImageCache {
  typedef boost::shared_ptr<cpcl::IOStream> Value;
  typedef std::map<std::string, Value> Map;
  typedef Map::iterator MapIterator;
  typedef std::queue<MapIterator> Queue;
  typedef boost::unique_lock<boost::timed_mutex> scoped_lock;

  Map map;
  Queue queue;
  size_t items_cap;
  boost::timed_mutex mutex;

  void Push(MapIterator v);
  DISALLOW_COPY_AND_ASSIGN(ImageCache);
public:
  typedef std::pair<boost::shared_ptr<cpcl::IOStream>, bool> ItemHit;

  explicit ImageCache(size_t items_cap);
  ~ImageCache();

  ItemHit Get(std::string const &k);
  void Put(std::string const &k, boost::shared_ptr<cpcl::IOStream> v);

  void State();
};

#endif // __IMAGE_CACHE_H

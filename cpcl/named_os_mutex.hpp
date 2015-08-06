// named_os_mutex.hpp
#pragma once

#ifndef __CPCL_NAMED_OS_MUTEX_HPP
#define __CPCL_NAMED_OS_MUTEX_HPP

#include <boost/thread/thread.hpp>
#include <boost/thread/exceptions.hpp>
#include <boost/thread/locks.hpp>

namespace cpcl {

// named mutex - use from different binaries - plugin && exe
class NamedMutexOS : boost::noncopyable {
  void *mutex_os;
  unsigned long ecode;
  bool timed_lock_(unsigned long milliseconds);
public:
  bool abadoned;
  
  NamedMutexOS(wchar_t const *name);
  ~NamedMutexOS();

  bool try_lock() {
    return timed_lock_(0);
  }
  void lock() {
    timed_lock_(~(unsigned long)0);
  }
  bool timed_lock(boost::posix_time::ptime const &wait_until) {
    return timed_lock_(boost::detail::get_milliseconds_until(wait_until));
  }
  template<typename Duration>
  bool timed_lock(Duration const &timeout/*relative_time*/) {
    boost::int64_t v = timeout.total_milliseconds();
    return timed_lock_(v < 0 ? 0 : (unsigned long)v);
  }

  void unlock();

  typedef boost::unique_lock<NamedMutexOS> scoped_timed_lock;
  typedef boost::detail::try_lock_wrapper<NamedMutexOS> scoped_try_lock;
  typedef scoped_timed_lock scoped_lock;
};

}  // namespace cpcl

#endif  // __CPCL_NAMED_OS_MUTEX_HPP

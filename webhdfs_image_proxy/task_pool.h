// task_pool.h
#pragma once

#ifndef __TASK_POOL_H
#define __TASK_POOL_H

#include <queue>

#include <boost/shared_ptr.hpp>

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

namespace net {
class Connection;
}
namespace plcl {
class Page;
}
namespace cpcl {
class IOStream;
}

class TaskPool {
  struct Task {
    boost::shared_ptr<net::Connection> connection;
    boost::shared_ptr<plcl::Page> page;
    boost::shared_ptr<cpcl::IOStream> out;

    Task()
    {}
    Task(boost::shared_ptr<net::Connection> connection, boost::shared_ptr<plcl::Page> page, boost::shared_ptr<cpcl::IOStream> out)
      : connection(connection), page(page), out(out)
    {}
    bool operator!() const { return !connection || !page || !out; }
  };
  boost::condition_variable tasks_cv;
  boost::mutex tasks_mutex;
  typedef boost::unique_lock<boost::mutex> scoped_lock;
  typedef std::vector<boost::shared_ptr<boost::thread> > Threads;

  Threads threads;
  std::queue<Task> tasks;

  bool exit_requested;
  void WorkerThread();
  Task NextTask();
public:
  TaskPool() : exit_requested(false)
  {}
  ~TaskPool();

  bool Init(int num_threads);

  bool AddTask(boost::shared_ptr<net::Connection> connection, boost::shared_ptr<plcl::Page> page, boost::shared_ptr<cpcl::IOStream> out);

  void Stop(bool join = true);
};

#endif // __TASK_POOL_H

#include <cpcl/basic.h>

#include "task_pool.h"
#include "jpeg_rendering_device.h"
#include "connection.h"

#include <cpcl/trace.h>

bool TaskPool::Init(int num_threads) {
  if (!threads.empty() || num_threads < 1)
    return false;

  threads.reserve(static_cast<size_t>(num_threads));
  for (int i = 0; i < num_threads; ++i) {
    boost::shared_ptr<boost::thread> thread(new boost::thread(boost::bind(&TaskPool::WorkerThread, this)));
    threads.push_back(thread);
  }
  return true;
}

void TaskPool::WorkerThread() {
  bool exit(false);
  while (!exit) {
    TaskPool::Task task;
    {
      scoped_lock lock(tasks_mutex);
      task = NextTask();
      while (!task && !exit_requested) {
        tasks_cv.wait(lock);
        task = NextTask();
      }
      exit = exit_requested;
    }
    if (!!task && !exit) {
      int status_code = 200;
      try {
        JpegRenderingDevice rendering_device(task.out);
        task.page->Render(&rendering_device);
      } catch (std::exception const &e) {
        char const *s = e.what();
        if (!!s)
          cpcl::Trace(CPCL_TRACE_LEVEL_ERROR, "TaskPool::WorkerThread(): page->Render fails: exception: %s", s);
        else
          cpcl::Error(cpcl::StringPieceFromLiteral("TaskPool::WorkerThread(): page->Render fails: exception"));
        status_code = 500;
      }
      {
        scoped_lock lock(tasks_mutex);
        exit = exit_requested;
      }
      if (!exit)
        task.connection->SendResponse(status_code);
    }
  }
}

bool TaskPool::AddTask(boost::shared_ptr<net::Connection> connection, boost::shared_ptr<plcl::Page> page, boost::shared_ptr<cpcl::IOStream> out) {
  if (threads.empty() || !connection || !page || !out)
    return false;
  
  scoped_lock lock(tasks_mutex);
  tasks.push(TaskPool::Task(connection, page, out));
  tasks_cv.notify_all();
  return true;
}

TaskPool::Task TaskPool::NextTask() {
  TaskPool::Task r;
  if (!tasks.empty()) {
    r = tasks.front();
    tasks.pop();
  }
  return r;
}

TaskPool::~TaskPool() {
  Stop();
}

void TaskPool::Stop(bool join) {
  if (threads.empty())
    return;

  {
    scoped_lock lock(tasks_mutex);
    if (!tasks.empty()) {
      std::queue<Task> tmp;
      tmp.swap(tasks);
    }
    exit_requested = true;
    tasks_cv.notify_all();
  }
  if (join) {
    for (Threads::iterator it = threads.begin(), tail = threads.end(); it != tail; ++it) {
      if ((*it)->joinable())
        (*it)->join();
    }
    threads.clear();
  }
}

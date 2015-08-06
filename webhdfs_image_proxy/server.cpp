#include <cpcl/basic.h>

#include <vector>

#include <boost/thread/thread.hpp>

#include <cpcl/trace.h>

#include "server.h"
#include <boost/asio/signal_set.hpp>

static void ThreadEntryPoint(boost::asio::io_service *io_service) {
  try {
    for (;;) {
      try {
        io_service->run();
        break;
      } catch (std::exception const &e) {
        char const *s = e.what();
        if (!!s && (*s))
          cpcl::Trace(CPCL_TRACE_LEVEL_ERROR, "ThreadEntryPoint(): exception: %s", s);
        else
          cpcl::Error(cpcl::StringPieceFromLiteral("ThreadEntryPoint(): exception"));
      }
    }
  } catch (...) {
    // cpcl::Error(cpcl::StringPieceFromLiteral("thread interrupted"));
    cpcl::Error(cpcl::StringPieceFromLiteral("ThreadEntryPoint(): unspecified exception, thread exits"));
  }
}

namespace net {

namespace ip = boost::asio::ip;

Server::Server(ip::tcp::endpoint endpoint, Server::ConnectionCtor ctor)
  : acceptor(io_service), image_cache(new ImageCache(0x100)), task_pool(new TaskPool()), ctor(ctor), stop(false) {
  new_connection.reset(ctor(io_service, image_cache, task_pool));

  acceptor.open(endpoint.protocol());
  acceptor.set_option(ip::tcp::acceptor::reuse_address(true));
  acceptor.bind(endpoint);
  acceptor.listen();
  acceptor.async_accept(new_connection->Socket(),
    boost::bind(&Server::handle_accept, shared_from_this(), boost::asio::placeholders::error));
}

void Server::handle_accept(boost::system::error_code const &ec) {
  if (!ec) {
    new_connection->Start();
    new_connection.reset(ctor(io_service, image_cache, task_pool));
    acceptor.async_accept(new_connection->Socket(),
      boost::bind(&Server::handle_accept, shared_from_this(), boost::asio::placeholders::error));
  } else {
    cpcl::Trace(CPCL_TRACE_LEVEL_ERROR,
      "Server::handle_accept() fails: %s",
      ec.message().c_str());
  }
}

void Server::handle_signal(boost::system::error_code const &ec) {
  if (!ec) {
    boost::lock_guard<boost::mutex> lock(signal_mutex);
    stop = true;
    signal_cv.notify_one();
  } else {
    cpcl::Trace(CPCL_TRACE_LEVEL_ERROR,
      "Server::handle_signal() fails: %s",
      ec.message().c_str());
  }
}

void Server::Run() {
  task_pool->Init(1);

  // Create a pool of threads to run all of the io_services.
  std::vector<boost::shared_ptr<boost::thread> > threads;
  threads.reserve(boost::thread::hardware_concurrency() * 2);
  for (size_t i = 0, n = threads.capacity(); i < n; ++i) {
    boost::shared_ptr<boost::thread> thread(new boost::thread(
      boost::bind(ThreadEntryPoint, &io_service)));
    threads.push_back(thread);
  }

  boost::asio::signal_set signals(io_service, SIGINT, SIGTERM);
  signals.async_wait(boost::bind(&Server::handle_signal, shared_from_this(), boost::asio::placeholders::error));

  {
    boost::unique_lock<boost::mutex> lock(signal_mutex);
    while (!stop)
      signal_cv.wait(lock);
  }
  
  task_pool->Stop(true);
  io_service.stop();
  for (size_t i = 0; i < threads.size(); ++i) {
    if (threads[i]->joinable())
      threads[i]->join();
  }
}

} // namespace net

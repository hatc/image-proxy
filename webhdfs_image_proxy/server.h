// server.h
#pragma once

#ifndef __SERVER_H
#define __SERVER_H

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

#include "connection.h"

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

namespace net {

class Server : public boost::enable_shared_from_this<Connection>, private boost::noncopyable {
public:
  typedef boost::function<Connection*(boost::asio::io_service&, boost::shared_ptr<ImageCache>, boost::shared_ptr<TaskPool>)> ConnectionCtor;
private:
  // Handle completion of an asynchronous accept operation.
  void handle_accept(boost::system::error_code const &ec);
  
  // The io_service used to perform asynchronous operations.
  boost::asio::io_service io_service;
  
  // Acceptor used to listen for incoming connections.
  boost::asio::ip::tcp::acceptor acceptor;

  // The next connection to be accepted.
  boost::shared_ptr<Connection> new_connection;

  boost::shared_ptr<ImageCache> image_cache;
  boost::shared_ptr<TaskPool> task_pool;
  ConnectionCtor ctor;

  boost::condition_variable signal_cv;
  boost::mutex signal_mutex;
  bool stop;
  void handle_signal(boost::system::error_code const &ec);
public:
  Server(boost::asio::ip::tcp::endpoint endpoint, ConnectionCtor ctor);

  // Run the server's io_service loop.
  void Run();
};

} // namespace net

#endif // __SERVER_H

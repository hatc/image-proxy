// connection.h
#pragma once

#ifndef __CONNECTION_H
#define __CONNECTION_H

#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/array.hpp>

#include <boost/asio/buffer.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/read.hpp>
// #include <boost/asio/strand.hpp>
#include <boost/asio/time_traits.hpp>
#include <boost/asio/write.hpp>

#include "task_pool.h"
#include "image_cache.h"
#include "http_parse.hpp"

#include <plcl/plugin_list.h>

namespace net {

class Connection : public boost::enable_shared_from_this<Connection>, private boost::noncopyable {
  // boost::asio::io_service::strand strand;

  // sockets for the connection.
  boost::asio::ip::tcp::socket client_socket;
  boost::asio::ip::tcp::socket webhdfs_socket;
  
  boost::asio::ip::tcp::resolver resolver;
  std::string host, port;
  
  boost::array<unsigned char, 0x1000> buffer;
  // BOOST_STATIC_CONSTANT(size_t, BUFFER_SIZE = 0x1000);
  // boost::array<unsigned char, BUFFER_SIZE> in_buffer;
  // boost::array<unsigned char, BUFFER_SIZE> out_buffer;
  static size_t const CHUNK_OFFSET = 6; // 4(chunk-size, FFFF) + 2(CRLF)
  static size_t const MAX_CHUNK_SIZE = 0x1000 - 8; // 4(chunk-size, FFFF) + 4(2 * CRLF)
  
  // actual payload
  HttpParser parser;
  boost::shared_ptr<cpcl::IOStream> image;
  std::string webhdfs_path, image_path;
  int status_code;
  boost::shared_ptr<ImageCache> image_cache;
  boost::shared_ptr<TaskPool> task_pool;

  plcl::PluginList *plugin_list;
  unsigned int page_width, page_height, page_pixfmt;
  struct Query {
    cpcl::StringPiece request_path;
    unsigned int width, height;
    bool json;
    
    Query() : width(0), height(0), json(false)
    {}
  } query;
  
  // resolve && connect to webhdfs server
  void handle_resolve(boost::system::error_code const &ec, boost::asio::ip::tcp::resolver::iterator endpoint_iterator);
  void handle_connect(boost::system::error_code const &ec, boost::asio::ip::tcp::resolver::iterator endpoint_iterator);

  // handle completion of a read some date from a socket - i.e. handle_read will be called after some data readed from socket or error occurred.
  void handle_read_request(boost::system::error_code const &ec, size_t bytes_transferred);
  void handle_read_webhdfs_response(boost::system::error_code const &ec, size_t bytes_transferred);

  // the handler to be called when the write operation completes - i.e. the bytes transferred is equal to the sum of the buffer sizes or error occurred.
  void handle_write_request(boost::system::error_code const &ec, size_t bytes_transferred);
  void handle_write_response(boost::system::error_code const &ec, size_t bytes_transferred);

  Query GetQuery(std::string const &uri);
  size_t BuildRequest(std::string const &request_path);
  void SendRequest(std::string const &request_path);
  bool SetLocation(cpcl::StringPiece const &uri);
  void SendPage();
  size_t BuildResponse(int code, size_t response_len);
  boost::asio::const_buffers_1 BuildChunk(size_t chunk_size);
  void SendChunk(unsigned char *chunk, size_t chunk_size);
public:
  Connection(boost::asio::io_service &io_service, boost::shared_ptr<ImageCache> image_cache, boost::shared_ptr<TaskPool> task_pool, std::string host, std::string port, plcl::PluginList *plugin_list);
  ~Connection();

  // get the socket associated with the in connection.
  boost::asio::ip::tcp::socket& Socket() { return client_socket; }

  // start the first asynchronous operation for the connection.
  void Start();
  void SendResponse(int code);
};

} // namespace net

#endif // __CONNECTION_H

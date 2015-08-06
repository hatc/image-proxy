#include <cpcl/basic.h>

#include <string.h> // memcpy

#include <algorithm>

#include "connection.h"
#include <boost/make_shared.hpp>
#include <boost/thread/locks.hpp>

#include <cpcl/string_util.hpp>
#include <cpcl/split_iterator.hpp>
#include <cpcl/string_cast.hpp>
#include <cpcl/trace.h>
#include <cpcl/dynamic_memory_stream.h>

// using boost::asio::ip::tcp;
namespace ip = boost::asio::ip;
using namespace cpcl;

static inline std::pair<StringPiece, StringPiece> SplitPair(StringPiece const &s, char c) { // s.split(c, 1)
  StringPiece first, second;
  StringSplitIterator it(s, c), tail;
  first = *it++;
  if (it != tail)
    second.set((*it).data(), s.size() - first.size() - 1/* if s.size() < 1 -> it == tail */);
  return std::make_pair(first, second);
}

/* 
 * append string to buf, increment buf and decrement buf_len respectively
 * returns number of character copied, exclude NULL character                               
 * StringCopy always append NULL character to output string, if it's size > 0
 */
static inline void StringAdvance(char *&buf, size_t &buf_len, StringPiece const &s) {
  size_t n = StringCopy(buf, buf_len, s);
  buf += n;
  buf_len -= n;
}

static inline void WriteHeader(char *&buf, size_t &buf_len, StringPiece const &name, StringPiece const &value) {
  StringAdvance(buf, buf_len, name);
  StringAdvance(buf, buf_len, StringPieceFromLiteral(": "));
  StringAdvance(buf, buf_len, value);
  StringAdvance(buf, buf_len, StringPieceFromLiteral("\r\n"));
}

// state chart:
// Start
//  |
// handle_read_request -loop- until eof || headers_complete
//  |            |
// SendRequest SendResponse(400)
// ...

namespace net {

Connection::Connection(boost::asio::io_service &io_service, boost::shared_ptr<ImageCache> image_cache, boost::shared_ptr<TaskPool> task_pool,
  std::string host, std::string port, plcl::PluginList *plugin_list)
  : client_socket(io_service), webhdfs_socket(io_service), resolver(io_service), host(host), port(port),
  parser(true), status_code(-1), image_cache(image_cache), task_pool(task_pool), plugin_list(plugin_list),
  page_width(0), page_height(0), page_pixfmt(PLCL_PIXEL_FORMAT_INVALID) {
  Trace(CPCL_TRACE_LEVEL_DEBUG, "Connection::Connection(%08X)", (int)this);
}
Connection::~Connection() {
  Trace(CPCL_TRACE_LEVEL_DEBUG, "Connection::~Connection(%08X)", (int)this);
}

void Connection::Start() {
  parser.content = boost::static_pointer_cast<IOStream>(boost::make_shared<DynamicMemoryStream>());
  client_socket.async_read_some(boost::asio::buffer(buffer),
    boost::bind(&Connection::handle_read_request, shared_from_this(),
    boost::asio::placeholders::error,
    boost::asio::placeholders::bytes_transferred));
}

Connection::Query Connection::GetQuery(std::string const &uri) {
  Query r;
  
  http_parser_url url;
  http_parser_parse_url(uri.data(), uri.size(), /*parser.method == HTTP_CONNECT*/0, &url);
  if ((1 << UF_PATH) & url.field_set) {
    r.request_path.set(uri.data() + url.field_data[UF_PATH].off, url.field_data[UF_PATH].len);
  } else {
    cpcl::Trace(CPCL_TRACE_LEVEL_ERROR,
      "Connection(%08X)::GetQuery(): uri path field is missed",
      (int)this);
    return r;
  }
  if ((1 << UF_QUERY) & url.field_set) {
    StringPiece query(uri.data() + url.field_data[UF_QUERY].off, url.field_data[UF_QUERY].len);
    
    StringPiece width_key = StringPieceFromLiteral("w");
    StringPiece height_key = StringPieceFromLiteral("h");
    StringPiece json_key = StringPieceFromLiteral("info");
    for (StringSplitIterator it(query, '&'), tail; it != tail; ++it) {
      if (StringEqualsIgnoreCaseASCII(json_key, *it)) {
        r.json = true;
        break; // if json, w && h not used
      } else {
        std::pair<StringPiece, StringPiece> key_value = SplitPair(*it, '=');
        if (!key_value.second.empty()) {
          StringPiece keys[] = { width_key, height_key };
          unsigned int Query::*values[] = { &Query::width, &Query::height };
          for (size_t i = 0; i < arraysize(keys); ++i) {
            if (StringEqualsIgnoreCaseASCII(key_value.first, keys[i])) {
              unsigned int value;
              if (TryConvert(key_value.second, &value))
                r.*values[i] = value;
            }
          }
        }
      }
    }
  }
  
  return r;
}

void Connection::handle_read_request(boost::system::error_code const &ec, size_t bytes_transferred) {
  if (!ec || boost::asio::error::eof == ec) {
    bool invalid_request(false);
    if (bytes_transferred > 0)
      invalid_request = !parser.Parse(reinterpret_cast<char const*>(buffer.data()), bytes_transferred);
    if (!invalid_request && boost::asio::error::eof == ec)
      invalid_request = !parser.headers_complete;

    if (invalid_request) {
      SendResponse(400);
    } else {
      if (parser.headers_complete) {
        query = GetQuery(parser.url);
        if (parser.HttpMethod() != HTTP_GET || query.request_path.size() < 2) {
          SendResponse(400);
        } else {
          webhdfs_path.assign(query.request_path.data(), query.request_path.size());
          SendRequest(webhdfs_path);
        }
      } else {
        client_socket.async_read_some(boost::asio::buffer(buffer),
          boost::bind(&Connection::handle_read_request, shared_from_this(),
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
      }
    }
  } else {
    Trace(CPCL_TRACE_LEVEL_ERROR,
      "Connection(%08X)::handle_read_request() fails: %s",
      (int)this, ec.message().c_str());

    // No new asynchronous operations are started. This means that all shared_ptr
    // references to the connection object will disappear and the object will be
    // destroyed automatically after this handler returns. The connection class's
    // destructor closes the sockets.
  }
}

static StringPiece const WEBHDFS_REQUEST_PREFIX = StringPieceFromLiteral("/webhdfs/v1");
size_t Connection::BuildRequest(std::string const &request_path) {
  char *buf = reinterpret_cast<char*>(buffer.data());
  size_t buf_len(buffer.size());
  
  buf_len -= StringFormat(buf, buf_len, "GET /webhdfs/v1%s?op=OPEN HTTP/1.1\r\n", request_path.c_str());
  buf += buffer.size() - buf_len;
  
  WriteHeader(buf, buf_len, StringPieceFromLiteral("Host"), host);
  WriteHeader(buf, buf_len, StringPieceFromLiteral("Connection"), StringPieceFromLiteral("close"));
  StringAdvance(buf, buf_len, StringPieceFromLiteral("\r\n"));
  
  if (TRACE_LEVEL & CPCL_TRACE_LEVEL_DEBUG) {
    Trace(CPCL_TRACE_LEVEL_DEBUG,
      "request: \"%s\"",
      std::string(reinterpret_cast<char*>(buffer.data()), buffer.size() - buf_len).c_str());
  }
  return buffer.size() - buf_len;
}

void Connection::SendRequest(std::string const &request_path) {
  if (StringEqualsIgnoreCaseASCII(request_path, StringPieceFromLiteral("/favicon.ico"))) {
    SendResponse(404);
    return;
  }

  webhdfs_path = request_path;
  if (!image) {
    image_path = webhdfs_path;
    ImageCache::ItemHit r = image_cache->Get(image_path);
    image = r.first;
    if (r.second) {
      SendPage();
      return;
    }
  } else
    image->Seek(0, SEEK_SET, NULL);
  parser.content = image;
  
  ip::tcp::resolver::query query(host, port, ip::resolver_query_base::v4_mapped |
    /*ip::resolver_query_base::numeric_host |*/ ip::resolver_query_base::numeric_service);
  resolver.async_resolve(query,
    boost::bind(&Connection::handle_resolve, shared_from_this(),
    boost::asio::placeholders::error,
    boost::asio::placeholders::iterator));
}

void Connection::handle_resolve(boost::system::error_code const &ec, ip::tcp::resolver::iterator endpoint_iterator) {
  if (!ec) {
    // Attempt a connection to the first endpoint in the list.
    // Each endpoint will be tried until we successfully establish a connection.
    ip::tcp::endpoint endpoint = *endpoint_iterator;
    webhdfs_socket.async_connect(endpoint,
      boost::bind(&Connection::handle_connect, shared_from_this(),
      boost::asio::placeholders::error,
      ++endpoint_iterator));
  } else {
    Trace(CPCL_TRACE_LEVEL_ERROR,
      "Connection(%08X)::handle_resolve() fails: %s",
      (int)this, ec.message().c_str());
  }
}

void Connection::handle_connect(boost::system::error_code const &ec, ip::tcp::resolver::iterator endpoint_iterator) {
  if (!ec) {
    // The connection was successful. Send request.
    size_t n(BuildRequest(webhdfs_path));
    
    boost::asio::async_write(webhdfs_socket, boost::asio::buffer(buffer, n),
      boost::bind(&Connection::handle_write_request, shared_from_this(),
      boost::asio::placeholders::error,
      boost::asio::placeholders::bytes_transferred));
  } else if (endpoint_iterator != ip::tcp::resolver::iterator()) {
    // The connection failed. Try the next endpoint in the list.
    webhdfs_socket.close();
    ip::tcp::endpoint endpoint = *endpoint_iterator;
    webhdfs_socket.async_connect(endpoint,
      boost::bind(&Connection::handle_connect, shared_from_this(),
      boost::asio::placeholders::error,
      ++endpoint_iterator));
  } else {
    Trace(CPCL_TRACE_LEVEL_ERROR,
      "Connection(%08X)::handle_connect() fails: %s",
      (int)this, ec.message().c_str());
  }
}

void Connection::handle_write_request(boost::system::error_code const &ec, size_t bytes_transferred) {
  if (!ec) {
    parser.Reset(false);
    
    webhdfs_socket.async_read_some(boost::asio::buffer(buffer),
      boost::bind(&Connection::handle_read_webhdfs_response, shared_from_this(),
      boost::asio::placeholders::error,
      boost::asio::placeholders::bytes_transferred));
  } else {
    Trace(CPCL_TRACE_LEVEL_ERROR,
      "Connection(%08X)::handle_write_request() fails: %s",
      (int)this, ec.message().c_str());
  }
}

bool Connection::SetLocation(cpcl::StringPiece const &uri) {
  http_parser_url url;
  http_parser_parse_url(uri.data(), uri.size(), /*parser.method == HTTP_CONNECT*/0, &url);
  if ((1 << UF_HOST) & url.field_set) {
    host.assign(uri.data() + url.field_data[UF_HOST].off, url.field_data[UF_HOST].len);
  } else {
    Trace(CPCL_TRACE_LEVEL_ERROR,
      "Connection(%08X)::SetLocation(): redirect uri host field is missed",
      (int)this);
    return false;
  }
  if ((1 << UF_PORT) & url.field_set) {
    port.assign(uri.data() + url.field_data[UF_PORT].off, url.field_data[UF_PORT].len);
  } else {
    port.assign("80", 2);
  }
  if ((1 << UF_PATH) & url.field_set) {
    // UF_PATH doesn't contain query(i.e. ?op=OPEN), so we then use webhdfs, BuildRequest method use same mask for build request line
    if (cpcl::StringPiece(uri.data() + url.field_data[UF_PATH].off, url.field_data[UF_PATH].len).starts_with(WEBHDFS_REQUEST_PREFIX)) {
      url.field_data[UF_PATH].off += WEBHDFS_REQUEST_PREFIX.size();
      url.field_data[UF_PATH].len -= WEBHDFS_REQUEST_PREFIX.size();
    }
    webhdfs_path.assign(uri.data() + url.field_data[UF_PATH].off, url.field_data[UF_PATH].len);
  } else {
    Trace(CPCL_TRACE_LEVEL_ERROR,
      "Connection(%08X)::SetLocation(): redirect uri path field is missed",
      (int)this);
    return false;
  }
  return true;
}

static inline void CloseSocket(ip::tcp::socket &socket) {
  boost::system::error_code ignored_ec;
  socket.shutdown(ip::tcp::socket::shutdown_both, ignored_ec);
  ignored_ec = boost::system::error_code();
  socket.close(ignored_ec);
}

void Connection::handle_read_webhdfs_response(boost::system::error_code const &ec, size_t bytes_transferred) {
  if (!ec || boost::asio::error::eof == ec) {
    bool invalid_response(false);
    if (bytes_transferred > 0)
      invalid_response = !parser.Parse(reinterpret_cast<char const*>(buffer.data()), bytes_transferred);
    if (!invalid_response && boost::asio::error::eof == ec && !parser.headers_complete) {
      // invalid_response = !parser.headers_complete;
      invalid_response = true;
      cpcl::Trace(CPCL_TRACE_LEVEL_ERROR,
        "Connection(%08X)::handle_read_webhdfs_response(): eof, !parser.headers_complete",
        (int)this);
    }
    
    if (invalid_response) {
      SendResponse(500);
    } else {
      bool redirect(false);
      if (parser.headers_complete) {
        int redirect_status_codes[] = { 301, 302, 303, 307 };
        redirect = std::binary_search(redirect_status_codes, redirect_status_codes + arraysize(redirect_status_codes), parser.status_code);
      }
      if (redirect) {
        cpcl::StringPiece uri;
        if (!parser.GetHeader(StringPieceFromLiteral("Location"), &uri)) {
          cpcl::Trace(CPCL_TRACE_LEVEL_ERROR,
            "Connection(%08X)::handle_read_webhdfs_response(): redirect response from webhdfs doesn't contain location field",
            (int)this);
          SendResponse(500);
        } else {
          if (!SetLocation(uri)) {
            SendResponse(500);
          } else {
            CloseSocket(webhdfs_socket);
            SendRequest(webhdfs_path);
          }
        }
      } else {
        bool read_more(true);
        if (parser.headers_complete) {
          if (parser.status_code != 200) {
            read_more = false;
            SendResponse(parser.status_code);
          } else {
            if (parser.message_complete) {
              read_more = false;
              SendPage();
            } else if (boost::asio::error::eof == ec) {
              read_more = false;
              cpcl::Trace(CPCL_TRACE_LEVEL_ERROR,
                "Connection(%08X)::handle_read_webhdfs_response(): eof, !parser.message_complete",
                (int)this);
              SendResponse(500);
            }
          }
        }

        if (read_more) {
          webhdfs_socket.async_read_some(boost::asio::buffer(buffer),
            boost::bind(&Connection::handle_read_webhdfs_response, shared_from_this(),
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
        } else {
          CloseSocket(webhdfs_socket);
        }
      }
    }
  } else {
    cpcl::Trace(CPCL_TRACE_LEVEL_ERROR,
      "Connection(%08X)::handle_read_webhdfs_response() fails: %s",
      (int)this, ec.message().c_str());
  }
}

static inline void FitPage(boost::shared_ptr<plcl::Page> page, unsigned int sw, unsigned int sh) {
  page->Width(sw);
  if (page->Height() > sh)
    page->Height(sh);
}
void Connection::SendPage() {
  image->Seek(0, SEEK_SET, NULL);

  boost::shared_ptr<plcl::Doc> doc = plugin_list->LoadDoc(image.get());
  if (doc) {
    boost::shared_ptr<plcl::Page> page = doc->GetPage(0);
    if (page) {
      image_cache->Put(image_path, image);
      if (query.json) {
        page_width = page->Width(); page_height = page->Height(); page_pixfmt = page->GuessPixfmt();

        image.reset();
        SendResponse(200);
      } else {
        if (query.width > 0 && query.height > 0)
          FitPage(page, query.width, query.height);
        else if (!query.width && query.height > 0)
          page->Height(query.height);
        else if (query.width > 0 && !query.height)
          page->Width(query.width);

        image.reset(new DynamicMemoryStream());
        if (!task_pool->AddTask(shared_from_this(), page, image))
          SendResponse(500);
      }
    } else {
      cpcl::Trace(CPCL_TRACE_LEVEL_ERROR,
        "Connection(%08X)::SendPage(): unable to get page 0 from document \"%s\"",
        (int)this, image_path.c_str());
    }
  } else {
    cpcl::Trace(CPCL_TRACE_LEVEL_ERROR,
      "Connection(%08X)::SendPage(): unable to load document \"%s\"",
      (int)this, image_path.c_str());

    SendResponse(500);
  }
}

struct Response {
  int code;
  char const *message;
} static responses[] = {
  { 200, "OK" },
  { 302, "Found" },
  { 400, "Invalid request" },
  { 404, "Not Found" },
  { 500, "Server error" }
};
struct CompareResponse {
  bool operator()(Response const &a, Response const &b) const {
    return a.code < b.code;
  }
};
size_t Connection::BuildResponse(int code, size_t response_len) {
  Response v; v.code = code;
  Response *it = std::lower_bound(responses, responses + arraysize(responses), v, CompareResponse());
  char const *message = "Not Implemented";
  if (!(responses + arraysize(responses) == it || it->code != code))
    message = it->message;
  
  char *buf = reinterpret_cast<char*>(buffer.data());
  size_t buf_len(buffer.size());
  
  buf_len -= StringFormat(buf, buf_len, "HTTP/1.1 %d %s\r\n", code, message);
  buf += buffer.size() - buf_len;
  
  char json_response[0x100];
  if (200 == code) {
    // webhdfs
    WriteHeader(buf, buf_len, cpcl::StringPieceFromLiteral("Connection"), cpcl::StringPieceFromLiteral("close"));
    if (!query.json) {
      WriteHeader(buf, buf_len, cpcl::StringPieceFromLiteral("Content-Type"), cpcl::StringPieceFromLiteral("image/jpeg"));
      WriteHeader(buf, buf_len, cpcl::StringPieceFromLiteral("Transfer-Encoding"), cpcl::StringPieceFromLiteral("chunked"));
    } else {
      WriteHeader(buf, buf_len, cpcl::StringPieceFromLiteral("Content-Type"), cpcl::StringPieceFromLiteral("application/json"));
      
      char const* pf_s[] = { "invalid", "gray8", "rgb24", "bgr24", "rgba32", "argb32", "abgr32", "bgra32" };
      unsigned int pf[] = { PLCL_PIXEL_FORMAT_INVALID, PLCL_PIXEL_FORMAT_GRAY_8, PLCL_PIXEL_FORMAT_RGB_24, PLCL_PIXEL_FORMAT_BGR_24, PLCL_PIXEL_FORMAT_RGBA_32, PLCL_PIXEL_FORMAT_ARGB_32, PLCL_PIXEL_FORMAT_ABGR_32, PLCL_PIXEL_FORMAT_BGRA_32 };
      unsigned int *it = std::lower_bound(pf, pf + arraysize(pf), page_pixfmt);
      size_t i(0);
      if (!(pf + arraysize(pf) == it || *it != page_pixfmt))
        i = it - pf;
      response_len = cpcl::StringFormat(json_response,
        "{'width' : '%u', 'height' : '%u', 'pf' : '%s'}",
        page_width, page_height, pf_s[i]);
      
      char json_response_len_buf[0x10];
      WriteHeader(buf, buf_len, cpcl::StringPieceFromLiteral("Content-Length"), StringPiece(json_response_len_buf, StringFormat(json_response_len_buf, "%u", response_len)));
    }
  }
  StringAdvance(buf, buf_len, StringPieceFromLiteral("\r\n"));
  
  if (200 == code && query.json) {
    response_len = (std::min)(response_len, buf_len);
    memcpy(buf, json_response, response_len);
    buf_len -= response_len;
  }
  return buffer.size() - buf_len;
}

void Connection::SendResponse(int code) {
  size_t response_len(0);
  if (200 == code && !query.json) {
    if (!image) {
      Error(StringPieceFromLiteral("Connection::SendResponse(): no image for non-json query"));
      code = 500;
    } else {
      image->Seek(0, SEEK_SET, NULL);
      response_len = (size_t)image->Size();
      if (!response_len) {
        Error(StringPieceFromLiteral("Connection::SendResponse(): empty image response"));
        code = 500;
      }
    }
  } else
    image.reset();
  status_code = code;
  size_t n(BuildResponse(code, response_len));

  boost::asio::async_write(client_socket, boost::asio::buffer(buffer, n),
    boost::bind(&Connection::handle_write_response, shared_from_this(),
    boost::asio::placeholders::error,
    boost::asio::placeholders::bytes_transferred));
}

boost::asio::const_buffers_1 Connection::BuildChunk(size_t chunk_size) {
  char *buf = reinterpret_cast<char*>(buffer.data());
  char tmp[CHUNK_OFFSET + 1];
  size_t n = cpcl::StringFormat(tmp, "%x\r\n", chunk_size);
  // CHUNK_OFFSET - n;
  ::memcpy(buf + (CHUNK_OFFSET - n), tmp, n);
  ::memcpy(buf + CHUNK_OFFSET + chunk_size, "\r\n", 2);
  return boost::asio::const_buffers_1(buf + (CHUNK_OFFSET - n), chunk_size + n + 2);
}
void Connection::handle_write_response(boost::system::error_code const &ec, size_t bytes_transferred) {
  if (!ec) {
    if (200 == status_code && !!image) {
      size_t n = image->Read(buffer.data() + CHUNK_OFFSET, MAX_CHUNK_SIZE);
      if (!n)
        image.reset();
      
      boost::asio::async_write(client_socket, BuildChunk(n),
        boost::bind(&Connection::handle_write_response, shared_from_this(),
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
    }
  } else {
    cpcl::Trace(CPCL_TRACE_LEVEL_ERROR,
      "Connection(%08X)::handle_write_response() fails: %s",
      (int)this, ec.message().c_str());
  }
}

//socket.close(); - abort async_read / async_write
//	or 
//boost::system::error_code ignored_ec;
//socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
//socket.close();

} // namespace net

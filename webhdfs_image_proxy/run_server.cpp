#include <cpcl/basic.h>
#include "server.h"
#include <cpcl/trace.h>

static net::Connection* ctor(boost::asio::io_service &io_service, boost::shared_ptr<ImageCache> image_cache, boost::shared_ptr<TaskPool> task_pool,
  std::string host, std::string port, plcl::PluginList *plugin_list) {
  return new net::Connection(io_service, image_cache, task_pool, host, port, plugin_list);
}

namespace ip = boost::asio::ip;
void RunServer(std::string const &in_host, std::string const &in_port, std::string const &out_host, std::string const &out_port) {
  std::auto_ptr<plcl::PluginList> plugin_list(plcl::PluginList::Create()); // Server::Run joins to all threads, so control must return only when all Connections deleted and PluginList not used
  if (!plugin_list.get()) {
    cpcl::Error(cpcl::StringPieceFromLiteral("RunServer(): no plugins loaded"));
    return;
  }
  
  boost::shared_ptr<net::Server> server;
  try {
    ip::tcp::endpoint endpoint;
    {
      boost::asio::io_service io_service;
      ip::tcp::resolver resolver(io_service);
      
      ip::tcp::resolver::query query(in_host, in_port, ip::resolver_query_base::v4_mapped |
        /* ip::resolver_query_base::numeric_host | */ ip::resolver_query_base::numeric_service);
      ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
      endpoint = *endpoint_iterator;
    }
    
    server.reset(new net::Server(endpoint, boost::bind(ctor, _1, _2, _3, out_host, out_port, plugin_list.get())));
    server->Run();
  } catch (boost::system::system_error const &e) {
    cpcl::Trace(CPCL_TRACE_LEVEL_ERROR,
      "RunServer() fails: %s\n%s",
      e.what(), e.code().message().c_str());
  } catch (std::exception const &e) {
    char const *s = e.what();
    if (!!s)
      cpcl::Trace(CPCL_TRACE_LEVEL_ERROR, "RunServer() fails: exception: %s", s);
    else
      cpcl::Error(cpcl::StringPieceFromLiteral("RunServer() fails: exception"));
  }
}

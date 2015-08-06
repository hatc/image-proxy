#include <cpcl/basic.h>
#ifdef _MSC_VER
#include <stdio.h>
#include <tchar.h>
#endif
#include <iostream>

#include <cpcl/file_util.h>
#include <cpcl/trace.h>

//struct Query {
//	/*cpcl::StringPiece request_path;*/
//	unsigned int width, height;
//	bool json;
//
//	/*explicit Query(request_path_)*/Query() : width(0), height(0), json(false)
//	{}
//};
//namespace std {
//std::ostream& operator<<(std::ostream &out, Query const &v) {
//  out << "w=" << v.width << " h=" << v.height << " json?" << std::boolalpha << v.json << std::endl;
//  return out;
//}
//}

void RunServer(std::string const &in_host, std::string const &in_port, std::string const &out_host, std::string const &out_port);

#ifdef _MSC_VER
static inline std::string w2c(wchar_t const *s) {
  return std::string(s, s + ::wcslen(s));
}
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char **argv)
#endif
{
  if (argc < 5) {
    // hadoop.namenode.virtu.com 50070
    std::cout << "<listen-host> <listen-port> <namenode-host> <namenode-port>" << std::endl;
    return 0;
  }
  {
    FilePathChar buf[] = { '.', 'l', 'o', 'g', '\0' };
    std::basic_string<FilePathChar> module_path;
    if (argc > 0 && cpcl::GetModuleFilePath((void*)0, &module_path))
      cpcl::SetTraceFilePath(cpcl::Join(module_path, cpcl::BaseName(argv[0])) + buf);
  }
  cpcl::Debug(cpcl::StringPieceFromLiteral("Log started"));
#ifdef _MSC_VER 
  RunServer(w2c(argv[1]), w2c(argv[2]), w2c(argv[3]), w2c(argv[4]));
#else
  RunServer(argv[1], argv[2], argv[3], argv[4]);
#endif
  // RunServer("127.0.0.1", "8080", "google.com", "80");
  return 0;
}

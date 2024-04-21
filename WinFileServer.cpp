// WinFileServer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "httplib.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>

using namespace httplib;

// HTTP
httplib::Server svr;

std::string dump_headers(const Headers &headers) {
  std::string s;
  char buf[BUFSIZ];

  for (auto it = headers.begin(); it != headers.end(); ++it) {
    const auto &x = *it;
    snprintf(buf, sizeof(buf), "%s: %s\n", x.first.c_str(), x.second.c_str());
    s += buf;
  }

  return s;
}

std::string log(const Request &req, const Response &res) {
  std::string s;
  char buf[BUFSIZ];

  s += "================================\n";

  snprintf(buf, sizeof(buf), "%s %s %s", req.method.c_str(),
           req.version.c_str(), req.path.c_str());
  s += buf;

  std::string query;
  for (auto it = req.params.begin(); it != req.params.end(); ++it) {
    const auto &x = *it;
    snprintf(buf, sizeof(buf), "%c%s=%s",
             (it == req.params.begin()) ? '?' : '&', x.first.c_str(),
             x.second.c_str());
    query += buf;
  }
  snprintf(buf, sizeof(buf), "%s\n", query.c_str());
  s += buf;

  s += dump_headers(req.headers);

  s += "--------------------------------\n";

  snprintf(buf, sizeof(buf), "%d %s\n", res.status, res.version.c_str());
  s += buf;
  s += dump_headers(res.headers);
  s += "\n";

  if (!res.body.empty()) { s += res.body; }

  s += "\n";

  return s;
}


void write_storage_files_to_html() {
  namespace fs = std::filesystem;

  // 获取当前工作目录下www文件夹路径
  fs::path www_dir = fs::current_path() / "www";

  // 检查www文件夹是否存在且为目录
  if (!fs::exists(www_dir) || !fs::is_directory(www_dir)) {
    std::cerr << "Directory 'www' does not exist or is not a directory.\n";
    return;
  }

  // 获取www文件夹下名为"storage"的子目录路径
  fs::path storage_dir = www_dir / "storage";

  // 检查"storage"子目录是否存在且为目录
  if (!fs::exists(storage_dir) || !fs::is_directory(storage_dir)) {
    std::cerr
        << "Directory 'www/storage' does not exist or is not a directory.\n";
    return;
  }

  // 获取www文件夹下名为"pages"的子目录路径
  fs::path pages_dir = www_dir / "pages";

  // 检查"pages"子目录是否存在，如果不存在则创建
  if (!fs::exists(pages_dir)) {
    if (!fs::create_directory(pages_dir)) {
      std::cerr << "Failed to create directory 'www/pages'.\n";
      return;
    }
  }

  // 获取输出HTML文件路径
  fs::path output_file = pages_dir / "fileindex.html";

  // 打开输出HTML文件，指定UTF-8编码
  std::ofstream html_file(
      output_file, std::ios_base::out | std::ios_base::binary | std::ios::ate);
  if (!html_file.is_open()) {
    std::cerr << "Failed to open output HTML file: " << output_file.string()
              << "\n";
    return;
  }

  // 在文件开头写入UTF-8 BOM标记，以指示文件使用UTF-8编码
  html_file.put(char(0xEF));
  html_file.put(char(0xBB));
  html_file.put(char(0xBF));

  // 使用std::stringstream构建HTML内容
  std::stringstream ss;
  ss << "<!DOCTYPE html>\n"
     << "<html lang=\"en\">\n"
     << "<head>\n"
     << "  <meta charset=\"UTF-8\">\n"
     << "  <title>Files in 'storage' Directory</title>\n"
     << "</head>\n"
     << "<body>\n"
     << "<h1>Files in 'storage' Directory</h1>\n"
     << "<ul>\n";

  // 遍历"storage"子目录下的文件并写入HTML链接
  for (const auto &entry : fs::directory_iterator(storage_dir)) {
    if (entry.is_regular_file()) {
      // 直接使用u8string()获取UTF-8编码的文件名
      std::string filename = entry.path().filename().u8string();
      ss << "  <li><a href=\"../../storage/" 
         << filename 
         << "\" download>"
         << filename 
         << "</a></li>\n";
    }
  }

  ss << "</ul>\n"
     << "</body>\n"
     << "</html>\n";

  // 将std::stringstream中的内容写入文件
  html_file.write(ss.str().data(), ss.str().size());
  html_file.close();

  std::cout << "Successfully wrote file names to '" << output_file.string()
            << "'\n";
}


int main(void) {
  Server svr;
  svr.set_base_dir("./");
  svr.set_mount_point("/", "./www");
  svr.set_file_extension_and_mimetype_mapping("cc", "text/x-c");
  svr.set_file_extension_and_mimetype_mapping("cpp", "text/x-c");
  svr.set_file_extension_and_mimetype_mapping("hh", "text/x-h");
  svr.set_file_extension_and_mimetype_mapping("h", "text/x-h");
  svr.set_file_extension_and_mimetype_mapping("mp3", "audio/mpeg");
  svr.set_file_extension_and_mimetype_mapping("mp4", "video/mpeg");
  svr.set_file_extension_and_mimetype_mapping("avi", "video/x-msvideo");
  svr.set_file_extension_and_mimetype_mapping("json", "application/json");
  auto ret = svr.set_mount_point("/", "./www");

  if (!ret) {
    // The specified base directory doesn't exist...
    std::cout << "The specified base directory doesn't exist..." << std::endl;
  }

  write_storage_files_to_html();

  svr.Get("/hi", [](const Request &req, Response &res) {
    res.set_content("Hello World!", "text/plain");
  });

  svr.Get("/", [](const Request &req, Response &res) {
    res.set_content("./index.html", "text/html");
  });
  svr.Get("/page/fileindex.html", [](const Request &req, Response &res) {
    res.set_content("./page/fileindex.html", "text/html");
  });
  svr.Get(R"(/numbers/(\d+))", [&](const Request &req, Response &res) {
    auto numbers = req.matches[1];
    res.set_content(numbers, "text/plain");
  });

  svr.Get("/body-header-param", [](const Request &req, Response &res) {
    if (req.has_header("Content-Length")) {
      auto val = req.get_header_value("Content-Length");
    }
    if (req.has_param("key")) { auto val = req.get_param_value("key"); }
    res.set_content(req.body, "text/plain");
  });

  svr.Get("/stop", [&](const Request &req, Response &res) { svr.stop(); });

  /// listen
  svr.listen("localhost", 1234);
}

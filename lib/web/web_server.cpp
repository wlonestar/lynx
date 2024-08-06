#include "lynx/web/web_server.h"
#include "lynx/http/http_server.h"
#include "lynx/logger/logging.h"

#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <regex>

namespace lynx {

namespace fs = std::filesystem;

namespace detail {

void handleIndex(const HttpRequest &req, HttpResponse *resp) {
  resp->setStatusCode(HttpResponse::Ok200);
  resp->setStatusMessage("OK");
  resp->setContentType("text/html");
  resp->addHeader("Server", "lynx");
  std::string now = Timestamp::now().toFormattedString();
  resp->setBody("<html><head><title>This is title</title></head>"
                "<body><h1>Hello</h1>Now is " +
                now + "</body></html>");
}

void handleFavicon(const HttpRequest &req, HttpResponse *resp) {
  resp->setStatusCode(HttpResponse::Ok200);
  resp->setStatusMessage("OK");
  resp->setContentType("image/png");
  resp->setBody(
      std::string(reinterpret_cast<char *>(favicon_jpg), favicon_jpg_len));
}

void handleNotFound(const HttpRequest &req, HttpResponse *resp) {
  resp->setStatusCode(HttpResponse::NotFound404);
  resp->setStatusMessage("Not Found");
  resp->setCloseConnection(true);
}

std::string getExecutableDir() {
  char result[PATH_MAX];
  ssize_t count = readlink("/proc/self/exe", result, sizeof(result) - 1);
  if (count == -1) {
    throw std::runtime_error("Failed to read /proc/self/exe");
  }
  result[count] = '\0';

  std::string full_path(result);
  size_t last_slash = full_path.rfind('/');
  if (last_slash != std::string::npos) {
    return full_path.substr(0, last_slash + 1); /// Include the trailing slash
  }
  return ""; /// This should never happen with /proc/self/exe
}

} // namespace detail

WebServer::WebServer(EventLoop *loop, const std::string &filename) {
  /// Load config file
  auto path = detail::getExecutableDir() + "conf/" + filename;
  if (!fs::exists(path)) {
    fprintf(stderr, "Error: can not find config file: %s\n", path.c_str());
    abort();
  }
  loadConfig(path);

  /// Create Http server
  server_ = std::make_unique<HttpServer>(
      loop, InetAddress(atoi(config_map_["server"]["port"].c_str())),
      config_map_["server"]["name"]);
  server_->setThreadNum(atoi(config_map_["server"]["threads"].c_str()));
  server_->setHttpCallback([this](auto &&PH1, auto &&PH2) {
    onRequest(std::forward<decltype(PH1)>(PH1),
              std::forward<decltype(PH2)>(PH2));
  });
  addRoute("GET", "/", detail::handleIndex);
  addRoute("GET", "/favicon.ico", detail::handleFavicon);

  /// Create Pg connection pool if find `db` key
  if (config_map_.find("db") != config_map_.end()) {
    pool_ = std::make_unique<PgConnectionPool>(
        config_map_["db"]["host"], config_map_["db"]["port"],
        config_map_["db"]["user"], config_map_["db"]["password"],
        config_map_["db"]["dbname"], atoi(config_map_["db"]["timeout"].c_str()),
        atoi(config_map_["db"]["min_size"].c_str()),
        atoi(config_map_["db"]["max_size"].c_str()), config_map_["db"]["name"]);
  }
}

void WebServer::start() {
  server_->start();
  if (pool_ != nullptr) {
    pool_->start();
  }
}

PgConnectionPool &WebServer::pool() const {
  assert(pool_ != nullptr);
  return *pool_;
}

void WebServer::addRoute(const std::string &method, const std::string &path,
                         HttpHandler handler) {
  route_table_[std::make_pair(stringToHttpMethod(method), path)] = handler;
}

void WebServer::printRoutes() {
  std::cout << "Route Table:\n";
  for (auto &[pair, handler] : route_table_) {
    printf("%6s - %s\n", methodToString(pair.first), pair.second.c_str());
  }
}

void WebServer::loadConfig(const std::string &filePath) {
  YAML::Node config = YAML::LoadFile(filePath);
  for (const auto &pair : config) {
    auto section_name = pair.first.as<std::string>();
    /// Fill in default key and value
    if (section_name == "server") {
      config_map_["server"]["name"] = "WebServer";
      config_map_["server"]["port"] = "8000";
      config_map_["server"]["threads"] = "5";
    } else if (section_name == "db") {
      config_map_["db"]["name"] = "PgConnectionPool";
      config_map_["db"]["timeout"] = "10";
      config_map_["db"]["min_size"] = "5";
      config_map_["db"]["max_size"] = "10";
    }

    std::map<std::string, std::string> section_map;
    for (const auto &sub_pair : pair.second) {
      auto key = sub_pair.first.as<std::string>();
      auto value = sub_pair.second.as<std::string>();
      section_map[key] = value;
    }

    config_map_[section_name] = section_map;
  }

  if (config_map_.find("db") != config_map_.end()) {
    auto db = config_map_["db"];
    bool valid = db.find("host") != db.end() && db.find("port") != db.end() &&
                 db.find("user") != db.end() &&
                 db.find("password") != db.end() &&
                 db.find("dbname") != db.end();
    if (!valid) {
      fprintf(stderr, "Error: invalid database connection arguments\n");
      abort();
    }
  }
}

void WebServer::onRequest(const lynx::HttpRequest &req,
                          lynx::HttpResponse *resp) {
  std::cout << req.toString() << "\n";

  /// Searching for path with query
  std::string path = std::string(req.path());
  if (!req.query().empty()) {
    path += "?" + std::string(req.query());
  }
  LOG_DEBUG << "search for '" << path << "'";

  bool flag = false;
  auto method = req.method();
  for (auto &[pair, handler] : route_table_) {
    if (method == pair.first) {
      std::regex path_regex(pair.second);
      bool match = std::regex_match(path, path_regex);
      if (match) {
        flag = true;
        handler(req, resp);
        break;
      }
    }
  }
  if (!flag) {
    LOG_DEBUG << "not found";
    detail::handleNotFound(req, resp);
  }
}

} // namespace lynx

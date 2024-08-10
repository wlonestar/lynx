#include "lynx/app/application.h"
#include "lynx/logger/logging.h"

#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <regex>

namespace lynx {

namespace fs = std::filesystem;

namespace detail {

void handleNotFound(const HttpRequest &req, HttpResponse *resp) {
  resp->setStatusCode(HttpResponse::NotFound404);
  resp->setStatusMessage("Not Found");
  std::string now = Timestamp::now().toFormattedString();
  resp->setBody("<html><body><h1>Error Page</h1><p>" + now +
                "</p><p>There was an unexcepted error (type=Not Found, "
                "status=404).</p></body></html>");
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

Application::Application(EventLoop *loop, const std::string &filename) {
  /// Load config file
  auto path = detail::getExecutableDir() + "conf/" + filename;
  if (!fs::exists(path)) {
    fprintf(stderr, "Error: can not find config file: %s\n", path.c_str());
    abort();
  }
  LOG_TRACE << "read config from " << path;
  loadConfig(path);

  /// Create Http server
  server_ = std::make_unique<HttpServer>(
      loop,
      InetAddress(
          static_cast<uint16_t>(atoi(config_map_["server"]["port"].c_str()))),
      config_map_["server"]["name"]);

  /// Set Http server params
  server_->setThreadNum(atoi(config_map_["server"]["threads"].c_str()));
  server_->setHttpCallback([this](auto &&PH1, auto &&PH2) {
    onRequest(std::forward<decltype(PH1)>(PH1),
              std::forward<decltype(PH2)>(PH2));
  });

  /// Create Pg connection pool if find `db` key
  if (config_map_.find("db") != config_map_.end()) {
    ConnectionPoolConfig config(
        config_map_["db"]["host"], atoi(config_map_["db"]["port"].c_str()),
        config_map_["db"]["user"], config_map_["db"]["password"],
        config_map_["db"]["dbname"],
        atoi(config_map_["db"]["min_size"].c_str()),
        atoi(config_map_["db"]["max_size"].c_str()),
        atoi(config_map_["db"]["timeout"].c_str()),
        atoi(config_map_["db"]["max_idle_time"].c_str()));
    pool_ = std::make_unique<ConnectionPool>(config, config_map_["db"]["name"]);
  }
}

void Application::start() {
  LOG_DEBUG << "init http server";
  server_->start();
  if (pool_ != nullptr) {
    LOG_DEBUG << "init database connection pool";
    pool_->start();
  }
}

ConnectionPool &Application::pool() const {
  if (pool_ == nullptr) {
    LOG_SYSFATAL << "connection pool is not correctly created!";
  }
  return *pool_;
}

int Application::addRoute(const std::string &method, const std::string &path,
                          HttpHandler handler) {
  route_table_[std::make_pair(stringToHttpMethod(method), path)] = handler;
  return 0;
}

void Application::printRouteTable() {
  std::cout << "Route Table:\n";
  for (auto &[pair, handler] : route_table_) {
    printf("%6s - %s\n", methodToString(pair.first), pair.second.c_str());
  }
}

void Application::loadConfig(const std::string &filePath) {
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
      config_map_["db"]["min_size"] = "5";
      config_map_["db"]["max_size"] = "10";
      config_map_["db"]["timeout"] = "10";
      config_map_["db"]["max_idle_time"] = "5000";
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

  for (auto &section : config_map_) {
    for (auto &[first, second] : section.second) {
      LOG_DEBUG << section.first << "." << first << " = " << second;
    }
  }
}

void Application::onRequest(const lynx::HttpRequest &req,
                            lynx::HttpResponse *resp) {
  LOG_INFO << lynx::methodToString(req.method()) << " " << req.path();

  /// Log request headers
  std::stringstream ss;
  for (const auto &header : req.headers()) {
    ss << header.first << ": " << header.second << "|";
  }
  LOG_INFO << ss.str();

  /// Concat path and query
  std::string path = std::string(req.path());
  if (!req.query().empty()) {
    path += "?" + std::string(req.query());
  }
  LOG_DEBUG << "searching for '" << path << "'";

  /// Searching in route table by method and path
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

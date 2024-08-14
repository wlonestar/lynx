#include "lynx/app/application.h"
#include "lynx/logger/logging.h"
#include "lynx/net/event_loop.h"

#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <regex>

namespace lynx {

namespace fs = std::filesystem;

namespace detail {

void handleNotFound(const HttpRequest &req, HttpResponse *resp) {
  resp->setStatusCode(HttpStatus::NOT_FOUND);
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

Application::Application(const std::string &filename) {
  /// Create a new event loop
  loop_ = new EventLoop();

  /// If no configuration file is provided, create a default HTTP server
  if (filename.empty()) {
    server_ = std::make_unique<HttpServer>(loop_, InetAddress(8000), "App");

    /// Set HTTP server parameters
    server_->setThreadNum(5);
    server_->setHttpCallback([this](auto &&req, auto &&resp) {
      onRequest(std::forward<decltype(req)>(req),
                std::forward<decltype(resp)>(resp));
    });
    return;
  }

  /// Load configuration file
  auto path = detail::getExecutableDir() + "conf/" + filename;
  if (!fs::exists(path)) {
    fprintf(stderr, "Error: can not find config file: %s\n", path.c_str());
    abort();
  }
  LOG_TRACE << "Reading config from " << path;
  loadConfig(path);

  /// Create HTTP server with parameters from the configuration file
  server_ = std::make_unique<HttpServer>(
      loop_,
      InetAddress(
          static_cast<uint16_t>(atoi(config_map_["server"]["port"].c_str()))),
      config_map_["server"]["name"]);

  /// Set HTTP server parameters
  server_->setThreadNum(atoi(config_map_["server"]["threads"].c_str()));
  server_->setHttpCallback([this](auto &&PH1, auto &&PH2) {
    onRequest(std::forward<decltype(PH1)>(PH1),
              std::forward<decltype(PH2)>(PH2));
  });

  /// Create PostgreSQL connection pool if configuration is present
  if (config_map_.find("db") != config_map_.end()) {
    ConnectionPoolConfig config(
        config_map_["db"]["host"],                       // Host
        atoi(config_map_["db"]["port"].c_str()),         // Port
        config_map_["db"]["user"],                       // Username
        config_map_["db"]["password"],                   // Password
        config_map_["db"]["dbname"],                     // Database name
        atoi(config_map_["db"]["min_size"].c_str()),     // Minimum pool size
        atoi(config_map_["db"]["max_size"].c_str()),     // Maximum pool size
        atoi(config_map_["db"]["timeout"].c_str()),      // Connection timeout
        atoi(config_map_["db"]["max_idle_time"].c_str()) // Maximum idle time
    );
    pool_ = std::make_unique<ConnectionPool>(config, config_map_["db"]["name"]);
  }
}

Application::~Application() {
  if (pool_ != nullptr) {
    pool_->stop();
  }
  loop_->quit();
  delete loop_;
}

void Application::start() {
  /// Initialize the HTTP server
  LOG_DEBUG << "Initializing HTTP server";
  server_->start();

  /// Initialize the database connection pool if it is configured
  if (pool_ != nullptr) {
    LOG_DEBUG << "Initializing database connection pool";
    pool_->start();
  }
}

void Application::listen() { loop_->loop(); }

ConnectionPool &Application::pool() const {
  if (pool_ == nullptr) {
    LOG_SYSFATAL << "connection pool is not correctly created!";
  }
  return *pool_;
}

void Application::addRoute(const std::string &method, const std::string &path,
                           HttpHandler handler) {
  route_table_[std::make_pair(stringToHttpMethod(method), path)] = handler;
}

void Application::printRouteTable() {
  std::cout << "Route Table:\n";
  for (auto &[pair, handler] : route_table_) {
    printf("%6s - %s\n", methodToString(pair.first), pair.second.c_str());
  }
}

void Application::loadConfig(const std::string &filePath) {
  /// Load the YAML configuration file
  YAML::Node config = YAML::LoadFile(filePath);

  /// Iterate over each section in the configuration file
  for (const auto &pair : config) {
    auto section_name = pair.first.as<std::string>();

    /// Fill in default key-value pairs for the "server" section
    if (section_name == "server") {
      config_map_["server"]["name"] = "WebServer";
      config_map_["server"]["port"] = "8000";
      config_map_["server"]["threads"] = "5";
    }
    /// Fill in default key-value pairs for the "db" section
    else if (section_name == "db") {
      config_map_["db"]["name"] = "PgConnectionPool";
      config_map_["db"]["min_size"] = "5";
      config_map_["db"]["max_size"] = "10";
      config_map_["db"]["timeout"] = "10";
      config_map_["db"]["max_idle_time"] = "5000";
    }

    /// Store the key-value pairs for the current section
    std::map<std::string, std::string> section_map;
    for (const auto &sub_pair : pair.second) {
      auto key = sub_pair.first.as<std::string>();
      auto value = sub_pair.second.as<std::string>();
      section_map[key] = value;
    }

    config_map_[section_name] = section_map;
  }

  /// Check if the "db" section exists in the configuration
  if (config_map_.find("db") != config_map_.end()) {
    auto db = config_map_["db"];
    /// Check if the required keys for database connection are present
    bool valid = db.find("host") != db.end() && db.find("port") != db.end() &&
                 db.find("user") != db.end() &&
                 db.find("password") != db.end() &&
                 db.find("dbname") != db.end();
    if (!valid) {
      fprintf(stderr, "Error: invalid database connection arguments\n");
      abort();
    }
  }

  /// Log the configuration values
  for (auto &section : config_map_) {
    for (auto &[first, second] : section.second) {
      LOG_DEBUG << section.first << "." << first << " = " << second;
    }
  }
}

void Application::onRequest(const lynx::HttpRequest &req,
                            lynx::HttpResponse *resp) {
  /// Log the request method and path
  LOG_INFO << lynx::methodToString(req.method()) << " " << req.uri();

  /// Log the request headers
  LOG_DEBUG << "Request headers:";
  for (const auto &header : req.headers()) {
    LOG_DEBUG << header.first << ": " << header.second;
  }

  /// Search for a matching route in the route table
  std::string uri = std::string(req.uri());
  LOG_DEBUG << "Searching for '" << uri << "'";
  bool flag = false;
  auto method = req.method();
  for (auto &[pair, handler] : route_table_) {
    if (method == pair.first) {
      std::regex path_regex(pair.second);
      bool match = std::regex_match(uri, path_regex);
      if (match) {
        flag = true;
        LOG_DEBUG << "Found route '" << pair.second << "'";
        // Invoke the handler for the matching route
        handler(req, resp);
        break;
      }
    }
  }

  /// If no matching route is found, handle it as a not found request
  if (!flag) {
    LOG_DEBUG << "Not found";
    detail::handleNotFound(req, resp);
  }
}

} // namespace lynx

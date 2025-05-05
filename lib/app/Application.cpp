#include "lynx/app/Application.h"
#include "lynx/logger/Logging.h"
#include "lynx/net/EventLoop.h"

#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <regex>

namespace lynx {

namespace fs = std::filesystem;

namespace detail {

void handleNotFound(const HttpRequest &Req, HttpResponse *Resp) {
  Resp->setStatusCode(HttpStatus::NOT_FOUND);
  std::string Curr = Timestamp::now().toFormattedString();
  Resp->setBody("<html><body><h1>Error Page</h1><p>" + Curr +
                "</p><p>There was an unexcepted error (type=Not Found, "
                "status=404).</p></body></html>");
}

std::string getExecutableDir() {
  char Result[PATH_MAX];
  ssize_t Count = readlink("/proc/self/exe", Result, sizeof(Result) - 1);
  if (Count == -1)
    throw std::runtime_error("Failed to read /proc/self/exe");

  Result[Count] = '\0';

  std::string FullPath(Result);
  size_t LastSlash = FullPath.rfind('/');
  if (LastSlash != std::string::npos)
    return FullPath.substr(0, LastSlash + 1); /// Include the trailing slash

  return ""; /// This should never happen with /proc/self/exe
}

} // namespace detail

Application::Application(const std::string &Filename) {
  /// Create a new event loop
  Loop = new EventLoop();

  /// If no configuration file is provided, create a default HTTP server
  if (Filename.empty()) {
    Server = std::make_unique<HttpServer>(Loop, InetAddress(8000), "App");

    /// Set HTTP server parameters
    Server->setThreadNum(5);
    Server->setHttpCallback([this](auto &&Req, auto &&Resp) {
      onRequest(std::forward<decltype(Req)>(Req),
                std::forward<decltype(Resp)>(Resp));
    });
    return;
  }

  /// Load configuration file
  auto Path = detail::getExecutableDir() + "conf/" + Filename;
  if (!fs::exists(Path)) {
    fprintf(stderr, "Error: can not find Config file: %s\n", Path.c_str());
    abort();
  }
  LOG_TRACE << "Reading Config from " << Path;
  loadConfig(Path);

  /// Create HTTP server with parameters from the configuration file
  Server = std::make_unique<HttpServer>(
      Loop,
      InetAddress(
          static_cast<uint16_t>(atoi(ConfigMap["server"]["port"].c_str()))),
      ConfigMap["server"]["name"]);

  /// Set HTTP server parameters
  Server->setThreadNum(atoi(ConfigMap["server"]["threads"].c_str()));
  Server->setHttpCallback([this](auto &&PH1, auto &&PH2) {
    onRequest(std::forward<decltype(PH1)>(PH1),
              std::forward<decltype(PH2)>(PH2));
  });

  /// Create PostgreSQL connection pool if configuration is present
  if (ConfigMap.find("db") != ConfigMap.end()) {
    ConnectionPoolConfig Config(
        ConfigMap["db"]["host"],                       // Host
        atoi(ConfigMap["db"]["port"].c_str()),         // Port
        ConfigMap["db"]["user"],                       // Username
        ConfigMap["db"]["password"],                   // Password
        ConfigMap["db"]["dbname"],                     // Database name
        atoi(ConfigMap["db"]["min_size"].c_str()),     // Minimum pool size
        atoi(ConfigMap["db"]["max_size"].c_str()),     // Maximum pool size
        atoi(ConfigMap["db"]["timeout"].c_str()),      // Connection timeout
        atoi(ConfigMap["db"]["max_idle_time"].c_str()) // Maximum idle time
    );
    Pool = std::make_unique<ConnectionPool>(Config, ConfigMap["db"]["name"]);
  }
}

Application::~Application() {
  if (Pool != nullptr)
    Pool->stop();

  Loop->quit();
  delete Loop;
}

void Application::start() {
  /// Initialize the HTTP server
  LOG_DEBUG << "Initializing HTTP server";
  Server->start();

  /// Initialize the database connection pool if it is configured
  if (Pool != nullptr) {
    LOG_DEBUG << "Initializing database connection pool";
    Pool->start();
  }
}

void Application::listen() { Loop->loop(); }

ConnectionPool &Application::pool() const {
  if (Pool == nullptr)
    LOG_SYSFATAL << "connection pool is not correctly created!";

  return *Pool;
}

void Application::addRoute(const std::string &Method, const std::string &Path,
                           HttpHandler Handler) {
  RouteTable[std::make_pair(stringToHttpMethod(Method), Path)] = Handler;
}

void Application::printRouteTable() {
  std::cout << "Route Table:\n";
  for (auto &[pair, Handler] : RouteTable)
    printf("%6s - %s\n", methodToString(pair.first), pair.second.c_str());
}

void Application::loadConfig(const std::string &FilePath) {
  /// Load the YAML configuration file
  YAML::Node Config = YAML::LoadFile(FilePath);

  /// Iterate over each section in the configuration file
  for (const auto &Pair : Config) {
    auto SectionName = Pair.first.as<std::string>();

    /// Fill in default key-value pairs for the "server" section
    if (SectionName == "server") {
      ConfigMap["server"]["name"] = "WebServer";
      ConfigMap["server"]["port"] = "8000";
      ConfigMap["server"]["threads"] = "5";
    }
    /// Fill in default key-value pairs for the "db" section
    else if (SectionName == "db") {
      ConfigMap["db"]["name"] = "PgConnectionPool";
      ConfigMap["db"]["min_size"] = "5";
      ConfigMap["db"]["max_size"] = "10";
      ConfigMap["db"]["timeout"] = "10";
      ConfigMap["db"]["max_idle_time"] = "5000";
    }

    /// Store the key-value pairs for the current section
    std::map<std::string, std::string> SectionMap;
    for (const auto &SubPair : Pair.second) {
      auto Key = SubPair.first.as<std::string>();
      auto Value = SubPair.second.as<std::string>();
      SectionMap[Key] = Value;
    }

    ConfigMap[SectionName] = SectionMap;
  }

  /// Check if the "db" section exists in the configuration
  if (ConfigMap.find("db") != ConfigMap.end()) {
    auto Db = ConfigMap["db"];
    /// Check if the required keys for database connection are present
    bool Valid = Db.find("host") != Db.end() && Db.find("port") != Db.end() &&
                 Db.find("user") != Db.end() &&
                 Db.find("password") != Db.end() &&
                 Db.find("dbname") != Db.end();
    if (!Valid) {
      fprintf(stderr, "Error: invalid database connection arguments\n");
      abort();
    }
  }

  /// Log the configuration values
  for (auto &Section : ConfigMap) {
    for (auto &[first, second] : Section.second)
      LOG_DEBUG << Section.first << "." << first << " = " << second;
  }
}

void Application::onRequest(const lynx::HttpRequest &Req,
                            lynx::HttpResponse *Resp) {
  /// Log the request Method and Path
  LOG_INFO << lynx::methodToString(Req.method()) << " " << Req.uri();

  /// Log the request headers
  LOG_DEBUG << "Request headers:";
  for (const auto &Header : Req.headers())
    LOG_DEBUG << Header.first << ": " << Header.second;

  /// Search for a matching route in the route table
  std::string Uri = std::string(Req.uri());
  LOG_DEBUG << "Searching for '" << Uri << "'";
  bool Flag = false;
  auto Method = Req.method();
  for (auto &[pair, Handler] : RouteTable) {
    if (Method == pair.first) {
      std::regex PathRegex(pair.second);
      bool Match = std::regex_match(Uri, PathRegex);
      if (Match) {
        Flag = true;
        LOG_DEBUG << "Found route '" << pair.second << "'";
        // Invoke the Handler for the matching route
        Handler(Req, Resp);
        break;
      }
    }
  }

  /// If no matching route is found, handle it as a not found request
  if (!Flag) {
    LOG_DEBUG << "Not found";
    detail::handleNotFound(Req, Resp);
  }
}

} // namespace lynx

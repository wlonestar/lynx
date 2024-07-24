#ifndef LYNX_WEB_BASE_REST_CONTROLLER_H
#define LYNX_WEB_BASE_REST_CONTROLLER_H

#include "lynx/http/http_request.h"
#include "lynx/http/http_response.h"
#include "lynx/web/web_server.h"

#include <nlohmann/json.hpp>

#include <map>
#include <memory>
#include <utility>

namespace lynx {

using json = nlohmann::json;

void setRespOk(lynx::HttpResponse *resp) {
  resp->setStatusCode(lynx::HttpResponse::Ok200);
  resp->setStatusMessage("OK");
  resp->setContentType("application/json");
  resp->addHeader("Server", "lynx");
}

template <typename T> struct Result {
  int status;          // NOLINT
  std::string message; // NOLINT
  T data;              // NOLINT
};

template <typename T>
Result<T> makeOkResult(const std::string &message, const T &data) {
  return lynx::Result<T>{200, message, data};
}

Result<std::string> makeErrorResult(const std::string &message,
                                    const std::string &hint) {
  return lynx::Result<std::string>{200, message, hint};
}

template <typename T> // NOLINTNEXTLINE
void to_json(json &j, const Result<T> &result) {
  j["status"] = result.status;
  j["message"] = result.message;
  j["data"] = result.data;
}

template <typename T> // NOLINTNEXTLINE
void from_json(const json &j, Result<T> &result) {
  j.at("status").get_to(result.status);
  j.at("message").get_to(result.message);
  j.at("data").get_to(result.data);
}

using HttpHandler = std::function<void(const HttpRequest &, HttpResponse *)>;

class BaseController {
public:
  template <typename Func>
  void requestMapping(const std::string &method, const std::string &path,
                      Func func) {
    route_table_[std::make_pair(method, path)] =
        [func](const lynx::HttpRequest &req, lynx::HttpResponse *resp) {
          setRespOk(resp);
          resp->setBody(func().dump());
        };
  }

  template <typename PathType, typename BodyType, typename Func>
  void requestMapping(const std::string &method, const std::string &path,
                      Func func) {
    route_table_[std::make_pair(method, path)] =
        [func](const lynx::HttpRequest &req, lynx::HttpResponse *resp) {
          auto &path = req.path();
          PathType arg1;
          if constexpr (std::is_same_v<PathType, int64_t> ||
                        std::is_same_v<PathType, uint64_t>) {
            arg1 = atoll(path.substr(path.find_last_of('/') + 1).c_str());
          }
          json j = json::parse(req.body());
          BodyType arg2 = j;
          setRespOk(resp);
          resp->setBody(func(arg1, arg2).dump());
        };
  }

  template <typename PathType, typename Func>
  void requestMappingWithPath(const std::string &method,
                              const std::string &path, Func func) {
    route_table_[std::make_pair(method, path)] =
        [func](const lynx::HttpRequest &req, lynx::HttpResponse *resp) {
          auto &path = req.path();
          PathType arg;
          if constexpr (std::is_same_v<PathType, int64_t> ||
                        std::is_same_v<PathType, uint64_t>) {
            arg = atoll(path.substr(path.find_last_of('/') + 1).c_str());
          }
          setRespOk(resp);
          resp->setBody(func(arg).dump());
        };
  }

  template <typename BodyType, typename Func>
  void requestMappingWithBody(const std::string &method,
                              const std::string &path, Func func) {
    route_table_[std::make_pair(method, path)] =
        [func](const lynx::HttpRequest &req, lynx::HttpResponse *resp) {
          json j = json::parse(req.body());
          BodyType arg = j;
          setRespOk(resp);
          resp->setBody(func(arg).dump());
        };
  }

  void registerHandler(WebServer &server) {
    for (auto &[pair, handler] : route_table_) {
      server.addRoute(pair.first, pair.second, handler);
    }
  }

private:
  std::map<std::pair<std::string, std::string>, HttpHandler> route_table_;
};

} // namespace lynx

#endif

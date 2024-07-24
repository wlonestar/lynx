#ifndef LYNX_WEB_BASE_REST_CONTROLLER_H
#define LYNX_WEB_BASE_REST_CONTROLLER_H

#include "lynx/http/http_request.h"
#include "lynx/http/http_response.h"
#include "lynx/web/web_server.h"

#include <map>
#include <memory>
#include <nlohmann/json.hpp>

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

#define RequestMapping(Controller, _method_, _path_, func)                     \
  void func##Handler(const lynx::HttpRequest &req, lynx::HttpResponse *resp) { \
    setRespOk(resp);                                                           \
    resp->setBody(func().dump());                                              \
  }

#define RequestMappingWithPathVariable(Controller, _method_, _path_, func,     \
                                       path_type)                              \
  void func##Handler(const lynx::HttpRequest &req, lynx::HttpResponse *resp) { \
    auto &p = req.path();                                                      \
    path_type arg;                                                             \
    if constexpr (std::is_same_v<path_type, int64_t> ||                        \
                  std::is_same_v<path_type, uint64_t>) {                       \
      arg = atoll(p.substr(p.find_last_of('/') + 1).c_str());                  \
    }                                                                          \
    setRespOk(resp);                                                           \
    resp->setBody(func(arg).dump());                                           \
  }

#define RequestMappingWithBody(Controller, _method_, _path_, func, body_type)  \
  void func##Handler(const lynx::HttpRequest &req, lynx::HttpResponse *resp) { \
    auto &body = req.body();                                                   \
    lynx::json j = lynx::json::parse(body);                                    \
    body_type arg = j;                                                         \
    setRespOk(resp);                                                           \
    resp->setBody(func(arg).dump());                                           \
  }

#define RequestMappingWithPathVariableAndBody(Controller, _method_, _path_,    \
                                              func, path_type, body_type)      \
  void func##Handler(const lynx::HttpRequest &req, lynx::HttpResponse *resp) { \
    auto &p = req.path();                                                      \
    path_type arg1;                                                            \
    if constexpr (std::is_same_v<path_type, int64_t> ||                        \
                  std::is_same_v<path_type, uint64_t>) {                       \
      arg1 = atoll(p.substr(p.find_last_of('/') + 1).c_str());                 \
    }                                                                          \
    auto &body = req.body();                                                   \
    lynx::json j = lynx::json::parse(body);                                    \
    body_type arg2 = j;                                                        \
    setRespOk(resp);                                                           \
    resp->setBody(func(arg1, arg2).dump());                                    \
  }

#define RestController(Controller)                                             \
  inline std::map<std::pair<std::string, std::string>, lynx::HttpHandler>      \
      Controller##RouteTable;                                                  \
  void Controller##Register(lynx::WebServer &server) {                         \
    for (auto &[pair, handler] : Controller##RouteTable) {                     \
      server.addRoute(pair.first, pair.second, handler);                       \
    }                                                                          \
  }

#define RegisterHandler(Controller, _method_, _path_, func)                    \
  Controller##RouteTable[std::make_pair(_method_, _path_)] =                   \
      [this](auto &&PH1, auto &&PH2) {                                         \
        func##Handler(std::forward<decltype(PH1)>(PH1),                        \
                      std::forward<decltype(PH2)>(PH2));                       \
      };

} // namespace lynx

#endif

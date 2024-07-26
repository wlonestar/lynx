#ifndef LYNX_WEB_BASE_CONTROLLER_H
#define LYNX_WEB_BASE_CONTROLLER_H

#include "lynx/http/http_request.h"
#include "lynx/http/http_response.h"
#include "lynx/logger/logging.h"
#include "lynx/web/web_server.h"

namespace lynx {

void setRespOk(lynx::HttpResponse *resp) {
  resp->setStatusCode(lynx::HttpResponse::Ok200);
  resp->setStatusMessage("OK");
  resp->setContentType("application/json");
  resp->addHeader("Server", "lynx");
}

/**
 * Common Return Type
 */

template <typename T> struct Result {
  int status;          // NOLINT
  std::string message; // NOLINT
  T data;              // NOLINT
};

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

template <typename T>
Result<T> makeOkResult(const std::string &message, const T &data) {
  return lynx::Result<T>{200, message, data};
}

Result<std::string> makeErrorResult(const std::string &message,
                                    const std::string &hint) {
  return lynx::Result<std::string>{200, message, hint};
}

template <typename T> struct RequestBody { RequestBody() = default; };
template <typename T> struct PathVariable { PathVariable() = default; };
template <typename T> struct RequestParam {
  RequestParam(const std::string &name) : name_(name) {}
  std::string name_;
};

/// TODO: Support more type
class BaseController {
public:
  template <typename Func>
  void requestMapping(const std::string &method, const std::string &path,
                      Func &&func) {
    route_table_[std::make_pair(method, path)] =
        [func](const lynx::HttpRequest &req, lynx::HttpResponse *resp) {
          setRespOk(resp);
          resp->setBody(func().dump());
        };
  }

  template <typename PathType, typename Func>
  void requestMapping(const std::string &method, const std::string &path,
                      Func &&func, PathVariable<PathType> /*unused*/) {
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
  void requestMapping(const std::string &method, const std::string &path,
                      Func &&func, RequestBody<BodyType> /*unused*/) {
    route_table_[std::make_pair(method, path)] =
        [func](const lynx::HttpRequest &req, lynx::HttpResponse *resp) {
          LOG_DEBUG << req.body();
          json j = json::parse(req.body());
          LOG_DEBUG << j.dump();
          BodyType arg = j;
          setRespOk(resp);
          resp->setBody(func(arg).dump());
        };
  }

  template <typename PathType, typename BodyType, typename Func>
  void requestMapping(const std::string &method, const std::string &path,
                      Func &&func, PathVariable<PathType> /*unused*/,
                      RequestBody<BodyType> /*unused*/) {
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

  template <typename T>
  T processRequestParam(const MapType &params, const RequestParam<T> &param) {
    if (params.find(param.name_) == params.end()) {
      LOG_FATAL << "error in request params";
    }
    T arg;
    std::string value = params.at(param.name_);
    if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>) {
      arg = atoll(value.c_str());
    }
    return arg;
  }

  template <typename... ParamType, typename Func>
  void requestMapping(const std::string &method, const std::string &path,
                      Func &&func, RequestParam<ParamType>... params) {
    route_table_[std::make_pair(method, path)] =
        [this, func,
         ... params = std::forward<RequestParam<ParamType>>(params)](
            const lynx::HttpRequest &req, lynx::HttpResponse *resp) {
          setRespOk(resp);
          resp->setBody(
              func(processRequestParam<ParamType>(req.params(), params)...)
                  .dump());
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

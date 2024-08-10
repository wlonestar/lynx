#ifndef LYNX_WEB_BASE_CONTROLLER_H
#define LYNX_WEB_BASE_CONTROLLER_H

#include "lynx/logger/logging.h"
#include "lynx/web/web_server.h"

namespace lynx {

template <typename BodyType> struct RequestBody {
  explicit RequestBody() = default;
};

template <typename VarType> struct PathVariable {
  explicit PathVariable() = default;
};

template <typename ParamType> struct RequestParam {
  explicit RequestParam(const std::string &name) : name_(name) {}

  std::string name_;
};

class BaseController {
public:
  BaseController() = default;

  template <typename Func>
  void requestMapping(const std::string &method, const std::string &path,
                      Func &&func);

  template <typename PathType, typename Func>
  void requestMapping(const std::string &method, const std::string &path,
                      Func &&func, PathVariable<PathType> /*unused*/);

  template <typename BodyType, typename Func>
  void requestMapping(const std::string &method, const std::string &path,
                      Func &&func, RequestBody<BodyType> /*unused*/);

  template <typename PathType, typename BodyType, typename Func>
  void requestMapping(const std::string &method, const std::string &path,
                      Func &&func, PathVariable<PathType> /*unused*/,
                      RequestBody<BodyType> /*unused*/);

  template <typename... ParamType, typename Func>
  void requestMapping(const std::string &method, const std::string &path,
                      Func &&func, RequestParam<ParamType>... params);

  void registerHandler(WebServer &server) {
    for (auto &[pair, handler] : route_table_) {
      server.addRoute(pair.first, pair.second, handler);
    }
  }

private:
  template <typename ParamType>
  ParamType processRequestParam(const MapType &params,
                                const RequestParam<ParamType> &param);

  void setRespOk(lynx::HttpResponse *resp) {
    resp->setStatusCode(lynx::HttpResponse::Ok200);
    resp->setStatusMessage("OK");
    resp->setContentType("application/json");
    resp->addHeader("Server", "lynx");
  }

  std::map<std::pair<std::string, std::string>, HttpHandler> route_table_;
};

template <typename Func>
void BaseController::requestMapping(const std::string &method,
                                    const std::string &path, Func &&func) {
  auto handler = [&, func](const lynx::HttpRequest &req,
                           lynx::HttpResponse *resp) {
    setRespOk(resp);
    resp->setBody(func().dump());
  };
  route_table_[std::make_pair(method, path)] = handler;
}

template <typename PathType, typename Func>
void BaseController::requestMapping(const std::string &method,
                                    const std::string &path, Func &&func,
                                    PathVariable<PathType> /*unused*/) {
  auto handler = [&, func](const lynx::HttpRequest &req,
                           lynx::HttpResponse *resp) {
    auto &path = req.path();
    PathType arg;
    if constexpr (std::is_same_v<PathType, int64_t> ||
                  std::is_same_v<PathType, uint64_t>) {
      arg = atoll(path.substr(path.find_last_of('/') + 1).c_str());
    }
    setRespOk(resp);
    resp->setBody(func(arg).dump());
  };
  route_table_[std::make_pair(method, path)] = handler;
}

template <typename BodyType, typename Func>
void BaseController::requestMapping(const std::string &method,
                                    const std::string &path, Func &&func,
                                    RequestBody<BodyType> /*unused*/) {
  auto handler = [&, func](const lynx::HttpRequest &req,
                           lynx::HttpResponse *resp) {
    json j = json::parse(req.body());
    BodyType arg = j;
    setRespOk(resp);
    resp->setBody(func(arg).dump());
  };
  route_table_[std::make_pair(method, path)] = handler;
}

template <typename PathType, typename BodyType, typename Func>
void BaseController::requestMapping(const std::string &method,
                                    const std::string &path, Func &&func,
                                    PathVariable<PathType> /*unused*/,
                                    RequestBody<BodyType> /*unused*/) {
  auto handler = [&, func](const lynx::HttpRequest &req,
                           lynx::HttpResponse *resp) {
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
  route_table_[std::make_pair(method, path)] = handler;
}

template <typename ParamType>
ParamType
BaseController::processRequestParam(const MapType &params,
                                    const RequestParam<ParamType> &param) {
  if (params.find(param.name_) == params.end()) {
    LOG_FATAL << "error in request params";
  }
  ParamType arg;
  std::string value = params.at(param.name_);
  if constexpr (std::is_same_v<ParamType, int64_t> ||
                std::is_same_v<ParamType, uint64_t>) {
    arg = atoll(value.c_str());
  }
  return arg;
}

template <typename... ParamType, typename Func>
void BaseController::requestMapping(const std::string &method,
                                    const std::string &path, Func &&func,
                                    RequestParam<ParamType>... params) {
  auto handler = [&, func,
                  ... params = std::forward<RequestParam<ParamType>>(params)](
                     const lynx::HttpRequest &req, lynx::HttpResponse *resp) {
    setRespOk(resp);
    resp->setBody(
        func(processRequestParam<ParamType>(req.params(), params)...).dump());
  };
  route_table_[std::make_pair(method, path)] = handler;
}

} // namespace lynx

#endif

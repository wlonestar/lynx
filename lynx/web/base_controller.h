#ifndef LYNX_WEB_BASE_CONTROLLER_H
#define LYNX_WEB_BASE_CONTROLLER_H

#include "lynx/app/application.h"
#include "lynx/logger/logging.h"

namespace lynx {

/**
 * @struct RequestBody
 * @brief A template struct that represents a request body with a specific type.
 *
 * This struct is used to represent a request body with a specific type.
 * It is typically used in conjunction with the requestMapping function in
 * the BaseController class.
 *
 * @tparam BodyType the type of the request body
 */
template <typename BodyType> struct RequestBody {
  explicit RequestBody() = default;
};

/**
 * @struct PathVariable
 * @brief A template struct that represents a path variable with a specific
 * type.
 *
 * This struct is used to represent a path variable with a specific type.
 * It is typically used in conjunction with the requestMapping function in
 * the BaseController class.
 *
 * @tparam VarType the type of the path variable
 */
template <typename VarType> struct PathVariable {
  explicit PathVariable() = default;
};

/**
 * @struct RequestParam
 * @brief A template struct that represents a request parameter with a specific
 * type.
 *
 * This struct is used to represent a request parameter with a specific type.
 * It is typically used in conjunction with the requestMapping function in
 * the BaseController class.
 *
 * @tparam ParamType the type of the request parameter
 */
template <typename ParamType> struct RequestParam {
  explicit RequestParam(const std::string &name) : name_(name) {}

  std::string name_;
};

/**
 * @class BaseController
 * @brief Base class for HTTP request handlers.
 *
 * This class serves as a base for implementing HTTP request handlers. It
 * provides a convenient way to map HTTP methods to handler functions.
 */
class BaseController {
public:
  /// Default constructor.
  BaseController() = default;

  /**
   * @brief Maps an HTTP request to a handler function.
   *
   * @tparam Func The type of the handler function.
   * @param method The HTTP method of the request.
   * @param path The path of the request.
   * @param func The handler function.
   */
  template <typename Func>
  void requestMapping(const std::string &method, const std::string &path,
                      Func &&func);

  /**
   * @brief Maps an HTTP request to a handler function with a path variable.
   *
   * @tparam PathType The type of the path variable.
   * @tparam Func The type of the handler function.
   * @param method The HTTP method of the request.
   * @param path The path of the request.
   * @param func The handler function.
   * @param unused Unused parameter.
   */
  template <typename PathType, typename Func>
  void requestMapping(const std::string &method, const std::string &path,
                      Func &&func, PathVariable<PathType> /*unused*/);

  /**
   * @brief Maps an HTTP request to a handler function with a request body.
   *
   * @tparam BodyType The type of the request body.
   * @tparam Func The type of the handler function.
   * @param method The HTTP method of the request.
   * @param path The path of the request.
   * @param func The handler function.
   * @param unused Unused parameter.
   */
  template <typename BodyType, typename Func>
  void requestMapping(const std::string &method, const std::string &path,
                      Func &&func, RequestBody<BodyType> /*unused*/);

  /**
   * @brief Maps an HTTP request to a handler function with both a path variable
   *        and a request body.
   *
   * @tparam PathType The type of the path variable.
   * @tparam BodyType The type of the request body.
   * @tparam Func The type of the handler function.
   * @param method The HTTP method of the request.
   * @param path The path of the request.
   * @param func The handler function.
   * @param unused1 Unused parameter.
   * @param unused2 Unused parameter.
   */
  template <typename PathType, typename BodyType, typename Func>
  void requestMapping(const std::string &method, const std::string &path,
                      Func &&func, PathVariable<PathType> /*unused1*/,
                      RequestBody<BodyType> /*unused2*/);

  /**
   * @brief Maps an HTTP request to a handler function with request parameters.
   *
   * @tparam ParamType The type of the request parameters.
   * @tparam Func The type of the handler function.
   * @param method The HTTP method of the request.
   * @param path The path of the request.
   * @param func The handler function.
   * @param params The request parameters.
   */
  template <typename... ParamType, typename Func>
  void requestMapping(const std::string &method, const std::string &path,
                      Func &&func, RequestParam<ParamType>... params);

  /**
   * @brief Registers the HTTP routes with the application.
   *
   * @param app The application to register the routes with.
   */
  void registerHandler(Application &app) {
    for (auto &[pair, handler] : route_table_) {
      app.addRoute(pair.first, pair.second, handler);
    }
  }

private:
  /**
   * @brief Processes a request parameter.
   *
   * @tparam ParamType The type of the request parameter.
   * @param params The map of request parameters.
   * @param param The request parameter to process.
   * @return The processed request parameter.
   */
  template <typename ParamType>
  ParamType processRequestParam(const HttpRequest::MapType &params,
                                const RequestParam<ParamType> &param);

  /**
   * @brief Sets the response to OK with JSON content type.
   *
   * @param resp The HTTP response object.
   */
  void setRespOk(lynx::HttpResponse *resp) {
    resp->setStatusCode(lynx::HttpStatus::OK);
    resp->setContentType("application/json");
    resp->addHeader("Server", "lynx");
  }

  /// The map of HTTP routes.
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
BaseController::processRequestParam(const HttpRequest::MapType &params,
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

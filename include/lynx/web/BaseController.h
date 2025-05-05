#ifndef LYNX_WEB_BASE_CONTROLLER_H
#define LYNX_WEB_BASE_CONTROLLER_H

#include "lynx/app/Application.h"
#include "lynx/logger/Logging.h"

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
 * @brief A template struct that represents a Path variable with a specific
 * type.
 *
 * This struct is used to represent a Path variable with a specific type.
 * It is typically used in conjunction with the requestMapping function in
 * the BaseController class.
 *
 * @tparam VarType the type of the Path variable
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
  explicit RequestParam(const std::string &Name) : Name(Name) {}

  std::string Name;
};

/**
 * @class BaseController
 * @brief Base class for HTTP request handlers.
 *
 * This class serves as a base for implementing HTTP request handlers. It
 * provides a convenient way to map HTTP methods to Handler functions.
 */
class BaseController {
public:
  BaseController() = default;

  /**
   * @brief Maps an HTTP request to a Handler function.
   *
   * @tparam Fn The type of the Handler function.
   * @Param Method The HTTP Method of the request.
   * @Param Path The Path of the request.
   * @Param Func The Handler function.
   */
  template <typename Fn>
  void requestMapping(const std::string &Method, const std::string &Path,
                      Fn &&Func);

  /**
   * @brief Maps an HTTP request to a Handler function with a Path variable.
   *
   * @tparam PathType The type of the Path variable.
   * @tparam Fn The type of the Handler function.
   * @Param Method The HTTP Method of the request.
   * @Param Path The Path of the request.
   * @Param Func The Handler function.
   * @Param unused Unused parameter.
   */
  template <typename PathType, typename Fn>
  void requestMapping(const std::string &Method, const std::string &Path,
                      Fn &&Func, PathVariable<PathType> /*unused*/);

  /**
   * @brief Maps an HTTP request to a Handler function with a request body.
   *
   * @tparam BodyType The type of the request body.
   * @tparam Fn The type of the Handler function.
   * @Param Method The HTTP Method of the request.
   * @Param Path The Path of the request.
   * @Param Func The Handler function.
   * @Param unused Unused parameter.
   */
  template <typename BodyType, typename Fn>
  void requestMapping(const std::string &Method, const std::string &Path,
                      Fn &&Func, RequestBody<BodyType> /*unused*/);

  /**
   * @brief Maps an HTTP request to a Handler function with both a Path variable
   *        and a request body.
   *
   * @tparam PathType The type of the Path variable.
   * @tparam BodyType The type of the request body.
   * @tparam Fn The type of the Handler function.
   * @Param Method The HTTP Method of the request.
   * @Param Path The Path of the request.
   * @Param Func The Handler function.
   * @Param unused1 Unused parameter.
   * @Param unused2 Unused parameter.
   */
  template <typename PathType, typename BodyType, typename Fn>
  void requestMapping(const std::string &Method, const std::string &Path,
                      Fn &&Func, PathVariable<PathType> /*unused1*/,
                      RequestBody<BodyType> /*unused2*/);

  /**
   * @brief Maps an HTTP request to a Handler function with request parameters.
   *
   * @tparam ParamType The type of the request parameters.
   * @tparam Fn The type of the Handler function.
   * @Param Method The HTTP Method of the request.
   * @Param Path The Path of the request.
   * @Param Func The Handler function.
   * @Param Params The request parameters.
   */
  template <typename... ParamType, typename Fn>
  void requestMapping(const std::string &Method, const std::string &Path,
                      Fn &&Func, RequestParam<ParamType>... Params);

  /**
   * @brief Registers the HTTP routes with the application.
   *
   * @Param App The application to register the routes with.
   */
  void registerHandler(Application &App) {
    for (auto &[pair, Handler] : RouteTable)
      App.addRoute(pair.first, pair.second, Handler);
  }

private:
  /**
   * @brief Processes a request parameter.
   *
   * @tparam ParamType The type of the request parameter.
   * @Param Params The map of request parameters.
   * @Param Param The request parameter to process.
   * @return The processed request parameter.
   */
  template <typename ParamType>
  ParamType processRequestParam(const HttpRequest::MapType &Params,
                                const RequestParam<ParamType> &Param);

  /**
   * @brief Sets the response to OK with JSON content type.
   *
   * @Param Resp The HTTP response object.
   */
  void setRespOk(lynx::HttpResponse *Resp) {
    Resp->setStatusCode(lynx::HttpStatus::OK);
    Resp->setContentType("application/json");
    Resp->addHeader("Server", "lynx");
  }

  /// The map of HTTP routes.
  std::map<std::pair<std::string, std::string>, HttpHandler> RouteTable;
};

template <typename Fn>
void BaseController::requestMapping(const std::string &Method,
                                    const std::string &Path, Fn &&Func) {
  auto Handler = [&, Func](const lynx::HttpRequest &Req,
                           lynx::HttpResponse *Resp) {
    setRespOk(Resp);
    Resp->setBody(Func().dump());
  };
  RouteTable[std::make_pair(Method, Path)] = Handler;
}

template <typename PathType, typename Fn>
void BaseController::requestMapping(const std::string &Method,
                                    const std::string &Path, Fn &&Func,
                                    PathVariable<PathType> /*unused*/) {
  auto Handler = [&, Func](const lynx::HttpRequest &Req,
                           lynx::HttpResponse *Resp) {
    auto &Path = Req.path();
    PathType Arg;
    if constexpr (std::is_same_v<PathType, int64_t> ||
                  std::is_same_v<PathType, uint64_t>)
      Arg = atoll(Path.substr(Path.find_last_of('/') + 1).c_str());

    setRespOk(Resp);
    Resp->setBody(Func(Arg).dump());
  };
  RouteTable[std::make_pair(Method, Path)] = Handler;
}

template <typename BodyType, typename Fn>
void BaseController::requestMapping(const std::string &Method,
                                    const std::string &Path, Fn &&Func,
                                    RequestBody<BodyType> /*unused*/) {
  auto Handler = [&, Func](const lynx::HttpRequest &Req,
                           lynx::HttpResponse *Resp) {
    json J = json::parse(Req.body());
    BodyType Arg = J;
    setRespOk(Resp);
    Resp->setBody(Func(Arg).dump());
  };
  RouteTable[std::make_pair(Method, Path)] = Handler;
}

template <typename PathType, typename BodyType, typename Fn>
void BaseController::requestMapping(const std::string &Method,
                                    const std::string &Path, Fn &&Func,
                                    PathVariable<PathType> /*unused*/,
                                    RequestBody<BodyType> /*unused*/) {
  auto Handler = [&, Func](const lynx::HttpRequest &Req,
                           lynx::HttpResponse *Resp) {
    auto &Path = Req.path();
    PathType Arg1;
    if constexpr (std::is_same_v<PathType, int64_t> ||
                  std::is_same_v<PathType, uint64_t>)
      Arg1 = atoll(Path.substr(Path.find_last_of('/') + 1).c_str());

    json J = json::parse(Req.body());
    BodyType Arg2 = J;

    setRespOk(Resp);
    Resp->setBody(Func(Arg1, Arg2).dump());
  };
  RouteTable[std::make_pair(Method, Path)] = Handler;
}

template <typename ParamType>
ParamType
BaseController::processRequestParam(const HttpRequest::MapType &Params,
                                    const RequestParam<ParamType> &Param) {
  if (Params.find(Param.Name) == Params.end())
    LOG_FATAL << "error in request Params";

  ParamType Arg;
  std::string Value = Params.at(Param.Name);
  if constexpr (std::is_same_v<ParamType, int64_t> ||
                std::is_same_v<ParamType, uint64_t>)
    Arg = atoll(Value.c_str());

  return Arg;
}

template <typename... ParamType, typename Fn>
void BaseController::requestMapping(const std::string &Method,
                                    const std::string &Path, Fn &&Func,
                                    RequestParam<ParamType>... Params) {
  auto Handler = [&, Func,
                  ... Params = std::forward<RequestParam<ParamType>>(Params)](
                     const lynx::HttpRequest &Req, lynx::HttpResponse *Resp) {
    setRespOk(Resp);
    Resp->setBody(
        Func(processRequestParam<ParamType>(Req.params(), Params)...).dump());
  };
  RouteTable[std::make_pair(Method, Path)] = Handler;
}

} // namespace lynx

#endif

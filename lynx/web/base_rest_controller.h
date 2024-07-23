#ifndef LYNX_WEB_BASE_REST_CONTROLLER_H
#define LYNX_WEB_BASE_REST_CONTROLLER_H

#include "lynx/http/http_request.h"
#include "lynx/http/http_response.h"
#include "lynx/web/web_server.h"

#include <memory>

void setRespOk(lynx::HttpResponse *resp) {
  resp->setStatusCode(lynx::HttpResponse::Ok200);
  resp->setStatusMessage("OK");
  resp->setContentType("application/json");
  resp->addHeader("Server", "lynx");
}

namespace lynx {

template <typename Derived> class BaseController {
public:
  virtual void registr(WebServer &server, Derived &d) { d.assign(server, d); }
};

} // namespace lynx

#endif

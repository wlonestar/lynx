#ifndef LYNX_WEB_BASE_REST_CONTROLLER_H
#define LYNX_WEB_BASE_REST_CONTROLLER_H

#include "lynx/http/http_request.h"
#include "lynx/http/http_response.h"

#include <memory>

void setRespOk(lynx::HttpResponse *resp) {
  resp->setStatusCode(lynx::HttpResponse::Ok200);
  resp->setStatusMessage("OK");
  resp->setContentType("application/json");
  resp->addHeader("Server", "lynx");
}

#endif

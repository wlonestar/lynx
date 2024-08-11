#include "lynx/http/http_status.h"

namespace lynx {

const char *statusToString(const HttpStatus &s) {
  switch (s) {
#define XX(code, name, msg)                                                    \
  case HttpStatus::name:                                                       \
    return #msg;
    HTTP_STATUS_MAP(XX)
#undef XX
  default:
    return "unknown";
  }
}

} // namespace lynx

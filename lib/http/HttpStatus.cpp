#include "lynx/http/HttpStatus.h"

namespace lynx {

const char *statusToString(const HttpStatus &S) {
  switch (S) {
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

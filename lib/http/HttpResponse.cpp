#include "lynx/http/HttpResponse.h"
#include "lynx/net/Buffer.h"

#include <cstdio>

namespace lynx {

void HttpResponse::appendToBuffer(Buffer *Output) const {
  char Buf[32];
  snprintf(Buf, sizeof(Buf), "HTTP/1.1 %d ", Status);
  Output->append(Buf);
  Output->append(statusToString(Status));
  Output->append("\r\n");

  if (CloseConnection) {
    Output->append("Connection: close\r\n");
  } else {
    snprintf(Buf, sizeof(Buf), "Content-Length: %zd\r\n", Body.size());
    Output->append(Buf);
    Output->append("Connection: Keep-Alive\r\n");
  }

  for (const auto &Header : Headers) {
    Output->append(Header.first);
    Output->append(": ");
    Output->append(Header.second);
    Output->append("\r\n");
  }

  Output->append("\r\n");
  Output->append(Body);
}

} // namespace lynx

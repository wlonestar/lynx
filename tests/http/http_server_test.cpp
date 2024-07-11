#include "lynx/http/http_request.h"
#include "lynx/http/http_response.h"
#include "lynx/http/http_server.h"
#include "lynx/logger/logging.h"
#include "lynx/net/event_loop.h"

#include <iostream>
#include <map>

extern unsigned char favicon_jpg[];
extern unsigned int favicon_jpg_len;

bool benchmark = false;

void onRequest(const lynx::HttpRequest &req, lynx::HttpResponse *resp) {
  std::cout << "Headers " << req.methodString() << " " << req.path()
            << std::endl;
  if (!benchmark) {
    const std::map<std::string, std::string> &headers = req.headers();
    for (const auto &header : headers) {
      std::cout << header.first << ": " << header.second << std::endl;
    }
  }

  if (req.path() == "/") {
    resp->setStatusCode(lynx::HttpResponse::Ok200);
    resp->setStatusMessage("OK");
    resp->setContentType("text/html");
    resp->addHeader("Server", "lynx");
    std::string now = lynx::Timestamp::now().toFormattedString();
    resp->setBody("<html><head><title>This is title</title></head>"
                  "<body><h1>Hello</h1>Now is " +
                  now + "</body></html>");
  } else if (req.path() == "/favicon.ico") {
    resp->setStatusCode(lynx::HttpResponse::Ok200);
    resp->setStatusMessage("OK");
    resp->setContentType("image/png");
    resp->setBody(
        std::string(reinterpret_cast<char *>(favicon_jpg), favicon_jpg_len));
  } else if (req.path() == "/hello") {
    resp->setStatusCode(lynx::HttpResponse::Ok200);
    resp->setStatusMessage("OK");
    resp->setContentType("text/plain");
    resp->addHeader("Server", "lynx");
    resp->setBody("hello, world!\n");
  } else {
    resp->setStatusCode(lynx::HttpResponse::NotFound404);
    resp->setStatusMessage("Not Found");
    resp->setCloseConnection(true);
  }
}

int main(int argc, char *argv[]) {
  int num_threads = 0;
  if (argc > 1) {
    benchmark = true;
    lynx::Logger::setLogLevel(lynx::WARN);
    num_threads = atoi(argv[1]);
  }
  lynx::EventLoop loop;
  lynx::HttpServer server(&loop, lynx::InetAddress(8000), "dummy");
  server.setHttpCallback(onRequest);
  server.setThreadNum(num_threads);
  server.start();
  loop.loop();
}

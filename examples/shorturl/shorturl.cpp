#include "lynx/http/http_request.h"
#include "lynx/http/http_response.h"
#include "lynx/http/http_server.h"
#include "lynx/logger/logging.h"
#include "lynx/net/event_loop.h"
#include "lynx/net/event_loop_thread_pool.h"
#include "lynx/net/inet_address.h"
#include "lynx/net/tcp_server.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

bool benchmark = false;

extern unsigned char favicon_jpg[];
extern unsigned int favicon_jpg_len;

std::map<std::string, std::string> redirections;

void onRequest(const lynx::HttpRequest &req, lynx::HttpResponse *resp) {
  LOG_INFO << "Headers " << lynx::methodToString(req.method()) << " "
           << req.path();
  if (!benchmark) {
    const auto &headers = req.headers();
    for (const auto &header : headers) {
      LOG_DEBUG << header.first << ": " << header.second;
    }
  }

  auto it = redirections.find(req.path());
  if (it != redirections.end()) {
    resp->setStatusCode(lynx::HttpResponse::MovedPermanently301);
    resp->setStatusMessage("Moved Permanently");
    resp->addHeader("Location", it->second);
    // resp->setCloseConnection(true);
  } else if (req.path() == "/") {
    resp->setStatusCode(lynx::HttpResponse::Ok200);
    resp->setStatusMessage("OK");
    resp->setContentType("text/html");
    std::string now = lynx::Timestamp::now().toFormattedString();
    auto i = redirections.begin();
    std::string text;
    for (; i != redirections.end(); ++i) {
      text.append("<ul>" + i->first + " =&gt; " + i->second + "</ul>");
    }

    resp->setBody("<html><head><title>My tiny short url service</title></head>"
                  "<body><h1>Known redirections</h1>" +
                  text + "Now is " + now + "</body></html>");
  } else if (req.path() == "/favicon.ico") {
    resp->setStatusCode(lynx::HttpResponse::Ok200);
    resp->setStatusMessage("OK");
    resp->setContentType("image/png");
    resp->setBody(
        std::string(reinterpret_cast<char *>(favicon_jpg), favicon_jpg_len));
  } else {
    resp->setStatusCode(lynx::HttpResponse::NotFound404);
    resp->setStatusMessage("Not Found");
    resp->setCloseConnection(true);
  }
}

int main(int argc, char *argv[]) {
  redirections["/1"] = "https://wangjialei.xyz";
  redirections["/2"] = "https://github.com/wlonestar";

  int num_threads = 0;
  if (argc > 1) {
    benchmark = true;
    lynx::Logger::setLogLevel(lynx::Logger::WARN);
    num_threads = atoi(argv[1]);
  }

  lynx::EventLoop loop;
  lynx::EventLoopThreadPool thread_pool(&loop, "shorturl");
  if (num_threads > 1) {
    thread_pool.setThreadNum(num_threads);
  } else {
    num_threads = 1;
  }
  thread_pool.start();

  std::vector<std::unique_ptr<lynx::HttpServer>> servers;
  for (int i = 0; i < num_threads; i++) {
    servers.emplace_back(
        new lynx::HttpServer(thread_pool.getNextLoop(), lynx::InetAddress(8000),
                             "shorturl", lynx::TcpServer::REUSE_PORT));
    servers.back()->setHttpCallback(onRequest);
    servers.back()->getLoop()->runInLoop(
        [capture = servers.back().get()] { capture->start(); });
  }
  loop.loop();
}

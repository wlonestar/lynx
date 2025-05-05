#include "lynx/app/Application.h"
#include "lynx/logger/AsyncLogging.h"

extern unsigned char FaviconJpg[];
extern unsigned int FaviconJpgLen;

void handleIndex(const lynx::HttpRequest &Req, lynx::HttpResponse *Resp) {
  Resp->setStatusCode(lynx::HttpStatus::OK);
  Resp->setContentType("text/html");
  Resp->addHeader("Server", "lynx");
  std::string Now = lynx::Timestamp::now().toFormattedString(true);
  Resp->setBody("<html><head><title>This is title</title></head>"
                "<body><h1>Hello</h1>Now is " +
                Now + "</body></html>");
}

void handleFavicon(const lynx::HttpRequest &Req, lynx::HttpResponse *Resp) {
  Resp->setStatusCode(lynx::HttpStatus::OK);
  Resp->setContentType("image/png");
  Resp->setBody(
      std::string(reinterpret_cast<char *>(FaviconJpg), FaviconJpgLen));
}

int main(int argc, char *argv[]) {
  /// Init Async logger.
  off_t RollSize = 500 * 1000 * 1000;
  char Name[256] = {'\0'};
  strncpy(Name, argv[0], sizeof(Name) - 1);
  lynx::AsyncLogging Log(::basename(Name), RollSize);
  Log.start();
  lynx::Logger::setOutput(
      [&](const char *Msg, int Len) { Log.append(Msg, Len); });

  /// Create app by reading from config file.
  lynx::Application App("simple_config_1.yml");
  /// Init app.
  App.start();

  /// Add route.
  App.addRoute("GET", "/", handleIndex);
  App.addRoute("GET", "/favicon.ico", handleFavicon);
  App.addRoute("GET", "/hello\\?name=(\\w+)",
               [](const lynx::HttpRequest &Req, lynx::HttpResponse *Resp) {
                 auto Name = Req.getParam("name");

                 Resp->setStatusCode(lynx::HttpStatus::OK);
                 Resp->setContentType("text/html");
                 Resp->addHeader("Server", "lynx");
                 std::string Now =
                     lynx::Timestamp::now().toFormattedString(true);
                 Resp->setBody("<h1>Hello " + Name + "!</h1>");
               });

  /// Start listening.
  App.listen();
}

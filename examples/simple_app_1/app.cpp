#include "lynx/app/application.h"

extern unsigned char favicon_jpg[];
extern unsigned int favicon_jpg_len;

void handleIndex(const lynx::HttpRequest &req, lynx::HttpResponse *resp) {
  resp->setStatusCode(lynx::HttpStatus::OK);
  resp->setContentType("text/html");
  resp->addHeader("Server", "lynx");
  std::string now = lynx::Timestamp::now().toFormattedString(true);
  resp->setBody("<html><head><title>This is title</title></head>"
                "<body><h1>Hello</h1>Now is " +
                now + "</body></html>");
}

void handleFavicon(const lynx::HttpRequest &req, lynx::HttpResponse *resp) {
  resp->setStatusCode(lynx::HttpStatus::OK);
  resp->setContentType("image/png");
  resp->setBody(
      std::string(reinterpret_cast<char *>(favicon_jpg), favicon_jpg_len));
}

int main() {
  /// Create app by reading from config file.
  lynx::Application app("simple_config_1.yml");
  /// Init app.
  app.start();

  /// Add route
  app.addRoute("GET", "/", handleIndex);
  app.addRoute("GET", "/favicon.ico", handleFavicon);
  app.addRoute("GET", "/hello\\?name=(\\w+)",
               [](const lynx::HttpRequest &req, lynx::HttpResponse *resp) {
                 auto name = req.getParam("name");

                 resp->setStatusCode(lynx::HttpStatus::OK);
                 resp->setContentType("text/html");
                 resp->addHeader("Server", "lynx");
                 std::string now =
                     lynx::Timestamp::now().toFormattedString(true);
                 resp->setBody("<h1>Hello " + name + "!</h1>");
               });

  /// Start listening.
  app.listen();
}

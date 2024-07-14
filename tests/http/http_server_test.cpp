#include "lynx/db/pg_connection_pool.h"
#include "lynx/http/http_request.h"
#include "lynx/http/http_response.h"
#include "lynx/http/http_server.h"
#include "lynx/logger/logging.h"
#include "lynx/net/event_loop.h"

#include <iostream>
#include <map>
#include <sstream>

extern unsigned char favicon_jpg[];
extern unsigned int favicon_jpg_len;

enum Gender : int {
  Male,
  Female,
};

struct Person {
  short id;      // NOLINT
  char name[10]; // NOLINT
  Gender gender; // NOLINT
  int age;       // NOLINT
  float score;   // NOLINT
} __attribute__((packed));

REFLECTION_TEMPLATE_WITH_NAME(Person, "person", id, name, gender, age, score)

bool benchmark = false;
lynx::PgConnectionPool *g_pool = nullptr;

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

    auto conn = g_pool->acquire();
    auto pn1 = conn->query<Person>()
                   .where(VALUE(Person::age) > 27 && VALUE(Person::id) < 3)
                   .limit(2)
                   .toVector();
    g_pool->release(conn);

    std::stringstream ss;
    for (auto it : pn1) {
      ss << it.id << " " << it.name << " " << it.gender << " " << it.age << " "
         << it.score << std::endl;
    }
    std::string body = ss.str() + "\n";

    resp->setBody(body);
  } else {
    resp->setStatusCode(lynx::HttpResponse::NotFound404);
    resp->setStatusMessage("Not Found");
    resp->setCloseConnection(true);
  }
}

void initDB(lynx::PgConnectionPool &pool) {
  // connect database
  auto conn = pool.acquire();

  // delete table
  conn->execute("drop table person;");
  // create table
  lynx::KeyMap key_map{"id"};
  lynx::NotNullMap not_null_map;
  not_null_map.fields = {"id", "age"};
  conn->createTable<Person>(key_map, not_null_map);

  // insert data
  Person p1{1, "wjl1", Gender::Female, 30, 101.1F};
  Person p2{2, "wjl2", Gender::Female, 28, 102.2F};
  Person p3{3, "wjl3", Gender::Male, 27, 103.3F};
  Person p4{4, "wjl4", Gender::Female, 26, 104.4F};
  Person p5{5, "wjl1", Gender::Male, 30, 108.1F};
  Person p6{6, "wjl3", Gender::Female, 30, 109.1F};

  conn->insert(p1);
  conn->insert(p2);
  conn->insert(p3);
  conn->insert(p4);
  conn->insert(p5);
  conn->insert(p6);

  pool.release(conn);
}

int main(int argc, char *argv[]) {
  int num_threads = 0;
  if (argc > 1) {
    benchmark = true;
    lynx::Logger::setLogLevel(lynx::Logger::WARN);
    num_threads = atoi(argv[1]);
  }

  lynx::EventLoop loop;
  lynx::HttpServer server(&loop, lynx::InetAddress(8000), "dummy");
  server.setHttpCallback(onRequest);
  server.setThreadNum(num_threads);

  lynx::PgConnectionPool pool("127.0.0.1", "5432", "postgres", "123456",
                              "demo");
  pool.start();
  g_pool = &pool;

  initDB(pool);

  server.start();
  loop.loop();
}

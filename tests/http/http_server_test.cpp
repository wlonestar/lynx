#include "lynx/logger/async_logging.h"
#include "lynx/logger/logging.h"
#include "student.h"

#include "lynx/db/pg_connection_pool.h"
#include "lynx/http/http_request.h"
#include "lynx/http/http_response.h"
#include "lynx/http/http_server.h"
#include "lynx/net/event_loop.h"

#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <regex>
#include <sstream>
#include <utility>

extern unsigned char favicon_jpg[];
extern unsigned int favicon_jpg_len;

off_t roll_size = 500 * 1000 * 1000;
lynx::AsyncLogging *g_async_log = nullptr;

void asyncOutput(const char *msg, int len) { g_async_log->append(msg, len); }

void handleIndex(const lynx::HttpRequest &req, lynx::HttpResponse *resp);
void handleFavicon(const lynx::HttpRequest &req, lynx::HttpResponse *resp);
void handleNotFound(const lynx::HttpRequest &req, lynx::HttpResponse *resp);
void setupRoutes();

void initDB(lynx::PgConnectionPool &pool);
void onRequest(const lynx::HttpRequest &req, lynx::HttpResponse *resp);

int main(int argc, char *argv[]) {
  char name[256] = {'\0'};
  strncpy(name, argv[0], sizeof(name) - 1);
  lynx::AsyncLogging log(::basename(name), roll_size);
  lynx::Logger::setOutput(asyncOutput);
  log.start();
  g_async_log = &log;

  int num_threads = 5;
  if (argc > 1) {
    num_threads = atoi(argv[1]);
  }
  lynx::EventLoop loop;
  lynx::HttpServer server(&loop, lynx::InetAddress(8000), "dummy");
  server.setHttpCallback(onRequest);
  server.setThreadNum(num_threads);
  LOG_INFO << "start HTTP server with " << num_threads << " threads";

  lynx::PgConnectionPool pool("127.0.0.1", "5432", "postgres", "123456",
                              "demo");
  pool.start();

  initDB(pool);
  setupRoutes();

  StudentRepository repository(pool);
  StudentService service(repository);
  g_student_controller = std::make_shared<StudentController>(service);

  server.start();
  loop.loop();
}

void initDB(lynx::PgConnectionPool &pool) {
  /// Connect database
  lynx::PgConnection conn("PgConnection");
  conn.connect("localhost", "5432", "postgres", "123456", "demo");

  /// Create table (drop if table already exists)
  conn.execute("drop table student; drop sequence student_id_seq;");
  lynx::AutoKeyMap key_map{"id"};
  lynx::NotNullMap not_null_map;
  not_null_map.fields = {"id", "name", "gender", "entry_year"};
  bool flag = conn.createTable<Student>(key_map, not_null_map);
  if (!flag) {
    abort();
  }

  /// Insert 1: insert one
  Student s1{2024033001, "Li Ming 1", Gender::Male, 2024, "SE", 3.8};
  Student s2{2024033002, "Li Ming 2", Gender::Male, 2024, "SE", 3.82};
  Student s3{2024033003, "Li Ming 3", Gender::Female, 2024, "CS", 3.78};
  Student s4{2023033024, "Liu Hua 1", Gender::Male, 2023, "SE", 3.98};
  Student s5{2024033035, "Liu Hua 2", Gender::Female, 2023, "CS", 3.89};
  conn.insert(s1);
  conn.insert(s2);
  conn.insert(s3);
  conn.insert(s4);
  conn.insert(s5);

  /// Insert 2: insert many
  std::vector<Student> students;
  for (int i = 0; i < 10; i++) {
    Student s;
    s.id = 2023033001 + i;
    s.name = "Che hen " + std::to_string(i);
    s.gender = rand() % 2 == 0 ? Gender::Female : Gender::Male;
    s.entry_year = 2023;
    s.major = rand() % 2 == 0 ? "CS" : "SE";
    s.gpa = 3.5 + (rand() % 10) * 0.05;
    students.push_back(s);
  }
  conn.insert(students);
}

void setupRoutes() {
  lynx::addRoute("GET", "/", handleIndex);
  lynx::addRoute("GET", "/favicon.ico", handleFavicon);
}

void handleIndex(const lynx::HttpRequest &req, lynx::HttpResponse *resp) {
  resp->setStatusCode(lynx::HttpResponse::Ok200);
  resp->setStatusMessage("OK");
  resp->setContentType("text/html");
  resp->addHeader("Server", "lynx");
  std::string now = lynx::Timestamp::now().toFormattedString();
  resp->setBody("<html><head><title>This is title</title></head>"
                "<body><h1>Hello</h1>Now is " +
                now + "</body></html>");
}

void handleFavicon(const lynx::HttpRequest &req, lynx::HttpResponse *resp) {
  resp->setStatusCode(lynx::HttpResponse::Ok200);
  resp->setStatusMessage("OK");
  resp->setContentType("image/png");
  resp->setBody(
      std::string(reinterpret_cast<char *>(favicon_jpg), favicon_jpg_len));
}

void handleNotFound(const lynx::HttpRequest &req, lynx::HttpResponse *resp) {
  resp->setStatusCode(lynx::HttpResponse::NotFound404);
  resp->setStatusMessage("Not Found");
  resp->setCloseConnection(true);
}

void onRequest(const lynx::HttpRequest &req, lynx::HttpResponse *resp) {
  LOG_INFO << lynx::methodToString(req.method()) << " " << req.path();

  const std::map<std::string, std::string, lynx::CaseInsensitiveLess> &headers =
      req.headers();
  std::stringstream ss;
  for (const auto &header : headers) {
    ss << header.first << ": " << header.second << "|";
  }
  LOG_INFO << ss.str();

  auto method = req.method();
  auto &path = req.path();

  bool flag = false;
  for (auto &[pair, handler] : lynx::g_route_table) {
    if (method == pair.first) {
      std::regex path_regex(pair.second);
      bool match = std::regex_match(path, path_regex);
      if (match) {
        flag = true;
        handler(req, resp);
        break;
      }
    }
  }
  if (!flag) {
    handleNotFound(req, resp);
  }
}

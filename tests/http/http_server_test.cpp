#include "student.h"

#include "lynx/db/pg_connection_pool.h"
#include "lynx/http/http_request.h"
#include "lynx/http/http_response.h"
#include "lynx/http/http_server.h"
#include "lynx/net/event_loop.h"

#include <iostream>
#include <map>
#include <sstream>

extern unsigned char favicon_jpg[];
extern unsigned int favicon_jpg_len;

StudentController *controller;

void onRequest(const lynx::HttpRequest &req, lynx::HttpResponse *resp) {
  std::cout << "Headers " << lynx::methodToString(req.method()) << " "
            << req.path() << std::endl;

  const std::map<std::string, std::string, lynx::CaseInsensitiveLess> &headers =
      req.headers();
  for (const auto &header : headers) {
    std::cout << header.first << ": " << header.second << std::endl;
  }

  controller->registr(req, resp);

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
  } else {
    resp->setStatusCode(lynx::HttpResponse::NotFound404);
    resp->setStatusMessage("Not Found");
    resp->setCloseConnection(true);
  }
}

void initDB(lynx::PgConnectionPool &pool) {
  /// Connect database
  lynx::PgConnection conn("PgConnection");
  conn.connect("127.0.0.1", "5432", "postgres", "123456", "demo");

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

int main(int argc, char *argv[]) {
  int num_threads = 5;

  lynx::EventLoop loop;
  lynx::HttpServer server(&loop, lynx::InetAddress(8000), "dummy");
  server.setHttpCallback(onRequest);
  server.setThreadNum(num_threads);

  lynx::PgConnectionPool pool("127.0.0.1", "5432", "postgres", "123456",
                              "demo");
  pool.start();

  initDB(pool);

  StudentRepository repository(pool);
  StudentService service(repository);
  controller = new StudentController(service);

  server.start();
  loop.loop();
}

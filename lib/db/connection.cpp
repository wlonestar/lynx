#include "lynx/db/connection.h"
#include "lynx/logger/logging.h"

#include <libpq-fe.h>

#include <chrono>
#include <sstream>

namespace lynx {

std::atomic_int32_t Connection::num_created;

Connection::Connection(const std::string &name) : name_(name) {
  setDefaultName();
}

Connection::~Connection() {
  if (conn_ != nullptr) {
    PQfinish(conn_);
    LOG_DEBUG << name_ << " disconnected";
    conn_ = nullptr;
  }
}

bool Connection::connect(const std::string &host, size_t port,
                         const std::string &user, const std::string &password,
                         const std::string &dbname) {
  auto fields = std::make_tuple("host", "port", "user", "password", "dbname");
  auto args_tp = std::make_tuple(host, port, user, password, dbname);
  auto index = std::make_index_sequence<5>();
  std::string sql = generateConnectSql(fields, args_tp, index);
  LOG_DEBUG << name_ << " connect: " << sql;
  conn_ = PQconnectdb(sql.data());
  if (PQstatus(conn_) != CONNECTION_OK) {
    LOG_ERROR << PQerrorMessage(conn_);
    return false;
  }
  return true;
}

bool Connection::execute(const std::string &sql) {
  LOG_DEBUG << "exec: " << sql;
  res_ = PQexec(conn_, sql.data());
  bool ret = PQresultStatus(res_) == PGRES_COMMAND_OK;
  PQclear(res_);
  return ret;
}

void Connection::refreshAliveTime() {
  alive_time_ = std::chrono::steady_clock::now();
}

uint64_t Connection::getAliveTime() {
  auto res = std::chrono::steady_clock::now() - alive_time_;
  auto millsec = std::chrono::duration_cast<std::chrono::milliseconds>(res);
  return millsec.count();
}

void Connection::setDefaultName() {
  int num = num_created.fetch_add(1);
  if (name_.empty()) {
    char buf[32];
    snprintf(buf, sizeof(buf), "PgConn%d", num);
    name_ = buf;
  }
}

} // namespace lynx

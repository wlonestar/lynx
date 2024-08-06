#include "lynx/db/pg_connection.h"

namespace lynx {

std::atomic_int32_t PgConnection::num_created;

PgConnection::PgConnection(const std::string &name) : name_(name) {
  setDefaultName();
}

PgConnection::~PgConnection() {
  if (conn_ != nullptr) {
    PQfinish(conn_);
    LOG_DEBUG << name_ << " disconnect";
    conn_ = nullptr;
  }
}

bool PgConnection::connect(const std::string &host, const std::string &port,
                           const std::string &user, const std::string &password,
                           const std::string &dbname, size_t connectTimeout) {
  auto fields = std::make_tuple("host", "port", "user", "password", "dbname",
                                "connect_timeout");
  auto args_tp =
      std::make_tuple(host, port, user, password, dbname, connectTimeout);
  auto index = std::make_index_sequence<6>();
  std::string sql = generateConnectSql(fields, args_tp, index);
  LOG_DEBUG << name_ << " connect: " << sql;
  conn_ = PQconnectdb(sql.data());
  if (PQstatus(conn_) != CONNECTION_OK) {
    LOG_ERROR << PQerrorMessage(conn_);
    return false;
  }
  return true;
}

bool PgConnection::execute(const std::string &sql) {
  LOG_DEBUG << "exec: " << sql;
  res_ = PQexec(conn_, sql.data());
  bool ret = PQresultStatus(res_) == PGRES_COMMAND_OK;
  PQclear(res_);
  return ret;
}

void PgConnection::setDefaultName() {
  int num = num_created.fetch_add(1);
  if (name_.empty()) {
    char buf[32];
    snprintf(buf, sizeof(buf), "PgConn%d", num);
    name_ = buf;
  }
}

} // namespace lynx

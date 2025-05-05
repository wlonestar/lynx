#include "lynx/db/Connection.h"
#include "lynx/logger/Logging.h"

#include <chrono>
#include <sstream>

#include <fmt/format.h>

namespace lynx {

namespace detail {

template <typename Tuple1, typename Tuple2, size_t... Idx>
std::string generateConnectSql(const Tuple1 &T1, const Tuple2 &T2,
                               std::index_sequence<Idx...> /*unused*/) {
  std::stringstream OS;
  auto Serialize = [&OS](const std::string &Key, const auto &Val) {
    OS << Key << "=" << Val << " ";
  };

  int Unused[] = {0, (Serialize(std::get<Idx>(T1), std::get<Idx>(T2)), 0)...};
  (void)Unused;
  return OS.str();
}

} // namespace detail

std::atomic_int32_t Connection::NumCreated;

Connection::Connection(const std::string &Name) : Name(Name) {
  setDefaultName();
}

Connection::~Connection() {
  if (Conn != nullptr) {
    PQfinish(Conn);
    LOG_DEBUG << Name << " disconnected";
    Conn = nullptr;
  }
}

bool Connection::connect(const std::string &Host, size_t Port,
                         const std::string &User, const std::string &Password,
                         const std::string &Dbname) {
  auto Fields = std::make_tuple("host", "port", "user", "password", "dbname");
  auto ArgsTp = std::make_tuple(Host, Port, User, Password, Dbname);
  auto Index = std::make_index_sequence<5>();
  std::string Sql = detail::generateConnectSql(Fields, ArgsTp, Index);
  LOG_DEBUG << Name << " connect: " << Sql;
  Conn = PQconnectdb(Sql.data());
  if (PQstatus(Conn) != CONNECTION_OK) {
    LOG_ERROR << PQerrorMessage(Conn);
    return false;
  }
  return true;
}

bool Connection::execute(const std::string &Sql) {
  LOG_DEBUG << "exec: " << Sql;
  Res = PQexec(Conn, Sql.data());
  bool Ret = PQresultStatus(Res) == PGRES_COMMAND_OK;
  PQclear(Res);
  return Ret;
}

void Connection::refreshAliveTime() {
  AliveTime = std::chrono::steady_clock::now();
}

uint64_t Connection::getAliveTime() {
  auto Rest = std::chrono::steady_clock::now() - AliveTime;
  auto Millsec = std::chrono::duration_cast<std::chrono::milliseconds>(Rest);
  return Millsec.count();
}

void Connection::setDefaultName() {
  int Num = NumCreated.fetch_add(1);
  if (Name.empty())
    Name = fmt::format("PgConn{}", Num);
}

} // namespace lynx

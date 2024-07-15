#ifndef LYNX_WEB_BASE_REPOSITORY_H
#define LYNX_WEB_BASE_REPOSITORY_H

#include "lynx/db/pg_connection_pool.h"

namespace lynx {

template <typename T, typename ID> class BaseRepository {
public:
  explicit BaseRepository(lynx::PgConnectionPool &pool) : pool_(pool) {}

  virtual std::vector<T> selectTop100() {
    auto conn = pool_.acquire();
    auto ret = conn->query<T>().limit(100).toVector();
    pool_.release(conn);
    return ret;
  }

  virtual int insert(T &t) {
    auto conn = pool_.acquire();
    auto ret = conn->insert(t);
    pool_.release(conn);
    return ret;
  }

  virtual int insert(std::vector<T> &t) {
    auto conn = pool_.acquire();
    auto ret = conn->insert(t);
    pool_.release(conn);
    return ret;
  }

protected:
  lynx::PgConnectionPool &pool_;
};

}; // namespace lynx

#endif

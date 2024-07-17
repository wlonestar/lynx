#ifndef LYNX_WEB_BASE_REPOSITORY_H
#define LYNX_WEB_BASE_REPOSITORY_H

#include "lynx/db/pg_connection_pool.h"
#include "lynx/orm/key_util.h"
#include "lynx/orm/pg_query_object.h"
#include "lynx/reflection.h"

#include <optional>

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

  virtual std::optional<T> selectById(ID id) {
    auto conn = pool_.acquire();
    auto ret = conn->query<T>().template where<T, ID>(id).toVector();
    pool_.release(conn);
    if (ret.empty()) {
      return std::nullopt;
    }
    return ret[0];
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

  virtual bool delById(ID id) {
    auto conn = pool_.acquire();
    auto ret = conn->del<T>().template where<T, ID>(id).execute();
    pool_.release(conn);
    return ret;
  }

protected:
  lynx::PgConnectionPool &pool_;
};

}; // namespace lynx

#endif

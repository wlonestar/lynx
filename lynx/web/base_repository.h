#ifndef LYNX_WEB_BASE_REPOSITORY_H
#define LYNX_WEB_BASE_REPOSITORY_H

#include "lynx/db/pg_connection_pool.h"

#include <optional>

namespace lynx {

template <typename T, typename ID> class BaseRepository {
public:
  explicit BaseRepository(lynx::PgConnectionPool &pool) : pool_(pool) {}

  virtual std::vector<T> selectTop100() {
    auto conn = pool_.acquire();
    auto ret = conn->query<T, ID>().limit(100).toVector();
    pool_.release(conn);
    return ret;
  }

  virtual std::vector<T> selectByPage(size_t page, size_t size) {
    auto conn = pool_.acquire();
    auto ret =
        conn->query<T, ID>().limit(size).offset((page - 1) * size).toVector();
    pool_.release(conn);
    return ret;
  }

  virtual std::optional<T> selectById(ID id) {
    auto conn = pool_.acquire();
    auto ret = conn->query<T, ID>().where(id).toVector();
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

  virtual bool updateById(ID id, T &&t) {
    auto conn = pool_.acquire();
    auto ret = conn->update<T, ID>().set(std::move(t)).where(id).execute();
    pool_.release(conn);
    return ret;
  }

  virtual bool delById(ID id) {
    auto conn = pool_.acquire();
    auto ret = conn->del<T, ID>().where(id).execute();
    pool_.release(conn);
    return ret;
  }

protected:
  lynx::PgConnectionPool &pool_;
};

}; // namespace lynx

#endif

#ifndef LYNX_WEB_BASE_REPOSITORY_H
#define LYNX_WEB_BASE_REPOSITORY_H

#include "lynx/db/connection_pool.h"

#include <optional>

namespace lynx {

/**
 * @class BaseRepository
 * @brief Base class for repositories that manage the database operations for a
 * specific entity.
 *
 * This class is a base class for repositories that manage the database
 * operations for a specific entity. It provides a common interface for
 * selecting, inserting, updating, and deleting entities from the database. Each
 * derived repository class implements the specific database operations for a
 * particular entity.
 *
 * @tparam T The type of the entity being managed.
 * @tparam ID The type of the entity's identifier.
 */
template <typename T, typename ID> class BaseRepository {
public:
  explicit BaseRepository(lynx::ConnectionPool &pool) : pool_(pool) {}

  /**
   * @brief Retrieves the top 100 entities from the database.
   *
   * @return A vector of entities.
   */
  virtual std::vector<T> selectTop100();

  /**
   * @brief Retrieves a page of entities from the database.
   *
   * @param page The page number of the entities to be retrieved.
   * @param size The number of entities per page.
   * @return A vector of entities.
   */
  virtual std::vector<T> selectByPage(size_t page, size_t size);

  /**
   * @brief Retrieves an entity by its identifier.
   *
   * @param id The identifier of the entity.
   * @return An optional containing the entity if found, or std::nullopt
   * otherwise.
   */
  virtual std::optional<T> selectById(ID id);

  /**
   * @brief Inserts an entity into the database.
   *
   * @param t The entity to be inserted.
   * @return The number of rows affected.
   */
  virtual int insert(T &t);

  /**
   * @brief Inserts a vector of entities into the database.
   *
   * @param t The vector of entities to be inserted.
   * @return The number of rows affected.
   */
  virtual int insert(std::vector<T> &t);

  /**
   * @brief Updates an entity by its identifier.
   *
   * @param id The identifier of the entity.
   * @param t The updated entity.
   * @return True if the update is successful, false otherwise.
   */
  virtual bool updateById(ID id, T &&t);

  /**
   * @brief Deletes an entity by its identifier.
   *
   * @param id The identifier of the entity.
   * @return True if the deletion is successful, false otherwise.
   */
  virtual bool delById(ID id);

protected:
  /// The connection pool used for database operations.
  lynx::ConnectionPool &pool_;
};

template <typename T, typename ID>
std::vector<T> BaseRepository<T, ID>::selectTop100() {
  auto conn = pool_.acquire();
  auto ret = conn->query<T, ID>().limit(100).toVector();
  return ret;
}

template <typename T, typename ID>
std::vector<T> BaseRepository<T, ID>::selectByPage(size_t page, size_t size) {
  auto conn = pool_.acquire();
  auto ret =
      conn->query<T, ID>().limit(size).offset((page - 1) * size).toVector();
  return ret;
}

template <typename T, typename ID>
std::optional<T> BaseRepository<T, ID>::selectById(ID id) {
  auto conn = pool_.acquire();
  auto ret = conn->query<T, ID>().where(id).toVector();
  if (ret.empty()) {
    return std::nullopt;
  }
  return ret[0];
}

template <typename T, typename ID> int BaseRepository<T, ID>::insert(T &t) {
  auto conn = pool_.acquire();
  auto ret = conn->insert(t);
  return ret;
}

template <typename T, typename ID>
int BaseRepository<T, ID>::insert(std::vector<T> &t) {
  auto conn = pool_.acquire();
  auto ret = conn->insert(t);
  return ret;
}

template <typename T, typename ID>
bool BaseRepository<T, ID>::updateById(ID id, T &&t) {
  auto conn = pool_.acquire();
  auto ret = conn->update<T, ID>().set(std::move(t)).where(id).execute();
  return ret;
}

template <typename T, typename ID> bool BaseRepository<T, ID>::delById(ID id) {
  auto conn = pool_.acquire();
  auto ret = conn->del<T, ID>().where(id).execute();
  return ret;
}

}; // namespace lynx

#endif

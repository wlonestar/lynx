#ifndef LYNX_WEB_BASE_REPOSITORY_H
#define LYNX_WEB_BASE_REPOSITORY_H

#include "lynx/db/ConnectionPool.h"

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
 * @tparam Ty The type of the entity being managed.
 * @tparam ID The type of the entity's identifier.
 */
template <typename Ty, typename ID> class BaseRepository {
public:
  explicit BaseRepository(lynx::ConnectionPool &Pool) : Pool(Pool) {}

  /**
   * @brief Retrieves the top 100 entities from the database.
   *
   * @return A vector of entities.
   */
  virtual std::vector<Ty> selectTop100();

  /**
   * @brief Retrieves a page of entities from the database.
   *
   * @param page The page number of the entities to be retrieved.
   * @param size The number of entities per page.
   * @return A vector of entities.
   */
  virtual std::vector<Ty> selectByPage(size_t Page, size_t Size);

  /**
   * @brief Retrieves an entity by its identifier.
   *
   * @param id The identifier of the entity.
   * @return An optional containing the entity if found, or std::nullopt
   * otherwise.
   */
  virtual std::optional<Ty> selectById(ID Id);

  /**
   * @brief Inserts an entity into the database.
   *
   * @param T The entity to be inserted.
   * @return The number of rows affected.
   */
  virtual int insert(Ty &T);

  /**
   * @brief Inserts a vector of entities into the database.
   *
   * @param T The vector of entities to be inserted.
   * @return The number of rows affected.
   */
  virtual int insert(std::vector<Ty> &T);

  /**
   * @brief Updates an entity by its identifier.
   *
   * @param id The identifier of the entity.
   * @param T The updated entity.
   * @return True if the update is successful, false otherwise.
   */
  virtual bool updateById(ID Id, Ty &&T);

  /**
   * @brief Deletes an entity by its identifier.
   *
   * @param id The identifier of the entity.
   * @return True if the deletion is successful, false otherwise.
   */
  virtual bool delById(ID Id);

protected:
  /// The connection pool used for database operations.
  lynx::ConnectionPool &Pool;
};

template <typename Ty, typename ID>
std::vector<Ty> BaseRepository<Ty, ID>::selectTop100() {
  auto Conn = Pool.acquire();
  return Conn->query<Ty, ID>().limit(100).toVector();
}

template <typename Ty, typename ID>
std::vector<Ty> BaseRepository<Ty, ID>::selectByPage(size_t Page, size_t Size) {
  auto Conn = Pool.acquire();
  return Conn->query<Ty, ID>().limit(Size).offset((Page - 1) * Size).toVector();
}

template <typename Ty, typename ID>
std::optional<Ty> BaseRepository<Ty, ID>::selectById(ID Id) {
  auto Conn = Pool.acquire();
  auto Ret = Conn->query<Ty, ID>().where(Id).toVector();
  if (Ret.empty())
    return std::nullopt;

  return Ret[0];
}

template <typename Ty, typename ID> int BaseRepository<Ty, ID>::insert(Ty &T) {
  auto Conn = Pool.acquire();
  return Conn->insert(T);
}

template <typename Ty, typename ID>
int BaseRepository<Ty, ID>::insert(std::vector<Ty> &T) {
  auto Conn = Pool.acquire();
  return Conn->insert(T);
}

template <typename Ty, typename ID>
bool BaseRepository<Ty, ID>::updateById(ID Id, Ty &&T) {
  auto Conn = Pool.acquire();
  return Conn->update<Ty, ID>().set(std::move(T)).where(Id).execute();
}

template <typename Ty, typename ID>
bool BaseRepository<Ty, ID>::delById(ID Id) {
  auto Conn = Pool.acquire();
  return Conn->del<Ty, ID>().where(Id).execute();
}

}; // namespace lynx

#endif

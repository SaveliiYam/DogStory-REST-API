#pragma once

#include <memory>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>

constexpr const char LEAVE_GAME_DB_URL_ENV_NAME[]{"GAME_DB_URL"};
constexpr const char LEAVE_GAME_DB_URL_ENV_VALUE[]{"postgres://postgres:qazwsxedc@localhost:5432/leave_game_db"};

class ConnectionPool
{
    using PoolType = ConnectionPool;
    using ConnectionPtr = std::shared_ptr<pqxx::connection>;

public:
    class ConnectionWrapper
    {
    public:
        ConnectionWrapper(std::shared_ptr<pqxx::connection> &&conn, PoolType &pool) noexcept
            : conn_{std::move(conn)}, pool_{&pool}
        {
        }

        ConnectionWrapper(const ConnectionWrapper &) = delete;
        ConnectionWrapper &operator=(const ConnectionWrapper &) = delete;

        ConnectionWrapper(ConnectionWrapper &&) = default;
        ConnectionWrapper &operator=(ConnectionWrapper &&) = default;

        pqxx::connection &operator*() const & noexcept
        {
            return *conn_;
        }
        pqxx::connection &operator*() const && = delete;

        pqxx::connection *operator->() const & noexcept
        {
            return conn_.get();
        }

        ~ConnectionWrapper()
        {
            if (conn_)
            {
                pool_->ReturnConnection(std::move(conn_));
            }
        }

    private:
        std::shared_ptr<pqxx::connection> conn_;
        PoolType *pool_;
    };

    // ConnectionFactory is a functional object returning std::shared_ptr<pqxx::connection>
    template <typename ConnectionFactory>
    ConnectionPool(size_t capacity, ConnectionFactory &&connection_factory)
    {
        pool_.reserve(capacity);
        for (size_t i = 0; i < capacity; ++i)
        {
            pool_.emplace_back(connection_factory());
        }
    }

    ConnectionWrapper GetConnection()
    {
        std::unique_lock lock{mutex_};
        // Блокируем текущий поток и ждём, пока cond_var_ не получит уведомление и не освободится
        // хотя бы одно соединение
        cond_var_.wait(lock, [this]
                       { return used_connections_ < pool_.size(); });
        // После выхода из цикла ожидания мьютекс остаётся захваченным

        return {std::move(pool_[used_connections_++]), *this};
    }

private:
    void ReturnConnection(ConnectionPtr &&conn)
    {
        // Возвращаем соединение обратно в пул
        {
            std::lock_guard lock{mutex_};
            assert(used_connections_ != 0);
            pool_[--used_connections_] = std::move(conn);
        }
        // Уведомляем один из ожидающих потоков об изменении состояния пула
        cond_var_.notify_one();
    }

    std::mutex mutex_;
    std::condition_variable cond_var_;
    std::vector<ConnectionPtr> pool_;
    size_t used_connections_ = 0;
};

namespace
{
    class ConnectionPoolSingleton
    {
    private:
        static std::once_flag initInstanceFlag;
        static ConnectionPoolSingleton *instance;
        ConnectionPool *pool;
        ConnectionPoolSingleton()
        {
            const auto *db_url = std::getenv(LEAVE_GAME_DB_URL_ENV_NAME);
            const unsigned num_threads = std::max(1u, std::thread::hardware_concurrency());
            pool = new ConnectionPool{num_threads, [db_url]
                                      {
                                          auto conn = std::make_shared<pqxx::connection>(db_url);
                                          return conn;
                                      }};
        }

        ~ConnectionPoolSingleton() = default;

    public:
        ConnectionPoolSingleton(const ConnectionPoolSingleton &) = delete;
        ConnectionPoolSingleton &operator=(const ConnectionPoolSingleton &) = delete;

        static ConnectionPoolSingleton *getInstance()
        {
            call_once(initInstanceFlag, ConnectionPoolSingleton::initSingleton);
            return instance;
        }

        ConnectionPool *GetPool() { return pool; }

        static void initSingleton()
        {
            instance = new ConnectionPoolSingleton();
        }
    };
    ConnectionPoolSingleton *ConnectionPoolSingleton::instance = nullptr;
    std::once_flag ConnectionPoolSingleton::initInstanceFlag;
}
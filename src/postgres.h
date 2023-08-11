#pragma once
#include <pqxx/connection>
#include <pqxx/transaction>
#include "model.h"

namespace postgres
{
    class RetiredRepositoryImpl
    {
    public:
        explicit RetiredRepositoryImpl(pqxx::connection &connection)
            : connection_{connection}
        {
        }

        void SaveRetired(const model::PlayerRecordItem &retired);
        std::vector<model::PlayerRecordItem> GetRetired(int start = 0, int max_items = 100);

    private:
        pqxx::connection &connection_;
    };

    class Database
    {
    public:
        explicit Database(pqxx::connection connection);
        void CreateTable();

    private:
        pqxx::connection connection_;
    };

} // namespace postgres
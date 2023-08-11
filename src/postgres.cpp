#include "postgres.h"
#include <string_view>
#include <string>
#include <boost/format.hpp>
#include <pqxx/pqxx>
#include <pqxx/except>

namespace postgres
{

	using namespace std::literals;
	using pqxx::operator"" _zv;

	void RetiredRepositoryImpl::SaveRetired(const model::PlayerRecordItem &retired)
	{
		pqxx::work work{connection_};
		work.exec_params(
			R"(INSERT INTO retired_players (id, name, score, play_time_ms) VALUES ($1, $2, $3, $4);)"_zv,
			retired.id, retired.name, retired.score, retired.playTime);
		work.commit();
	}

	std::vector<model::PlayerRecordItem> RetiredRepositoryImpl::GetRetired(int start, int max_items)
	{
		pqxx::read_transaction rd(connection_);
		auto req = boost::format("SELECT id, name, score, play_time_ms FROM retired_players ORDER BY score DESC, play_time_ms LIMIT %1% OFFSET %2%;") % max_items % start;
		std::vector<model::PlayerRecordItem> res;
		// Выполняем запрос и итерируемся по строкам ответа
		for (auto [id, name, score, play_time_ms] : rd.query<std::string, std::string, int, int>(req.str()))
		{
			model::PlayerRecordItem retired{id, name, score, play_time_ms};
			res.push_back(retired);
		}

		return res;
	}

	Database::Database(pqxx::connection connection)
		: connection_{std::move(connection)}
	{
	}

	void Database::CreateTable()
	{
		pqxx::work work{connection_};
		work.exec(R"(
		CREATE TABLE IF NOT EXISTS retired_players (
    		id UUID  PRIMARY KEY,
    		name varchar(100) NOT NULL,
			score integer NOT NULL,
			play_time_ms integer NOT NULL );
		)"_zv);

		work.exec(R"(CREATE INDEX IF NOT EXISTS score_time_name_idx ON retired_players (score DESC, play_time_ms, name);)"_zv);
		// коммитим изменения
		work.commit();
	}

} // namespace postgres
#include "model.h"
#include "server_exceptions.h"
#include "model_serialization.h"
#include <algorithm>
#include "utility_functions.h"
#include <mutex>

namespace model
{
	using namespace std::literals;

	std::recursive_mutex db_update_mutex;

	void Map::AddOffice(const Office &office)
	{
		if (warehouse_id_to_index_.contains(office.GetId()))
		{
			throw std::invalid_argument("Duplicate warehouse");
		}

		const size_t index = offices_.size();
		Office &o = offices_.emplace_back(std::move(office));
		try
		{
			warehouse_id_to_index_.emplace(o.GetId(), index);
		}
		catch (...)
		{
			// Удаляем офис из вектора, если не удалось вставить в unordered_map
			offices_.pop_back();
			throw;
		}
	}

	void Game::AddMap(const Map &map)
	{
		const size_t index = maps_.size();
		if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted)
		{
			throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
		}
		else
		{
			try
			{
				maps_.emplace_back(std::move(map));
			}
			catch (...)
			{
				map_id_to_index_.erase(it);
				throw;
			}
		}
	}

	void Map::AddLoot(Loot loot)
	{
		loots_.emplace_back(std::move(loot));
	}

	std::shared_ptr<GameSession> Game::FindSession(const std::string &map_name)
	{
		auto itFind = std::find_if(sessions_.begin(), sessions_.end(), [&map_name](std::shared_ptr<GameSession> &session)
								   { return session->GetMap() == map_name; });

		if (itFind != sessions_.end())
			return *itFind;

		std::shared_ptr<GameSession> res;
		return res;
	}

	size_t Game::GetNumPlayersInAllSessions()
	{
		size_t players_coutner = 0;
		std::for_each(sessions_.begin(), sessions_.end(), [&players_coutner](std::shared_ptr<GameSession> &session)
					  { players_coutner += session->GetNumPlayers(); });

		return players_coutner;
	}

	Game::PlayerAuthInfo Game::AddPlayer(const std::string &map_id, const std::string &player_name)
	{
		const Map *mapToAdd = FindMap(Map::Id(map_id));
		if (player_name.empty())
			throw EmptyNameException();

		if (!mapToAdd)
			throw MapNotFoundException();

		std::shared_ptr<GameSession> session = FindSession(map_id);
		if (!session)
		{
			auto [loot_period, loot_probability] = GetLootParameters();
			session = std::make_shared<GameSession>(map_id, loot_period, loot_probability);
			sessions_.push_back(session);
		}
		auto player = session->AddPlayer(player_name, const_cast<Map *>(mapToAdd), spawn_in_random_points_, default_bag_capacity_);
		return {player->GetToken(), player->GetId()};
	}

	std::shared_ptr<GameSession> Game::GetSessionForToken(const std::string &auth_token)
	{
		auto itFind = std::find_if(sessions_.begin(), sessions_.end(), [&auth_token](std::shared_ptr<GameSession> &session)
								   { return session->HasPlayerWithAuthToken(auth_token) == true; });

		if (itFind == sessions_.end())
			return std::shared_ptr<GameSession>();

		return (*itFind);
	}

	const std::vector<std::shared_ptr<Player>> Game::FindAllPlayersForAuthInfo(const std::string &auth_token)
	{
		auto session = GetSessionForToken(auth_token);
		if (!session)
			return {};

		return session->GetAllPlayers();
	}

	const vector<LootInfo> Game::GetLootsForAuthInfo(const std::string &auth_token)
	{
		auto session = GetSessionForToken(auth_token);
		if (!session)
			return {};

		return session->GetLootsInfo();
	}

	std::shared_ptr<Player> Game::GetPlayerWithAuthToken(const std::string &auth_token)
	{
		auto itFind = std::find_if(sessions_.begin(), sessions_.end(), [&auth_token](std::shared_ptr<GameSession> &session)
								   { return session->HasPlayerWithAuthToken(auth_token) == true; });

		if (itFind == sessions_.end())
			throw PlayerAbsentException();

		return (*itFind)->GetPlayerWithAuthToken(auth_token);
	}

	bool Game::HasSessionWithAuthInfo(const std::string &auth_token)
	{
		auto itFind = std::find_if(sessions_.begin(), sessions_.end(), [&auth_token](std::shared_ptr<GameSession> &session)
								   { return session->HasPlayerWithAuthToken(auth_token) == true; });

		if (itFind == sessions_.end())
			return false;

		return true;
	}

	std::shared_ptr<GameSession> Game::GetSessionWithAuthInfo(const std::string &auth_token)
	{
		auto itFind = std::find_if(sessions_.begin(), sessions_.end(), [&auth_token](std::shared_ptr<GameSession> &session)
								   { return session->HasPlayerWithAuthToken(auth_token) == true; });

		if (itFind == sessions_.end())
			throw InvalidSessionException();

		return *itFind;
	}

	void Game::MoveDogs(int deltaTime)
	{
		std::for_each(sessions_.begin(), sessions_.end(), [deltaTime](std::shared_ptr<GameSession> &session)
					  { session->MoveDogs(deltaTime); });
	}

	void Game::GenerateLoot(int deltaTime)
	{
		std::for_each(sessions_.begin(), sessions_.end(), [this, deltaTime](std::shared_ptr<GameSession> &session)
					  {
		const auto& mapName =  session->GetMap();
		const Map* pMap = this->FindMap(model::Map::Id(mapName));
		if(pMap)
			session->GenerateLoot(deltaTime, pMap); });
	}

	void Game::SetLootParameters(double period, double probability)
	{
		loot_period_ = period;
		loot_probability_ = probability;
	}
	std::shared_ptr<GameSessionsStates> Game::GetGameSessionsStates() const
	{
		std::shared_ptr<GameSessionsStates> res = std::make_shared<GameSessionsStates>();

		for (const auto &session : sessions_)
			res->states.push_back(session->GetState());

		return res;
	}

	void Game::SaveSessions(int deltaTime)
	{
		if (!save_period_)
			return;
		SerializeSessions(*this, deltaTime, save_period_);
	}

	void Game::RestoreSessions(const model::GameSessionsStates &sessions)
	{
		std::for_each(sessions.states.begin(), sessions.states.end(), [this](auto &state)
					  {
		auto [loot_period, loot_probability] = GetLootParameters();
		auto session = std::make_shared<GameSession>(state.map_id_, loot_period, loot_probability);
		session->SetPlayerId(state.player_id_);
		session->SetLootsInfo(state.loots_info_state);
		const Map* mapToAdd = FindMap(Map::Id(state.map_id_));

		std::for_each(state.player_state_.begin(), state.player_state_.end(),
					  [this, mapToAdd, &session](auto& pl_state){
					   auto player = session->AddPlayer(pl_state.name_, const_cast<Map*>(mapToAdd),
							   	   	   	   	   	   	    spawn_in_random_points_, default_bag_capacity_);
					   player->SetToken(pl_state.token_);
					   player->SetId(pl_state.id_);
					   auto dog = player->GetDog();

					   dog->SetDirection(pl_state.dog_direction_);
					   dog->SetPositionOnMap(pl_state.dog_position_);
					   dog->SetGatheredLoot(pl_state.gathered_loots_);
					   dog->SetBagCapacity(pl_state.bag_capacity_);
					   dog->SetScore(pl_state.score_);
					   dog->SetPlayTime(pl_state.play_time_);
					 });

		sessions_.push_back(session); });
	}

	void Game::HandleRetiredPlayers()
	{
		std::lock_guard lg(db_update_mutex);
		auto expired_players = FindExpiredPlayers();
		if (expired_players.empty())
			return;
		SaveExpiredPlayers(expired_players);
		DeleteExpiredPlayers(expired_players);
	}

	std::vector<RetiredSessionPlayers> Game::FindExpiredPlayers()
	{
		std::vector<RetiredSessionPlayers> res;

		for (auto itSession = sessions_.begin(); itSession != sessions_.end(); ++itSession)
		{
			RetiredSessionPlayers pairs;
			const auto &players = (*itSession)->GetPlayers();
			for (auto itPlayer = players.begin(); itPlayer != players.end(); ++itPlayer)
			{
				auto dog = (*itPlayer)->GetDog();
				auto idle_time = dog->GetIdleTime();
				if (idle_time >= dog_retierement_time_)
				{
					pairs.second.push_back(*itPlayer);
				}
			}
			if (!pairs.second.empty())
			{
				pairs.first = *itSession;
				res.push_back(pairs);
			}
		}
		return res;
	}

	void Game::SaveExpiredPlayers(const std::vector<RetiredSessionPlayers> &expired_sessions_players)
	{

		for (auto itSesPlrs = expired_sessions_players.begin(); itSesPlrs != expired_sessions_players.end(); ++itSesPlrs)
		{
			for (auto itPlayer = itSesPlrs->second.begin(); itPlayer != itSesPlrs->second.end(); ++itPlayer)
			{
				auto dog = (*itPlayer)->GetDog();
				SaveRetiredPlayer((*itPlayer)->GetName(), dog->GetScore(), dog->GetPlayTime());
			}
		}
	}

	void Game::DeleteExpiredPlayers(const std::vector<RetiredSessionPlayers> &expired_sessions_players)
	{
		for (auto itSesPlrs = expired_sessions_players.begin(); itSesPlrs != expired_sessions_players.end(); ++itSesPlrs)
		{
			auto itSes = std::find_if(sessions_.begin(), sessions_.end(), [itSesPlrs](auto &elem)
									  { return elem == itSesPlrs->first; });

			if (itSes == sessions_.end())
				continue;

			(*itSes)->DeleteRetiredPlayers(itSesPlrs->second);
			if (!(*itSes)->GetNumPlayers())
			{
				const auto new_end{std::remove(std::begin(sessions_), std::end(sessions_), *itSes)};
				sessions_.erase(new_end, std::end(sessions_));
			}
		}
	}

	std::vector<PlayerRecordItem> Game::GetRecords(int start, int max_items) const
	{
		return GetRetiredPlayers(start, max_items);
	}

} // namespace model
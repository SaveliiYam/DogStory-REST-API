#include "game_session.h"
#include "player_tokens.h"
#include "model.h"
#include "server_exceptions.h"

namespace model
{

	Player::Player(unsigned int id, const std::string &name, const std::string &token, const model::Map *map, bool spawn_dog_in_random_point)
		: id_(id), name_(name), token_(token)
	{
		dog_ = std::make_shared<Dog>(map, spawn_dog_in_random_point);
	}

	std::shared_ptr<Player> GameSession::AddPlayer(const std::string player_name, const model::Map *map, bool spawn_dog_in_random_point)
	{
		auto itFind = std::find_if(players_.begin(), players_.end(), [&player_name](std::shared_ptr<Player> &player)
								   { return player->GetName() == player_name; });

		if (itFind != players_.end())
			return *itFind;

		PlayerTokens tk;
		auto token = tk.GetToken();
		auto player = std::make_shared<Player>(player_id, player_name, token, map, spawn_dog_in_random_point);

		players_.push_back(player);
		player_id++;

		return players_.back();
	}

	bool GameSession::HasPlayerWithAuthToken(const std::string &auth_token)
	{
		auto itFind = std::find_if(players_.begin(), players_.end(), [&auth_token](std::shared_ptr<Player> &player)
								   { return player->GetToken() == auth_token; });

		if (itFind != players_.end())
			return true;

		return false;
	}

	std::shared_ptr<Player> GameSession::GetPlayerWithAuthToken(const std::string &auth_token)
	{
		auto itFind = std::find_if(players_.begin(), players_.end(), [&auth_token](std::shared_ptr<Player> &player)
								   { return player->GetToken() == auth_token; });

		if (itFind != players_.end())
			return *itFind;

		throw PlayerAbsentException();
	}

	const std::vector<std::shared_ptr<Player>> GameSession::GetAllPlayers()
	{
		return players_;
	}

	void GameSession::MoveDogs(int deltaTime)
	{
		std::for_each(players_.begin(), players_.end(), [deltaTime](std::shared_ptr<Player> &player)
					  { player->GetDog()->Move(deltaTime); });
	}

}

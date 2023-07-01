#pragma once
#include "dog.h"

using namespace std;

class Dog;

namespace model
{
	class Map;

	class Player
	{
	public:
		Player(unsigned int id, const std::string &name, const std::string &token, const model::Map *map, bool spawn_dog_in_random_point);
		const std::string &GetToken() { return token_; }
		const std::string &GetName() { return name_; }
		unsigned int GetId() { return id_; }
		std::shared_ptr<Dog> GetDog() { return dog_; }

	private:
		std::string name_;
		std::string token_;
		unsigned int id_{0};
		std::shared_ptr<Dog> dog_;
	};

	class GameSession
	{
	public:
		GameSession(const std::string &map_id) : map_(map_id) {}
		std::shared_ptr<Player> AddPlayer(const std::string player_name, const model::Map *map, bool spawn_dog_in_random_point);
		const std::string &GetMap() { return map_; }
		bool HasPlayerWithAuthToken(const std::string &auth_token);
		const std::vector<std::shared_ptr<Player>> GetAllPlayers();
		std::shared_ptr<Player> GetPlayerWithAuthToken(const std::string &auth_token);
		void MoveDogs(int deltaTime);
		size_t GetNumPlayers() { return players_.size(); }

	private:
		vector<std::shared_ptr<Player>> players_;
		std::string map_;
		unsigned int player_id = 0;
	};

}

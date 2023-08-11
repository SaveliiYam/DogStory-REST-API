#pragma once
#include "dog.h"
#include <memory>
#include <fstream>
#include <boost/serialization/vector.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

namespace loot_gen
{
	class LootGenerator;
}

namespace model
{
	struct PlayerState
	{
		PlayerState(const std::string &name, const std::string &token, unsigned int id, std::shared_ptr<Dog> dog)
			: name_{name}, token_{token}, id_{id}
		{
			dog_direction_ = dog->GetDirection();
			dog_position_ = dog->GetPositionOnMap();
			gathered_loots_ = dog->GetGatheredLoot();
			bag_capacity_ = dog->GetBagCapacity();
			score_ = dog->GetScore();
			play_time_ = dog->GetPlayTime();
		}

		PlayerState() {}
		std::string name_;
		std::string token_;
		unsigned int id_{};

		DogDirection dog_direction_;
		DogPos dog_position_;
		std::vector<model::LootInfo> gathered_loots_;
		unsigned bag_capacity_{};
		int score_{};
		int play_time_{};

	private:
		// Allow serialization to access non-public data members.
		friend class boost::serialization::access;
		// Serialize the std::vector member of Info

		template <class Archive>
		void serialize(Archive &ar, const unsigned int version)
		{
			ar &name_;
			ar &token_;
			ar &id_;

			ar &dog_direction_;
			ar &dog_position_;
			ar &gathered_loots_;
			ar &bag_capacity_;
			ar &score_;
			ar &play_time_;
		}
	};

	class Player
	{
	public:
		Player(unsigned int id, const std::string &name, const std::string &token,
			   const model::Map *map, bool spawn_dog_in_random_point, unsigned defaultBagCapacity);
		const std::string &GetToken() const { return token_; }
		void SetToken(const std::string &token) { token_ = token; }
		const std::string &GetName() const { return name_; }
		unsigned int GetId() const { return id_; }
		void SetId(unsigned int id) { id_ = id; }
		std::shared_ptr<Dog> GetDog() { return dog_; }
		PlayerState GetState();

	private:
		std::string name_;
		std::string token_;
		unsigned int id_{0};
		std::shared_ptr<Dog> dog_;
	};

	struct GameSessionState
	{
		std::vector<PlayerState> player_state_;
		std::vector<LootInfo> loots_info_state;
		std::string map_id_;
		unsigned int player_id_;

	private:
		// Allow serialization to access non-public data members.
		friend class boost::serialization::access;

		// Serialize the std::vector member of Info
		template <class Archive>
		void serialize(Archive &ar, const unsigned int version)
		{
			ar &map_id_;
			ar &player_id_;
			ar &player_state_;
			ar &loots_info_state;
		}
	};

	struct GameSessionsStates
	{
		std::vector<GameSessionState> states;

	private:
		// Allow serialization to access non-public data members.
		friend class boost::serialization::access;

		// Serialize the std::vector member of Info
		template <class Archive>
		void serialize(Archive &ar, const unsigned int version)
		{
			ar &states;
		}
	};

	class GameSession
	{
	public:
		GameSession(const std::string &map_id, double loot_period, double loot_probability)
			: map_id_(map_id)
		{
			InitLootGenerator(loot_period, loot_probability);
		}
		std::shared_ptr<Player> AddPlayer(const std::string player_name, model::Map *map,
										  bool spawn_dog_in_random_point, unsigned defaultBagCapacity);
		const std::string &GetMap() { return map_id_; }
		bool HasPlayerWithAuthToken(const std::string &auth_token);
		const std::vector<std::shared_ptr<Player>> GetAllPlayers();
		std::shared_ptr<Player> GetPlayerWithAuthToken(const std::string &auth_token);
		void MoveDogs(int deltaTime);
		size_t GetNumPlayers() { return players_.size(); }
		const std::vector<model::LootInfo> &GetLootsInfo() { return loots_info_; };
		void GenerateLoot(int deltaTime, const Map *pMap);
		GameSessionState GetState() const;
		void SetPlayerId(unsigned int id) { player_id = id; }
		void SetLootsInfo(const std::vector<LootInfo> &loots) { loots_info_ = loots; }
		const std::vector<std::shared_ptr<Player>> &GetPlayers() { return players_; }
		void DeleteRetiredPlayers(const std::vector<std::shared_ptr<model::Player>> &retired_players);

	private:
		void InitLootGenerator(double loot_period, double loot_probability);
		std::vector<std::shared_ptr<Player>> players_;
		std::vector<LootInfo> loots_info_;
		std::string map_id_;
		unsigned int player_id = 0;
		model::Map *map_{};
		std::shared_ptr<loot_gen::LootGenerator> lootGen_;
		const int thousand_for_generation = 1000;
	};
}
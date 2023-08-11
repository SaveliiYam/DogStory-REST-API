#pragma once
#include <boost/serialization/vector.hpp>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <sstream>
#include <fstream>
#include <iostream>
#include "model.h"
#include "dog.h"
#include "game_session.h"
#include "geom.h"
#include <chrono>

namespace geom
{

	template <typename Archive>
	void serialize(Archive &ar, Point2D &point, [[maybe_unused]] const unsigned version)
	{
		ar &point.x;
		ar &point.y;
	}

	template <typename Archive>
	void serialize(Archive &ar, Vec2D &vec, [[maybe_unused]] const unsigned version)
	{
		ar &vec.x;
		ar &vec.y;
	}

} // namespace geom

namespace model
{
	template <typename Archive>
	void serialize(Archive &ar, DogPosition &pos, [[maybe_unused]] const unsigned version)
	{
		ar &pos.x;
		ar &pos.y;
	}

	template <typename Archive>
	void serialize(Archive &ar, DogSpeed &speed, [[maybe_unused]] const unsigned version)
	{
		ar &speed.vx;
		ar &speed.vy;
	}

	template <typename Archive>
	void serialize(Archive &ar, DogPos &pos, [[maybe_unused]] const unsigned version)
	{
		ar &pos.current_road_index;
		ar &pos.curr_position;
		ar &pos.curr_speed;
	}
	template <typename Archive>
	void serialize(Archive &ar, LootInfo &loot, [[maybe_unused]] const unsigned version)
	{
		ar &loot.id;
		ar &loot.type;
		ar &loot.x;
		ar &loot.y;
	}
} // namespace model

namespace /*serialization*/
{

	using InputArchive = boost::archive::text_iarchive;
	using OutputArchive = boost::archive::text_oarchive;
	using TimeInterval = std::chrono::milliseconds;
	TimeInterval time_without_saving_{};

	void DeserializeSessions(model::Game &game)
	{
		model::GameSessionsStates states;
		if (std::filesystem::exists(game.GetSavePath()))
		{
			std::ifstream file{game.GetSavePath()};
			InputArchive ia{file};
			ia >> states;
		}
		game.RestoreSessions(states);
	}

	void SerializeSessions(const model::Game &game)
	{
		std::stringstream ss;
		OutputArchive oa{ss};
		oa << *game.GetGameSessionsStates();

		std::ofstream ofs(game.GetSavePath());
		ofs << ss.str();
	}

	void SerializeSessions(const model::Game &game, int deltaTime, int savePeriod)
	{
		time_without_saving_ += TimeInterval{deltaTime};
		if (time_without_saving_ < TimeInterval{savePeriod})
			return;

		SerializeSessions(game);

		time_without_saving_ = {};
	}

	void SerializeGameSession(const model::GameSession &session)
	{
		std::stringstream ss;
		OutputArchive oa{ss};

		oa << session.GetState();
		std::ofstream ofs("filename.arch");
		ofs << ss.str();
	}

	model::GameSessionState DeserializeGameSession()
	{
		std::stringstream ss;
		InputArchive ia{ss};

		model::GameSessionState state;

		ia >> state;
		return state;
	}

} // namespace serialization
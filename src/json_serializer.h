#pragma once
#include <map>
#include "model.h"

namespace model
{
	class Player;
}

namespace json_serializer
{
	std::string MakeBadRequestResponce();
	std::string MakeMapNotFoundResponce();
	std::string MakeAuthResponce(const std::string &auth_key, unsigned playerId);
	std::string MakeMappedResponce(const std::map<std::string, std::string> &key_values);
	std::string GetMapListResponce(const model::Game &game);
	std::string GetMapContentResponce(const model::Game &game, const std::string &map_id);
	std::string GetPlayerInfoResponce(const std::vector<std::shared_ptr<model::Player>> &players_info);
	std::string GetPlayersDogInfoResponce(const std::vector<std::shared_ptr<model::Player>> &players, const std::vector<model::LootInfo> &loots);
	std::string MakeRecordsResponce(const model::Game &game, int start, int max_items);
} // namespace json_serializer
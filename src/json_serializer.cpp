#include "json_serializer.h"
#include <utility>
#include <boost/json.hpp>
#include "game_session.h"

namespace json = boost::json;
using namespace std::literals;

const int MILLISECONDS_IN_SECOND = 1000;
namespace json_serializer
{

	std::string MakeErrorResponce(const std::string &codeMessage, const std::string &errorMessage)
	{
		json::object resp_object;
		resp_object["code"] = codeMessage;
		resp_object["message"] = errorMessage;
		return json::serialize(resp_object);
	}

	std::string MakeMapNotFoundResponce()
	{
		return MakeErrorResponce("mapNotFound", "Map not found");
	}

	std::string MakeBadRequestResponce()
	{
		return MakeErrorResponce("badRequest", "Bad request");
	}

	std::string MakeAuthResponce(const std::string &auth_key, unsigned playerId)
	{
		json::object resp_object;

		resp_object["authToken"] = auth_key;
		resp_object["playerId"] = playerId;

		return json::serialize(resp_object);
	}

	std::string MakeMappedResponce(const std::map<std::string, std::string> &key_values)
	{
		json::object resp_object;

		for (const auto &[key, value] : key_values)
			resp_object[key] = value;

		return json::serialize(resp_object);
	}

	std::string GetPlayerInfoResponce(const std::vector<std::shared_ptr<model::Player>> &players_info)
	{
		json::object resp_object;

		for (auto &player : players_info)
		{
			json::object name_object;
			name_object["name"] = player->GetName();

			resp_object[std::to_string(player->GetId())] = name_object;
		}

		return json::serialize(resp_object);
	}

	json::array SerializeDogBag(const std::vector<model::LootInfo> &loots)
	{
		json::array bag_ar;

		for (const auto &cur_loot : loots)
		{
			json::object loot_object;

			loot_object["id"] = cur_loot.id;
			loot_object["type"] = cur_loot.type;

			bag_ar.emplace_back(loot_object);
		}

		return bag_ar;
	}

	json::object SerializePlayers(const std::vector<std::shared_ptr<model::Player>> &players)
	{
		json::object players_object;

		for (auto &player : players)
		{
			auto dog = player->GetDog();

			json::object dog_object;
			json::array pos_ar, speed_ar;

			auto pos = dog->GetPosition();
			pos_ar.emplace_back(pos.x);
			pos_ar.emplace_back(pos.y);

			dog_object["pos"] = pos_ar;

			auto speed = dog->GetSpeed();
			speed_ar.emplace_back(speed.vx);
			speed_ar.emplace_back(speed.vy);

			dog_object["speed"] = speed_ar;

			DogDirection dir = dog->GetDirection();
			dog_object["dir"] = model::ConvertDogDirectionToString(dir);

			dog_object["bag"] = SerializeDogBag(dog->GetGatheredLoot());
			dog_object["score"] = dog->GetScore();
			players_object[std::to_string(player->GetId())] = dog_object;
		}

		return players_object;
	}

	json::object SerializeLoots(const std::vector<model::LootInfo> &loots)
	{
		json::object loots_object;

		for (size_t i = 0; i < loots.size(); ++i)
		{
			json::object loot_object;
			json::array pos_ar;

			pos_ar.emplace_back(loots[i].x);
			pos_ar.emplace_back(loots[i].y);

			loot_object["pos"] = pos_ar;
			loot_object["type"] = loots[i].type;
			loots_object[std::to_string(i)] = loot_object;
		}

		return loots_object;
	}

	std::string GetPlayersDogInfoResponce(const std::vector<std::shared_ptr<model::Player>> &players, const std::vector<model::LootInfo> &loots)
	{
		json::object resp_object;

		resp_object["players"] = SerializePlayers(players);
		resp_object["lostObjects"] = SerializeLoots(loots);
		return json::serialize(resp_object);
	}
	void SerializeOffices(const model::Map &map, json::object &root)
	{
		json::array offices_ar;
		for (const auto &office : map.GetOffices())
		{
			json::object office_obj;

			office_obj["id"] = *office.GetId();
			office_obj["x"] = office.GetPosition().x;
			office_obj["y"] = office.GetPosition().y;
			office_obj["offsetX"] = office.GetOffset().dx;
			office_obj["offsetY"] = office.GetOffset().dy;

			offices_ar.emplace_back(office_obj);
		}
		root["offices"] = offices_ar;
	}

	void SerializeBuildings(const model::Map &map, json::object &root)
	{
		json::array buildings_ar;
		for (const auto &building : map.GetBuildings())
		{
			json::object building_obj;

			const auto &bounds = building.GetBounds();

			building_obj["x"] = bounds.position.x;
			building_obj["y"] = bounds.position.y;
			building_obj["w"] = bounds.size.width;
			building_obj["h"] = bounds.size.height;

			buildings_ar.emplace_back(building_obj);
		}
		root["buildings"] = buildings_ar;
	}

	void SerializeRoads(const model::Map &map, json::object &root)
	{
		json::array roads_ar;
		for (const auto &road : map.GetRoads())
		{
			json::object road_obj;

			model::Point start = road.GetStart();
			model::Point end = road.GetEnd();

			road_obj["x0"] = start.x;
			road_obj["y0"] = start.y;

			if (road.IsHorizontal())
				road_obj["x1"] = end.x;
			else
				road_obj["y1"] = end.y;

			roads_ar.emplace_back(road_obj);
		}
		root["roads"] = roads_ar;
	}

	void SerializeLoots(const model::Map &map, json::object &root)
	{
		json::array loots_ar;
		for (const auto &loot : map.GetLoots())
		{
			json::object loot_obj;

			loot_obj["name"] = loot.GetName();
			loot_obj["file"] = loot.GetFile();
			loot_obj["type"] = loot.GetType();

			if (loot.GetRotation() >= 0)
				loot_obj["rotation"] = loot.GetRotation();

			if (!loot.GetColor().empty())
				loot_obj["color"] = loot.GetColor();

			loot_obj["scale"] = loot.GetScale();
			loot_obj["value"] = loot.GetScore();
			loots_ar.emplace_back(loot_obj);
		}
		root["lootTypes"] = loots_ar;
	}

	std::string GetMapListResponce(const model::Game &game)
	{
		json::array map_ar;
		for (const auto &map : game.GetMaps())
		{
			json::object map_obj;

			map_obj["id"] = *map.GetId();
			map_obj["name"] = map.GetName();
			map_ar.emplace_back(map_obj);
		}

		return json::serialize(map_ar);
	}

	std::string GetMapContentResponce(const model::Game &game, const std::string &map_id)
	{
		const model::Map *mapFound = game.FindMap(model::Map::Id(map_id));

		if (!mapFound)
			return ("");

		json::object root;

		root["id"] = *mapFound->GetId();
		root["name"] = mapFound->GetName();

		SerializeRoads(*mapFound, root);
		SerializeBuildings(*mapFound, root);
		SerializeOffices(*mapFound, root);
		SerializeLoots(*mapFound, root);
		return json::serialize(root);
	}

	std::string MakeRecordsResponce(const model::Game &game, int start, int max_items)
	{
		json::array map_ar;
		for (const auto &record : game.GetRecords(start, max_items))
		{
			json::object map_obj;

			map_obj["name"] = record.name;
			map_obj["score"] = record.score;
			map_obj["playTime"] = (double)record.playTime / MILLISECONDS_IN_SECOND;
			map_ar.emplace_back(map_obj);
		}

		return json::serialize(map_ar);
	}
} // namespace json_serializer
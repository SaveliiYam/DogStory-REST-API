#include "json_loader.h"
#include <fstream>
#include <boost/json.hpp>
#include "server_exceptions.h"
#include <iostream>
namespace json = boost::json;
namespace json_loader
{
    std::string x = "x";
    std::string y = "y";
    std::string w = "w";
    std::string h = "h";
    std::string id = "id";
    std::string offsetX = "offsetX";
    std::string offsetY = "offsetY";

    std::string x0 = "x0";
    std::string y0 = "y0";
    std::string x1 = "x1";
    std::string y1 = "y1";
    std::string timeDelta = "timeDelta";
    std::string userName = "userName";
    std::string mapId = "mapId";

    std::string name = "name";
    std::string file = "file";
    std::string type = "type";
    std::string rotation = "rotation";
    std::string color = "color";
    std::string scale = "scale";
    std::string value = "value";
    std::string dog_Speed = "dogSpeed";
    std::string bag_Capacity = "bagCapacity";
    std::string roads_key = "roads";
    std::string buildings_key = "buildings";
    std::string offices_key = "offices";
    std::string lootTypes = "lootTypes";
    std::string default_Dog_Speed = "defaultDogSpeed";
    std::string default_Dog_Retirement = "dogRetirementTime";
    std::string lootGeneratorConfig = "lootGeneratorConfig";
    std::string probability = "probability";
    std::string bagCapacityDefault = "defaultBagCapacity";
    std::string maps = "maps";
    std::string move_key = "move";
    std::string period = "period";
    boost::json::string directionRight = "R";
    boost::json::string directionLeft = "L";
    boost::json::string directionDown = "D";
    boost::json::string directionUp = "U";

    model::Road ParseRoad(const json::object &road_object)
    {
        if (road_object.contains("x1"))
        {
            model::Road road(model::Road::HORIZONTAL, model::Point(road_object.at(x0).as_int64(), road_object.at(y0).as_int64()),
                             road_object.at(x1).as_int64());
            return road;
        }
        model::Road road(model::Road::VERTICAL, model::Point(road_object.at(x0).as_int64(), road_object.at(y0).as_int64()),
                         road_object.at(y1).as_int64());
        return road;
    }

    model::Building ParseBuilding(const json::object &building_object)
    {
        model::Building building(model::Rectangle(model::Point(building_object.at(x).as_int64(),
                                                               building_object.at(y).as_int64()),
                                                  model::Size(building_object.at(w).as_int64(),
                                                              building_object.at(h).as_int64())));
        return building;
    }

    model::Office ParseOffice(const json::object &office_object)
    {
        model::Office office(model::Office::Id(office_object.at(id).as_string().data()),
                             model::Point(office_object.at(x).as_int64(),
                                          office_object.at(y).as_int64()),
                             model::Offset(office_object.at(offsetX).as_int64(),
                                           office_object.at(offsetY).as_int64()));
        return office;
    }

    model::Loot ParseLoot(const json::object &loot_object)
    {
        model::Loot loot(loot_object.at(name).as_string().data(),
                         loot_object.at(file).as_string().data(),
                         loot_object.at(type).as_string().data(),
                         loot_object.contains(rotation) ? loot_object.at(rotation).as_int64() : -1,
                         loot_object.contains(color) ? loot_object.at(color).as_string().data() : "",
                         loot_object.at(scale).as_double(),
                         loot_object.contains(value) ? loot_object.at(value).as_int64() : 0);

        return loot;
    }

    template <typename T>
    std::vector<T> ParseObjects(const json::object &object, const std::string &object_tag,
                                std::function<T(const json::object &office_object)> parseFunc)
    {
        std::vector<T> objects;

        auto objectsArray = object.at(object_tag).as_array();
        for (auto index = 0; index < objectsArray.size(); ++index)
        {
            auto curObject = objectsArray.at(index).as_object();
            objects.emplace_back(parseFunc(curObject));
        }

        return objects;
    }

    model::Map ParseMapObject(const json::object &map_object)
    {
        auto idVal = map_object.at(id).as_string();
        auto nameVal = map_object.at(name).as_string();

        model::Map map(model::Map::Id(idVal.data()), nameVal.data());

        if (map_object.contains(dog_Speed))
        {
            double dogSpeed = map_object.at(dog_Speed).as_double();
            map.SetDogSpeed(dogSpeed);
        }

        if (map_object.contains(bag_Capacity))
        {
            unsigned bag_capacity = map_object.at(bag_Capacity).as_int64();
            map.SetBagCapacity(bag_capacity);
        }

        auto roads = ParseObjects<model::Road>(map_object, roads_key, ParseRoad);
        for (const auto &road : roads)
        {
            map.AddRoad(road);
        }

        auto buildings = ParseObjects<model::Building>(map_object, buildings_key, ParseBuilding);
        for (const auto &building : buildings)
        {
            map.AddBuilding(building);
        }

        auto offices = ParseObjects<model::Office>(map_object, offices_key, ParseOffice);
        for (const auto &office : offices)
        {
            map.AddOffice(office);
        }

        auto loots = ParseObjects<model::Loot>(map_object, lootTypes, ParseLoot);
        for (const auto &loot : loots)
        {
            map.AddLoot(loot);
        }
        return map;
    }

    void ParseMaps(const std::filesystem::path &json_path, model::Game &game)
    {
        if (!std::filesystem::exists(json_path))
            throw std::filesystem::filesystem_error(std::string("File not exists:") + json_path.c_str(), std::error_code());

        std::ifstream input(json_path);
        std::stringstream sstr;

        while (input >> sstr.rdbuf())
            ;
        auto value = json::parse(sstr.str());

        json::object object_to_read = value.as_object();
        if (object_to_read.contains(default_Dog_Speed))
        {
            double defaultDogSpeed = object_to_read.at(default_Dog_Speed).as_double();
            game.SetDefaultDogSpeed(defaultDogSpeed);
        }

        if (object_to_read.contains(default_Dog_Retirement))
        {
            double dogRetirementTime = object_to_read.at(default_Dog_Retirement).as_double();
            game.SetDogRetirementTime(dogRetirementTime);
        }

        if (object_to_read.contains(lootGeneratorConfig))
        {
            auto loot = value.at(lootGeneratorConfig).as_object();
            game.SetLootParameters(loot.at(period).as_double(), loot.at(probability).as_double());
        }

        unsigned defaultBagCapacity = 3;
        if (object_to_read.contains(bagCapacityDefault))
            defaultBagCapacity = object_to_read.at(bagCapacityDefault).as_int64();

        game.SetDefaultBagCapacity(defaultBagCapacity);

        auto arr = value.at(maps).as_array();

        for (auto index = 0; index < arr.size(); ++index)
        {
            auto curMap = arr.at(index).as_object();
            game.AddMap(ParseMapObject(curMap));
        }
    }

    model::Game LoadGame(const std::filesystem::path &json_path, const std::filesystem::path &base_path)
    {
        // Загрузить содержимое файла json_path, например, в виде строки
        // Распарсить строку как JSON, используя boost::json::parse
        // Загрузить модель игры из файла
        model::Game game;
        try
        {
            ParseMaps(json_path, game);
            game.AddBasePath(base_path);
        }
        catch (const std::exception &ex)
        {
            std::cerr << "Exception:" << ex.what() << std::endl;
        }

        return game;
    }

    std::map<std::string, std::string> ParseJoinGameRequest(const std::string &body)
    {
        std::map<std::string, std::string> result;

        auto value = json::parse(body);

        result[userName] = value.at(userName).as_string();
        result[mapId] = value.at(mapId).as_string();

        return result;
    }

    DogDirection GetMoveDirection(const std::string &body)
    {
        auto value = json::parse(body);

        auto direction = value.at(move_key).as_string();

        const auto getDirection = [](const boost::json::string &direct) -> DogDirection
        {
            std::map<boost::json::string, DogDirection> dir{{directionRight, DogDirection::EAST},
                                                            {
                                                                directionLeft,
                                                                DogDirection::WEST,
                                                            },
                                                            {directionDown, DogDirection::SOUTH},
                                                            {directionUp, DogDirection::NORTH},
                                                            {"", DogDirection::STOP}};
            auto itFind = dir.find(direct);
            return itFind->second;
        };

        return getDirection(direction);
    }

    int ParseDeltaTimeRequest(const std::string &body)
    {
        int result{};
        try
        {
            auto json_object = json::parse(body).as_object();
            if (!json_object.contains(timeDelta))
                throw BadDeltaTimeException();

            auto delta = json_object.at(timeDelta).as_int64();
            result = static_cast<int>(delta);
            return result;
        }
        catch (const std::exception &ex)
        {
            throw BadDeltaTimeException();
        }
    }

} // namespace json_loader
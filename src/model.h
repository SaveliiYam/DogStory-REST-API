#pragma once
#include <vector>
#include <filesystem>
#include "tagged.h"
#include "game_session.h"
#include <memory>
#include <functional>

namespace model
{
    class Player;
    class GameSession;
}

namespace model
{

    using Dimension = int;
    using Coord = Dimension;

    struct Point
    {
        Point(Coord newx, Coord newy) : x{newx}, y{newy}
        {
        }
        bool operator==(const Point &other)
        {
            return (x == other.x) && (y == other.y);
        }

        bool operator!=(const Point &other)
        {
            return (x != other.x) || (y != other.y);
        }
        Coord x, y;
    };

    struct Size
    {
        Size(Dimension w, Dimension h) : width{w}, height{h}
        {
        }
        Dimension width, height;
    };

    struct Rectangle
    {
        Rectangle(const Point &pnt, const Size &sz) : position{pnt}, size{sz}
        {
        }
        Point position;
        Size size;
    };

    struct Offset
    {
        Offset(Dimension x, Dimension y) : dx{x}, dy{y}
        {
        }
        Dimension dx, dy;
    };

    class Road
    {
        struct HorizontalTag
        {
            explicit HorizontalTag() = default;
        };

        struct VerticalTag
        {
            explicit VerticalTag() = default;
        };

    public:
        constexpr static HorizontalTag HORIZONTAL{};
        constexpr static VerticalTag VERTICAL{};

        Road(HorizontalTag, Point start, Coord end_x) noexcept
            : start_{start}, end_{end_x, start.y}
        {
        }

        Road(VerticalTag, Point start, Coord end_y) noexcept
            : start_{start}, end_{start.x, end_y}
        {
        }

        bool IsHorizontal() const noexcept
        {
            return start_.y == end_.y;
        }

        bool IsVertical() const noexcept
        {
            return start_.x == end_.x;
        }

        Point GetStart() const noexcept
        {
            return start_;
        }

        Point GetEnd() const noexcept
        {
            return end_;
        }

    private:
        Point start_;
        Point end_;
    };

    class Building
    {
    public:
        explicit Building(Rectangle bounds) noexcept
            : bounds_{bounds} {}

        const Rectangle &GetBounds() const noexcept
        {
            return bounds_;
        }

    private:
        Rectangle bounds_;
    };

    class Office
    {
    public:
        using Id = util::Tagged<std::string, Office>;

        Office(Id id, Point position, Offset offset) noexcept
            : id_{std::move(id)}, position_{position}, offset_{offset}
        {
        }

        const Id &GetId() const noexcept
        {
            return id_;
        }

        Point GetPosition() const noexcept
        {
            return position_;
        }

        Offset GetOffset() const noexcept
        {
            return offset_;
        }

    private:
        Id id_;
        Point position_;
        Offset offset_;
    };

    class Map
    {
    public:
        using Id = util::Tagged<std::string, Map>;
        using Roads = std::vector<Road>;
        using Buildings = std::vector<Building>;
        using Offices = std::vector<Office>;

        Map(Id id, std::string name) noexcept
            : id_(std::move(id)), name_(std::move(name))
        {
        }

        const Id &GetId() const noexcept
        {
            return id_;
        }

        const std::string &GetName() const noexcept
        {
            return name_;
        }

        const Buildings &GetBuildings() const noexcept
        {
            return buildings_;
        }

        const Roads &GetRoads() const noexcept
        {
            return roads_;
        }

        const Offices &GetOffices() const noexcept
        {
            return offices_;
        }

        void AddRoad(const Road &road)
        {
            roads_.emplace_back(road);
        }

        void AddBuilding(const Building &building)
        {
            buildings_.emplace_back(building);
        }

        void AddOffice(Office office);
        void SetDogSpeed(double speed) { dog_speed_ = speed; }
        double GetDogSpeed() const { return dog_speed_; }

    private:
        using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

        Id id_;
        std::string name_;
        Roads roads_;
        Buildings buildings_;

        OfficeIdToIndex warehouse_id_to_index_;
        Offices offices_;
        double dog_speed_{0.0};
    };

    class Game
    {
    public:
        using Maps = std::vector<Map>;
        using PlayerAuthInfo = std::pair<std::string, unsigned int>;
        void AddMap(Map map);

        void AddBasePath(const std::filesystem::path &base_path)
        {
            base_path_ = base_path;
        }

        const std::filesystem::path &GetBasePath()
        {
            return base_path_;
        }

        const Maps &GetMaps() const noexcept
        {
            return maps_;
        }

        const Map *FindMap(const Map::Id &id) const noexcept
        {
            if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end())
            {
                return &maps_.at(it->second);
            }
            return nullptr;
        }

        const std::vector<std::shared_ptr<Player>> FindAllPlayersForAuthInfo(const std::string &auth_token);
        std::shared_ptr<Player> GetPlayerWithAuthToken(const std::string &auth_token);
        bool HasSessionWithAuthInfo(const std::string &auth_token);
        std::shared_ptr<GameSession> GetSessionWithAuthInfo(const std::string &auth_token);
        Game::PlayerAuthInfo AddPlayer(const std::string &map_id, const std::string &player_name);
        void SetDefaultDogSpeed(double speed) { default_dog_speed_ = speed; }
        double GetDefaultDogSpeed() { return default_dog_speed_; }
        void MoveDogs(int deltaTime);
        void SetTickPeriod(int period) { tick_period_ = period; }
        void SetSpawnInRandomPoint(bool random_spawn) { spawn_in_random_points_ = random_spawn; }
        int GetTickPeriod() { return tick_period_; }
        bool GetSpawnInRandomPoint() { return spawn_in_random_points_; }
        size_t GetNumPlayersInAllSessions();

    private:
        std::shared_ptr<GameSession> FindSession(const std::string &map_name);

    private:
        using MapIdHasher = util::TaggedHasher<Map::Id>;
        using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

        std::vector<Map> maps_;
        MapIdToIndex map_id_to_index_;
        std::filesystem::path base_path_;
        std::vector<std::shared_ptr<GameSession>> sessions_;
        double default_dog_speed_{0.0};
        int tick_period_{-1};
        bool spawn_in_random_points_{false};
    };
}
// namespace model

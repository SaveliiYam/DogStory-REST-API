#pragma once
#include <vector>
#include <filesystem>
#include "tagged.h"
#include <memory>
#include <functional>

namespace model
{
    class Player;
    class GameSession;
    struct GameSessionsStates;
}
using RetiredSessionPlayers = std::pair<std::shared_ptr<model::GameSession>, std::vector<std::shared_ptr<model::Player>>>;
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

    struct LootInfo
    {
        LootInfo(unsigned _id, unsigned _type, double coordx, double coordy)
            : id{_id}, type{_type}, x{coordx}, y{coordy}
        {
        }
        LootInfo() {}
        unsigned id{};
        unsigned type{};
        double x{};
        double y{};
    };

    class Road
    {
        struct HorizontalTag
        {
            HorizontalTag() = default;
        };

        struct VerticalTag
        {
            VerticalTag() = default;
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

    class Loot
    {
    public:
        Loot(std::string name, std::string file, std::string type,
             Coord rotation, std::string color, double scale, int score) noexcept
            : name_{name}, file_{file}, type_{type}, rotation_{rotation}, color_{color}, scale_{scale}, score_{score}
        {
        }

        const std::string &GetName() const noexcept { return name_; }
        const std::string &GetFile() const noexcept { return file_; }
        const std::string &GetType() const noexcept { return type_; }
        Coord GetRotation() const noexcept { return rotation_; }
        const std::string &GetColor() const noexcept { return color_; }
        double GetScale() const noexcept { return scale_; }
        int GetScore() const noexcept { return score_; }

    private:
        std::string name_;
        std::string file_;
        std::string type_;
        Coord rotation_{-1};
        std::string color_;
        double scale_{};
        int score_{};
    };

    class Map
    {
    public:
        using Id = util::Tagged<std::string, Map>;
        using Roads = std::vector<Road>;
        using Buildings = std::vector<Building>;
        using Offices = std::vector<Office>;
        using Loots = std::vector<Loot>;
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

        const size_t GetNumRoads() const noexcept
        {
            return roads_.size();
        }

        const Offices &GetOffices() const noexcept
        {
            return offices_;
        }

        const Loots &GetLoots() const noexcept
        {
            return loots_;
        }

        void AddRoad(const Road &road)
        {
            roads_.emplace_back(road);
        }

        void AddBuilding(const Building &building)
        {
            buildings_.emplace_back(building);
        }

        void AddOffice(const Office &office);
        void AddLoot(Loot loot);
        size_t GetNumLoots() const noexcept { return loots_.size(); }
        void SetDogSpeed(double speed) { dog_speed_ = speed; }
        double GetDogSpeed() const { return dog_speed_; }
        void SetBagCapacity(unsigned capacity) { bag_capacity_ = capacity; }
        unsigned GetBagCapacity() const noexcept { return bag_capacity_; }

    private:
        using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

        Id id_;
        std::string name_;
        Roads roads_;
        Buildings buildings_;

        OfficeIdToIndex warehouse_id_to_index_;
        Offices offices_;
        Loots loots_;
        double dog_speed_{0.0};
        unsigned bag_capacity_{};
    };

    struct DogPosition
    {
        DogPosition(double newx, double newy) : x{newx}, y{newy} {}
        double x{0.0};
        double y{0.0};
    };

    struct DogSpeed
    {
        double vx{0.0};
        double vy{0.0};
    };

    struct DogPos
    {
        size_t current_road_index{0};
        DogPosition curr_position{0.0, 0.0};
        DogSpeed curr_speed{0.0, 0.0};
    };

    enum class DogDirection
    {
        NORTH,
        SOUTH,
        WEST,
        EAST,
        STOP
    };

    struct PlayerRecordItem
    {
        std::string id;
        std::string name;
        int score;
        int playTime;
    };

    class Game
    {
    public:
        using Maps = std::vector<Map>;
        using PlayerAuthInfo = std::pair<std::string, unsigned int>;
        void AddMap(const Map &map);

        void AddBasePath(const std::filesystem::path &base_path)
        {
            base_path_ = base_path;
        }

        void AddSavePath(const std::filesystem::path &save_path)
        {
            save_path_ = save_path;
        }

        const std::filesystem::path &GetBasePath()
        {
            return base_path_;
        }

        const std::filesystem::path &GetSavePath() const
        {
            return save_path_;
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
        const std::vector<LootInfo> GetLootsForAuthInfo(const std::string &auth_token);
        std::shared_ptr<Player> GetPlayerWithAuthToken(const std::string &auth_token);
        bool HasSessionWithAuthInfo(const std::string &auth_token);
        std::shared_ptr<GameSession> GetSessionWithAuthInfo(const std::string &auth_token);
        Game::PlayerAuthInfo AddPlayer(const std::string &map_id, const std::string &player_name);
        void SetDefaultDogSpeed(double speed) { default_dog_speed_ = speed; }
        double GetDefaultDogSpeed() { return default_dog_speed_; }
        void SetDogRetirementTime(double ret_time) { dog_retierement_time_ = ret_time * 1000; }
        void MoveDogs(int deltaTime);
        void GenerateLoot(int deltaTime);
        void SetTickPeriod(int period) { tick_period_ = period; }
        int GetTickPeriod() { return tick_period_; }
        void SetSpawnInRandomPoint(bool random_spawn) { spawn_in_random_points_ = random_spawn; }
        void SetSavePeriod(int period) { save_period_ = period; }
        int GetSavePeriod() { return save_period_; }
        bool GetSpawnInRandomPoint() { return spawn_in_random_points_; }
        size_t GetNumPlayersInAllSessions();
        void SetLootParameters(double period, double probability);
        std::pair<double, double> GetLootParameters() { return {loot_period_, loot_probability_}; }
        void SetDefaultBagCapacity(unsigned capacity) { default_bag_capacity_ = capacity; }
        std::shared_ptr<GameSessionsStates> GetGameSessionsStates() const;
        void SaveSessions(int deltaTime);
        void RestoreSessions(const model::GameSessionsStates &sessions);
        std::vector<PlayerRecordItem> GetRecords(int start, int max_items) const;
        void HandleRetiredPlayers();

    private:
        std::shared_ptr<GameSession> FindSession(const std::string &map_name);
        std::shared_ptr<GameSession> GetSessionForToken(const std::string &auth_token);
        std::vector<RetiredSessionPlayers> FindExpiredPlayers();
        void SaveExpiredPlayers(const std::vector<RetiredSessionPlayers> &expired_sessions_players);
        void DeleteExpiredPlayers(const std::vector<RetiredSessionPlayers> &expired_sessions_players);

    private:
        using MapIdHasher = util::TaggedHasher<Map::Id>;
        using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

        std::vector<Map> maps_;
        MapIdToIndex map_id_to_index_;
        std::filesystem::path base_path_;
        std::filesystem::path save_path_;
        std::vector<std::shared_ptr<GameSession>> sessions_;
        double default_dog_speed_{0.0};
        double dog_retierement_time_{60.0 * 1000};
        int tick_period_{-1};
        int save_period_{0};
        bool spawn_in_random_points_{false};
        double loot_period_{};
        double loot_probability_{};
        unsigned default_bag_capacity_{};
    };
}
// namespace model
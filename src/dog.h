#pragma once
#include "model.h"
#include <optional>

using namespace model;

namespace collision_detector
{
    enum class ItemType;
    struct GatheringEvent;
    struct Gatherer;
}

namespace model
{
    struct Point;
    class Map;
    class Road;
    struct LootInfo;
    enum class RoadType
    {
        Parallel,
        Adjacent,
        Crossed
    };

    struct RoadInfo
    {
        size_t road_index{};
        RoadType road_type{RoadType::Parallel};
        RoadInfo(size_t index, RoadType rdType) : road_index(index), road_type(rdType) {}
    };

    std::string ConvertDogDirectionToString(DogDirection direction);

    class DogNavigator
    {
    public:
        DogNavigator(const std::vector<model::Road> &roads, bool spawn_dog_in_random_point) : roads_(roads)
        {
            adjacent_roads_ = std::vector<std::vector<RoadInfo>>(roads.size());
            FindAdjacentRoads();
            if (spawn_dog_in_random_point)
            {
                SetStartPositionRandomRoad();
            }
            else
            {
                SetStartPositionFirstRoad();
            }
        }

    public:
        void MoveDog(DogDirection direction, int time);
        DogPos GetDogPosOnMap() { return dog_info_; }
        void SetDogPosOnMap(const DogPos &position) { dog_info_ = position; }
        DogPosition GetDogPosition() { return dog_info_.curr_position; }
        DogSpeed GetDogSpeed() { return dog_info_.curr_speed; }
        void SpawnDogInMap(bool spawn_in_random_point);
        void SetDogSpeed(const DogSpeed &speed) { dog_info_.curr_speed = speed; }

    private:
        void FindNewPosMovingHorizontal(const model::Road &road, DogPosition &newPos);
        void FindNewPosMovingVertical(const model::Road &road, DogPosition &newPos);

        void FindAdjacentRoads();
        void SetStartPositionFirstRoad();
        bool RoadsCrossed(const model::Road &road1, const model::Road &road2);
        bool RoadsAdjacent(const model::Road &road1, const model::Road &road2);
        std::optional<size_t> FindNearestAdjacentVerticalRoad(const DogPosition &edge_point);
        std::optional<size_t> FindNearestVerticalCrossRoad(const DogPosition &newPos);
        void FindNewPosPerpendicularHorizontal(const model::Road &road, DogDirection direction, DogPosition &newPos);
        std::optional<size_t> FindNearestHorizontalCrossRoad(const DogPosition &newPos);
        std::optional<size_t> FindNearestAdjacentHorizontalRoad(const DogPosition &edge_point);
        void FindNewPosPerpendicularVertical(const model::Road &road, DogDirection direction, DogPosition &newPos);
        void SetStartPositionRandomRoad();
        void CorrectDogPosition();

    private:
        const std::vector<model::Road> &roads_;
        std::vector<std::vector<RoadInfo>> adjacent_roads_;
        DogPos dog_info_;
    };

    class Dog
    {
    public:
        Dog(const model::Map *map, bool spawn_dog_in_random_point, unsigned defaultBagCapacity);
        void SetSpeed(DogDirection dir, double speed);
        void SetDirection(const DogDirection &dir) { direction_ = dir; }
        std::optional<collision_detector::Gatherer> Move(int deltaTime);
        DogDirection GetDirection() const { return direction_; }
        DogPosition GetPosition() const { return navigator_->GetDogPosition(); }
        void SetPositionOnMap(const DogPos &position) { navigator_->SetDogPosOnMap(position); }
        DogPos GetPositionOnMap() { return navigator_->GetDogPosOnMap(); }
        DogSpeed GetSpeed() { return navigator_->GetDogSpeed(); }
        void SpawnDogInMap(bool spawn_in_random_point) { navigator_->SpawnDogInMap(spawn_in_random_point); }
        const std::vector<model::LootInfo> &GetGatheredLoot() const { return gathered_loots_; }
        void SetGatheredLoot(const std::vector<model::LootInfo> &loots) { gathered_loots_ = loots; }
        bool AddLoot(const model::LootInfo &loot);
        void PassLootToOffice();
        int GetScore() const noexcept { return score_; }
        void SetScore(int score) { score_ = score; }
        unsigned GetBagCapacity() const { return bag_capacity_; }
        void SetBagCapacity(unsigned capacity) { bag_capacity_ = capacity; }
        unsigned int GetIdleTime() const { return idle_time_; }
        unsigned int GetPlayTime() const { return play_time_; }
        void SetPlayTime(unsigned int time) { play_time_ = time; }

    private:
        DogDirection direction_;
        const model::Map *map_;
        std::shared_ptr<model::DogNavigator> navigator_;
        std::vector<model::LootInfo> gathered_loots_;
        unsigned bag_capacity_{};
        int score_{0};
        unsigned int idle_time_{0};
        unsigned int play_time_{0};
    };
}
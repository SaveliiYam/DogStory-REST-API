#include "dog.h"
#include <map>
#include "server_exceptions.h"
#include "utils.h"
#include "collision_detector.h"

constexpr double dS = 0.4;
constexpr int millisescondsInSecond = 1000;
constexpr double gathererWidth = 0.6;
constexpr double epsilon = 0.0001;
namespace model
{
	std::string ConvertDogDirectionToString(DogDirection direction)
	{
		std::map<DogDirection, std::string> dir{{DogDirection::EAST, "R"},
												{DogDirection::WEST, "L"},
												{DogDirection::SOUTH, "D"},
												{DogDirection::NORTH, "U"}};

		if (auto itFind = dir.find(direction); itFind != dir.end())
			return itFind->second;

		return "U";
	}

	Dog::Dog(const model::Map *map, bool spawn_dog_in_random_point, unsigned defaultBagCapacity) : map_(map)
	{
		bag_capacity_ = map->GetBagCapacity() ? map->GetBagCapacity() : defaultBagCapacity;
		direction_ = DogDirection::NORTH;
		navigator_ = std::make_shared<DogNavigator>(map_->GetRoads(), spawn_dog_in_random_point);
	}

	void Dog::SetSpeed(DogDirection dir, double speed)
	{
		std::map<DogDirection, DogSpeed> velMap{{DogDirection::EAST, {speed, 0.0}}, {DogDirection::WEST, {-speed, 0.0}}, {DogDirection::SOUTH, {0.0, speed}}, {DogDirection::NORTH, {0.0, -speed}}, {DogDirection::STOP, {0.0, 0.0}}};

		auto find_vel = velMap.find(dir);
		if (find_vel == velMap.end())
			throw DogSpeedException();

		if (dir != DogDirection::STOP)
			direction_ = dir;
		idle_time_ = 0;
		navigator_->SetDogSpeed(find_vel->second);
	}

	std::optional<collision_detector::Gatherer> Dog::Move(int deltaTime)
	{
		std::optional<collision_detector::Gatherer> res;
		play_time_ += deltaTime;
		auto speed = navigator_->GetDogSpeed();
		if ((std::abs(speed.vx) <= epsilon) && (std::abs(speed.vy) <= epsilon))
		{
			idle_time_ += deltaTime;
			return res;
		}
		else
			idle_time_ = 0;

		DogPosition start = GetPosition();
		navigator_->MoveDog(direction_, deltaTime);

		DogPosition end = GetPosition();

		if ((std::abs(start.x - end.x) > epsilon) || (std::abs(start.y - end.y) > epsilon))
		{
			collision_detector::Gatherer gth({start.x, start.y}, {end.x, end.y}, gathererWidth);
			res = gth;
		}

		return res;
	}

	bool Dog::AddLoot(const model::LootInfo &loot)
	{
		if (gathered_loots_.size() >= bag_capacity_)
			return false;

		gathered_loots_.push_back(loot);
		return true;
	}

	void Dog::PassLootToOffice()
	{
		const auto &loots = map_->GetLoots();

		for (const auto &loot : gathered_loots_)
		{
			score_ += loots[loot.type].GetScore();
		}
		gathered_loots_.clear();
	}

	bool DogNavigator::RoadsCrossed(const model::Road &road1, const model::Road &road2)
	{
		if ((road1.IsHorizontal() && road2.IsVertical()) || (road1.IsVertical() && road2.IsHorizontal()))
		{
			const auto &first_start = road1.GetStart();

			const auto &second_start = road2.GetStart();
			const auto &second_end = road2.GetEnd();

			if (road1.IsHorizontal())
			{
				if (((second_start.y < first_start.y) && (second_end.y < first_start.y)) ||
					((second_start.y > first_start.y) && (second_end.y > first_start.y)))
					return false;
			}
			else if (road1.IsVertical())
			{
				if (((second_start.x < first_start.x) && (second_end.x < first_start.x)) ||
					((second_start.x > first_start.x) && (second_end.x > first_start.x)))
					return false;
			}
			return true;
		}
		return false;
	}

	bool DogNavigator::RoadsAdjacent(const model::Road &road1, const model::Road &road2)
	{
		if ((road1.IsHorizontal() && road2.IsHorizontal()) || (road1.IsVertical() && road2.IsVertical()))
		{
			auto first_start = road1.GetStart();
			auto first_end = road1.GetEnd();

			auto second_start = road2.GetStart();
			auto second_end = road2.GetEnd();

			if ((first_start == second_start) || (first_start == second_end) ||
				(first_end == second_start) || (first_end == second_end))
				return true;
		}
		return false;
	}

	void DogNavigator::FindAdjacentRoads()
	{
		for (size_t i = 0; i < roads_.size(); ++i)
		{
			for (size_t j = i + 1; j < roads_.size(); ++j)
			{
				RoadType road_type{RoadType::Parallel};

				if (RoadsAdjacent(roads_[i], roads_[j]))
					road_type = RoadType::Adjacent;
				else if (RoadsCrossed(roads_[i], roads_[j]))
					road_type = RoadType::Crossed;

				if (road_type != RoadType::Parallel)
				{
					adjacent_roads_[i].push_back(RoadInfo(j, road_type));
					adjacent_roads_[j].push_back(RoadInfo(i, road_type));
				}
			}
		}
	}

	void DogNavigator::SetStartPositionFirstRoad()
	{
		dog_info_.current_road_index = 0;
		auto start = roads_[dog_info_.current_road_index].GetStart();
		dog_info_.curr_position = DogPosition(start.x, start.y);
	}

	void DogNavigator::SetStartPositionRandomRoad()
	{
		dog_info_.current_road_index = utils::GetRandomNumber<size_t>(0, roads_.size() - 1);
		auto start = roads_[dog_info_.current_road_index].GetStart();
		auto end = roads_[dog_info_.current_road_index].GetEnd();

		if (roads_[dog_info_.current_road_index].IsHorizontal())
		{
			if (start.x > end.x)
				std::swap(start, end);
			dog_info_.curr_position = DogPosition(utils::GetRandomNumber<int>(start.x, end.x), start.y);
		}
		else
		{
			if (start.y > end.y)
				std::swap(start, end);
			dog_info_.curr_position = DogPosition(start.x, utils::GetRandomNumber<int>(start.y, end.y));
		}
	}

	void DogNavigator::FindNewPosMovingHorizontal(const model::Road &road, DogPosition &newPos)
	{
		auto start = road.GetStart();
		auto end = road.GetEnd();

		if (start.x > end.x)
			std::swap(start, end);

		double minXLeft = static_cast<double>(start.x) - dS;
		double maxXRight = static_cast<double>(end.x) + dS;

		if (newPos.x < minXLeft)
		{
			newPos.x = minXLeft;
			dog_info_.curr_speed = {0.0, 0.0};
		}
		else if (newPos.x > maxXRight)
		{
			newPos.x = maxXRight;
			dog_info_.curr_speed = {0.0, 0.0};
		}
		dog_info_.curr_position = newPos;
	}

	void DogNavigator::FindNewPosMovingVertical(const model::Road &road, DogPosition &newPos)
	{
		auto start = road.GetStart();
		auto end = road.GetEnd();

		if (start.y > end.y)
			std::swap(start, end);
		double minyDown = static_cast<double>(start.y) - dS;
		double maxyUp = static_cast<double>(end.y) + dS;

		if (newPos.y < minyDown)
		{
			newPos.y = minyDown;
			dog_info_.curr_speed = {0.0, 0.0};
		}
		else if (newPos.y > maxyUp)
		{
			newPos.y = maxyUp;
			dog_info_.curr_speed = {0.0, 0.0};
		}
		dog_info_.curr_position = newPos;
	}

	std::optional<size_t> DogNavigator::FindNearestVerticalCrossRoad(const DogPosition &newPos)
	{
		std::optional<size_t> res;

		const auto &adj_roads = adjacent_roads_[dog_info_.current_road_index];
		for (const auto &road_info : adj_roads)
		{
			if (road_info.road_type != RoadType::Crossed)
				continue;

			const auto &adj_road = roads_[road_info.road_index];

			if ((newPos.y < static_cast<double>(adj_road.GetStart().y)) && (newPos.y < static_cast<double>(adj_road.GetEnd().y)))
				continue;

			if ((newPos.y > static_cast<double>(adj_road.GetStart().y)) && (newPos.y > static_cast<double>(adj_road.GetEnd().y)))
				continue;

			double dist = std::abs(static_cast<double>(adj_road.GetStart().x) - newPos.x);

			if (dist <= dS)
			{
				res = road_info.road_index;
				return res;
			}
		}

		return res;
	}

	std::optional<size_t> DogNavigator::FindNearestAdjacentVerticalRoad(const DogPosition &edge_point)
	{
		std::optional<size_t> res;

		const auto &adj_roads = adjacent_roads_[dog_info_.current_road_index];
		for (const auto &road_info : adj_roads)
		{
			if (road_info.road_type != RoadType::Crossed)
				continue;

			const auto &adj_road = roads_[road_info.road_index]; // dog_info_.current_road_index];
			if (!adj_road.IsVertical())
				continue;

			double dist1 = std::abs(static_cast<double>(adj_road.GetStart().x) - edge_point.x);
			double dist2 = std::abs(static_cast<double>(adj_road.GetEnd().x) - edge_point.x);

			if ((dist1 <= dS) || (dist2 <= dS))
			{
				res = road_info.road_index;
				return res;
			}
		}

		return res;
	}

	void DogNavigator::FindNewPosPerpendicularHorizontal(const model::Road &road, DogDirection direction,
														 DogPosition &newPos)
	{
		std::optional<size_t> adjRoad = FindNearestAdjacentVerticalRoad(newPos);
		bool findRoad = false;
		if (adjRoad)
		{
			const auto &road_cand = roads_[*adjRoad];
			if (direction == DogDirection::NORTH)
			{
				if ((road_cand.GetStart().y <= road.GetStart().y) && (road_cand.GetEnd().y <= road.GetStart().y))
					findRoad = true;
			}
			else
			{
				if ((road_cand.GetStart().y >= road.GetStart().y) && (road_cand.GetEnd().y >= road.GetStart().y))
					findRoad = true;
			}
			if (findRoad)
			{
				dog_info_.curr_position = newPos;
				dog_info_.current_road_index = *adjRoad;
				CorrectDogPosition();
				return;
			}
		}
		adjRoad = FindNearestVerticalCrossRoad(newPos);
		if (adjRoad)
		{
			dog_info_.curr_position = newPos;
			dog_info_.current_road_index = *adjRoad;
			CorrectDogPosition();
			return;
		}

		if (std::abs(newPos.y - static_cast<double>(road.GetStart().y)) >= dS)
		{
			if (direction == DogDirection::NORTH)
				newPos.y = static_cast<double>(road.GetStart().y) - dS;
			else
				newPos.y = static_cast<double>(road.GetStart().y) + dS;
			dog_info_.curr_speed = {0.0, 0.0};
		}

		dog_info_.curr_position = newPos;
	}

	std::optional<size_t> DogNavigator::FindNearestAdjacentHorizontalRoad(const DogPosition &edge_point)
	{
		std::optional<size_t> res;

		const auto &adj_roads = adjacent_roads_[dog_info_.current_road_index];
		for (const auto &road_info : adj_roads)
		{
			if (road_info.road_type != RoadType::Crossed)
				continue;

			const auto &adj_road = roads_[road_info.road_index];
			if (!adj_road.IsHorizontal())
				continue;

			double dist1 = std::abs(static_cast<double>(adj_road.GetStart().y) - edge_point.y);
			double dist2 = std::abs(static_cast<double>(adj_road.GetEnd().y) - edge_point.y);

			if ((dist1 <= dS) || (dist2 <= dS))
			{
				res = road_info.road_index;
				return res;
			}
		}

		return res;
	}

	std::optional<size_t> DogNavigator::FindNearestHorizontalCrossRoad(const DogPosition &newPos)
	{
		std::optional<size_t> res;

		const auto &adj_roads = adjacent_roads_[dog_info_.current_road_index];
		for (const auto &road_info : adj_roads)
		{
			if (road_info.road_type != RoadType::Crossed)
				continue;

			const auto &adj_road = roads_[road_info.road_index];

			if ((newPos.x < static_cast<double>(adj_road.GetStart().x)) && (newPos.x < static_cast<double>(adj_road.GetEnd().x)))
				continue;

			if ((newPos.x > static_cast<double>(adj_road.GetStart().x)) && (newPos.x > static_cast<double>(adj_road.GetEnd().x)))
				continue;

			double dist = std::abs(static_cast<double>(adj_road.GetStart().y) - newPos.y);

			if (dist <= dS)
			{
				return road_info.road_index;
			}
		}

		return res;
	}

	void DogNavigator::FindNewPosPerpendicularVertical(const model::Road &road, DogDirection direction, DogPosition &newPos)
	{
		std::optional<size_t> adjRoad = FindNearestAdjacentHorizontalRoad(newPos);
		bool findRoad = false;
		if (adjRoad)
		{
			const auto &road_cand = roads_[*adjRoad];
			if (direction == DogDirection::WEST)
			{
				if ((road_cand.GetStart().x <= road.GetStart().x) && (road_cand.GetEnd().x <= road.GetStart().x))
					findRoad = true;
			}
			else
			{
				if ((road_cand.GetStart().x >= road.GetStart().x) && (road_cand.GetEnd().x >= road.GetStart().x))
					findRoad = true;
			}
			if (findRoad)
			{
				dog_info_.curr_position = newPos;
				dog_info_.current_road_index = *adjRoad;
				return;
			}
		}
		adjRoad = FindNearestHorizontalCrossRoad(newPos);
		if (adjRoad)
		{
			dog_info_.curr_position = newPos;
			dog_info_.current_road_index = *adjRoad;
			return;
		}

		if (std::abs(newPos.x - static_cast<double>(road.GetStart().x)) > dS)
		{
			if (direction == DogDirection::WEST)
				newPos.x = static_cast<double>(road.GetStart().x) - dS;
			else
				newPos.x = static_cast<double>(road.GetStart().x) + dS;
			dog_info_.curr_speed = {0.0, 0.0};
		}

		dog_info_.curr_position = newPos;
	}

	void DogNavigator::MoveDog(DogDirection direction, int time)
	{
		const auto &road = roads_[dog_info_.current_road_index];
		double dt = static_cast<double>(time) / millisescondsInSecond;

		DogPosition newPos{dog_info_.curr_position.x + dt * dog_info_.curr_speed.vx,
						   dog_info_.curr_position.y + dt * dog_info_.curr_speed.vy};

		if (road.IsHorizontal())
		{
			if ((direction == DogDirection::WEST) || (direction == DogDirection::EAST))
				FindNewPosMovingHorizontal(road, newPos);
			else
				FindNewPosPerpendicularHorizontal(road, direction, newPos);
		}
		else
		{
			if ((direction == DogDirection::NORTH) || (direction == DogDirection::SOUTH))
				FindNewPosMovingVertical(road, newPos);
			else
				FindNewPosPerpendicularVertical(road, direction, newPos);
		}
	}

	void DogNavigator::SpawnDogInMap(bool spawn_in_random_point)
	{
		if (spawn_in_random_point)
			SetStartPositionRandomRoad();
	}

	void DogNavigator::CorrectDogPosition()
	{
		const auto &road = roads_[dog_info_.current_road_index];

		auto [x_min, x_max] = std::minmax({road.GetStart().x, road.GetEnd().x});
		auto [y_min, y_max] = std::minmax({road.GetStart().y, road.GetEnd().y});

		if (dog_info_.curr_position.x < (static_cast<double>(x_min) - dS))
		{
			dog_info_.curr_position.x = static_cast<double>(x_min) - dS;
			dog_info_.curr_speed = {0.0, 0.0};
		}
		if (dog_info_.curr_position.x > (static_cast<double>(x_max) + dS))
		{
			dog_info_.curr_position.x = static_cast<double>(x_max) + dS;
			dog_info_.curr_speed = {0.0, 0.0};
		}
		if (dog_info_.curr_position.y < (static_cast<double>(y_min) - dS))
		{
			dog_info_.curr_position.y = static_cast<double>(y_min) - dS;
			dog_info_.curr_speed = {0.0, 0.0};
		}
		if (dog_info_.curr_position.y > (static_cast<double>(y_max) + dS))
		{
			dog_info_.curr_position.y = static_cast<double>(y_max) + dS;
			dog_info_.curr_speed = {0.0, 0.0};
		}
	}
}
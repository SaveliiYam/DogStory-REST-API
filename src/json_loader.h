#include <map>
#include "model.h"

namespace model
{
	enum class DogDirection;
}
namespace json_loader
{

	model::Game LoadGame(const std::filesystem::path &json_path, const std::filesystem::path &base_path);
	std::map<std::string, std::string> ParseJoinGameRequest(const std::string &body);
	model::DogDirection GetMoveDirection(const std::string &body);
	int ParseDeltaTimeRequest(const std::string &body);
} // namespace json_loader

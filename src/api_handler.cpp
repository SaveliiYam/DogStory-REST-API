#include "api_handler.h"
#include <set>

namespace http_handler
{

	const std::string game_endpoint = "/api/v1/game/join";
	const std::string players_endpoint = "/api/v1/game/players";
	const std::string state_endpoint = "/api/v1/game/state";
	const std::string action_endpoint = "/api/v1/game/player/action";
	const std::string tick_endpoint = "/api/v1/game/tick";

	const std::map<std::string, std::string> onlyPostMethodAllowedResp{{"code", "invalidMethod"}, {"message", "Only POST method is expected"}};

	const std::map<std::string, std::string> joinGameReqParseError{{"code", "invalidArgument"}, {"message", "Join game request parse error"}};

	const std::map<std::string, std::string> invalidNameResp{{"code", "invalidArgument"}, {"message", "Invalid name"}};

	const std::map<std::string, std::string> invaliMethodResp{{"code", "invalidMethod"}, {"message", "Invalid method"}};

	const std::map<std::string, std::string> authHeaderMissingResp{{"code", "invalidToken"}, {"message", "Authorization header is missing"}};

	const std::map<std::string, std::string> playerTokenNotFoundResp{{"code", "unknownToken"}, {"message", "Player token has not been found"}};

	const std::map<std::string, std::string> authHeaderRequiredResp{{"code", "invalidToken"}, {"message", "Authorization header is required"}};

	const std::map<std::string, std::string> invalidEndpointResp{{"code", "badRequest"}, {"message", "Invalid endpoint"}};

	const std::map<std::string, std::string> failedToParseTickResp{{"code", "invalidArgument"}, {"message", "Failed to parse tick request JSON"}};

	std::string GetAuthToken(std::string_view auth)
	{
		std::string_view prefix = "Bearer"sv;

		if (auth.find_first_of(prefix) == std::string_view::npos)
		{
			return "";
		}
		auth.remove_prefix(prefix.size());

		size_t pos = 0;
		do
		{
			pos = auth.find_first_of(' ');
			if (pos == 0)
				auth.remove_prefix(1);
		} while (pos != std::string_view::npos);

		return std::string(auth.begin(), auth.end());
	}

	StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
									  bool keep_alive, std::string_view content_type,
									  const std::initializer_list<std::pair<http::field, std::string_view>> &addition_headers)
	{
		StringResponse response(status, http_version);
		response.set(http::field::content_type, content_type);

		for (auto it = addition_headers.begin(); it != addition_headers.end(); ++it)
			response.set(it->first, it->second);

		response.body() = body;
		response.content_length(body.size());
		response.keep_alive(keep_alive);
		return response;
	}

	bool ApiHandler::IsApiRequest(const std::string &request)
	{
		std::set<std::string> api_endpoints{game_endpoint, players_endpoint, state_endpoint, action_endpoint, tick_endpoint};

		return api_endpoints.find(request) != api_endpoints.end();
	}

	StringResponse ApiHandler::HandleApiRequest(const std::string &request, http::verb method, std::string_view auth_type, const std::string &body, unsigned http_version, bool keep_alive)
	{
		StringResponse resp;
		auto it_handler = resp_map_.find(request);
		if (it_handler != resp_map_.end())
			return it_handler->second(method, auth_type, body, http_version, keep_alive);

		return resp;
	}

	void ApiHandler::InitApiRequestHandlers()
	{
		auto join_game_handler = [this](http::verb method, std::string_view auth_type, const std::string &body, unsigned http_version, bool keep_alive) -> StringResponse
		{
			return HandleJoinGameRequest(method, auth_type, body, http_version, keep_alive);
		};

		auto get_players_handler = [this](http::verb method, std::string_view auth_type, const std::string &body, unsigned http_version, bool keep_alive) -> StringResponse
		{
			return HandleGetPlayersRequest(method, auth_type, body, http_version, keep_alive);
		};

		auto get_state_handler = [this](http::verb method, std::string_view auth_type, const std::string &body, unsigned http_version, bool keep_alive) -> StringResponse
		{
			return HandleGetGameState(method, auth_type, body, http_version, keep_alive);
		};

		auto action_handler = [this](http::verb method, std::string_view auth_type, const std::string &body, unsigned http_version, bool keep_alive) -> StringResponse
		{
			return HandlePlayerAction(method, auth_type, body, http_version, keep_alive);
		};

		auto tick_handler = [this](http::verb method, std::string_view auth_type, const std::string &body, unsigned http_version, bool keep_alive) -> StringResponse
		{
			return HandleTickAction(method, auth_type, body, http_version, keep_alive);
		};

		resp_map_[game_endpoint] = join_game_handler;
		resp_map_[players_endpoint] = get_players_handler;
		resp_map_[state_endpoint] = get_state_handler;
		resp_map_[action_endpoint] = action_handler;
		resp_map_[tick_endpoint] = tick_handler;
	}

	StringResponse ApiHandler::HandleJoinGameRequest(http::verb method, std::string_view auth_type, const std::string &body, unsigned http_version, bool keep_alive)
	{
		StringResponse resp;
		if (method == http::verb::post)
		{
			resp = HandleAuthRequest(body, http_version, keep_alive);
		}
		else if (method == http::verb::head)
		{
			resp = MakeStringResponse(http::status::method_not_allowed, ""sv,
									  http_version, keep_alive,
									  ContentType::APPLICATION_JSON,
									  {{http::field::cache_control, CacheType::NO_CACHE},
									   {http::field::allow, HeaderType::ALLOW_POST}});
		}
		else
		{
			resp = MakeStringResponse(http::status::method_not_allowed,
									  json_serializer::MakeMappedResponce(onlyPostMethodAllowedResp),
									  http_version, keep_alive, ContentType::APPLICATION_JSON,
									  {{http::field::cache_control, CacheType::NO_CACHE},
									   {http::field::allow, HeaderType::ALLOW_POST}});
		}

		return resp;
	}

	StringResponse ApiHandler::HandleAuthRequest(const std::string &body, unsigned http_version, bool keep_alive)
	{
		std::map<std::string, std::string> respMap;
		try
		{
			respMap = json_loader::ParseJoinGameRequest(body);
		}
		catch (std::exception &e)
		{
			auto resp = MakeStringResponse(http::status::bad_request,
										   json_serializer::MakeMappedResponce(joinGameReqParseError),
										   http_version, keep_alive, ContentType::APPLICATION_JSON,
										   {{http::field::cache_control, CacheType::NO_CACHE}});
			return resp;
		}
		try
		{
			auto [token, playerId] = game_.AddPlayer(respMap["mapId"], respMap["userName"]);
			auto resp = MakeStringResponse(http::status::ok,
										   json_serializer::MakeAuthResponce(token, playerId), http_version,
										   keep_alive, ContentType::APPLICATION_JSON,
										   {{http::field::cache_control, CacheType::NO_CACHE}});

			if ((game_.GetNumPlayersInAllSessions() == 1) && ticker_)
			{
				ticker_->Start();
			}
			return resp;
		}
		catch (MapNotFoundException &e)
		{
			cout << e.what() << std::endl;
			auto resp = MakeStringResponse(http::status::not_found,
										   json_serializer::MakeMapNotFoundResponce(),
										   http_version, keep_alive,
										   ContentType::APPLICATION_JSON,
										   {{http::field::cache_control, CacheType::NO_CACHE}});
			return resp;
		}
		catch (EmptyNameException &e)
		{
			auto resp = MakeStringResponse(http::status::bad_request,
										   json_serializer::MakeMappedResponce(invalidNameResp),
										   http_version, keep_alive, ContentType::APPLICATION_JSON, {{http::field::cache_control, CacheType::NO_CACHE}});
			return resp;
		}
	}

	StringResponse ApiHandler::HandleGetPlayersRequest(http::verb method, std::string_view auth_type, const std::string &body, unsigned http_version, bool keep_alive)
	{
		if ((method != http::verb::get) && (method != http::verb::head))
		{
			auto resp = MakeStringResponse(http::status::method_not_allowed,
										   json_serializer::MakeMappedResponce(invaliMethodResp),
										   http_version, keep_alive, ContentType::APPLICATION_JSON,
										   {{http::field::cache_control, CacheType::NO_CACHE},
											{http::field::allow, HeaderType::ALLOW_HEADERS}});
			return resp;
		}

		std::string auth_token = GetAuthToken(auth_type);

		if (auth_token.empty())
		{
			auto resp = MakeStringResponse(http::status::unauthorized,
										   json_serializer::MakeMappedResponce(authHeaderMissingResp),
										   http_version, keep_alive, ContentType::APPLICATION_JSON,
										   {{http::field::cache_control, CacheType::NO_CACHE}});

			return resp;
		}
		if (!game_.HasSessionWithAuthInfo(auth_token))
		{
			auto resp = MakeStringResponse(http::status::unauthorized,
										   json_serializer::MakeMappedResponce(playerTokenNotFoundResp),
										   http_version, keep_alive, ContentType::APPLICATION_JSON,
										   {{http::field::cache_control, CacheType::NO_CACHE}});
			return resp;
		}

		auto players = game_.FindAllPlayersForAuthInfo(auth_token);
		StringResponse resp;
		if (method == http::verb::get)
			resp = MakeStringResponse(http::status::ok, json_serializer::GetPlayerInfoResponce(players), http_version, keep_alive, ContentType::APPLICATION_JSON, {{http::field::cache_control, "no-cache"sv}});
		else
			resp = MakeStringResponse(http::status::ok, "", http_version, keep_alive, ContentType::APPLICATION_JSON, {{http::field::cache_control, CacheType::NO_CACHE}});
		return resp;
	}

	bool IsValidAuthToken(const std::string &token, size_t valid_size)
	{
		if (token.size() != valid_size)
			return false;

		for (auto i = 0; i < token.size(); ++i)
			if ((token[i] < '0') || (token[i] > '9') || (token[i] < 'a') || (token[i] > 'f'))
				return false;

		return true;
	}

	StringResponse ApiHandler::HandleGetGameState(http::verb method, std::string_view auth_type, const std::string &body, unsigned http_version, bool keep_alive)
	{
		if ((method != http::verb::get) && (method != http::verb::head))
		{
			auto resp = MakeStringResponse(http::status::method_not_allowed,
										   json_serializer::MakeMappedResponce(invaliMethodResp),
										   http_version, keep_alive, ContentType::APPLICATION_JSON,
										   {{http::field::cache_control, CacheType::NO_CACHE},
											{http::field::allow, HeaderType::ALLOW_HEADERS}});
			return resp;
		}
		std::string auth_token = GetAuthToken(auth_type);
		if (auth_token.empty() || !game_.HasSessionWithAuthInfo(auth_token))
		{
			StringResponse resp;
			if (auth_token.empty() || !IsValidAuthToken(auth_token, 32) /*(auth_token.size() != 32)*/)
				resp = MakeStringResponse(http::status::unauthorized,
										  json_serializer::MakeMappedResponce(authHeaderMissingResp),
										  http_version, keep_alive, ContentType::APPLICATION_JSON,
										  {{http::field::cache_control, CacheType::NO_CACHE}});
			else
				resp = MakeStringResponse(http::status::unauthorized,
										  json_serializer::MakeMappedResponce(playerTokenNotFoundResp),
										  http_version, keep_alive, ContentType::APPLICATION_JSON,
										  {{http::field::cache_control, CacheType::NO_CACHE}});
			return resp;
		}

		auto players = game_.FindAllPlayersForAuthInfo(auth_token);
		if (method == http::verb::get)
		{
			auto resp = MakeStringResponse(http::status::ok, json_serializer::GetPlayersDogInfoResponce(players), http_version, keep_alive, ContentType::APPLICATION_JSON, {{http::field::cache_control, CacheType::NO_CACHE}});
			return resp;
		}
		else
		{
			auto resp = MakeStringResponse(http::status::ok, "", http_version, keep_alive, ContentType::APPLICATION_JSON, {{http::field::cache_control, CacheType::NO_CACHE}});
			return resp;
		}
	}

	StringResponse ApiHandler::HandlePlayerAction(http::verb method, std::string_view auth_type, const std::string &body, unsigned http_version, bool keep_alive)
	{
		if (method != http::verb::post)
		{
			auto resp = MakeStringResponse(http::status::method_not_allowed,
										   json_serializer::MakeMappedResponce(invaliMethodResp),
										   http_version, keep_alive, ContentType::APPLICATION_JSON,
										   {{http::field::cache_control, CacheType::NO_CACHE}});

			return resp;
		}
		std::string auth_token = GetAuthToken(auth_type);
		if (auth_token.empty())
		{
			auto resp = MakeStringResponse(http::status::unauthorized,
										   json_serializer::MakeMappedResponce(authHeaderRequiredResp),
										   http_version, keep_alive, ContentType::APPLICATION_JSON,
										   {{http::field::cache_control, CacheType::NO_CACHE}});

			return resp;
		}
		else if (!game_.HasSessionWithAuthInfo(auth_token))
		{
			auto resp = MakeStringResponse(http::status::unauthorized,
										   json_serializer::MakeMappedResponce(playerTokenNotFoundResp),
										   http_version, keep_alive, ContentType::APPLICATION_JSON,
										   {{http::field::cache_control, CacheType::NO_CACHE}});

			return resp;
		}

		auto player = game_.GetPlayerWithAuthToken(auth_token);
		auto session = game_.GetSessionWithAuthInfo(auth_token);
		auto map = game_.FindMap(model::Map::Id(session->GetMap()));
		auto map_speed = map->GetDogSpeed();

		player->GetDog()->SpawnDogInMap(game_.GetSpawnInRandomPoint());
		model::DogDirection dir = json_loader::GetMoveDirection(body);
		player->GetDog()->SetSpeed(dir, map_speed > 0.0 ? map_speed : game_.GetDefaultDogSpeed());

		auto resp = MakeStringResponse(http::status::ok, "{}", http_version, keep_alive, ContentType::APPLICATION_JSON, {{http::field::cache_control, CacheType::NO_CACHE}});
		return resp;
	}

	StringResponse ApiHandler::HandleTickAction(http::verb method, std::string_view auth_type, const std::string &body, unsigned http_version, bool keep_alive)
	{
		StringResponse resp;

		if (method != http::verb::post)
		{
			resp = MakeStringResponse(http::status::method_not_allowed,
									  json_serializer::MakeMappedResponce(invaliMethodResp),
									  http_version, keep_alive, ContentType::APPLICATION_JSON,
									  {{http::field::cache_control, CacheType::NO_CACHE}});

			return resp;
		}
		if (ticker_)
		{
			resp = MakeStringResponse(http::status::bad_request,
									  json_serializer::MakeMappedResponce(invalidEndpointResp),
									  http_version, keep_alive, ContentType::APPLICATION_JSON,
									  {{http::field::cache_control, CacheType::NO_CACHE}});
		}
		else
		{
			try
			{
				int deltaTime = json_loader::ParseDeltaTimeRequest(body);
				// std::cout << "HandleTickAction:" << deltaTime << std::endl;
				game_.MoveDogs(deltaTime);
				resp = MakeStringResponse(http::status::ok, "{}", http_version, keep_alive, ContentType::APPLICATION_JSON, {{http::field::cache_control, "no-cache"sv}});
			}
			catch (BadDeltaTimeException &ex)
			{
				resp = MakeStringResponse(http::status::bad_request,
										  json_serializer::MakeMappedResponce(failedToParseTickResp),
										  http_version, keep_alive, ContentType::APPLICATION_JSON,
										  {{http::field::cache_control, CacheType::NO_CACHE}});
			}
		}

		return resp;
	}

} // namespace http_handler

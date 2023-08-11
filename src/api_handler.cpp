#include "api_handler.h"
#include "game_session.h"
#include <set>
#include <boost/url.hpp>
#include "utility_functions.h"
using namespace boost::urls;

namespace http_handler
{
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
		std::string np_request = GetRequestStringWithoutParameters(request);
		std::set<std::string> api_endpoints{std::string(Endpoints::game_endpoint), std::string(Endpoints::players_endpoint),
											std::string(Endpoints::state_endpoint), std::string(Endpoints::action_endpoint), std::string(Endpoints::tick_endpoint),
											std::string(Endpoints::records_endpoint)};

		return api_endpoints.find(np_request) != api_endpoints.end();
	}

	std::map<std::string, std::string> GetRequestParameters(const std::string &request)
	{
		url_view u(request);
		std::map<std::string, std::string> result;

		for (auto param : u.params())
		{
			result[param.key] = param.value;
		}

		return result;
	}

	StringResponse ApiHandler::HandleApiRequest(const std::string &request, http::verb method, std::string_view auth_type, const std::string &body, unsigned http_version, bool keep_alive)
	{
		StringResponse resp;
		std::string np_request = GetRequestStringWithoutParameters(request);
		auto it_handler = resp_map_.find(np_request);
		if (it_handler != resp_map_.end())
		{
			auto parameters = GetRequestParameters(request);
			return it_handler->second(method, auth_type, body, http_version, keep_alive, parameters);
		}
		return resp;
	}

	void ApiHandler::InitApiRequestHandlers()
	{
		auto join_game_handler = [this](http::verb method, std::string_view auth_type, const std::string &body,
										unsigned http_version, bool keep_alive, const std::map<std::string, std::string> &params) -> StringResponse
		{
			return HandleJoinGameRequest(method, auth_type, body, http_version, keep_alive, params);
		};

		auto get_players_handler = [this](http::verb method, std::string_view auth_type, const std::string &body,
										  unsigned http_version, bool keep_alive, const std::map<std::string, std::string> &params) -> StringResponse
		{
			return HandleGetPlayersRequest(method, auth_type, body, http_version, keep_alive, params);
		};

		auto get_state_handler = [this](http::verb method, std::string_view auth_type, const std::string &body,
										unsigned http_version, bool keep_alive, const std::map<std::string, std::string> &params) -> StringResponse
		{
			return HandleGetGameState(method, auth_type, body, http_version, keep_alive, params);
		};

		auto action_handler = [this](http::verb method, std::string_view auth_type, const std::string &body,
									 unsigned http_version, bool keep_alive, const std::map<std::string, std::string> &params) -> StringResponse
		{
			return HandlePlayerAction(method, auth_type, body, http_version, keep_alive, params);
		};

		auto tick_handler = [this](http::verb method, std::string_view auth_type, const std::string &body,
								   unsigned http_version, bool keep_alive, const std::map<std::string, std::string> &params) -> StringResponse
		{
			return HandleTickAction(method, auth_type, body, http_version, keep_alive, params);
		};

		auto records_handler = [this](http::verb method, std::string_view auth_type, const std::string &body,
									  unsigned http_version, bool keep_alive, const std::map<std::string, std::string> &params) -> StringResponse
		{
			return HandleGetRecordsAction(method, auth_type, body, http_version, keep_alive, params);
		};
		resp_map_[std::string(Endpoints::game_endpoint)] = join_game_handler;
		resp_map_[std::string(Endpoints::players_endpoint)] = get_players_handler;
		resp_map_[std::string(Endpoints::state_endpoint)] = get_state_handler;
		resp_map_[std::string(Endpoints::action_endpoint)] = action_handler;
		resp_map_[std::string(Endpoints::tick_endpoint)] = tick_handler;
		resp_map_[std::string(Endpoints::records_endpoint)] = records_handler;
	}

	StringResponse ApiHandler::HandleJoinGameRequest(http::verb method, std::string_view auth_type, const std::string &body,
													 unsigned http_version, bool keep_alive, const std::map<std::string, std::string> &params)
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
									  {{http::field::cache_control, "no-cache"sv},
									   {http::field::allow, HeaderType::ALLOW_POST}});
		}
		else
		{
			resp = MakeStringResponse(http::status::method_not_allowed,
									  json_serializer::MakeMappedResponce(onlyPostMethodAllowedResp),
									  http_version, keep_alive, ContentType::APPLICATION_JSON,
									  {{http::field::cache_control, "no-cache"sv},
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
										   {{http::field::cache_control, "no-cache"sv}});
			return resp;
		}
		try
		{
			auto [token, playerId] = game_.AddPlayer(respMap["mapId"], respMap["userName"]);

			auto player = game_.GetPlayerWithAuthToken(token);
			player->GetDog()->SpawnDogInMap(game_.GetSpawnInRandomPoint());

			auto resp = MakeStringResponse(http::status::ok,
										   json_serializer::MakeAuthResponce(token, playerId), http_version,
										   keep_alive, ContentType::APPLICATION_JSON,
										   {{http::field::cache_control, "no-cache"sv}});

			if (ticker_ && !ticker_->HasStarted())
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
										   {{http::field::cache_control, "no-cache"sv}});
			return resp;
		}
		catch (EmptyNameException &e)
		{
			auto resp = MakeStringResponse(http::status::bad_request,
										   json_serializer::MakeMappedResponce(invalidNameResp),
										   http_version, keep_alive, ContentType::APPLICATION_JSON,
										   {{http::field::cache_control, "no-cache"sv}});
			return resp;
		}
	}

	StringResponse ApiHandler::HandleGetPlayersRequest(http::verb method, std::string_view auth_type,
													   const std::string &body, unsigned http_version, bool keep_alive,
													   const std::map<std::string, std::string> &params)
	{
		if ((method != http::verb::get) && (method != http::verb::head))
		{
			return MakeStringResponse(http::status::method_not_allowed,
									  json_serializer::MakeMappedResponce(invaliMethodResp),
									  http_version, keep_alive, ContentType::APPLICATION_JSON,
									  {{http::field::cache_control, "no-cache"sv},
									   {http::field::allow, HeaderType::ALLOW_HEADERS}});
		}

		std::string auth_token = GetAuthToken(auth_type);

		if (auth_token.empty())
		{
			return MakeStringResponse(http::status::unauthorized,
									  json_serializer::MakeMappedResponce(authHeaderMissingResp),
									  http_version, keep_alive, ContentType::APPLICATION_JSON,
									  {{http::field::cache_control, "no-cache"sv}});
		}
		if (!game_.HasSessionWithAuthInfo(auth_token))
		{
			return MakeStringResponse(http::status::unauthorized,
									  json_serializer::MakeMappedResponce(playerTokenNotFoundResp),
									  http_version, keep_alive, ContentType::APPLICATION_JSON,
									  {{http::field::cache_control, "no-cache"sv}});
		}

		auto players = game_.FindAllPlayersForAuthInfo(auth_token);
		StringResponse resp;
		if (method == http::verb::get)
			resp = MakeStringResponse(http::status::ok, json_serializer::GetPlayerInfoResponce(players), http_version, keep_alive,
									  ContentType::APPLICATION_JSON, {{http::field::cache_control, "no-cache"sv}});
		else
			resp = MakeStringResponse(http::status::ok, "", http_version, keep_alive,
									  ContentType::APPLICATION_JSON, {{http::field::cache_control, "no-cache"sv}});
		return resp;
	}

	bool IsValidAuthToken(const std::string &token, size_t valid_size)
	{
		if (token.size() != valid_size)
			return false;
		for (const auto i : token)
			if ((token[i] < '0') || (token[i] > '9') || (token[i] < 'a') || (token[i] > 'f'))
				return false;

		return true;
	}

	StringResponse ApiHandler::HandleGetGameState(http::verb method, std::string_view auth_type, const std::string &body,
												  unsigned http_version, bool keep_alive, const std::map<std::string, std::string> &params)
	{
		if ((method != http::verb::get) && (method != http::verb::head))
		{
			auto resp = MakeStringResponse(http::status::method_not_allowed,
										   json_serializer::MakeMappedResponce(invaliMethodResp),
										   http_version, keep_alive, ContentType::APPLICATION_JSON,
										   {{http::field::cache_control, "no-cache"sv},
											{http::field::allow, HeaderType::ALLOW_HEADERS}});
			return resp;
		}
		std::string auth_token = GetAuthToken(auth_type);
		if (auth_token.empty() || !game_.HasSessionWithAuthInfo(auth_token))
		{
			StringResponse resp;
			if (auth_token.empty() || !IsValidAuthToken(auth_token, 32))
				return MakeStringResponse(http::status::unauthorized,
										  json_serializer::MakeMappedResponce(authHeaderMissingResp),
										  http_version, keep_alive, ContentType::APPLICATION_JSON,
										  {{http::field::cache_control, "no-cache"sv}});

			return MakeStringResponse(http::status::unauthorized,
									  json_serializer::MakeMappedResponce(playerTokenNotFoundResp),
									  http_version, keep_alive, ContentType::APPLICATION_JSON,
									  {{http::field::cache_control, "no-cache"sv}});
		}

		if (method == http::verb::get)
		{
			auto players = game_.FindAllPlayersForAuthInfo(auth_token);
			auto loots = game_.GetLootsForAuthInfo(auth_token);
			auto resp = MakeStringResponse(http::status::ok, json_serializer::GetPlayersDogInfoResponce(players, loots),
										   http_version, keep_alive, ContentType::APPLICATION_JSON,
										   {{http::field::cache_control, "no-cache"sv}});
			return resp;
		}
		else
		{
			auto resp = MakeStringResponse(http::status::ok, "", http_version, keep_alive,
										   ContentType::APPLICATION_JSON, {{http::field::cache_control, "no-cache"sv}});
			return resp;
		}
	}

	StringResponse ApiHandler::HandlePlayerAction(http::verb method, std::string_view auth_type,
												  const std::string &body, unsigned http_version,
												  bool keep_alive, const std::map<std::string, std::string> &params)
	{
		if (method != http::verb::post)
		{
			auto resp = MakeStringResponse(http::status::method_not_allowed,
										   json_serializer::MakeMappedResponce(invaliMethodResp),
										   http_version, keep_alive, ContentType::APPLICATION_JSON,
										   {{http::field::cache_control, "no-cache"sv}});

			return resp;
		}
		std::string auth_token = GetAuthToken(auth_type);
		if (auth_token.empty())
		{
			auto resp = MakeStringResponse(http::status::unauthorized,
										   json_serializer::MakeMappedResponce(authHeaderRequiredResp),
										   http_version, keep_alive, ContentType::APPLICATION_JSON,
										   {{http::field::cache_control, "no-cache"sv}});

			return resp;
		}
		else if (!game_.HasSessionWithAuthInfo(auth_token))
		{
			auto resp = MakeStringResponse(http::status::unauthorized,
										   json_serializer::MakeMappedResponce(playerTokenNotFoundResp),
										   http_version, keep_alive, ContentType::APPLICATION_JSON,
										   {{http::field::cache_control, "no-cache"sv}});

			return resp;
		}

		auto session = game_.GetSessionWithAuthInfo(auth_token);
		auto map = game_.FindMap(model::Map::Id(session->GetMap()));
		auto map_speed = map->GetDogSpeed();
		auto player = game_.GetPlayerWithAuthToken(auth_token);

		DogDirection dir = json_loader::GetMoveDirection(body);
		player->GetDog()->SetSpeed(dir, map_speed > 0.0 ? map_speed : game_.GetDefaultDogSpeed());

		auto resp = MakeStringResponse(http::status::ok, "{}", http_version, keep_alive, ContentType::APPLICATION_JSON,
									   {{http::field::cache_control, "no-cache"sv}});
		return resp;
	}

	StringResponse ApiHandler::HandleTickAction(http::verb method, std::string_view auth_type,
												const std::string &body, unsigned http_version,
												bool keep_alive, const std::map<std::string, std::string> &params)
	{
		StringResponse resp;

		if (method != http::verb::post)
		{
			resp = MakeStringResponse(http::status::method_not_allowed,
									  json_serializer::MakeMappedResponce(invaliMethodResp),
									  http_version, keep_alive, ContentType::APPLICATION_JSON,
									  {{http::field::cache_control, "no-cache"sv}});

			return resp;
		}
		if (ticker_)
		{
			resp = MakeStringResponse(http::status::bad_request,
									  json_serializer::MakeMappedResponce(invalidEndpointResp),
									  http_version, keep_alive, ContentType::APPLICATION_JSON,
									  {{http::field::cache_control, "no-cache"sv}});
		}
		else
		{
			try
			{
				int deltaTime = json_loader::ParseDeltaTimeRequest(body);

				game_.GenerateLoot(deltaTime);
				game_.MoveDogs(deltaTime);
				game_.SaveSessions(deltaTime);
				game_.HandleRetiredPlayers();
				resp = MakeStringResponse(http::status::ok, "{}", http_version, keep_alive,
										  ContentType::APPLICATION_JSON, {{http::field::cache_control, "no-cache"sv}});
			}
			catch (BadDeltaTimeException &ex)
			{
				resp = MakeStringResponse(http::status::bad_request,
										  json_serializer::MakeMappedResponce(failedToParseTickResp),
										  http_version, keep_alive, ContentType::APPLICATION_JSON,
										  {{http::field::cache_control, "no-cache"sv}});
			}
		}

		return resp;
	}

	std::pair<int, int> ParseParameters(const std::map<std::string, std::string> &params)
	{
		int start = 0;
		int max_items = MAX_DB_RECORDS;

		try
		{
			auto it = params.find("start");
			if (it != params.end())
				start = std::stoi(it->second);

			it = params.find("maxItems");
			if (it != params.end())
				max_items = std::stoi(it->second);
		}
		catch (std::exception &ex)
		{
		}
		return {start, max_items};
	}

	StringResponse ApiHandler::HandleGetRecordsAction(http::verb method, std::string_view auth_type,
													  const std::string &body, unsigned http_version,
													  bool keep_alive, const std::map<std::string, std::string> &params)
	{
		StringResponse resp;

		if ((method != http::verb::get) && (method != http::verb::head))
		{
			resp = MakeStringResponse(http::status::method_not_allowed,
									  json_serializer::MakeMappedResponce(invaliMethodResp),
									  http_version, keep_alive, ContentType::APPLICATION_JSON,
									  {{http::field::cache_control, "no-cache"sv}});

			return resp;
		}
		auto [start, max_items] = ParseParameters(params);

		if (max_items > MAX_DB_RECORDS)
			resp = MakeStringResponse(http::status::bad_request,
									  json_serializer::MakeMappedResponce(invalidNameResp),
									  http_version, keep_alive, ContentType::APPLICATION_JSON,
									  {{http::field::cache_control, "no-cache"sv}});
		else
			resp = MakeStringResponse(http::status::ok, json_serializer::MakeRecordsResponce(game_, start, max_items), http_version, keep_alive,
									  ContentType::APPLICATION_JSON, {{http::field::cache_control, "no-cache"sv}});

		return resp;
	}

} // namespace http_handler
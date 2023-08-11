#pragma once
#include "http_server.h"
#include "model.h"
#include "json_serializer.h"
#include "json_loader.h"
#include "event_logger.h"
#include "server_exceptions.h"
#include <boost/asio/io_context.hpp>
#include "ticker.h"

namespace net = boost::asio;

namespace http_handler
{

    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace sys = boost::system;

    using namespace std::literals;

    using StringResponse = http::response<http::string_body>;
    using Strand = boost::asio::strand<boost::asio::io_context::executor_type>;

    struct Endpoints
    {
        Endpoints() = delete;
        constexpr static std::string_view game_endpoint = "/api/v1/game/join";
        constexpr static std::string_view players_endpoint = "/api/v1/game/players";
        constexpr static std::string_view state_endpoint = "/api/v1/game/state";
        constexpr static std::string_view action_endpoint = "/api/v1/game/player/action";
        constexpr static std::string_view tick_endpoint = "/api/v1/game/tick";
        constexpr static std::string_view records_endpoint = "/api/v1/game/records";
    };

    struct ContentType
    {
        ContentType() = delete;
        constexpr static std::string_view APPLICATION_JSON = "application/json"sv;
        constexpr static std::string_view TEXT_PLAIN = "text/plain"sv;
    };

    struct HeaderType
    {
        HeaderType() = delete;
        constexpr static std::string_view ALLOW_HEADERS = "GET, HEAD"sv;
        constexpr static std::string_view ALLOW_POST = "POST"sv;
    };

    StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                      bool keep_alive, std::string_view content_type = ContentType::APPLICATION_JSON,
                                      const std::initializer_list<std::pair<http::field, std::string_view>> &addition_headers = {});

    class ApiHandler
    {
    public:
        explicit ApiHandler(model::Game &game, Strand &strand) : game_{game}, strand_{strand}
        {
            InitApiRequestHandlers();
            if (game_.GetTickPeriod() > 0)
            {
                ticker_ = std::make_shared<Ticker>(strand_, std::chrono::milliseconds(game_.GetTickPeriod()),
                                                   [this](std::chrono::milliseconds ticks)
                                                   {
                                                       game_.GenerateLoot(ticks.count());
                                                       game_.MoveDogs(ticks.count());
                                                       game_.SaveSessions(ticks.count());
                                                       game_.HandleRetiredPlayers();
                                                   });
            }
        }

        ApiHandler(const ApiHandler &) = delete;
        ApiHandler &operator=(const ApiHandler &) = delete;

        bool IsApiRequest(const std::string &request);
        StringResponse HandleApiRequest(const std::string &request, http::verb method, std::string_view auth_type,
                                        const std::string &body, unsigned http_version, bool keep_alive);

    private:
        void InitApiRequestHandlers();
        StringResponse HandleJoinGameRequest(http::verb method, std::string_view auth_type,
                                             const std::string &body, unsigned http_version, bool keep_alive, const std::map<std::string, std::string> &params);
        StringResponse HandleAuthRequest(const std::string &body, unsigned http_version, bool keep_alive);
        StringResponse HandleGetPlayersRequest(http::verb method, std::string_view auth_type, const std::string &body,
                                               unsigned http_version, bool keep_alive, const std::map<std::string, std::string> &params);
        StringResponse HandleGetGameState(http::verb method, std::string_view auth_type, const std::string &body,
                                          unsigned http_version, bool keep_alive, const std::map<std::string, std::string> &params);
        StringResponse HandlePlayerAction(http::verb method, std::string_view auth_type, const std::string &body,
                                          unsigned http_version, bool keep_alive, const std::map<std::string, std::string> &params);
        StringResponse HandleTickAction(http::verb method, std::string_view auth_type, const std::string &body,
                                        unsigned http_version, bool keep_alive, const std::map<std::string, std::string> &params);

        StringResponse HandleGetRecordsAction(http::verb method, std::string_view auth_type, const std::string &body,
                                              unsigned http_version, bool keep_alive, const std::map<std::string, std::string> &params);

    private:
        model::Game &game_;
        std::map<std::string,
                 std::function<StringResponse(http::verb, std::string_view, const std::string &, unsigned, bool, const std::map<std::string, std::string> &)>>
            resp_map_;
        std::shared_ptr<Ticker> ticker_;
        Strand &strand_;
    };
} // namespace http_handler
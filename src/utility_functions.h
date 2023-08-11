#pragma once
#include <boost/program_options.hpp>
#include "model.h"
#include "tagged_uuid.h"
#include "postgres.h"
#include "connection_engine.h"

struct Args
{
    int tick_period{0};
    int save_period{0};
    std::string config_file;
    std::string www_root;
    std::string save_file;
    bool spawn_random_points{false};
};

struct AppConfig
{
    std::string db_url;
};
const int MAX_DB_RECORDS = 100;

namespace
{

    [[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char *const argv[])
    {
        namespace po = boost::program_options;

        po::options_description desc{"All options"s};
        Args args;
        std::string tick_period;
        std::string save_period;
        desc.add_options()                                                                                     //
            ("help,h", "produce help message")                                                                 //
            ("tick-period,t", po::value(&tick_period)->value_name("milliseconds"s), " set tick period")        //
            ("config-file,c", po::value(&args.config_file)->value_name("file"s), "set config file path")       //
            ("www-root,w", po::value(&args.www_root)->value_name("dir"s), "set static files root")             //
            ("randomize-spawn-points", "spawn dogs at random positions")                                       //
            ("state-file,f", po::value(&args.save_file)->value_name("file"s), "set file to save server state") //
            ("save-state-period,p", po::value(&save_period)->value_name("milliseconds"s), "time period to save server state in milliseconds");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.contains("help"s))
        {
            std::cout << desc;
            return std::nullopt;
        }

        if (!vm.contains("config-file"s))
        {
            throw std::runtime_error("Config file path has not been specified"s);
        }

        if (!vm.contains("www-root"s))
        {
            throw std::runtime_error("Static files root path is not specified"s);
        }

        if (vm.contains("tick-period"s))
        {
            args.tick_period = std::stoi(tick_period);
        }

        if (vm.contains("state-file"s) && vm.contains("save-state-period"s))
        {
            args.save_period = std::stoi(save_period);
        }

        args.spawn_random_points = vm.contains("randomize-spawn-points"s) ? true : false;

        return args;
    }

    AppConfig GetConfigFromEnv()
    {
        AppConfig config;
        if (const auto *url = std::getenv(LEAVE_GAME_DB_URL_ENV_NAME))
        {
            config.db_url = url;
        }
        else
        {
            throw std::runtime_error(LEAVE_GAME_DB_URL_ENV_NAME + " environment variable not found"s);
        }
        return config;
    }

    void SaveRetiredPlayer(const std::string &player_name, int score, int play_time)
    {
        struct PlayerTag
        {
        };
        using PlayerId = util::TaggedUUID<PlayerTag>;

        model::PlayerRecordItem record{PlayerId::New().ToString(), player_name, score, play_time};

        ConnectionPoolSingleton *inst = ConnectionPoolSingleton::getInstance();
        auto *conn_pool = inst->GetPool();
        auto conn = conn_pool->GetConnection();
        postgres::RetiredRepositoryImpl rep{*conn};
        rep.SaveRetired(record);
    }

    std::vector<model::PlayerRecordItem> GetRetiredPlayers(int start, int max_items)
    {
        ConnectionPoolSingleton *inst = ConnectionPoolSingleton::getInstance();
        auto *conn_pool = inst->GetPool();
        auto conn = conn_pool->GetConnection();
        postgres::RetiredRepositoryImpl rep{*conn};
        return rep.GetRetired(start, max_items);
    }

    double ConvertPlayTimeToDouble(int play_time)
    {
        const int millisec_In_Second = 1000;
        return static_cast<double>(play_time) / millisec_In_Second;
    }
    std::string GetRequestStringWithoutParameters(const std::string &request)
    {
        std::string np_request = request;
        auto pos = request.rfind('?');
        if (pos != std::string::npos)
            np_request = request.substr(0, pos);

        return np_request;
    }

}
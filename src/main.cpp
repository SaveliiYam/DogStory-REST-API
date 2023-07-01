#include "sdk.h"
#include <boost/program_options.hpp>
#include <boost/asio/io_context.hpp>
#include <thread>
#include <boost/asio/signal_set.hpp>
#include "request_handler.h"
#include "event_logger.h"

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;

struct Args
{
    int tick_period{-1};
    std::string config_file;
    std::string www_root;
    bool spawn_random_points{false};
};

namespace
{

    // Запускает функцию fn на num_threads потоках, включая текущий
    template <typename Fn>
    void RunWorkers(unsigned num_threads, const Fn &fn)
    {
        num_threads = std::max(1u, num_threads);
        std::vector<std::jthread> workers;
        workers.reserve(num_threads - 1);
        // Запускаем num_threads-1 рабочих потоков, выполняющих функцию fn
        while (--num_threads)
        {
            workers.emplace_back(fn);
        }
        fn();
    }

    [[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char *const argv[])
    {
        namespace po = boost::program_options;

        po::options_description desc{"All options"s};
        Args args;
        std::string tick_period;
        desc.add_options()                     //
            ("help,h", "produce help message") //
            ("tick-period,t", po::value(&tick_period)->value_name("milliseconds"s), " set tick period")("config-file,c", po::value(&args.config_file)->value_name("file"s), "set config file path")("www-root,w", po::value(&args.www_root)->value_name("dir"s), "set static files root")("randomize-spawn-points", "spawn dogs at random positions");

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

        args.spawn_random_points = vm.contains("randomize-spawn-points"s) ? true : false;

        return args;
    }
} // namespace

int main(int argc, const char *argv[])
{
    std::optional<Args> args;
    args = ParseCommandLine(argc, argv);
    if (!args)
        return EXIT_FAILURE;

    try
    {
        // 1. Загружаем карту из файла и строим модель игры
        model::Game game = json_loader::LoadGame(args->config_file, args->www_root);
        if (args->tick_period > 0)
            game.SetTickPeriod(args->tick_period);

        game.SetSpawnInRandomPoint(args->spawn_random_points);
        // 2. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        // 3. Инициализируем логи
        event_logger::InitLogger();

        // 4. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        // Подписываемся на сигналы и при их получении завершаем работу сервера
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code &ec, [[maybe_unused]] int signal_number)
                           {
        		if (!ec) {
        			ioc.stop();
        			event_logger::LogServerEnd("server exited", EXIT_SUCCESS);
        		} });

        // 5. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        auto handler = std::make_shared<http_handler::RequestHandler>(game, ioc);

        // 6. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;

        http_server::ServeHttp(ioc, {address, port}, [&handler](auto &&req, auto &&send)
                               { handler->operator()(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send)); });

        // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
        event_logger::LogStartServer(address.to_string(), port, "server started");

        // 7. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc]
                   { ioc.run(); });
    }
    catch (const std::exception &ex)
    {
        event_logger::LogServerEnd("server exited", EXIT_FAILURE, ex.what());
        return EXIT_FAILURE;
    }
}

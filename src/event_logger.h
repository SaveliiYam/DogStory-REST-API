#pragma once
#include <boost/asio.hpp>
#include <boost/system.hpp>

namespace sys = boost::system;
namespace net = boost::asio;
using namespace std::literals;

namespace event_logger
{
    struct Where
    {
        Where() = delete;
        constexpr static std::string_view READ = "read"sv;
        constexpr static std::string_view WRITE = "write"sv;
        constexpr static std::string_view ACCEPT = "accept"sv;
    };

    void InitLogger();
    void LogStartServer(const std::string &address, unsigned int port, const std::string &message);
    void LogServerEnd(const std::string &message, int code, const std::string &exception_descr = "");
    void LogServerRequestReceived(const std::string &uri, const std::string &http_method);
    void LogServerResponseSend(int response_time, unsigned code, const std::string &content_type);
    void LogServerError(const sys::error_code ec, std::string_view where);
}

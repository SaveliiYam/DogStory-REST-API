#include <iostream>
#include <boost/log/core.hpp> // для logging::core
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp> // для выражения, задающего фильтр
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/date_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/json.hpp>
#include <boost/asio.hpp>

using namespace std::literals;

namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace sinks = boost::log::sinks;
namespace expr = boost::log::expressions;
namespace attrs = boost::log::attributes;
namespace json = boost::json;
namespace sys = boost::system;
namespace net = boost::asio;

BOOST_LOG_ATTRIBUTE_KEYWORD(line_id, "LineID", unsigned int)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)

BOOST_LOG_ATTRIBUTE_KEYWORD(file, "File", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(line, "Line", int)

BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", json::value)

namespace event_logger
{
  void MyFormatter(logging::record_view const &rec, logging::formatting_ostream &strm)
  {
    strm << rec[additional_data] << std::endl;
  }

  std::string GetLogTime()
  {
    boost::posix_time::ptime tm(boost::posix_time::microsec_clock::universal_time());
    return to_iso_extended_string(tm);
  }

  void InitLogger()
  {
    logging::add_common_attributes();
    logging::add_console_log(std::clog, keywords::auto_flush = true, keywords::format = &MyFormatter);
  }

  void LogStartServer(const std::string &address, unsigned int port, const std::string &message)
  {
    json::object resp_object;
    resp_object["message"] = message;
    resp_object["timestamp"] = GetLogTime();

    json::object data_object;
    data_object["address"] = address;
    data_object["port"] = port;

    resp_object["data"] = data_object;

    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, resp_object);
  }

  void LogServerEnd(const std::string &message, int code, const std::string &exception_descr)
  {
    json::object resp_object;
    resp_object["message"] = message;
    resp_object["timestamp"] = GetLogTime();

    json::object data_object;
    data_object["code"] = code;
    if (!exception_descr.empty())
      data_object["exception"] = exception_descr;

    resp_object["data"] = data_object;

    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, resp_object);
  }

  void LogServerRequestReceived(const std::string &uri, const std::string &http_method)
  {
    json::object resp_object;
    resp_object["message"] = "request received";
    resp_object["timestamp"] = GetLogTime();

    json::object data_object;
    data_object["URI"] = uri;
    data_object["method"] = http_method;

    resp_object["data"] = data_object;

    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, resp_object);
  }

  void LogServerResponseSend(int response_time, unsigned code, const std::string &content_type)
  {
    json::object resp_object;
    resp_object["message"] = "response sent";
    resp_object["timestamp"] = GetLogTime();

    json::object data_object;
    data_object["response_time"] = response_time;
    data_object["code"] = code;
    data_object["content_type"] = content_type;

    resp_object["data"] = data_object;

    BOOST_LOG_TRIVIAL(trace) << logging::add_value(additional_data, resp_object);
  }

  void LogServerError(const sys::error_code ec, std::string_view where)
  {
    std::string where_str{where.begin(), where.end()};
    json::object error_object;
    error_object["message"] = "error";

    json::object data_object;
    data_object["code"] = ec.value();
    data_object["text"] = ec.message();
    data_object["where"] = where_str;

    error_object["data"] = data_object;

    BOOST_LOG_TRIVIAL(warning) << logging::add_value(additional_data, error_object);
  }

} // namespace event_logger

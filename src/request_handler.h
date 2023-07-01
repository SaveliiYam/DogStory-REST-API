#pragma once
#include "http_server.h"
#include "model.h"
#include "event_logger.h"
#include "api_handler.h"

namespace net = boost::asio;

const std::string_view apiPrefix = "/api/";
const std::string_view mapPrefix = "/api/v1/maps";

namespace http_handler
{
	namespace http = beast::http;

	using StaticFileResponce = http::response<http::file_body>;

	StaticFileResponce MakeFileResponce(const std::filesystem::path &json_path, const std::string &mime_type);
	std::string_view GetFileExtension(std::string_view path);
	std::string GetMimeType(std::string_view extension);
	std::filesystem::path GetResourcePath(std::string_view target);

	class RequestHandler : public std::enable_shared_from_this<RequestHandler>
	{
	public:
		explicit RequestHandler(model::Game &game, net::io_context &ioc)
			: game_{game}, strand_(net::make_strand(ioc))
		{
			api_handler_ = std::make_shared<ApiHandler>(game, strand_);
		}

		RequestHandler(const RequestHandler &) = delete;
		RequestHandler &operator=(const RequestHandler &) = delete;

		template <typename Body, typename Allocator, typename Send>
		void operator()(http::request<Body, http::basic_fields<Allocator>> &&req, Send &&send)
		{
			std::string request = {req.target().begin(), req.target().end()};
			if (api_handler_->IsApiRequest(request))
			{
				return net::dispatch(strand_, [self = shared_from_this(), request, send, req]
									 {
//    			    			std::string request = {req.target().begin(), req.target().end()};

    			    	       // Этот assert не выстрелит, так как лямбда-функция будет выполняться внутри strand
    			    		   assert(self->strand_.running_in_this_thread());
    			    	       auto resp = self->api_handler_->HandleApiRequest(request, req.method(), req[http::field::authorization], req.body(), req.version(),req.keep_alive());
    			    	       send(std::move(resp)); });
			}

			if (req.method() != http::verb::get)
			{
				auto resp = MakeStringResponse(http::status::method_not_allowed,
											   json_serializer::MakeMappedResponce({{"code", "invalidMethod"}, {"message", "Invalid method"}}),
											   req.version(), req.keep_alive(), ContentType::APPLICATION_JSON);

				send(std::move(resp));
				return;
			}

			std::string_view target = req.target();
			const std::string uri = {target.begin(), target.end()};
			event_logger::LogServerRequestReceived(uri, "GET");
			StringResponse resp;

			if (target.starts_with(apiPrefix) && !target.starts_with(mapPrefix))
			{
				resp = MakeStringResponse(http::status::bad_request, json_serializer::MakeBadRequestResponce(), req.version(), req.keep_alive());
				send(std::move(resp));
				return;
			}

			if (target.starts_with(mapPrefix))
			{
				target.remove_prefix(mapPrefix.size());
				if (target.empty())
				{
					resp = MakeStringResponse(http::status::ok, json_serializer::GetMapListResponce(game_), req.version(), req.keep_alive(), ContentType::APPLICATION_JSON, {{http::field::cache_control, "no-cache"sv}});
				}
				else
				{
					target.remove_prefix(1);
					const auto &responce = json_serializer::GetMapContentResponce(game_, {target.begin(), target.end()});
					if (!responce.empty())
					{
						resp = MakeStringResponse(http::status::ok, responce, req.version(), req.keep_alive(), ContentType::APPLICATION_JSON, {{http::field::cache_control, "no-cache"sv}});
					}
					else
					{
						resp = MakeStringResponse(http::status::not_found, json_serializer::MakeMapNotFoundResponce(), req.version(), req.keep_alive(), ContentType::APPLICATION_JSON, {{http::field::cache_control, "no-cache"sv}});
					}
				}
				send(std::move(resp));
				return;
			}

			if (!target.starts_with(apiPrefix))
			{
				try
				{
					if ((target.size() == 1) && target.starts_with('/'))
						target = "/index.html";

					auto extension = GetFileExtension(target);
					std::string mimeType = GetMimeType(extension);
					std::filesystem::path basePath = game_.GetBasePath();
					basePath += target;

					auto first_tick = std::chrono::steady_clock::now();

					auto fileResp = MakeFileResponce(basePath, mimeType);
					send(std::move(fileResp));

					auto last_tick = std::chrono::steady_clock::now();
					auto time_delta = std::chrono::duration_cast<std::chrono::microseconds>(last_tick - first_tick);
					event_logger::LogServerResponseSend(time_delta.count(), static_cast<unsigned>(http::status::ok), mimeType);
				}
				catch (const std::filesystem::filesystem_error &ex)
				{
					auto notFoundResp = MakeStringResponse(http::status::not_found, json_serializer::MakeMapNotFoundResponce(),
														   req.version(), req.keep_alive(), ContentType::TEXT_PLAIN);
					send(std::move(notFoundResp));
				}
			}
		}

	private:
		Strand strand_;
		model::Game &game_;
		std::shared_ptr<ApiHandler> api_handler_;
	};

	class SyncWriteOStreamAdapter
	{
	public:
		explicit SyncWriteOStreamAdapter(std::ostream &os)
			: os_{os}
		{
		}

		template <typename ConstBufferSequence>
		size_t write_some(const ConstBufferSequence &cbs, sys::error_code &ec)
		{
			const size_t total_size = net::buffer_size(cbs);
			if (total_size == 0)
			{
				ec = {};
				return 0;
			}
			size_t bytes_written = 0;
			for (const auto &cb : cbs)
			{
				const size_t size = cb.size();
				const char *const data = reinterpret_cast<const char *>(cb.data());
				if (size > 0)
				{
					if (!os_.write(reinterpret_cast<const char *>(data), size))
					{
						ec = make_error_code(boost::system::errc::io_error);
						return bytes_written;
					}
					bytes_written += size;
				}
			}
			ec = {};
			return bytes_written;
		}

		template <typename ConstBufferSequence>
		size_t write_some(const ConstBufferSequence &cbs)
		{
			sys::error_code ec;
			const size_t bytes_written = write_some(cbs, ec);
			if (ec)
			{
				throw std::runtime_error("Failed to write");
			}
			return bytes_written;
		}

	private:
		std::ostream &os_;
	};

} // namespace http_handler

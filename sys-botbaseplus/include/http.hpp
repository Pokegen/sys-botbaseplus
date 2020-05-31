#pragma once

#include "httplib/httplib.h"
#include <memory>

namespace BotBasePlus
{
	namespace Http
	{
		enum Content_Type 
		{
			APPLICATION_JSON,
			APPLICATION_XML,
			TEXT_PLAIN
		};

		void registerRoutes(httplib::Server* svr);
		void setContent(httplib::Response& res, std::shared_ptr<std::string> content, BotBasePlus::Http::Content_Type contentType = BotBasePlus::Http::APPLICATION_JSON);
		void setContent(httplib::Response& res, std::string content, BotBasePlus::Http::Content_Type contentType = BotBasePlus::Http::APPLICATION_JSON);
		bool validateAuthentication(const httplib::Request& req);
		void badRequest(httplib::Response& res, std::string message);
		void unauthorized(httplib::Response& res, std::string message);
	} // namespace Http

} // namespace BotBasePlus

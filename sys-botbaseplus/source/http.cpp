#include "http.hpp"
#include "json.hpp"
#include "constants.hpp"
#include "commands.hpp"
#include "util.hpp"
#include "variables.hpp"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

using namespace rapidjson;
using namespace BotBasePlus;

void Http::registerRoutes(httplib::Server *svr)
{
	svr->Get("/version", [](const httplib::Request &req, httplib::Response &res)
			 {
		if (validateAuthentication(req, res) == false)
			return;

		std::shared_ptr<Document> d = Json::createDocument();

		d->AddMember("version", Constants::version, d->GetAllocator());

		setContent(res, Json::toString(d));
	});

	svr->Get("/healthz", [](const httplib::Request &req, httplib::Response &res)
			 {
		if (validateAuthentication(req, res) == false)
			return;

		std::shared_ptr<Document> d = Json::createDocument();

		d->AddMember("status", "UP", d->GetAllocator());

		setContent(res, Json::toString(d));
	});

	svr->Get("/metadata", [](const httplib::Request &req, httplib::Response &res)
			 {
		if (validateAuthentication(req, res) == false)
			return;

		std::shared_ptr<Document> d = Json::createDocument();

		Commands::MetaData meta = Commands::getMetaData();

		d->AddMember("titleId", Util::str_fmt("%16lX", meta.titleID), d->GetAllocator());
		d->AddMember("buildId", Util::str_fmt("%02x%02x%02x%02x%02x%02x%02x%02x", meta.buildID[0], meta.buildID[1], meta.buildID[2], meta.buildID[3], meta.buildID[4], meta.buildID[5], meta.buildID[6], meta.buildID[7]), d->GetAllocator());

		setContent(res, Json::toString(d));
	});

	svr->Post("/", [](const httplib::Request &req, httplib::Response &res)
			  {
		if (validateAuthentication(req, res) == false)
			return;

		std::unique_ptr<Document> document = std::make_unique<Document>();

		document->Parse(req.body);

		if (document->IsObject() == false)
		{
			badRequest(res, "Expected Object as Root");
			return;
		}

		if (document->HasMember("command") == false)
		{
			badRequest(res, "Expected command property to be present");
			return;
		}

		if ((*document)["command"].IsString() == false)
		{
			badRequest(res, "Expected command property to have type String");
			return;
		}

		std::string command = (*document)["command"].GetString();

		auto &f = std::use_facet<std::ctype<char>>(std::locale());
		f.tolower(command.data(), command.data() + command.size());

		if (command == "press" || command == "click" || command == "release")
		{
			if (document->HasMember("button") == false)
			{
				badRequest(res, "Expected button property to be present");
				return;
			}

			if ((*document)["button"].IsString() == false)
			{
				badRequest(res, "Expected button property to have type String");
				return;
			}

			std::string btn = (*document)["button"].GetString();

			f.toupper(btn.data(), btn.data() + btn.size());

			HidNpadButton key = Util::parseStringToButton((char *)btn.c_str());

			if (command == "press")
				Commands::press(key);
			else if (command == "click")
				Commands::click(key);
			else if (command == "release")
				Commands::release(key);
		}
		else if (command == "peek" || command == "poke")
		{
			if (document->HasMember("offset") == false)
			{
				badRequest(res, "Expected offset property to be present");
				return;
			}

			if ((*document)["offset"].IsString() == false)
			{
				badRequest(res, "Expected offset property to have type String");
				return;
			}

			u8 *val = NULL;
			u64 size = 0;

			if (command == "poke")
			{
				if (document->HasMember("value") == false)
				{
					badRequest(res, "Expected value property to be present");
					return;
				}

				if ((*document)["value"].IsString() == false)
				{
					badRequest(res, "Expected value property to have type String");
					return;
				}

				val = Util::parseStringToByteBuffer((char *)(*document)["value"].GetString(), &size);
			}
			else
			{
				if (document->HasMember("size") == false)
				{
					badRequest(res, "Expected size property to be present");
					return;
				}

				if ((*document)["size"].IsUint64() == false)
				{
					badRequest(res, "Expected size property to have type uint64");
					return;
				}
			}

			Commands::MetaData meta = Commands::getMetaData();
			u64 offset = Util::parseStringToInt((char *)(*document)["offset"].GetString());

			if (command == "poke")
			{
				Commands::poke(meta.heap_base + offset, size, val);

				delete val;
			}
			else
			{
				size = (*document)["size"].GetUint64();

				std::string str = Commands::peekReturn(meta.heap_base + offset, size);

				std::shared_ptr<Document> d = Json::createDocument();

				d->AddMember("value", str, d->GetAllocator());

				setContent(res, Json::toString(d));

				return;
			}
		}
		else
		{
			badRequest(res, "Invalid Command");
			return;
		}

		res.status = 204;
	});
}

bool Http::validateAuthentication(const httplib::Request &req, httplib::Response &res)
{
	if (req.has_header("Authorization") == false)
	{
		unauthorized(res, "Missing Authorization header");
		return false;
	}

	auto val = req.get_header_value("Authorization");

	if (Variables::authenticationToken == "")
	{
		std::ifstream ifs("/config/sys-botbaseplus/password.txt");
		if (ifs.good())
		{
			std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
			Variables::authenticationToken = content;
		}
		else
		{
			Variables::authenticationToken = "youshallnotpass";
		}
	}

	if (Variables::authenticationToken != val)
	{
		unauthorized(res, "Invalid Authentication");
		return false;
	}

	return true;
}

void Http::badRequest(httplib::Response &res, std::string message)
{
	std::shared_ptr<Document> d = Json::createDocument();

	d->AddMember("message", message, d->GetAllocator());

	res.status = 400;

	setContent(res, Json::toString(d));
}

void Http::unauthorized(httplib::Response &res, std::string message)
{
	std::shared_ptr<Document> d = Json::createDocument();

	d->AddMember("message", message, d->GetAllocator());

	res.status = 401;

	setContent(res, Json::toString(d));
}

void Http::setContent(httplib::Response &res, std::shared_ptr<std::string> content, Http::Content_Type contentType)
{
	Http::setContent(res, *content.get(), contentType);
}

void Http::setContent(httplib::Response &res, std::string content, Http::Content_Type contentType)
{
	const char *type;

	switch (contentType)
	{
	case BotBasePlus::Http::APPLICATION_JSON:
		type = "application/json";
		break;
	case BotBasePlus::Http::APPLICATION_XML:
		type = "application/xml";
		break;
	default:
		type = "text/plain";
		break;
	}

	res.set_content(content, type);
}

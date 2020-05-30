#define RAPIDJSON_HAS_STDSTRING 1

#include "json.hpp"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

using namespace rapidjson;
using namespace BotBasePlus;

std::shared_ptr<rapidjson::Document> Json::createDocument() 
{
	std::shared_ptr<rapidjson::Document> document = std::make_shared<rapidjson::Document>();

	document->SetObject();

	return document;
}

std::shared_ptr<std::string> Json::toString(std::shared_ptr<rapidjson::Document> document) 
{
	std::unique_ptr<StringBuffer> buffer = std::make_unique<StringBuffer>();
	std::unique_ptr<Writer<StringBuffer>> writer(new Writer<StringBuffer>(*buffer.get()));  
	document->Accept(*writer.get());

	return std::make_shared<std::string>(buffer->GetString());
}

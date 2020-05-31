#pragma once

#define RAPIDJSON_HAS_STDSTRING 1

#include <memory>
#include "rapidjson/document.h"

namespace BotBasePlus 
{
	namespace Json 
	{
		std::shared_ptr<rapidjson::Document> createDocument();
		
		std::shared_ptr<std::string> toString(std::shared_ptr<rapidjson::Document> document);
	}
}

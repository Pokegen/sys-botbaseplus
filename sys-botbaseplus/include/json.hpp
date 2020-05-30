#pragma once

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

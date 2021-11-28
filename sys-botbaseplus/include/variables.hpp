#pragma once

#include <switch.h>
#include <string>

namespace BotBasePlus
{
	namespace Variables
	{
		extern u64 mainLoopSleepTime;
		extern u64 freezeRate;
		extern bool debugResultCodes;

		extern bool echoCommands;

		extern std::string authenticationToken;
	} // namespace Variables

} // namespace BotBasePlus

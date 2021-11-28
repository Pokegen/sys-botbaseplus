#include "variables.hpp"

using namespace BotBasePlus;

u64 Variables::mainLoopSleepTime = 50;
u64 Variables::freezeRate = 3;
bool Variables::debugResultCodes = false;

bool Variables::echoCommands = false;

std::string Variables::authenticationToken = "";

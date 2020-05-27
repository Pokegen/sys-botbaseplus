#pragma once

namespace BotBasePlus
{
	namespace Args
	{
		int parseArgs(char *argstr, int (*callback)(int, char **));
	} // namespace Args

} // namespace BotBasePlus

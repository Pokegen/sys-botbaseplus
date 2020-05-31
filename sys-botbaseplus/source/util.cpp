#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <switch.h>
#include <memory>
#include <stdarg.h>
#include "util.hpp"
#include "commands.hpp"

using namespace BotBasePlus;

int Util::setupServerSocket()
{
	int lissock;
	int yes = 1;
	struct sockaddr_in server;
	lissock = socket(AF_INET, SOCK_STREAM, 0);

	setsockopt(lissock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(6000);

	while (bind(lissock, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		svcSleepThread(1e+9L);
	}
	listen(lissock, 3);
	return lissock;
}

u64 Util::parseStringToInt(char *arg)
{
	if (strlen(arg) > 2)
	{
		if (arg[1] == 'x')
		{
			u64 ret = strtoul(arg, NULL, 16);
			return ret;
		}
	}
	u64 ret = strtoul(arg, NULL, 10);
	return ret;
}

u8 *Util::parseStringToByteBuffer(char *arg, u64 *size)
{
	char toTranslate[2];
	int length = strlen(arg);
	bool isHex = false;

	if (length > 2)
	{
		if (arg[1] == 'x')
		{
			isHex = true;
			length -= 2;
			arg = &arg[2]; //cut off 0x
		}
	}

	bool isFirst = true;
	bool isOdd = (length % 2 == 1);
	u64 bufferSize = length / 2;
	if (isOdd)
		bufferSize++;
	u8 *buffer = (u8 *)malloc(bufferSize);

	u64 i;
	for (i = 0; i < bufferSize; i++)
	{
		if (isOdd)
		{
			if (isFirst)
			{
				toTranslate[0] = '0';
				toTranslate[1] = arg[i];
			}
			else
			{
				toTranslate[0] = arg[(2 * i) - 1];
				toTranslate[1] = arg[(2 * i)];
			}
		}
		else
		{
			toTranslate[0] = arg[i * 2];
			toTranslate[1] = arg[(i * 2) + 1];
		}
		isFirst = false;
		if (isHex)
		{
			buffer[i] = strtoul(toTranslate, NULL, 16);
		}
		else
		{
			buffer[i] = strtoul(toTranslate, NULL, 10);
		}
	}
	*size = bufferSize;
	return buffer;
}

HidControllerKeys Util::parseStringToButton(char *arg)
{
	if (strcmp(arg, "A") == 0)
	{
		return KEY_A;
	}
	else if (strcmp(arg, "B") == 0)
	{
		return KEY_B;
	}
	else if (strcmp(arg, "X") == 0)
	{
		return KEY_X;
	}
	else if (strcmp(arg, "Y") == 0)
	{
		return KEY_Y;
	}
	else if (strcmp(arg, "RSTICK") == 0)
	{
		return KEY_RSTICK;
	}
	else if (strcmp(arg, "LSTICK") == 0)
	{
		return KEY_LSTICK;
	}
	else if (strcmp(arg, "L") == 0)
	{
		return KEY_L;
	}
	else if (strcmp(arg, "R") == 0)
	{
		return KEY_R;
	}
	else if (strcmp(arg, "ZL") == 0)
	{
		return KEY_ZL;
	}
	else if (strcmp(arg, "ZR") == 0)
	{
		return KEY_ZR;
	}
	else if (strcmp(arg, "PLUS") == 0)
	{
		return KEY_PLUS;
	}
	else if (strcmp(arg, "MINUS") == 0)
	{
		return KEY_MINUS;
	}
	else if (strcmp(arg, "DLEFT") == 0)
	{
		return KEY_DLEFT;
	}
	else if (strcmp(arg, "DUP") == 0)
	{
		return KEY_DUP;
	}
	else if (strcmp(arg, "DRIGHT") == 0)
	{
		return KEY_DRIGHT;
	}
	else if (strcmp(arg, "DDOWN") == 0)
	{
		return KEY_DDOWN;
	}
	else if (strcmp(arg, "HOME") == 0)
	{
		return KEY_HOME;
	}
	else if (strcmp(arg, "CAPTURE") == 0)
	{
		return KEY_CAPTURE;
	}
	return KEY_A; //I guess lol
}

Result Util::capsscCaptureForDebug(void *buffer, size_t buffer_size, u64 *size)
{
	struct
	{
		u32 a;
		u64 b;
	} in = {0, 10000000000};
	return serviceDispatchInOut(capsscGetServiceSession(), 1204, in, *size,
								.buffer_attrs = {SfBufferAttr_HipcMapTransferAllowsNonSecure | SfBufferAttr_HipcMapAlias | SfBufferAttr_Out},
								.buffers = {{buffer, buffer_size}}, );
}

std::string Util::str_fmt(const std::string fmt_str, ...) {
    int final_n, n = ((int)fmt_str.size()) * 2; /* Reserve two times as much as the length of the fmt_str */
    std::unique_ptr<char[]> formatted;
    va_list ap;
    while(1) {
        formatted.reset(new char[n]); /* Wrap the plain char array into the unique_ptr */
        strcpy(&formatted[0], fmt_str.c_str());
        va_start(ap, fmt_str);
        final_n = vsnprintf(&formatted[0], n, fmt_str.c_str(), ap);
        va_end(ap);
        if (final_n < 0 || final_n >= n)
            n += abs(final_n - n + 1);
        else
            break;
    }
    return std::string(formatted.get());
}

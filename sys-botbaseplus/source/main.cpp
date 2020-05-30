#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <math.h>
#include <switch.h>
#include "commands.hpp"
#include "args.hpp"
#include "util.hpp"
#include "variables.hpp"
#include <poll.h>

#define TITLE_ID 0x430000000000000C

using namespace BotBasePlus;

extern "C" {
	#define HEAP_SIZE 0x000540000
	
	// we aren't an applet
	u32 __nx_applet_type = AppletType_None;

	// setup a fake heap (we don't need the heap anyway)
	char fake_heap[HEAP_SIZE];

	// we override libnx internals to do a minimal init
	void __libnx_initheap(void)
	{
		extern char *fake_heap_start;
		extern char *fake_heap_end;

		// setup newlib fake heap
		fake_heap_start = fake_heap;
		fake_heap_end = fake_heap + HEAP_SIZE;
	}

	void __appInit(void)
	{
		Result rc;
		svcSleepThread(20000000000L);
		rc = smInitialize();
		if (R_FAILED(rc))
			fatalThrow(rc);
		if (hosversionGet() == 0)
		{
			rc = setsysInitialize();
			if (R_SUCCEEDED(rc))
			{
				SetSysFirmwareVersion fw;
				rc = setsysGetFirmwareVersion(&fw);
				if (R_SUCCEEDED(rc))
					hosversionSet(MAKEHOSVERSION(fw.major, fw.minor, fw.micro));
				setsysExit();
			}
		}
		rc = fsInitialize();
		if (R_FAILED(rc))
			fatalThrow(rc);
		rc = fsdevMountSdmc();
		if (R_FAILED(rc))
			fatalThrow(rc);
		rc = timeInitialize();
		if (R_FAILED(rc))
			fatalThrow(rc);
		rc = pmdmntInitialize();
		if (R_FAILED(rc))
		{
			fatalThrow(rc);
		}
		rc = ldrDmntInitialize();
		if (R_FAILED(rc))
		{
			fatalThrow(rc);
		}
		rc = pminfoInitialize();
		if (R_FAILED(rc))
		{
			fatalThrow(rc);
		}
		rc = socketInitializeDefault();
		if (R_FAILED(rc))
			fatalThrow(rc);

		rc = capsscInitialize();
		if (R_FAILED(rc))
			fatalThrow(rc);
	}

	void __appExit(void)
	{
		fsdevUnmountAll();
		fsExit();
		smExit();
		audoutExit();
		timeExit();
		socketExit();
	}
}

int argmain(int argc, char **argv)
{
	if (argc == 0)
		return 0;

	//peek <address in hex or dec> <amount of bytes in hex or dec>
	if (!strcmp(argv[0], "peek"))
	{
		if (argc != 3)
			return 0;

		Commands::MetaData meta = Commands::getMetaData();

		u64 offset = Util::parseStringToInt(argv[1]);
		u64 size = Util::parseStringToInt(argv[2]);
		Commands::peek(meta.heap_base + offset, size);
	}

	if (!strcmp(argv[0], "peekAbsolute"))
	{
		if (argc != 3)
			return 0;

		u64 offset = Util::parseStringToInt(argv[1]);
		u64 size = Util::parseStringToInt(argv[2]);
		Commands::peek(offset, size);
	}

	if (!strcmp(argv[0], "peekMain"))
	{
		if (argc != 3)
			return 0;

		Commands::MetaData meta = Commands::getMetaData();

		u64 offset = Util::parseStringToInt(argv[1]);
		u64 size = Util::parseStringToInt(argv[2]);
		Commands::peek(meta.main_nso_base + offset, size);
	}

	//poke <address in hex or dec> <amount of bytes in hex or dec> <data in hex or dec>
	if (!strcmp(argv[0], "poke"))
	{
		if (argc != 3)
			return 0;

		Commands::MetaData meta = Commands::getMetaData();

		u64 offset = Util::parseStringToInt(argv[1]);
		u64 size = 0;
		u8 *data = Util::parseStringToByteBuffer(argv[2], &size);
		Commands::poke(meta.heap_base + offset, size, data);
		free(data);
	}

	if (!strcmp(argv[0], "pokeAbsolute"))
	{
		if (argc != 3)
			return 0;

		u64 offset = Util::parseStringToInt(argv[1]);
		u64 size = 0;
		u8 *data = Util::parseStringToByteBuffer(argv[2], &size);
		Commands::poke(offset, size, data);
		free(data);
	}

	if (!strcmp(argv[0], "pokeMain"))
	{
		if (argc != 3)
			return 0;

		Commands::MetaData meta = Commands::getMetaData();

		u64 offset = Util::parseStringToInt(argv[1]);
		u64 size = 0;
		u8 *data = Util::parseStringToByteBuffer(argv[2], &size);
		Commands::poke(meta.main_nso_base + offset, size, data);
		free(data);
	}

	//click <buttontype>
	if (!strcmp(argv[0], "click"))
	{
		if (argc != 2)
			return 0;
		HidControllerKeys key = Util::parseStringToButton(argv[1]);
		Commands::click(key);
	}

	//hold <buttontype>
	if (!strcmp(argv[0], "press"))
	{
		if (argc != 2)
			return 0;
		HidControllerKeys key = Util::parseStringToButton(argv[1]);
		Commands::press(key);
	}

	//release <buttontype>
	if (!strcmp(argv[0], "release"))
	{
		if (argc != 2)
			return 0;
		HidControllerKeys key = Util::parseStringToButton(argv[1]);
		Commands::release(key);
	}

	//setStick <left or right stick> <x value> <y value>
	if (!strcmp(argv[0], "setStick"))
	{
		if (argc != 4)
			return 0;

		int side = 0;
		if (!strcmp(argv[1], "LEFT"))
		{
			side = JOYSTICK_LEFT;
		}
		else if (!strcmp(argv[1], "RIGHT"))
		{
			side = JOYSTICK_RIGHT;
		}
		else
		{
			return 0;
		}

		int dxVal = strtol(argv[2], NULL, 0);
		if (dxVal > JOYSTICK_MAX)
			dxVal = JOYSTICK_MAX; //0x7FFF
		if (dxVal < JOYSTICK_MIN)
			dxVal = JOYSTICK_MIN; //-0x8000
		int dyVal = strtol(argv[3], NULL, 0);
		if (dxVal > JOYSTICK_MAX)
			dxVal = JOYSTICK_MAX;
		if (dxVal < JOYSTICK_MIN)
			dxVal = JOYSTICK_MIN;

		Commands::setStickState(side, dxVal, dyVal);
	}

	//detachController
	if (!strcmp(argv[0], "detachController"))
	{
		Result rc = hiddbgDetachHdlsVirtualDevice(Commands::controllerHandle);
		if (R_FAILED(rc) && Variables::debugResultCodes)
			printf("hiddbgDetachHdlsVirtualDevice: %d\n", rc);
		rc = hiddbgReleaseHdlsWorkBuffer();
		if (R_FAILED(rc) && Variables::debugResultCodes)
			printf("hiddbgReleaseHdlsWorkBuffer: %d\n", rc);
		hiddbgExit();
		Commands::bControllerIsInitialised = false;
	}

	//configure <mainLoopSleepTime or buttonClickSleepTime> <time in ms>
	if (!strcmp(argv[0], "configure"))
	{
		if (argc != 3)
			return 0;

		if (!strcmp(argv[1], "mainLoopSleepTime"))
		{
			u64 time = Util::parseStringToInt(argv[2]);
			Variables::mainLoopSleepTime = time;
		}

		if (!strcmp(argv[1], "buttonClickSleepTime"))
		{
			u64 time = Util::parseStringToInt(argv[2]);
			Commands::buttonClickSleepTime = time;
		}

		if (!strcmp(argv[1], "echoCommands"))
		{
			u64 shouldActivate = Util::parseStringToInt(argv[2]);
			Variables::echoCommands = shouldActivate != 0;
		}

		if (!strcmp(argv[1], "printDebugResultCodes"))
		{
			u64 shouldActivate = Util::parseStringToInt(argv[2]);
			Variables::debugResultCodes = shouldActivate != 0;
		}
	}

	if (!strcmp(argv[0], "getTitleID"))
	{
		Commands::MetaData meta = Commands::getMetaData();
		printf("%16lX\n", meta.titleID);
	}

	if (!strcmp(argv[0], "getSystemLanguage"))
	{
		//thanks zaksa
		setInitialize();
		u64 languageCode = 0;
		SetLanguage language = SetLanguage_ENUS;
		setGetSystemLanguage(&languageCode);
		setMakeLanguage(languageCode, &language);
		printf("%d\n", language);
	}

	if (!strcmp(argv[0], "getMainNsoBase"))
	{
		Commands::MetaData meta = Commands::getMetaData();
		printf("%16lX\n", meta.main_nso_base);
	}

	if (!strcmp(argv[0], "getBuildID"))
	{
		Commands::MetaData meta = Commands::getMetaData();
		printf("%02x%02x%02x%02x%02x%02x%02x%02x\n", meta.buildID[0], meta.buildID[1], meta.buildID[2], meta.buildID[3], meta.buildID[4], meta.buildID[5], meta.buildID[6], meta.buildID[7]);
	}

	if (!strcmp(argv[0], "getHeapBase"))
	{
		Commands::MetaData meta = Commands::getMetaData();
		printf("%16lX\n", meta.heap_base);
	}

	if (!strcmp(argv[0], "pixelPeek"))
	{
		//errors with 0x668CE, unless debugunit flag is patched
		u64 bSize = 0x7D000;
		char *buf = (char *)malloc(bSize);
		u64 outSize = 0;

		Result rc = Util::capsscCaptureForDebug(buf, bSize, &outSize);

		if (R_FAILED(rc) && Variables::debugResultCodes)
			printf("capssc, 1204: %d\n", rc);

		u64 i;
		for (i = 0; i < outSize; i++)
		{
			printf("%02X", buf[i]);
		}
		printf("\n");

		free(buf);
	}

	return 0;
}

void add_to_pfds(struct pollfd *pfds[], int newfd, int *fd_count, int *fd_size)
{
	if (*fd_count == *fd_size)
	{
		*fd_size *= 2;

		*pfds = (pollfd *)realloc(*pfds, sizeof(**pfds) * (*fd_size));
	}

	(*pfds)[*fd_count].fd = newfd;
	(*pfds)[*fd_count].events = POLLIN;

	(*fd_count)++;
}

void del_from_pfds(struct pollfd pfds[], int i, int *fd_count)
{
	pfds[i] = pfds[*fd_count - 1];

	(*fd_count)--;
}

int main()
{
	char *linebuf = (char *)malloc(sizeof(char) * MAX_LINE_LENGTH);

	int c = sizeof(struct sockaddr_in);
	struct sockaddr_in client;

	int fd_count = 0;
	int fd_size = 5;
	struct pollfd *pfds = (pollfd *)malloc(sizeof *pfds * fd_size);

	int listenfd = Util::setupServerSocket();
	pfds[0].fd = listenfd;
	pfds[0].events = POLLIN;
	fd_count = 1;

	int newfd;
	while (appletMainLoop())
	{
		poll(pfds, fd_count, -1);
		for (int i = 0; i < fd_count; i++)
		{
			if (pfds[i].revents & POLLIN)
			{
				if (pfds[i].fd == listenfd)
				{
					newfd = accept(listenfd, (struct sockaddr *)&client, (socklen_t *)&c);
					if (newfd != -1)
					{
						add_to_pfds(&pfds, newfd, &fd_count, &fd_size);
					}
					else
					{
						svcSleepThread(1e+9L);
						close(listenfd);
						listenfd = Util::setupServerSocket();
						pfds[0].fd = listenfd;
						pfds[0].events = POLLIN;
						break;
					}
				}
				else
				{
					bool readEnd = false;
					int readBytesSoFar = 0;
					while (!readEnd)
					{
						int len = recv(pfds[i].fd, &linebuf[readBytesSoFar], 1, 0);
						if (len <= 0)
						{
							close(pfds[i].fd);
							del_from_pfds(pfds, i, &fd_count);
							readEnd = true;
						}
						else
						{
							readBytesSoFar += len;
							if (linebuf[readBytesSoFar - 1] == '\n')
							{
								readEnd = true;
								linebuf[readBytesSoFar - 1] = 0;

								fflush(stdout);
								dup2(pfds[i].fd, STDOUT_FILENO);

								Args::parseArgs(linebuf, &argmain);

								if (Variables::echoCommands)
								{
									printf("%s\n", linebuf);
								}
							}
						}
					}
				}
			}
		}
		svcSleepThread(Variables::mainLoopSleepTime * 1e+6L);
	}

	return 0;
}

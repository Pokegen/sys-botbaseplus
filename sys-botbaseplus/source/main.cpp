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
#include "http.hpp"
#include <thread>
#include "freeze.hpp"

#define TITLE_ID 0x430000000000000C

using namespace BotBasePlus;

extern "C"
{
	#define HEAP_SIZE 0x00C00000
	#define THREAD_SIZE 0x20000

	Thread freezeThread, touchThread, keyboardThread, clickThread;

	// prototype thread functions to give the illusion of cleanliness
	void sub_freeze(void *arg);
	void sub_touch(void *arg);
	void sub_key(void *arg);
	void sub_click(void *arg);

	// locks for thread
	Mutex freezeMutex, touchMutex, keyMutex, clickMutex;

	// events for releasing or idling threads
	Freeze::FreezeThreadState freeze_thr_state = Freeze::Active; 
	u8 clickThreadState = 0; // 1 = break thread
	// key and touch events currently being processed
	Commands::KeyData currentKeyEvent = {0};
	Commands::TouchData currentTouchEvent = {0};
	char* currentClick = NULL;

	// for cancelling the touch/click thread
	u8 touchToken = 0;
	u8 clickToken = 0;

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
		rc = viInitialize(ViServiceType_Default);
		if (R_FAILED(rc))
			fatalThrow(rc);
    	rc = lblInitialize();
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
		viExit();
		lblExit();
	}
}

void makeTouch(HidTouchState* state, u64 sequentialCount, u64 holdTime, bool hold)
{
    mutexLock(&touchMutex);
    memset(&currentTouchEvent, 0, sizeof currentTouchEvent);
    currentTouchEvent.states = state;
    currentTouchEvent.sequentialCount = sequentialCount;
    currentTouchEvent.holdTime = holdTime;
    currentTouchEvent.hold = hold;
    currentTouchEvent.state = 1;
    mutexUnlock(&touchMutex);
}

void makeKeys(HiddbgKeyboardAutoPilotState* states, u64 sequentialCount)
{
    mutexLock(&keyMutex);
    memset(&currentKeyEvent, 0, sizeof currentKeyEvent);
    currentKeyEvent.states = states;
    currentKeyEvent.sequentialCount = sequentialCount;
    currentKeyEvent.state = 1;
    mutexUnlock(&keyMutex);
}

void makeClickSeq(char* seq)
{
    mutexLock(&clickMutex);
    currentClick = seq;
    mutexUnlock(&clickMutex);
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

	if (!strcmp(argv[0], "peekMulti"))
    {
        if(argc < 3 || argc % 2 == 0)
            return 0;

		Commands::MetaData meta = Commands::getMetaData();

        u64 itemCount = (argc - 1)/2;
        u64 offsets[itemCount];
        u64 sizes[itemCount];

        for (u64 i = 0; i < itemCount; ++i)
        {
            offsets[i] = meta.heap_base + Util::parseStringToInt(argv[(i*2)+1]);
            sizes[i] = Util::parseStringToInt(argv[(i*2)+2]);
        }
        Commands::peekMulti(offsets, sizes, itemCount);
    }

	if (!strcmp(argv[0], "peekAbsolute"))
	{
		if (argc != 3)
			return 0;

		u64 offset = Util::parseStringToInt(argv[1]);
		u64 size = Util::parseStringToInt(argv[2]);

		Commands::peek(offset, size);
	}

	if (!strcmp(argv[0], "peekAbsoluteMulti"))
    {
        if(argc < 3 || argc % 2 == 0)
            return 0;

        u64 itemCount = (argc - 1)/2;
        u64 offsets[itemCount];
        u64 sizes[itemCount];

        for (u64 i = 0; i < itemCount; ++i)
        {
            offsets[i] = Util::parseStringToInt(argv[(i*2)+1]);
            sizes[i] = Util::parseStringToInt(argv[(i*2)+2]);
        }
        Commands::peekMulti(offsets, sizes, itemCount);
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

	if (!strcmp(argv[0], "peekMainMulti"))
    {
        if(argc < 3 || argc % 2 == 0)
            return 0;
		
		Commands::MetaData meta = Commands::getMetaData();

        u64 itemCount = (argc - 1)/2;
        u64 offsets[itemCount];
        u64 sizes[itemCount];

        for (u64 i = 0; i < itemCount; ++i)
        {
            offsets[i] = meta.main_nso_base + Util::parseStringToInt(argv[(i*2)+1]);
            sizes[i] = Util::parseStringToInt(argv[(i*2)+2]);
        }
        Commands::peekMulti(offsets, sizes, itemCount);
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
		HidNpadButton key = Util::parseStringToButton(argv[1]);
		Commands::click(key);
	}

	//hold <buttontype>
	if (!strcmp(argv[0], "press"))
	{
		if (argc != 2)
			return 0;
		HidNpadButton key = Util::parseStringToButton(argv[1]);
		Commands::press(key);
	}

	//release <buttontype>
	if (!strcmp(argv[0], "release"))
	{
		if (argc != 2)
			return 0;
		HidNpadButton key = Util::parseStringToButton(argv[1]);
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
		if (dyVal > JOYSTICK_MAX)
			dyVal = JOYSTICK_MAX;
		if (dyVal < JOYSTICK_MIN)
			dyVal = JOYSTICK_MIN;

		Commands::setStickState(side, dxVal, dyVal);
	}

	//detachController
	if (!strcmp(argv[0], "detachController"))
	{
		Commands::detachController();
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
       
	    if(!strcmp(argv[1], "keySleepTime")){
            u64 keyTime = Util::parseStringToInt(argv[2]);
            Commands::keyPressSleepTime = keyTime;
        }

        if(!strcmp(argv[1], "fingerDiameter")){
            u32 fDiameter = (u32) Util::parseStringToInt(argv[2]);
            Commands::fingerDiameter = fDiameter;
        }

        if(!strcmp(argv[1], "pollRate")){
            u64 fPollRate = Util::parseStringToInt(argv[2]);
            Commands::pollRate = fPollRate;
        }

        if(!strcmp(argv[1], "freezeRate")){
            u64 fFreezeRate = Util::parseStringToInt(argv[2]);
            Variables::freezeRate = fFreezeRate;
        }

        if(!strcmp(argv[1], "controllerType")){
            Commands::detachController();
            u8 fControllerType = (u8) Util::parseStringToInt(argv[2]);
            Commands::controllerInitializedType = (HidDeviceType) fControllerType;
        }
	}

	if (!strcmp(argv[0], "getTitleID"))
	{
		Commands::MetaData meta = Commands::getMetaData();
		printf("%016lX\n", meta.titleID);
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
		printf("%016lX\n", meta.main_nso_base);
	}

	if (!strcmp(argv[0], "getBuildID"))
	{
		Commands::MetaData meta = Commands::getMetaData();
		printf("%02x%02x%02x%02x%02x%02x%02x%02x\n", meta.buildID[0], meta.buildID[1], meta.buildID[2], meta.buildID[3], meta.buildID[4], meta.buildID[5], meta.buildID[6], meta.buildID[7]);
	}

	if (!strcmp(argv[0], "getHeapBase"))
	{
		Commands::MetaData meta = Commands::getMetaData();
		printf("%016lX\n", meta.heap_base);
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

	if (!strcmp(argv[0], "getVersion"))
	{
		printf("2.0\n");
	}

	if (!strcmp(argv[0], "pointer"))
	{
		if(argc < 2)
            return 0;
		s64 jumps[argc-1];
		for (int i = 1; i < argc; i++)
			jumps[i-1] = Util::parseStringToSignedLong(argv[i]);
		u64 solved = Commands::followMainPointer(jumps, argc-1);
		printf("%016lX\n", solved);
	}

	// pointerAll <first (main) jump> <additional jumps> <final jump in pointerexpr> 
    // possibly redundant between the one above, one needs to go eventually. (little endian, flip it yourself if required)
	if (!strcmp(argv[0], "pointerAll"))
	{
		if(argc < 3)
            return 0;
        s64 finalJump = Util::parseStringToSignedLong(argv[argc-1]);
        u64 count = argc - 2;
		s64 jumps[count];
		for (int i = 1; i < argc-1; i++)
			jumps[i-1] = Util::parseStringToSignedLong(argv[i]);
		u64 solved = Commands::followMainPointer(jumps, count);
        if (solved != 0)
            solved += finalJump;
		printf("%016lX\n", solved);
	}

	// pointerRelative <first (main) jump> <additional jumps> <final jump in pointerexpr> 
	// returns offset relative to heap
	if (!strcmp(argv[0], "pointerRelative"))
	{
		if(argc < 3)
            return 0;
        s64 finalJump = Util::parseStringToSignedLong(argv[argc-1]);
        u64 count = argc - 2;
		s64 jumps[count];
		for (int i = 1; i < argc-1; i++)
			jumps[i-1] = Util::parseStringToSignedLong(argv[i]);
		u64 solved = Commands::followMainPointer(jumps, count);
        if (solved != 0)
        {
            solved += finalJump;
			Commands::MetaData meta = Commands::getMetaData();
            solved -= meta.heap_base;
        }
		printf("%016lX\n", solved);
	}

	// pointerPeek <amount of bytes in hex or dec> <first (main) jump> <additional jumps> <final jump in pointerexpr>
    // warning: no validation
    if (!strcmp(argv[0], "pointerPeek"))
	{
		if(argc < 4)
            return 0;

        s64 finalJump = Util::parseStringToSignedLong(argv[argc-1]);
		u64 size = Util::parseStringToInt(argv[1]);
        u64 count = argc - 3;
		s64 jumps[count];
		for (int i = 2; i < argc-1; i++)
			jumps[i-2] = Util::parseStringToSignedLong(argv[i]);
		u64 solved = Commands::followMainPointer(jumps, count);
        solved += finalJump;
        Commands::peek(solved, size);
	}

	// pointerPeekMulti <amount of bytes in hex or dec> <first (main) jump> <additional jumps> <final jump in pointerexpr> split by asterisks (*)
    // warning: no validation
    if (!strcmp(argv[0], "pointerPeekMulti"))
	{
		if(argc < 4)
            return 0;

        // we guess a max of 40 for now
        u64 offsets[40];
        u64 sizes[40];
        u64 itemCount = 0;

        int currIndex = 1;
        int lastIndex = 1;

        while (currIndex < argc)
        {
            // count first
            char* thisArg = argv[currIndex];
            while (strcmp(thisArg, "*"))
            {   
                currIndex++;
                if (currIndex < argc)
                    thisArg = argv[currIndex];
                else 
                    break;
            }

            int thisCount = currIndex - lastIndex;

            s64 finalJump = Util::parseStringToSignedLong(argv[currIndex-1]);
            u64 size = Util::parseStringToSignedLong(argv[lastIndex]);
            int count = thisCount - 2;
            s64 jumps[count];
            for (int i = 1; i < count+1; i++)
                jumps[i-1] = Util::parseStringToSignedLong(argv[i+lastIndex]);
            u64 solved = Commands::followMainPointer(jumps, count);
            solved += finalJump;

            offsets[itemCount] = solved;
            sizes[itemCount] = size;
            itemCount++;
            currIndex++;
            lastIndex = currIndex;
        }

        Commands::peekMulti(offsets, sizes, itemCount);
	}

	// pointerPoke <data to be sent> <first (main) jump> <additional jumps> <final jump in pointerexpr>
    // warning: no validation
    if (!strcmp(argv[0], "pointerPoke"))
	{
		if(argc < 4)
            return 0;

        s64 finalJump = Util::parseStringToSignedLong(argv[argc-1]);
        u64 count = argc - 3;
		s64 jumps[count];
		for (int i = 2; i < argc-1; i++)
			jumps[i-2] = Util::parseStringToSignedLong(argv[i]);
		u64 solved = Commands::followMainPointer(jumps, count);
        solved += finalJump;

		u64 size;
        u8* data = Util::parseStringToByteBuffer(argv[1], &size);
        Commands::poke(solved, size, data);
        free(data);
	}


	// add to freeze map
	if (!strcmp(argv[0], "freeze"))
    {
        if(argc != 3)
            return 0;

		Commands::MetaData meta = Commands::getMetaData();

        u64 offset = Util::parseStringToInt(argv[1]);
        u64 size = 0;
        u8* data = Util::parseStringToByteBuffer(argv[2], &size);
        Freeze::addToFreezeMap(offset, data, size, meta.titleID);
    }

	// remove from freeze map
	if (!strcmp(argv[0], "unFreeze"))
    {
        if(argc != 2)
            return 0;

        u64 offset = Util::parseStringToInt(argv[1]);
        Freeze::removeFromFreezeMap(offset);
    }

	// get count of offsets being frozen
	if (!strcmp(argv[0], "freezeCount"))
	{
		Freeze::getFreezeCount(true);
	}

	// clear all freezes
	if (!strcmp(argv[0], "freezeClear"))
	{
		Freeze::clearFreezes();
		freeze_thr_state = Freeze::Idle;
	}

    if (!strcmp(argv[0], "freezePause"))
		freeze_thr_state = Freeze::Pause;

    if (!strcmp(argv[0], "freezeUnpause"))
		freeze_thr_state = Freeze::Active;

    //touch followed by arrayof: <x in the range 0-1280> <y in the range 0-720>. Array is sequential taps, not different fingers. Functions in its own thread, but will not allow the call again while running. tapcount * pollRate * 2
    if (!strcmp(argv[0], "touch"))
	{
        if(argc < 3 || argc % 2 == 0)
            return 0;

        u32 count = (argc-1)/2;
		HidTouchState* state = (HidTouchState*)calloc(count, sizeof(HidTouchState));
        u32 i, j = 0;
        for (i = 0; i < count; ++i)
        {
            state[i].diameter_x = state[i].diameter_y = Commands::fingerDiameter;
            state[i].x = (u32) Util::parseStringToInt(argv[++j]);
            state[i].y = (u32) Util::parseStringToInt(argv[++j]);
        }

        makeTouch(state, count, Commands::pollRate * 1e+6L, false);
	}

    //touchHold <x in the range 0-1280> <y in the range 0-720> <time in milliseconds (must be at least 15ms)>. Functions in its own thread, but will not allow the call again while running. pollRate + holdtime
    if(!strcmp(argv[0], "touchHold")){
        if(argc != 4)
            return 0;

        HidTouchState* state = (HidTouchState*)calloc(1, sizeof(HidTouchState));
        state->diameter_x = state->diameter_y = Commands::fingerDiameter;
        state->x = (u32) Util::parseStringToInt(argv[1]);
        state->y = (u32) Util::parseStringToInt(argv[2]);
        u64 time = Util::parseStringToInt(argv[3]);
        makeTouch(state, 1, time * 1e+6L, false);
    }

    //touchDraw followed by arrayof: <x in the range 0-1280> <y in the range 0-720>. Array is vectors of where finger moves to, then removes the finger. Functions in its own thread, but will not allow the call again while running. (vectorcount * pollRate * 2) + pollRate
    if (!strcmp(argv[0], "touchDraw"))
	{
        if(argc < 3 || argc % 2 == 0)
            return 0;

        u32 count = (argc-1)/2;
		HidTouchState* state = (HidTouchState*)calloc(count, sizeof(HidTouchState));
        u32 i, j = 0;
        for (i = 0; i < count; ++i)
        {
            state[i].diameter_x = state[i].diameter_y = Commands::fingerDiameter;
            state[i].x = (u32) Util::parseStringToInt(argv[++j]);
            state[i].y = (u32) Util::parseStringToInt(argv[++j]);
        }

        makeTouch(state, count, Commands::pollRate * 1e+6L * 2, true);
	}

    if (!strcmp(argv[0], "touchCancel"))
        touchToken = 1;

    //key followed by arrayof: <HidKeyboardKey> to be pressed in sequential order
    //thank you Red (hp3721) for this functionality
    if (!strcmp(argv[0], "key"))
	{
        if (argc < 2)
            return 0;

        u64 count = argc-1;
        HiddbgKeyboardAutoPilotState* keystates = (HiddbgKeyboardAutoPilotState*)calloc(count, sizeof (HiddbgKeyboardAutoPilotState));
        u64 i;
        for (i = 0; i < count; i++)
        {
            u8 key = (u8) Util::parseStringToInt(argv[i+1]);
            if (key < 4 || key > 231)
                continue;
            keystates[i].keys[key / 64] = 1UL << key;
            keystates[i].modifiers = 1024UL; //numlock
        }

        makeKeys(keystates, count);
    }

    //keyMod followed by arrayof: <HidKeyboardKey> <HidKeyboardModifier>(without the bitfield shift) to be pressed in sequential order
    if (!strcmp(argv[0], "keyMod"))
	{
        if (argc < 3 || argc % 2 == 0)
            return 0;

        u32 count = (argc-1)/2;
        HiddbgKeyboardAutoPilotState* keystates = (HiddbgKeyboardAutoPilotState*)calloc(count, sizeof (HiddbgKeyboardAutoPilotState));
        u64 i, j = 0;
        for (i = 0; i < count; i++)
        {
            u8 key = (u8) Util::parseStringToInt(argv[++j]);
            if (key < 4 || key > 231)
                continue;
            keystates[i].keys[key / 64] = 1UL << key;
            keystates[i].modifiers = BIT((u8) Util::parseStringToInt(argv[++j]));
        }

        makeKeys(keystates, count);
    }

    //keyMulti followed by arrayof: <HidKeyboardKey> to be pressed at the same time.
    if (!strcmp(argv[0], "keyMulti"))
	{
        if (argc < 2)
            return 0;

        u64 count = argc-1;
        HiddbgKeyboardAutoPilotState* keystate = (HiddbgKeyboardAutoPilotState*)calloc(1, sizeof (HiddbgKeyboardAutoPilotState));
        u64 i;
        for (i = 0; i < count; i++)
        {
            u8 key = (u8) Util::parseStringToInt(argv[i+1]);
            if (key < 4 || key > 231)
                continue;
            keystate[0].keys[key / 64] |= 1UL << key;
        }

        makeKeys(keystate, 1);
    }

    //turns off the screen (display)
    if (!strcmp(argv[0], "screenOff"))
	{
        ViDisplay temp_display;
        Result rc = viOpenDisplay("Internal", &temp_display);
        if (R_FAILED(rc))
            rc = viOpenDefaultDisplay(&temp_display);
        if (R_SUCCEEDED(rc))
        {
            rc = viSetDisplayPowerState(&temp_display, ViPowerState_NotScanning); // not scanning keeps the screen on but does not push new pixels to the display. Battery save is non-negligible and should be used where possible
            svcSleepThread(1e+6l);
            viCloseDisplay(&temp_display);
            lblSwitchBacklightOff(1ul);
        }
    }

    //turns on the screen (display)
    if (!strcmp(argv[0], "screenOn"))
	{
        ViDisplay temp_display;
        Result rc = viOpenDisplay("Internal", &temp_display);
        if (R_FAILED(rc))
            rc = viOpenDefaultDisplay(&temp_display);
        if (R_SUCCEEDED(rc))
        {
            rc = viSetDisplayPowerState(&temp_display, ViPowerState_On);
            svcSleepThread(1e+6l);
            viCloseDisplay(&temp_display);
            lblSwitchBacklightOn(1ul);
        }
    }

    if (!strcmp(argv[0], "charge"))
	{
        u32 charge;
        Result rc = psmInitialize();
        if (R_FAILED(rc))
            fatalThrow(rc);
        psmGetBatteryChargePercentage(&charge);
        printf("%d\n", charge);
        psmExit();
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

	std::shared_ptr<httplib::Server> svr = std::make_shared<httplib::Server>();

	Http::registerRoutes(svr.get());

	std::thread httpThread([](std::shared_ptr<httplib::Server> svr)
						   {
							   while (true)
							   {
								   svr->listen("0.0.0.0", 9999);
							   }
						   },
						   svr);

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

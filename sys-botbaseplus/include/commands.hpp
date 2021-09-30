#pragma once

#include <string>
#include <switch.h>

namespace BotBasePlus
{
	namespace Commands
	{
		extern Handle debughandle;
		extern bool bControllerIsInitialised;
		extern HiddbgHdlsHandle controllerHandle;
		extern HiddbgHdlsDeviceInfo controllerDevice;
		extern HiddbgHdlsState controllerState;
		extern u64 buttonClickSleepTime;

		typedef struct
		{
			u64 main_nso_base;
			u64 heap_base;
			u64 titleID;
			u8 buildID[0x20];
		} MetaData;

		typedef struct
		{
			HidTouchState *states;
			u64 sequentialCount;
			u64 holdTime;
			bool hold;
			u8 state;
		} TouchData;

		typedef struct
		{
			HiddbgKeyboardAutoPilotState *states;
			u64 sequentialCount;
			u8 state;
		} KeyData;

#define JOYSTICK_LEFT 0
#define JOYSTICK_RIGHT 1

		void attach();
		void detach();
		u64 getMainNsoBase(u64 pid);
		u64 getHeapBase(Handle handle);
		u64 getTitleId(u64 pid);
		void getBuildID(MetaData *meta, u64 pid);
		MetaData getMetaData(void);
		void initController();
		void detachController();

		void poke(u64 offset, u64 size, u8 *val);
		void peek(u64 offset, u64 size);
		void writeMem(u64 offset, u64 size, u8 *val);
		void readMem(u8 *out, u64 offset, u64 size);
		std::string peekReturn(u64 offset, u64 size);
		void click(HidNpadButton btn);
		void press(HidNpadButton btn);
		void release(HidNpadButton btn);
		void setStickState(int side, int dxVal, int dyVal);

	} // namespace Commands

} // namespace BotBasePlus

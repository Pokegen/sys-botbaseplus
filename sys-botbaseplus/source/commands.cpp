#include <switch.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "commands.hpp"
#include "util.hpp"
#include "variables.hpp"

using namespace BotBasePlus;

//Controller:
bool Commands::bControllerIsInitialised = false;
HiddbgHdlsHandle Commands::controllerHandle = {0};
HiddbgHdlsDeviceInfo Commands::controllerDevice = {0};
HiddbgHdlsState Commands::controllerState = {0};

//Keyboard:
HiddbgKeyboardAutoPilotState dummyKeyboardState = {0};

Handle Commands::debughandle = 0;
u64 Commands::buttonClickSleepTime = 50;
u64 keyPressSleepTime = 25;
u64 pollRate = 17; // polling is linked to screen refresh rate (system UI) or game framerate. Most cases this is 1/60 or 1/30
u32 fingerDiameter = 50;
HiddbgHdlsSessionId sessionId = {0};

void Commands::attach()
{
	u64 pid = 0;
	Result rc = pmdmntGetApplicationProcessId(&pid);
	if (R_FAILED(rc) && Variables::debugResultCodes)
		printf("pmdmntGetApplicationProcessId: %d\n", rc);

	if (Commands::debughandle != 0)
		svcCloseHandle(Commands::debughandle);

	rc = svcDebugActiveProcess(&Commands::debughandle, pid);
	if (R_FAILED(rc) && Variables::debugResultCodes)
		printf("svcDebugActiveProcess: %d\n", rc);
}

void Commands::detach()
{
	if (Commands::debughandle != 0)
		svcCloseHandle(Commands::debughandle);
}

u64 Commands::getMainNsoBase(u64 pid)
{
	LoaderModuleInfo proc_modules[2];
	s32 numModules = 0;
	Result rc = ldrDmntGetProcessModuleInfo(pid, proc_modules, 2, &numModules);
	if (R_FAILED(rc) && Variables::debugResultCodes)
		printf("ldrDmntGetProcessModuleInfo: %d\n", rc);

	LoaderModuleInfo *proc_module = 0;
	if (numModules == 2)
	{
		proc_module = &proc_modules[1];
	}
	else
	{
		proc_module = &proc_modules[0];
	}
	return proc_module->base_address;
}

u64 Commands::getHeapBase(Handle handle)
{
	MemoryInfo meminfo;
	memset(&meminfo, 0, sizeof(MemoryInfo));
	u64 heap_base = 0;
	u64 lastaddr = 0;
	do
	{
		lastaddr = meminfo.addr;
		u32 pageinfo;
		svcQueryDebugProcessMemory(&meminfo, &pageinfo, handle, meminfo.addr + meminfo.size);
		if ((meminfo.type & MemType_Heap) == MemType_Heap)
		{
			heap_base = meminfo.addr;
			break;
		}
	} while (lastaddr < meminfo.addr + meminfo.size);

	return heap_base;
}

u64 Commands::getTitleId(u64 pid)
{
	u64 titleId = 0;
	Result rc = pminfoGetProgramId(&titleId, pid);
	if (R_FAILED(rc) && Variables::debugResultCodes)
		printf("pminfoGetProgramId: %d\n", rc);
	return titleId;
}

void Commands::getBuildID(MetaData *meta, u64 pid)
{
	LoaderModuleInfo proc_modules[2];
	s32 numModules = 0;
	Result rc = ldrDmntGetProcessModuleInfo(pid, proc_modules, 2, &numModules);
	if (R_FAILED(rc) && Variables::debugResultCodes)
		printf("ldrDmntGetProcessModuleInfo: %d\n", rc);

	LoaderModuleInfo *proc_module = 0;
	if (numModules == 2)
	{
		proc_module = &proc_modules[1];
	}
	else
	{
		proc_module = &proc_modules[0];
	}
	memcpy(meta->buildID, proc_module->build_id, 0x20);
}

Commands::MetaData Commands::getMetaData()
{
	Commands::MetaData meta;
	attach();
	u64 pid = 0;
	Result rc = pmdmntGetApplicationProcessId(&pid);
	if (R_FAILED(rc) && Variables::debugResultCodes)
		printf("pmdmntGetApplicationProcessId: %d\n", rc);

	meta.main_nso_base = getMainNsoBase(pid);
	meta.heap_base = getHeapBase(Commands::debughandle);
	meta.titleID = getTitleId(pid);
	getBuildID(&meta, pid);

	detach();
	return meta;
}

void Commands::initController()
{
	if (Commands::bControllerIsInitialised)
		return;
	//taken from switchexamples github
	Result rc = hiddbgInitialize();
	if (R_FAILED(rc) && Variables::debugResultCodes)
		printf("hiddbgInitialize: %d\n", rc);
	// Set the controller type to Pro-Controller, and set the npadInterfaceType.
	Commands::controllerDevice.deviceType = HidDeviceType_FullKey3;
	Commands::controllerDevice.npadInterfaceType = HidNpadInterfaceType_Bluetooth;
	// Set the controller colors. The grip colors are for Pro-Controller on [9.0.0+].
	Commands::controllerDevice.singleColorBody = RGBA8_MAXALPHA(255, 255, 255);
	Commands::controllerDevice.singleColorButtons = RGBA8_MAXALPHA(0, 0, 0);
	Commands::controllerDevice.colorLeftGrip = RGBA8_MAXALPHA(230, 255, 0);
	Commands::controllerDevice.colorRightGrip = RGBA8_MAXALPHA(0, 40, 20);

	// Setup example controller state.
	Commands::controllerState.battery_level = 4; // Set battery charge to full.
	Commands::controllerState.analog_stick_l.x = 0x0;
	Commands::controllerState.analog_stick_l.y = -0x0;
	Commands::controllerState.analog_stick_r.x = 0x0;
	Commands::controllerState.analog_stick_r.y = -0x0;
	rc = hiddbgAttachHdlsWorkBuffer(&sessionId);
	if (R_FAILED(rc) && Variables::debugResultCodes)
		printf("hiddbgAttachHdlsWorkBuffer: %d\n", rc);
	rc = hiddbgAttachHdlsVirtualDevice(&Commands::controllerHandle, &Commands::controllerDevice);
	if (R_FAILED(rc) && Variables::debugResultCodes)
		printf("hiddbgAttachHdlsVirtualDevice: %d\n", rc);
	//init a dummy keyboard state for assignment between keypresses
	dummyKeyboardState.keys[3] = 0x800000000000000UL; // Hackfix found by Red: an unused key press (KBD_MEDIA_CALC) is required to allow sequential same-key presses. bitfield[3]
	Commands::bControllerIsInitialised = true;
}

void Commands::detachController()
{
	initController();

	Result rc = hiddbgDetachHdlsVirtualDevice(controllerHandle);
	if (R_FAILED(rc) && Variables::debugResultCodes)
		printf("hiddbgDetachHdlsVirtualDevice: %d\n", rc);
	rc = hiddbgReleaseHdlsWorkBuffer(sessionId);
	if (R_FAILED(rc) && Variables::debugResultCodes)
		printf("hiddbgReleaseHdlsWorkBuffer: %d\n", rc);
	hiddbgExit();
	Commands::bControllerIsInitialised = false;

	sessionId.id = 0;
}

void Commands::poke(u64 offset, u64 size, u8 *val)
{
	attach();
	Commands::writeMem(offset, size, val);
	detach();
}

void Commands::writeMem(u64 offset, u64 size, u8 *val)
{
	Result rc = svcWriteDebugProcessMemory(Commands::debughandle, val, offset, size);
	if (R_FAILED(rc) && Variables::debugResultCodes)
		printf("svcWriteDebugProcessMemory: %d\n", rc);
}

void Commands::peek(u64 offset, u64 size)
{
	u8 *out = (u8 *)malloc(sizeof(u8) * size);
	attach();
	Commands::readMem(out, offset, size);
	detach();

	u64 i;
	for (i = 0; i < size; i++)
	{
		printf("%02X", out[i]);
	}
	printf("\n");
	free(out);
}

void Commands::readMem(u8 *out, u64 offset, u64 size)
{
	Result rc = svcReadDebugProcessMemory(out, Commands::debughandle, offset, size);
	if (R_FAILED(rc) && Variables::debugResultCodes)
		printf("svcReadDebugProcessMemory: %d\n", rc);
}

std::string Commands::peekReturn(u64 offset, u64 size)
{
	u8 out[size];
	attach();
	Result rc = svcReadDebugProcessMemory(&out, debughandle, offset, size);
	if (R_FAILED(rc) && Variables::debugResultCodes)
		printf("svcReadDebugProcessMemory: %d\n", rc);
	detach();

	std::string res = "";
	u64 i;
	for (i = 0; i < size; i++)
	{
		res = res + Util::str_fmt("%02X", out[i]);
	}

	return res;
}

void Commands::click(HidNpadButton btn)
{
	initController();
	press(btn);
	svcSleepThread(Commands::buttonClickSleepTime * 1e+6L);
	release(btn);
}
void Commands::press(HidNpadButton btn)
{
	initController();
	Commands::controllerState.buttons |= btn;
	Result rc = hiddbgSetHdlsState(Commands::controllerHandle, &Commands::controllerState);
	if (R_FAILED(rc) && Variables::debugResultCodes)
		printf("hiddbgSetHdlsState: %d\n", rc);
}

void Commands::release(HidNpadButton btn)
{
	initController();
	Commands::controllerState.buttons &= ~btn;
	Result rc = hiddbgSetHdlsState(Commands::controllerHandle, &Commands::controllerState);
	if (R_FAILED(rc) && Variables::debugResultCodes)
		printf("hiddbgSetHdlsState: %d\n", rc);
}

void Commands::setStickState(int side, int dxVal, int dyVal)
{
	initController();
	if (side == JOYSTICK_LEFT)
	{
		controllerState.analog_stick_l.x = dxVal;
		controllerState.analog_stick_l.y = dyVal;
	}
	else
	{
		controllerState.analog_stick_r.x = dxVal;
		controllerState.analog_stick_r.y = dyVal;
	}
	hiddbgSetHdlsState(Commands::controllerHandle, &Commands::controllerState);
}

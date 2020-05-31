#include <switch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "commands.hpp"
#include "util.hpp"
#include "variables.hpp"

using namespace BotBasePlus;

Mutex actionLock;

//Controller:
bool Commands::bControllerIsInitialised = false;
u64 Commands::controllerHandle = 0;
HiddbgHdlsDeviceInfo Commands::controllerDevice = {0};
HiddbgHdlsState Commands::controllerState = {0};

Handle Commands::debughandle = 0;
u64 Commands::buttonClickSleepTime = 50;

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
	Commands::controllerDevice.npadInterfaceType = NpadInterfaceType_Bluetooth;
	// Set the controller colors. The grip colors are for Pro-Controller on [9.0.0+].
	Commands::controllerDevice.singleColorBody = RGBA8_MAXALPHA(255, 255, 255);
	Commands::controllerDevice.singleColorButtons = RGBA8_MAXALPHA(0, 0, 0);
	Commands::controllerDevice.colorLeftGrip = RGBA8_MAXALPHA(230, 255, 0);
	Commands::controllerDevice.colorRightGrip = RGBA8_MAXALPHA(0, 40, 20);

	// Setup example controller state.
	Commands::controllerState.batteryCharge = 4; // Set battery charge to full.
	Commands::controllerState.joysticks[JOYSTICK_LEFT].dx = 0x0;
	Commands::controllerState.joysticks[JOYSTICK_LEFT].dy = -0x0;
	Commands::controllerState.joysticks[JOYSTICK_RIGHT].dx = 0x0;
	Commands::controllerState.joysticks[JOYSTICK_RIGHT].dy = -0x0;
	rc = hiddbgAttachHdlsWorkBuffer();
	if (R_FAILED(rc) && Variables::debugResultCodes)
		printf("hiddbgAttachHdlsWorkBuffer: %d\n", rc);
	rc = hiddbgAttachHdlsVirtualDevice(&Commands::controllerHandle, &Commands::controllerDevice);
	if (R_FAILED(rc) && Variables::debugResultCodes)
		printf("hiddbgAttachHdlsVirtualDevice: %d\n", rc);
	Commands::bControllerIsInitialised = true;
}

void Commands::poke(u64 offset, u64 size, u8 *val)
{
	attach();
	Result rc = svcWriteDebugProcessMemory(Commands::debughandle, val, offset, size);
	if (R_FAILED(rc) && Variables::debugResultCodes)
		printf("svcWriteDebugProcessMemory: %d\n", rc);
	detach();
}

void Commands::peek(u64 offset, u64 size)
{
	u8 out[size];
    attach();
    Result rc = svcReadDebugProcessMemory(&out, debughandle, offset, size);
    if (R_FAILED(rc) && Variables::debugResultCodes)
        printf("svcReadDebugProcessMemory: %d\n", rc);
    detach();

    u64 i;
    for (i = 0; i < size; i++)
    {
        printf("%02X", out[i]);
    }
    printf("\n");
}

std::string Commands::peekReturn(u64 offset, u64 size) {
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

void Commands::click(HidControllerKeys btn)
{
	initController();
	press(btn);
	svcSleepThread(Commands::buttonClickSleepTime * 1e+6L);
	release(btn);
}
void Commands::press(HidControllerKeys btn)
{
	initController();
	Commands::controllerState.buttons |= btn;
	hiddbgSetHdlsState(Commands::controllerHandle, &Commands::controllerState);
}

void Commands::release(HidControllerKeys btn)
{
	initController();
	Commands::controllerState.buttons &= ~btn;
	hiddbgSetHdlsState(Commands::controllerHandle, &Commands::controllerState);
}

void Commands::setStickState(int side, int dxVal, int dyVal)
{
	initController();
	Commands::controllerState.joysticks[side].dx = dxVal;
	Commands::controllerState.joysticks[side].dy = dyVal;
	hiddbgSetHdlsState(Commands::controllerHandle, &Commands::controllerState);
}

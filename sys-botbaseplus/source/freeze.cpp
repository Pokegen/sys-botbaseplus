#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <switch.h>
#include "freeze.hpp"

using namespace BotBasePlus;

Freeze::FreezeBlock *Freeze::freezes;

void Freeze::initFreezes()
{
	Freeze::freezes = (FreezeBlock *)calloc(FREEZE_DIC_LENGTH, sizeof(FreezeBlock));
}

void Freeze::freeFreezes()
{
	free(Freeze::freezes);
}

int Freeze::findAddrSlot(u64 addr)
{
	for (int i = 0; i < FREEZE_DIC_LENGTH; i++)
	{
		if (Freeze::freezes[i].address == addr)
			return i;
	}

	return -1;
}

int Freeze::findNextEmptySlot()
{
	return findAddrSlot(0);
}

int Freeze::addToFreezeMap(u64 addr, u8 *v_data, u64 v_size, u64 tid)
{
	// update slot if already exists
	int slot = findAddrSlot(addr);
	if (slot == -1)
		slot = findNextEmptySlot();
	else
		removeFromFreezeMap(addr);

	if (slot == -1)
		return 0;

	Freeze::freezes[slot].address = addr;
	Freeze::freezes[slot].vData = v_data;
	Freeze::freezes[slot].size = v_size;
	Freeze::freezes[slot].state = 1;
	Freeze::freezes[slot].titleId = tid;

	return slot;
}

int Freeze::removeFromFreezeMap(u64 addr)
{
	int slot = findAddrSlot(addr);
	if (slot == -1)
		return 0;
	Freeze::freezes[slot].address = 0;
	Freeze::freezes[slot].state = 0;
	free(Freeze::freezes[slot].vData);
	return slot;
}

int Freeze::getFreezeCount(bool print)
{
	int count = 0;
	for (int i = 0; i < FREEZE_DIC_LENGTH; i++)
	{
		if (Freeze::freezes[i].state != 0)
			++count;
	}
	if (print)
	{
		printf("%02X", count);
		printf("\n");
	}
	return count;
}

// returns 0 if there was nothing to clear
u8 Freeze::clearFreezes()
{
	u8 clearedOne = 0;
	for (int i = 0; i < FREEZE_DIC_LENGTH; i++)
	{
		if (Freeze::freezes[i].state != 0)
		{
			removeFromFreezeMap(Freeze::freezes[i].address);
			clearedOne = 1;
		}
	}
	return clearedOne;
}

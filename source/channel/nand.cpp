/***************************************************************************
 * Copyright (C) 2011
 * by Miigotu
 * Rewritten code from Mighty Channels and Triiforce
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any
 * damages arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any
 * purpose, including commercial applications, and to alter it and
 * redistribute it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you
 * must not claim that you wrote the original software. If you use
 * this software in a product, an acknowledgment in the product
 * documentation would be appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and
 * must not be misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source
 * distribution.
 *
 * Nand/Emulation Handling Class
 *
 * for wiiflow 2011
 ***************************************************************************/
 
#include <stdio.h>
#include <ogcsys.h>
#include <malloc.h>
#include <string.h>
#include <cstdlib>

#include "nand.hpp"
#include "utils.h"
#include "gecko.h"
#include "mem2.hpp"
#include "cios.hpp"

static NandDevice NandDeviceList[] = {
	{ "Disable",						0,	0x00,	0x00 },
	{ "SD/SDHC Card",					1,	0xF0,	0xF1 },
	{ "USB 2.0 Mass Storage Device",	2,	0xF2,	0xF3 },
};

Nand * Nand::instance = NULL;

Nand * Nand::Instance()
{
	if(instance == NULL)
		instance = new Nand();
	return instance;
}

void Nand::DestroyInstance()
{
	if(instance) delete instance;
	instance = NULL;
}

void Nand::Init(const char *path, u8 partition, bool disable)
{
	EmuDevice = disable ? REAL_NAND : partition == 0 ? EMU_SD : EMU_USB;
	Partition = disable ? REAL_NAND : partition > 0 ? partition - 1 : partition;
	Set_NandPath(path);
	Disabled = disable;
}

s32 Nand::Nand_Mount(NandDevice *Device)
{
	u32 inlen = 0;
	ioctlv *vector = NULL;
	u32 *buffer = NULL;

	/* Open FAT module */
	s32 fd = IOS_Open("fat", 0);
	if (fd < 0)
	{
		gprintf("IOS_Open('fat', 0); FAILED\n");
		return fd;
	}

	/* Prepare vector */
	u8 baseIOS = 0;
	if(cIOSInfo::D2X(IOS_GetVersion(), &baseIOS))
	{		
		// NOTE: 
		// The official cIOSX rev21 by Waninkoko ignores the Partition argument
		// and the nand is always expected to be on the 1st Partition.
		// However this way earlier d2x betas having revision 21 take in 
		// consideration the Partition argument.
		inlen = 1;

		/* Allocate memory */
		buffer = (u32 *)MEM2_alloc(sizeof(u32)*3);

		/* Set vector pointer */
		vector = (ioctlv *)buffer;

		buffer[0] = (u32)(buffer + 2);
		buffer[1] = sizeof(u32);
		buffer[2] = (u32)Partition;
	}

	/* Mount device */
	s32 ret = IOS_Ioctlv(fd, Device->Mount, inlen, 0, vector);
	
	/* Free memory */
	SAFE_FREE(buffer);

	/* Close FAT module */
	IOS_Close(fd);

	return ret;
}

s32 Nand::Nand_Unmount(NandDevice *Device)
{
	// Open FAT module
	s32 fd = IOS_Open("fat", 0);
	if (fd < 0)
	{
		gprintf("IOS_Open('fat', 0) FAILED\n");
		return fd;
	}

	// Unmount device
	s32 ret = IOS_Ioctlv(fd, Device->Unmount, 0, 0, NULL);

	// Close FAT module
	IOS_Close(fd);

	return ret;
}

s32 Nand::Nand_Enable(NandDevice *Device)
{
	s32 ret;

	// Open /dev/fs
	s32 fd = IOS_Open("/dev/fs", 0);
	if (fd < 0)
	{
		gprintf("IOS_Open('/dev/fs', 0) FAILED");
		return fd;
	}

	u8 baseIOS;
	bool goodcIOS = cIOSInfo::D2X(IOS_GetVersion(), &baseIOS);

	// Set input buffer
	if(goodcIOS && Device->Mode != 0)
	{
		//FULL NAND emulation since rev18
		//needed for reading images on triiforce mrc folder using ISFS commands
		inbuf[0] = Device->Mode | FullMode;
	}
	else inbuf[0] = Device->Mode; //old method

	// Enable NAND emulator
	if(goodcIOS)
	{
		// NOTE: 
		// The official cIOSX rev21 by Waninkoko provides an undocumented feature
		// to set nand NandPath when mounting the device.
		// This feature has been discovered during d2x development.
		int NandPathlen = strlen(NandPath)+1;

		/* Allocate memory */
		u32 *buffer = (u32 *)MEM2_alloc((sizeof(u32) * 5) + NandPathlen);
	
		buffer[0] = (u32)(buffer + 4);
		buffer[1] = sizeof(u32);		// actually not used by cios
		buffer[2] = (u32)(buffer + 5);
		buffer[3] = NandPathlen;		// actually not used by cios
		buffer[4] = inbuf[0];			
		strcpy((char*)(buffer+5), NandPath);
		
		ret = IOS_Ioctlv(fd, 100, 2, 0, (ioctlv *)buffer);
		SAFE_FREE(buffer);
	}
	else ret = IOS_Ioctl(fd, 100, inbuf, sizeof(inbuf), NULL, 0);

	// Close /dev/fs
	IOS_Close(fd);

	return ret;
} 

s32 Nand::Nand_Disable(void)
{
	// Open /dev/fs
	s32 fd = IOS_Open("/dev/fs", 0);
	if (fd < 0)
	{
		gprintf("IOS_Open('/dev/fs', 0) Failed\n");
		return fd;
	}

	// Set input buffer
	inbuf[0] = 0;

	// Disable NAND emulator
	s32 ret = IOS_Ioctl(fd, 100, inbuf, sizeof(inbuf), NULL, 0);

	// Close /dev/fs
	IOS_Close(fd);

	return ret;
} 


s32 Nand::Enable_Emu()
{
	if(MountedDevice == EmuDevice || Disabled)
		return 0;

	Disable_Emu();

	NandDevice *Device = &NandDeviceList[EmuDevice];

	s32 ret = Nand_Mount(Device);
	if (ret < 0) 
	{
		gprintf(" ERROR Mount! (ret = %d)\n", ret);
		return ret;
	}

	ret = Nand_Enable(Device);
	if (ret < 0) 
	{
		gprintf(" ERROR Enable! (ret = %d)\n", ret);
		return ret;
	}

	MountedDevice = EmuDevice;

	return 0;
}	

s32 Nand::Disable_Emu()
{
	if(MountedDevice == 0)
		return 0;

	NandDevice * Device = &NandDeviceList[MountedDevice];

	Nand_Disable();
	Nand_Unmount(Device);

	MountedDevice = 0;

	return 0;
}

void Nand::Set_Partition(int partition)
{
	Partition = partition;
}

void Nand::Set_NandPath(const char* path)
{
	int i=0;
	if(path[0] != '/' && path[0] != '\0') i++;

	while(path[i] != '\0' && i < 31)
	{
		NandPath[i] = path[i];
		i++;
	}
	if(NandPath[i-1] == '/')
		NandPath[i-1] = '\0';

	NandPath[0] = '/';
	NandPath[i] = '\0';
}

void Nand::Set_FullMode(bool fullmode)
{
	FullMode = fullmode ? 0x100: 0;
}

const char* Nand::Get_NandPath(void)
{
	return NandPath;
}

int Nand::Get_Partition(void)
{
	return Partition;
}
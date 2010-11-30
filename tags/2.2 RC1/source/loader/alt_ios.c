// mload from uloader by Hermes

#include "mload.h"
#include "ehcmodule_2.h"
#include "dip_plugin_2.h"
#include "ehcmodule_3.h"
#include "dip_plugin_3.h"
#include "ehcmodule_5.h"
#include "dip_plugin_249.h"
#include "odip_frag.h"
#include "fs.h"
#include "wdvd.h"
#include "disc.h"
#include "usbstorage.h"
#include "mem2.hpp"
#include "alt_ios.h"
#include "ios_base.h"
#include "mload_modules.h"
#include "sys.h"
#include "wbfs.h"

#include <malloc.h>
#include <wiiuse/wpad.h>

#include "gecko.h"

extern int __Arena2Lo;

int mainIOSRev = 0;
static int load_ehc_module_ex(void)
{
	switch (IOS_GetRevision())
	{
		case 2:
			ehcmodule = ehcmodule_2;
			size_ehcmodule = size_ehcmodule_2;
			dip_plugin = dip_plugin_2;
			size_dip_plugin = size_dip_plugin_2;
			break;
		case 3:
			ehcmodule = ehcmodule_3;
			size_ehcmodule = size_ehcmodule_3;
			dip_plugin = dip_plugin_3;
			size_dip_plugin = size_dip_plugin_3;
			break;
		default:
			ehcmodule = ehcmodule_5;
			size_ehcmodule = size_ehcmodule_5;
			dip_plugin = odip_frag;
			size_dip_plugin = size_odip_frag;
			break;
	}
	u8 *ehc_cfg = search_for_ehcmodule_cfg(ehcmodule, size_ehcmodule);
	if (ehc_cfg)
	{
		ehc_cfg += 12;
		ehc_cfg[0] = use_port1;
		gprintf("EHC Port info = %i\n", ehc_cfg[0]);
		DCFlushRange((void *) (((u32)ehc_cfg[0]) & ~31), 32);
	}
	if(use_port1)	// release port 0 and use port 1
	{
		u32 dat=0;
		u32 addr;

		// get EHCI base registers
		mload_getw((void *) 0x0D040000, &addr);

		addr&=0xff;
		addr+=0x0D040000;
		
		mload_getw((void *) (addr+0x44), &dat);
		if((dat & 0x2001)==1) 
			mload_setw((void *) (addr+0x44), 0x2000);
			
		mload_getw((void *) (addr+0x48), &dat);
		
		if((dat & 0x2000)==0x2000)
			mload_setw((void *) (addr+0x48), 0x1001);

	}
	load_ehc_module();

	return 0;
}

void load_dip_249()
{
	int ret;
	if (is_ios_type(IOS_TYPE_WANIN) && IOS_GetRevision() >= 17)
	{
		gprintf("Starting mload\n");
		if(mload_init()<0) {
			return;
		}
//		mload_set_gecko_debug();
		gprintf("Loading 249 dip...");
		ret = mload_module((void *) dip_plugin_249, size_dip_plugin_249);
		gprintf("%d\n", ret);
		mload_close();
	}
}

void try_hello()
{
	int ret;
	gprintf("mload init\n");
	if(mload_init()<0) {
		sleep(3);
		return;
	}
	u32 base;
	int size;
   	mload_get_load_base(&base, &size);
	gprintf("base: %08x %x\n", base, size);
	mload_close();
	gprintf("disc init:\n");
	ret = Disc_Init();
	gprintf("= %d\n", ret);
	u32 x = 6;
	s32 WDVD_hello(u32 *status);
	ret = WDVD_hello(&x);
	gprintf("hello: %d %x %d\n", ret, x, x);
	ret = WDVD_hello(&x);
	gprintf("hello: %d %x %d\n", ret, x, x);
}

bool loadIOS(int n, bool launch_game, bool switch_port)
{
	bool iosOK;
	char partition[6];

	Close_Inputs();

	if (launch_game)
	{
		Unmount_All_Devices();
		int curIndex = WBFS_GetCurrentPartition();
		WBFS_GetPartitionName(curIndex, (char *) &partition);
		WBFS_Close();

		WDVD_Close();
		USBStorage_Deinit();

		mload_close();
		usleep(500000);
	}
	else USBStorage_Deinit();

	void *backup = COVER_allocMem1(0x200000);	// 0x126CA0 bytes were needed last time i checked. But take more just in case.
	if (backup != 0)
	{
		memcpy(backup, &__Arena2Lo, 0x200000);
		DCFlushRange(backup, 0x200000);
	}
	usleep(100000);
	gprintf("Reloading into ios %d...", n);
	u32 v = IOS_ReloadIOS(n);
	iosOK = v >= 0;
	gprintf("ret: %d, current IOS: %d\n", v, IOS_GetVersion());
	usleep(300000);

	if (!is_ios_type(IOS_TYPE_WANIN)) sleep(1); // Narolez: sleep after IOS reload lets power down/up the harddisk when cIOS 249 is used!

	if (backup != 0)
	{
		memcpy(&__Arena2Lo, backup, 0x200000);
		DCFlushRange(&__Arena2Lo, 0x200000);
		COVER_free(backup);
	}

	if (iosOK)
	{
		if (is_ios_type(IOS_TYPE_HERMES))
			load_ehc_module_ex();
		else if (is_ios_type(IOS_TYPE_WANIN))
		{
			gprintf("Loading dip\n");
			load_dip_249();
//			try_hello();
		}
	}

 	if (launch_game)
	{
		Mount_Devices();
		WBFS_OpenNamed((char *) &partition);
		Disc_Init();
	}
	else Open_Inputs();

	return iosOK;
}

u32 get_ios_base()
{
	u32 revision = IOS_GetRevision();
	if (is_ios_type(IOS_TYPE_WANIN) && revision >= 17)
	{
		u32 index = get_ios_info_from_tmd();

		if (index != 0xFF) return atoi(bases[index]);
		else return wanin_mload_get_IOS_base();
	} 
	else if (is_ios_type(IOS_TYPE_HERMES) && revision >= 4)
		return mload_get_IOS_base();

	return 0;
}
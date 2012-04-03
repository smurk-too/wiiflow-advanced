// by oggzee

#ifndef _WBFS_Ext_H
#define _WBFS_Ext_H

#include "libwbfs/libwbfs.h"

#ifdef __cplusplus
extern "C" {
#endif

wbfs_t* WBFS_Ext_OpenPart(char *fname);
void WBFS_Ext_ClosePart(wbfs_t* part);
wbfs_disc_t* WBFS_Ext_OpenDisc(char *discid, char *fname);
void WBFS_Ext_CloseDisc(wbfs_disc_t* disc);
s32  WBFS_Ext_DiskSpace(f32 *used, f32 *free);
s32  WBFS_Ext_RemoveGame(char *gamepath);
s32  WBFS_Ext_AddGame(progress_callback_t spinner, void *spinner_data);
s32  WBFS_Ext_DVD_Size(u64 *comp_size, u64 *real_size);

char* strcopy(char *dest, const char *src, int size);
#ifdef __cplusplus
}
#endif

#endif

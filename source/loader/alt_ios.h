#ifndef _ALT_IOS_H_
#define _ALT_IOS_H_

#ifdef __cplusplus
extern "C" {
#endif

enum { IOS_249_MIN_REV = 9, IOS_222_MIN_REV = 2, IOS_223_MIN_REV = 2 };

enum { MAIN_IOS = 249, MAIN_IOS_MIN_REV = IOS_249_MIN_REV };
//enum { MAIN_IOS = 222, MAIN_IOS_MIN_REV = IOS_222_MIN_REV };
//enum { MAIN_IOS = 223, MAIN_IOS_MIN_REV = IOS_223_MIN_REV };

bool loadIOS(int n, bool init);
void disableIOSReload(void);

#ifdef __cplusplus
}
#endif

#endif

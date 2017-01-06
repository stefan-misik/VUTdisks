#ifndef VUT_DISKS_H
#define VUT_DISKS_H

#include "common.h"

#define WM_MAP_NOTIFY (WM_USER + 0x0001)
#define MAX_DISKS 8
#define LOGIN_MAX_LENGTH 64
#define PASSWORD_MAX_LENGTH 64
#define LOCAL_NAME_MAX_LENGTH 16
#define REMOTE_NAME_MAX_LENGTH MAX_PATH
#define ERROR_MESSAGE_BUFFER_LENGTH 16384

#define VUT_DISK_NUM 6

extern LPTSTR g_lpCaption;

extern const LPTSTR g_lpDisks[VUT_DISK_NUM];
extern TCHAR g_lpLogin[LOGIN_MAX_LENGTH];
extern TCHAR g_lpId[LOGIN_MAX_LENGTH];
extern TCHAR g_lpPassword[PASSWORD_MAX_LENGTH];

VOID VUTDisksEnableSavePassword(
    HWND hwndDlg,
    BOOL bEnable
);


#endif /* VUT_DISKS_H */


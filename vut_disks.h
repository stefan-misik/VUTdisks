#ifndef VUT_DISKS_H
#define VUT_DISKS_H

#include "common.h"
#include "common.h"
#include "disk_select_dialog.h"

#define WM_MAP_NOTIFY (WM_USER + 0x0001)


extern LPTSTR g_lpCaption;

extern const LPTSTR g_lpDisks[VUT_DISK_NUM];
extern TCHAR g_lpLogin[LOGIN_MAX_LENGTH];
extern TCHAR g_lpId[LOGIN_MAX_LENGTH];
extern TCHAR g_lpPassword[PASSWORD_MAX_LENGTH];
extern DISKSELECTION g_ds;

VOID VUTDisksEnableSavePassword(
    HWND hwndDlg,
    BOOL bEnable
);


#endif /* VUT_DISKS_H */


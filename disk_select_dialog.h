#ifndef DISK_SELECT_DIALOG_H
#define	DISK_SELECT_DIALOG_H

#include "common.h"
#include "config.h"


/**
 * @brief Disk selection structure
 */
typedef struct tagDISKSELECTION
{
    TCHAR aDiskLetters[VUT_DISK_NUM];   /** < Disk letters */
    DWORD dwDiskEnable;                 /** < Disk enable flags */
} DISKSELECTION, *LPDISKSELECTION;


/**
 * @brief Load default disk selection
 * 
 * @param lpDs[out] Disk selector structure to be filled with default settings
 */
VOID DiskSelectGetDefaults(
    LPDISKSELECTION lpDs
);

/**
 * @brief Show disk select dialog
 * 
 * @param[in] hwndParent Window handle to the parent window
 * @param[in,out] lpDiskSelection Structure containing the disk selection
 */
INT_PTR ShowDiskSelectDialog(
    HWND hwndParent,
    LPDISKSELECTION lpDiskSelection
);

#endif	/* DISK_SELECT_DIALOG_H */


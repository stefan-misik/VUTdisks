/* 
 * File:   disk_select_dialog.h
 * Author: xmisik00
 *
 * Created on Piatok, 2017, januára 6, 10:37
 */

#ifndef DISK_SELECT_DIALOG_H
#define	DISK_SELECT_DIALOG_H

#include "common.h"
#include "vut_disks.h"


/**
 * @brief Disk selection structure
 */
typedef struct tagDISKSELECTION
{
    TCHAR aDiscLetters[VUT_DISK_NUM];   /** < Disk letters */
    DWORD dwDiskEnable;                 /** < Disk enable flags */
} DISKSELECTION, *LPDISKSELECTION;


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


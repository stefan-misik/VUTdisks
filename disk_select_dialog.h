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
 * @brief Show disk select dialog
 * 
 * @param[in] hwndParent Window handle to the parent window
 * @param[in,out] lpDiskSelector Array of logical values representing the
 *                selected disks
 */
INT_PTR ShowDiskSelectDialog(
    HWND hwndParent,
    INT * lpDisksSelector 
);

#endif	/* DISK_SELECT_DIALOG_H */


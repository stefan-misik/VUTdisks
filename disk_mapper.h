#ifndef DISK_MAPPER_H
#define DISK_MAPPER_H

#include "common.h"
#include "vut_disks.h"

extern DWORD g_lpdwErrors[];
extern UINT g_uErrorsCnt;
extern BOOL g_bErrors;

typedef struct tagDISKINFO
{
	LPTSTR	lpLocalName;
	LPTSTR	lpRemoteName;
	LPTSTR	lpLogin;
	LPTSTR	lpPassword;
	DWORD	dwResult;
} DISKINFO, *LPDISKINFO;


typedef struct tagMAPPARAM
{
	DWORD		dwGUIthreadId;
	HWND		hwndToNotify;
	UINT		uDisksCnt;
	LPDISKINFO	lpDisks;
	HANDLE		hStopEvent;
	HANDLE		hMapThread;
} MAPPARAM, *LPMAPPARAM;


BOOL RequestStopMapDisks(LPMAPPARAM g_lpMapParam);

BOOL EndMapDisks(LPMAPPARAM lpMapParam, DWORD dwTimeout);

LPMAPPARAM BeginMapDisks(HWND hwndDlg, LPTSTR lpLogin, 
	LPTSTR lpId, LPTSTR lpPassword);


#endif /* DISK_MAPPER_H */


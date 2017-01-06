#include "disk_mapper.h"
#include "vut_disks.h"

#include <wchar.h>

#define DISK_P_ADDR "\\\\ad.feec.vutbr.cz\\homes.id\\vutbr.cz\\%s"
#define DISK_Q_ADDR "\\\\deza.feec.vutbr.cz\\app"
#define DISK_R_ADDR "\\\\deza.feec.vutbr.cz\\doc"
#define DISK_S_ADDR "\\\\deza.feec.vutbr.cz\\grp"
#define DISK_T_ADDR "\\\\deza.feec.vutbr.cz\\inst"
#define DISK_V_ADDR "\\\\gigadisk2.ro.vutbr.cz\\GIGADISK2\\home\\%c\\%c\\%c\\%s"

#define BUT_DOMAIN "%s@vutbr.cz"


DWORD g_lpdwErrors[VUT_DISK_NUM];
BOOL g_bErrors = FALSE;

/******************************************************************************/
VOID static MyCopyTString(LPTSTR lpDest, DWORD cNum, LPCTSTR lpSource)
{
    #ifdef UNICODE
        wcsncpy((wchar_t *)lpDest, (const wchar_t *)lpSource, (size_t)cNum);
    #endif
}

/******************************************************************************/
size_t static MyTStrlen(LPCTSTR lpStr)
{
    #ifdef UNICODE
        return wcslen(lpStr);
    #else
        return strlen(lpStr);
    #endif
}

/******************************************************************************/
DWORD WINAPI static MapThread(LPVOID lpParameter)
{
	UINT c;
	LPMAPPARAM param = (LPMAPPARAM)lpParameter;
	NETRESOURCE nr;

	nr.dwType = RESOURCETYPE_DISK;	
	nr.lpProvider = (LPTSTR)NULL;

	for (c = 0; c < param->uDisksCnt; c++)
	{
		/* Check for stop event state */
		if (WAIT_TIMEOUT != WaitForSingleObjectEx(param->hStopEvent, 0, FALSE))
		{			
			break;
		}

		/* Set the resource names */
		nr.lpLocalName = param->lpDisks[c].lpLocalName;
		nr.lpRemoteName = param->lpDisks[c].lpRemoteName;
		
		/* Connect Resource */
		param->lpDisks[c].dwResult = WNetAddConnection2(&nr, 
			param->lpDisks[c].lpPassword, param->lpDisks[c].lpLogin, 
                CONNECT_TEMPORARY);

		/* Notify GUI thread */
		if (NULL != param->hwndToNotify)
		{
			PostThreadMessage(param->dwGUIthreadId, WM_MAP_NOTIFY, (WPARAM)c, 
                (LPARAM)param);
		}
		
	}

	/* Notify GUI thread */
	if (NULL != param->hwndToNotify)
	{
		PostThreadMessage(param->dwGUIthreadId, WM_MAP_NOTIFY, 
            (WPARAM)MAX_DISKS, (LPARAM)param);
	}

	ExitThread(c);
}

/******************************************************************************/
void static DestroyDiskInfoArray(LPDISKINFO lpDiskInfo, UINT uCount)
{
	UINT c;

	if (NULL != lpDiskInfo)
	{
		for (c = 0; c < uCount; c++)
		{
			if (NULL != lpDiskInfo[c].lpLogin)
			{
				HeapFree(g_hHeap, 0, lpDiskInfo[c].lpLogin);
			}
			if (NULL != lpDiskInfo[c].lpPassword)
			{
				ZeroMemory(lpDiskInfo[c].lpPassword, 
					sizeof(TCHAR) * PASSWORD_MAX_LENGTH);
				HeapFree(g_hHeap, 0, lpDiskInfo[c].lpPassword);
			}
			if (NULL != lpDiskInfo[c].lpLocalName)
			{
				HeapFree(g_hHeap, 0, lpDiskInfo[c].lpLocalName);
			}
			if (NULL != lpDiskInfo[c].lpRemoteName)
			{
				HeapFree(g_hHeap, 0, lpDiskInfo[c].lpRemoteName);
			}
		}

		HeapFree(g_hHeap, 0, lpDiskInfo);
	}
}

/******************************************************************************/
LPDISKINFO static BuildDiskInfoArray(LPTSTR lpLogin,
	LPTSTR lpId, LPTSTR lpPassword, LPUINT lpCount)
{
	LPDISKINFO lpDiskInfo = HeapAlloc(g_hHeap, HEAP_ZERO_MEMORY, sizeof(DISKINFO) * VUT_DISK_NUM);
	UINT c;
    UINT cDomainLen = MyTStrlen(TEXT(BUT_DOMAIN));
    UINT cIdLen = MyTStrlen(lpId);

	*lpCount = 0;

	if (NULL != lpDiskInfo)
	{
		for (c = 0; c < VUT_DISK_NUM; c ++)
		{
			lpDiskInfo[c].lpLogin = HeapAlloc(g_hHeap, 0, sizeof(TCHAR) * (LOGIN_MAX_LENGTH + cDomainLen));
			if (NULL == lpDiskInfo[c].lpLogin)
			{
				DestroyDiskInfoArray(lpDiskInfo, 5);
				return NULL;
			}
			lpDiskInfo[c].lpPassword = HeapAlloc(g_hHeap, 0, sizeof(TCHAR) * PASSWORD_MAX_LENGTH);
			if (NULL == lpDiskInfo[c].lpPassword)
			{
				DestroyDiskInfoArray(lpDiskInfo, 5);
				return NULL;
			}
			lpDiskInfo[c].lpLocalName = HeapAlloc(g_hHeap, 0, sizeof(TCHAR) * LOCAL_NAME_MAX_LENGTH);
			if (NULL == lpDiskInfo[c].lpLocalName)
			{
				DestroyDiskInfoArray(lpDiskInfo, 5);
				return NULL;
			}
			lpDiskInfo[c].lpRemoteName = HeapAlloc(g_hHeap, 0, sizeof(TCHAR) * REMOTE_NAME_MAX_LENGTH);
			if (NULL == lpDiskInfo[c].lpRemoteName)
			{
				DestroyDiskInfoArray(lpDiskInfo, 5);
				return NULL;
			}
		}

		*lpCount = VUT_DISK_NUM;

		/* Disk 0 */        
		MyCopyTString(lpDiskInfo[0].lpLogin, LOGIN_MAX_LENGTH, lpLogin);
		MyCopyTString(lpDiskInfo[0].lpPassword, PASSWORD_MAX_LENGTH, lpPassword);
		MyCopyTString(lpDiskInfo[0].lpLocalName, LOCAL_NAME_MAX_LENGTH, g_lpDisks[0]);        
        wsprintf(lpDiskInfo[0].lpRemoteName, 
                TEXT(DISK_P_ADDR), lpId);

		/* Disk 1 */
		MyCopyTString(lpDiskInfo[1].lpLogin, LOGIN_MAX_LENGTH, lpLogin);
		MyCopyTString(lpDiskInfo[1].lpPassword, PASSWORD_MAX_LENGTH, lpPassword);
		MyCopyTString(lpDiskInfo[1].lpLocalName, LOCAL_NAME_MAX_LENGTH, g_lpDisks[1]);
		MyCopyTString(lpDiskInfo[1].lpRemoteName, REMOTE_NAME_MAX_LENGTH,
			TEXT(DISK_Q_ADDR));

		/* Disk 2 */
		MyCopyTString(lpDiskInfo[2].lpLogin, LOGIN_MAX_LENGTH, lpLogin);
		MyCopyTString(lpDiskInfo[2].lpPassword, PASSWORD_MAX_LENGTH, lpPassword);
		MyCopyTString(lpDiskInfo[2].lpLocalName, LOCAL_NAME_MAX_LENGTH, g_lpDisks[2]);
		MyCopyTString(lpDiskInfo[2].lpRemoteName, REMOTE_NAME_MAX_LENGTH,
			TEXT(DISK_R_ADDR));

		/* Disk 3 */
		MyCopyTString(lpDiskInfo[3].lpLogin, LOGIN_MAX_LENGTH, lpLogin);
		MyCopyTString(lpDiskInfo[3].lpPassword, PASSWORD_MAX_LENGTH, lpPassword);
		MyCopyTString(lpDiskInfo[3].lpLocalName, LOCAL_NAME_MAX_LENGTH, g_lpDisks[3]);
		MyCopyTString(lpDiskInfo[3].lpRemoteName, REMOTE_NAME_MAX_LENGTH,
			TEXT(DISK_S_ADDR));

		/* Disk 4 */
		MyCopyTString(lpDiskInfo[4].lpLogin, LOGIN_MAX_LENGTH, lpLogin);
		MyCopyTString(lpDiskInfo[4].lpPassword, PASSWORD_MAX_LENGTH, lpPassword);
		MyCopyTString(lpDiskInfo[4].lpLocalName, LOCAL_NAME_MAX_LENGTH, g_lpDisks[4]);
		MyCopyTString(lpDiskInfo[4].lpRemoteName, REMOTE_NAME_MAX_LENGTH,
			TEXT(DISK_T_ADDR));
        
        /* Disk 5 */
        wsprintf(lpDiskInfo[5].lpLogin, TEXT(BUT_DOMAIN), lpLogin);
		MyCopyTString(lpDiskInfo[5].lpPassword, PASSWORD_MAX_LENGTH, lpPassword);
		MyCopyTString(lpDiskInfo[5].lpLocalName, LOCAL_NAME_MAX_LENGTH, g_lpDisks[5]);        
        wsprintf(lpDiskInfo[5].lpRemoteName, TEXT(DISK_V_ADDR), 
            lpId[cIdLen - 1], lpId[cIdLen - 2], lpId[cIdLen - 3], lpId);
	}
	
	return lpDiskInfo;
}


/******************************************************************************/
BOOL RequestStopMapDisks(LPMAPPARAM g_lpMapParam)
{
	if (NULL != g_lpMapParam->hStopEvent)
	{
		/* Set stop event */
		SetEvent(g_lpMapParam->hStopEvent);

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/******************************************************************************/
BOOL EndMapDisks(LPMAPPARAM lpMapParam, DWORD dwTimeout)
{
	DWORD uRetVal = 0;

	if (NULL != lpMapParam->hStopEvent)
	{
		/* Set stop event */
		SetEvent(lpMapParam->hStopEvent);

		/* Wait for thread to exit */
		if (WAIT_OBJECT_0 ==
			WaitForSingleObject(lpMapParam->hMapThread, dwTimeout))
		{

			/* Get Exit code of thread */
			if (FALSE == GetExitCodeThread(lpMapParam->hMapThread, &uRetVal))
			{
				uRetVal = 0;
			}

			/* Close Thread Handle */
			CloseHandle(lpMapParam->hMapThread);
			/* Close stop Event */
			CloseHandle(lpMapParam->hStopEvent);
			/* Clean-up disks info */
			DestroyDiskInfoArray(lpMapParam->lpDisks, lpMapParam->uDisksCnt);

			/* Free memory */
			HeapFree(g_hHeap, 0, lpMapParam);

			return TRUE;
		}
	}

	return FALSE;
}

/******************************************************************************/
LPMAPPARAM BeginMapDisks(HWND hwndDlg, LPTSTR lpLogin, 
	LPTSTR lpId, LPTSTR lpPassword)
{	
	LPMAPPARAM lpNewParam = NULL;
    
    lpNewParam = HeapAlloc(g_hHeap, 0, sizeof(MAPPARAM));
	if (NULL == lpNewParam)
	{		
		return NULL;
	}

	lpNewParam->dwGUIthreadId = GetCurrentThreadId();
	lpNewParam->hwndToNotify = hwndDlg;
    
    /* Build disks info */
	lpNewParam->lpDisks = BuildDiskInfoArray(lpLogin, lpId,
		lpPassword, &(lpNewParam->uDisksCnt));
	if (NULL == lpNewParam->lpDisks)
	{
        HeapFree(g_hHeap, 0, lpNewParam);
		return NULL;
	}

	/* Create stop event */
	lpNewParam->hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (NULL == lpNewParam->hStopEvent)
	{
        DestroyDiskInfoArray(lpNewParam->lpDisks, lpNewParam->uDisksCnt);
        HeapFree(g_hHeap, 0, lpNewParam);
		return NULL;
	}	

	/* Create Thread */
	lpNewParam->hMapThread = CreateThread(NULL, 0, MapThread,
		lpNewParam, CREATE_SUSPENDED, NULL);
	if (NULL == lpNewParam->hMapThread)
	{
		CloseHandle(lpNewParam->hStopEvent);
		DestroyDiskInfoArray(lpNewParam->lpDisks, lpNewParam->uDisksCnt);
		HeapFree(g_hHeap, 0, lpNewParam);
		return NULL;
	}

	/* Start Mapping */
	ResumeThread(lpNewParam->hMapThread);

	return lpNewParam;
}
#include "disk_mapper.h"

#include <wchar.h>

DWORD g_lpdwErrors[MAX_DISKS];
UINT g_uErrorsCnt = 0;
BOOL g_bErrors = FALSE;

/******************************************************************************/
VOID static MyCopyTString(LPTSTR lpDest, DWORD cNum, LPCTSTR lpSource)
{
    #ifdef UNICODE
        wcsncpy((wchar_t *)lpDest, (const wchar_t *)lpSource, (size_t)cNum);
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
	LPDISKINFO lpDiskInfo = HeapAlloc(g_hHeap, HEAP_ZERO_MEMORY, sizeof(DISKINFO) * 5);
	UINT c;

	*lpCount = 0;

	if (NULL != lpDiskInfo)
	{
		for (c = 0; c < 5; c++)
		{
			lpDiskInfo[c].lpLogin = HeapAlloc(g_hHeap, 0, sizeof(TCHAR) * LOGIN_MAX_LENGTH);
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

		*lpCount = 5;

		/* Disk P: */        
		MyCopyTString(lpDiskInfo[0].lpLogin, LOGIN_MAX_LENGTH, lpLogin);
		MyCopyTString(lpDiskInfo[0].lpPassword, PASSWORD_MAX_LENGTH, lpPassword);
		MyCopyTString(lpDiskInfo[0].lpLocalName, LOCAL_NAME_MAX_LENGTH, TEXT("P:"));
        
        MyCopyTString(lpDiskInfo[0].lpRemoteName, REMOTE_NAME_MAX_LENGTH,
			TEXT("\\\\ad.feec.vutbr.cz\\homes.id\\vutbr.cz\\"));
        lstrcat(lpDiskInfo[0].lpRemoteName, lpId);       

		/* Disk Q: */
		MyCopyTString(lpDiskInfo[1].lpLogin, LOGIN_MAX_LENGTH, lpLogin);
		MyCopyTString(lpDiskInfo[1].lpPassword, PASSWORD_MAX_LENGTH, lpPassword);
		MyCopyTString(lpDiskInfo[1].lpLocalName, LOCAL_NAME_MAX_LENGTH, TEXT("Q:"));
		MyCopyTString(lpDiskInfo[1].lpRemoteName, REMOTE_NAME_MAX_LENGTH,
			TEXT("\\\\deza.feec.vutbr.cz\\app"));

		/* Disk R: */
		MyCopyTString(lpDiskInfo[2].lpLogin, LOGIN_MAX_LENGTH, lpLogin);
		MyCopyTString(lpDiskInfo[2].lpPassword, PASSWORD_MAX_LENGTH, lpPassword);
		MyCopyTString(lpDiskInfo[2].lpLocalName, LOCAL_NAME_MAX_LENGTH, TEXT("R:"));
		MyCopyTString(lpDiskInfo[2].lpRemoteName, REMOTE_NAME_MAX_LENGTH,
			TEXT("\\\\deza.feec.vutbr.cz\\doc"));

		/* Disk S: */
		MyCopyTString(lpDiskInfo[3].lpLogin, LOGIN_MAX_LENGTH, lpLogin);
		MyCopyTString(lpDiskInfo[3].lpPassword, PASSWORD_MAX_LENGTH, lpPassword);
		MyCopyTString(lpDiskInfo[3].lpLocalName, LOCAL_NAME_MAX_LENGTH, TEXT("S:"));
		MyCopyTString(lpDiskInfo[3].lpRemoteName, REMOTE_NAME_MAX_LENGTH,
			TEXT("\\\\deza.feec.vutbr.cz\\grp"));

		/* Disk T: */
		MyCopyTString(lpDiskInfo[4].lpLogin, LOGIN_MAX_LENGTH, lpLogin);
		MyCopyTString(lpDiskInfo[4].lpPassword, PASSWORD_MAX_LENGTH, lpPassword);
		MyCopyTString(lpDiskInfo[4].lpLocalName, LOCAL_NAME_MAX_LENGTH, TEXT("T:"));
		MyCopyTString(lpDiskInfo[4].lpRemoteName, REMOTE_NAME_MAX_LENGTH,
			TEXT("\\\\deza.feec.vutbr.cz\\inst"));
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
	MAPPARAM tmpParam;

	tmpParam.dwGUIthreadId = GetCurrentThreadId();
	tmpParam.hwndToNotify = hwndDlg;

	/* Create stop event */
	tmpParam.hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (NULL == tmpParam.hStopEvent)
	{			
		return NULL;
	}

	/* Build disks info */
	tmpParam.lpDisks = BuildDiskInfoArray(lpLogin, lpId,
		lpPassword, &(tmpParam.uDisksCnt));
	if (NULL == tmpParam.lpDisks)
	{
		CloseHandle(lpNewParam->hStopEvent);
		return NULL;
	}

	lpNewParam = HeapAlloc(g_hHeap, 0, sizeof(MAPPARAM));
	if (NULL == lpNewParam)
	{
		CloseHandle(lpNewParam->hStopEvent);
		DestroyDiskInfoArray(tmpParam.lpDisks, tmpParam.uDisksCnt);
		return NULL;
	}

	/* Create Thread */
	tmpParam.hMapThread = CreateThread(NULL, 0, MapThread,
		lpNewParam, CREATE_SUSPENDED, NULL);
	if (NULL == tmpParam.hMapThread)
	{
		CloseHandle(lpNewParam->hStopEvent);
		DestroyDiskInfoArray(tmpParam.lpDisks, tmpParam.uDisksCnt);
		HeapFree(g_hHeap, 0, lpNewParam);

		return NULL;
	}

	/* Copy tmpParameters into allocated memory */
	*lpNewParam = tmpParam;		

	/* Start Mapping */
	ResumeThread(tmpParam.hMapThread);	

	return lpNewParam;
}
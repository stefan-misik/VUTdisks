/* ========================================================================== */
/*                                                                            */
/*   vut_disks.c                                                              */
/*   (c) 2014 Ing. Štefan Mišík                                               */
/*                                                                            */
/*   Description                                                              */
/*   Tool to connecting new VUT disks.                                        */
/*                                                                            */
/* ========================================================================== */

#pragma comment(linker, \
  "\"/manifestdependency:type='Win32' "\
  "name='Microsoft.Windows.Common-Controls' "\
  "version='6.0.0.0' "\
  "processorArchitecture='*' "\
  "publicKeyToken='6595b64144ccf1df' "\
  "language='*'\"")

#pragma comment(lib, "ComCtl32.lib")
#pragma comment(lib, "Mpr.lib")
#pragma comment(lib, "Crypt32.lib")

#ifndef UNICODE
    #define UNICODE
#endif

#define _WIN32_WINNT 0x600
#define OEMRESOURCE
#include <Windows.h>
#include <strsafe.h>
#include <CommCtrl.h>
#include <Wincrypt.h>


#ifdef _DEBUG
#include "MemDbg.h"
#endif

#include "resource.h"

#define WM_MAP_NOTIFY (WM_USER + 0x0001)
#define MAX_DISKS 8
#define LOGIN_MAX_LENGTH 64
#define PASSWORD_MAX_LENGTH 64
#define LOCAL_NAME_MAX_LENGTH 16
#define REMOTE_NAME_MAX_LENGTH MAX_PATH
#define ERROR_MESSAGE_MAX_LENGTH 16384


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

HINSTANCE g_hMyInstance;
HWND g_hwndMain;
HANDLE g_hLogoImage;
HANDLE g_hHeap;
LPTSTR g_lpCaption = TEXT("VUT Disk Mapper");
LPMAPPARAM g_lpMapParam = NULL;
BOOL g_bIsCancelling = FALSE;
BOOL g_bCloseAfterCancel = FALSE;
HKEY g_hMyRegKey;
BOOL g_bMyRegKeyValid = FALSE;

TCHAR g_lpLogin[LOGIN_MAX_LENGTH];
TCHAR g_lpId[LOGIN_MAX_LENGTH];
TCHAR g_lpPassword[PASSWORD_MAX_LENGTH];

HICON g_hSmallWarnIcon;
HMENU g_hMainMenu = NULL;

DWORD g_lpdwErrors[MAX_DISKS];
UINT g_uErrorsCnt = 0;
BOOL g_bErrors = FALSE;

/******************************************************************************/
BOOL RunAtStartup(BOOL bEnable)
{
	HKEY hKeyCurrUser, hKey;
	static TCHAR lpExeName[1024];

	/* Open CurrentUser */
	if (RegOpenCurrentUser(KEY_CREATE_SUB_KEY, &hKeyCurrUser) != ERROR_SUCCESS)
	{
		return FALSE;
	}

	/* Open Windows/Run key */
	if (RegOpenKeyEx(hKeyCurrUser,
		TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 0,
		KEY_SET_VALUE | KEY_WOW64_32KEY, &hKey) != 
		ERROR_SUCCESS)
	{
		RegCloseKey(hKeyCurrUser);
		return FALSE;
	}

	/* If id enabling */
	if (TRUE == bEnable)
	{
		DWORD dwRes;

		/* Get path to current EXE file */
		dwRes = GetModuleFileName(NULL, lpExeName, 1024);

		/* Return, if filename was nor received correctly */
		if (dwRes == 0 && dwRes >= 1024)
		{
			RegCloseKey(hKeyCurrUser);
			return FALSE;
		}	
		
		/* Ty to write filename of current executable to registry */
		if (ERROR_SUCCESS != RegSetValueEx(hKey, g_lpCaption, 0,
			REG_SZ, (LPBYTE)lpExeName, (dwRes + 1) * sizeof(TCHAR)))
		{
			RegCloseKey(hKey);
			RegCloseKey(hKeyCurrUser);
			return FALSE;
		}				
	}
	else
	{
		/* Delete registry value holding filename of VUT Disk Mapper */
		RegDeleteValue(hKey, g_lpCaption);
	}

	/* Close all registry keys */
	RegCloseKey(hKey);
	RegCloseKey(hKeyCurrUser);

	return TRUE;
}

/******************************************************************************/
BOOL IsRegisteredToRunAtStartup(VOID)
{
	HKEY hKeyCurrUser, hKey;
	LONG lRes;

	/* Open CurrentUser */
	if (RegOpenCurrentUser(KEY_CREATE_SUB_KEY, &hKeyCurrUser) != ERROR_SUCCESS)
	{
		return FALSE;
	}

	/* Open Windows/Run key */
	if (RegOpenKeyEx(hKeyCurrUser,
		TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 0,
		KEY_QUERY_VALUE | KEY_WOW64_32KEY, &hKey) != 
		ERROR_SUCCESS)
	{
		RegCloseKey(hKeyCurrUser);
		return FALSE;
	}

	/* Check if specified registry value exists */
	lRes = RegQueryValueEx(hKey, g_lpCaption, NULL,
		NULL, NULL, NULL);

	/* Close all registry keys */
	RegCloseKey(hKey);
	RegCloseKey(hKeyCurrUser);

	/* If value exists */
	if (ERROR_SUCCESS == lRes)
	{		
		return TRUE;		
	}

	return FALSE;
}

/******************************************************************************/
BOOL DeleteMyRegKey(VOID)
{
	HKEY	hKeyCurrUser;

	if (RegOpenCurrentUser(KEY_CREATE_SUB_KEY, &hKeyCurrUser) != ERROR_SUCCESS)
	{
		return FALSE;
	}

	if (RegDeleteKey(hKeyCurrUser, TEXT("Software\\VUTdisks")) != ERROR_SUCCESS)
	{
		return FALSE;
	}

	return TRUE;
}

/******************************************************************************/
BOOL OpenMyRegKey(PHKEY phKey)
{
	HKEY	hKeyCurrUser;
	DWORD	dwDisposition;

	if (RegOpenCurrentUser(KEY_CREATE_SUB_KEY, &hKeyCurrUser) != ERROR_SUCCESS)
	{
		return FALSE;
	}
	
	if (RegCreateKeyEx(hKeyCurrUser, TEXT("Software\\VUTdisks"), 0,
		NULL, 0, KEY_WRITE | KEY_READ | KEY_WOW64_32KEY,
		NULL, phKey, &dwDisposition) != ERROR_SUCCESS)
	{
		RegCloseKey(hKeyCurrUser);
		return FALSE;
	}

	RegCloseKey(hKeyCurrUser);

	if (dwDisposition == REG_CREATED_NEW_KEY)
	{
		RegSetValueEx(*phKey, TEXT("szLogin"), 0,
			REG_SZ, (LPBYTE)NULL, 0);

		RegSetValueEx(*phKey, TEXT("szId"), 0,
			REG_SZ, (LPBYTE)NULL, 0);

		RegSetValueEx(*phKey, TEXT("pbPassword"), 0,
			REG_BINARY, (LPBYTE)NULL, 0);
	}

	return TRUE;

}

/******************************************************************************/
VOID ReadRegistry(HKEY hKey, HWND hwnd)
{	
	DWORD dwLength;
	DWORD dwType;

	/* Read login */
	dwLength = (LOGIN_MAX_LENGTH - 1) * sizeof(TCHAR);
	RegQueryValueEx(hKey, TEXT("szLogin"), 0,
		&dwType, (LPBYTE)g_lpLogin, &dwLength);

	if (REG_SZ != dwType)
	{
		dwLength = 0;
	}	
	g_lpLogin[dwLength] = TEXT('\0');	
	SetDlgItemText(hwnd, IDC_LOGIN, g_lpLogin);

	/* Read ID */
	dwLength = (LOGIN_MAX_LENGTH - 1) * sizeof(TCHAR);
	RegQueryValueEx(hKey, TEXT("szId"), 0,
		&dwType, (LPBYTE)g_lpLogin, &dwLength);

	if (REG_SZ != dwType)
	{
		dwLength = 0;
	}
	g_lpLogin[dwLength] = TEXT('\0');
	SetDlgItemText(hwnd, IDC_ID, g_lpLogin);

	/* Read Password length*/
	if (ERROR_SUCCESS ==
		RegQueryValueEx(hKey, TEXT("pbPassword"), NULL,
		&dwType, NULL, &dwLength))
	{
		DATA_BLOB dbInput, dbOutput;
		PBYTE pbData = HeapAlloc(g_hHeap, 0, dwLength);

		/* Read Password */
		if (ERROR_SUCCESS ==
			RegQueryValueEx(hKey, TEXT("pbPassword"), NULL,
			&dwType, pbData, &dwLength))
		{
			dbInput.pbData = pbData;
			dbInput.cbData = dwLength;

			/* Try to decrypt Password */
			if (TRUE == CryptUnprotectData(
				&dbInput, NULL, NULL, NULL, NULL, 0, &dbOutput))
			{
				/* Set Password */
				CopyMemory(g_lpPassword, dbOutput.pbData, dbOutput.cbData);
				g_lpPassword[dbOutput.cbData] = TEXT('\0');

				SetDlgItemText(hwnd, IDC_PASSWD, (LPTSTR)g_lpPassword);
				SecureZeroMemory(dbOutput.pbData, dbOutput.cbData);
				SecureZeroMemory(g_lpPassword, PASSWORD_MAX_LENGTH);
				LocalFree(dbOutput.pbData);

				SendDlgItemMessage(hwnd, IDC_SAVE_PASS, BM_SETCHECK,
					(WPARAM)BST_CHECKED, 0);
			}
			else
			{
				SetDlgItemText(hwnd, IDC_PASSWD, NULL);
			}

		}
		else
		{
			SetDlgItemText(hwnd, IDC_PASSWD, NULL);
		}

		HeapFree(g_hHeap, 0, pbData);
	}
	else
	{
		SetDlgItemText(hwnd, IDC_PASSWD, NULL);
	}
}

/******************************************************************************/
VOID WriteRegistry(HKEY hKey, HWND hwnd)
{	
	DWORD dwLength;

	/* Save Login */
	dwLength = GetDlgItemText(hwnd, IDC_LOGIN, g_lpLogin, LOGIN_MAX_LENGTH);
	RegSetValueEx(hKey, TEXT("szLogin"), 0,
		REG_SZ, (LPBYTE)g_lpLogin, dwLength * sizeof(TCHAR));

	/* Save Id */
	dwLength = GetDlgItemText(hwnd, IDC_ID, g_lpId, LOGIN_MAX_LENGTH);
	RegSetValueEx(hKey, TEXT("szId"), 0,
		REG_SZ, (LPBYTE)g_lpId, dwLength * sizeof(TCHAR));

	/* If 'Save Password' is checked */	
	if (BST_CHECKED ==
		SendDlgItemMessage(hwnd, IDC_SAVE_PASS, BM_GETCHECK, 0, 0))
	{
		DATA_BLOB dbInput, dbOutput;

		dbInput.cbData = GetDlgItemText(hwnd, IDC_PASSWD, g_lpPassword,
			PASSWORD_MAX_LENGTH) * sizeof(TCHAR);
		dbInput.pbData = (PBYTE)g_lpPassword;

		/* Encrypt Password */
		if (FALSE ==
			CryptProtectData(&dbInput, NULL, NULL, NULL,
			NULL, 0, &dbOutput))
		{
			dbOutput.cbData = 0;
			dbOutput.pbData = NULL;
		}		

		/* Store Password */
		RegSetValueEx(hKey, TEXT("pbPassword"), 0,
			REG_BINARY, dbOutput.pbData, dbOutput.cbData);

		if (dbOutput.cbData > 0)
		{
			LocalFree(dbOutput.pbData);
		}
	}
	else
	{
		/* Clear Password */
		RegSetValueEx(hKey, TEXT("pbPassword"), 0,
			REG_BINARY, (LPBYTE)NULL, 0);
	}
}


/******************************************************************************/
BOOL ProgressBarMarquee(HWND hWnd, BOOL bState)
{
	DWORD dwStyle;
	BOOL bOld;

	dwStyle = GetWindowLong(hWnd, GWL_STYLE);

	bOld = ((dwStyle | PBS_MARQUEE) != 0);

	if (TRUE == bState)
	{
		dwStyle |= PBS_MARQUEE;
	}
	else
	{
		dwStyle &= (~PBS_MARQUEE);
	}

	SetWindowLong(hWnd, GWL_STYLE, dwStyle);

	return bOld;
}

/******************************************************************************/
DWORD WINAPI MapThread(LPVOID lpParameter)
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
			param->lpDisks[c].lpPassword, param->lpDisks[c].lpLogin, CONNECT_TEMPORARY);

		/* Notify GUI thread */
		if (NULL != param->hwndToNotify)
		{
			PostThreadMessage(param->dwGUIthreadId, WM_MAP_NOTIFY, (WPARAM)c, (LPARAM)param);
		}
		
	}

	/* Notify GUI thread */
	if (NULL != param->hwndToNotify)
	{
		PostThreadMessage(param->dwGUIthreadId, WM_MAP_NOTIFY, (WPARAM)MAX_DISKS, (LPARAM)param);
	}

	ExitThread(c);
}

/******************************************************************************/
void DestroyDiskInfoArray(LPDISKINFO lpDiskInfo, UINT uCount)
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
				SecureZeroMemory(lpDiskInfo[c].lpPassword, 
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
LPDISKINFO BuildDiskInfoArray(LPTSTR lpLogin,
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
		StringCchCopy(lpDiskInfo[0].lpLogin, LOGIN_MAX_LENGTH, lpLogin);
		StringCchCopy(lpDiskInfo[0].lpPassword, PASSWORD_MAX_LENGTH, lpPassword);
		StringCchCopy(lpDiskInfo[0].lpLocalName, LOCAL_NAME_MAX_LENGTH, TEXT("P:"));
		StringCchPrintf(lpDiskInfo[0].lpRemoteName, REMOTE_NAME_MAX_LENGTH,
			TEXT("\\\\ad.feec.vutbr.cz\\homes.id\\vutbr.cz\\%s"), lpId);

		/* Disk Q: */
		StringCchCopy(lpDiskInfo[1].lpLogin, LOGIN_MAX_LENGTH, lpLogin);
		StringCchCopy(lpDiskInfo[1].lpPassword, PASSWORD_MAX_LENGTH, lpPassword);
		StringCchCopy(lpDiskInfo[1].lpLocalName, LOCAL_NAME_MAX_LENGTH, TEXT("Q:"));
		StringCchCopy(lpDiskInfo[1].lpRemoteName, REMOTE_NAME_MAX_LENGTH,
			TEXT("\\\\deza.feec.vutbr.cz\\app"));

		/* Disk R: */
		StringCchCopy(lpDiskInfo[2].lpLogin, LOGIN_MAX_LENGTH, lpLogin);
		StringCchCopy(lpDiskInfo[2].lpPassword, PASSWORD_MAX_LENGTH, lpPassword);
		StringCchCopy(lpDiskInfo[2].lpLocalName, LOCAL_NAME_MAX_LENGTH, TEXT("R:"));
		StringCchCopy(lpDiskInfo[2].lpRemoteName, REMOTE_NAME_MAX_LENGTH,
			TEXT("\\\\deza.feec.vutbr.cz\\doc"));

		/* Disk S: */
		StringCchCopy(lpDiskInfo[3].lpLogin, LOGIN_MAX_LENGTH, lpLogin);
		StringCchCopy(lpDiskInfo[3].lpPassword, PASSWORD_MAX_LENGTH, lpPassword);
		StringCchCopy(lpDiskInfo[3].lpLocalName, LOCAL_NAME_MAX_LENGTH, TEXT("S:"));
		StringCchCopy(lpDiskInfo[3].lpRemoteName, REMOTE_NAME_MAX_LENGTH,
			TEXT("\\\\deza.feec.vutbr.cz\\grp"));

		/* Disk T: */
		StringCchCopy(lpDiskInfo[4].lpLogin, LOGIN_MAX_LENGTH, lpLogin);
		StringCchCopy(lpDiskInfo[4].lpPassword, PASSWORD_MAX_LENGTH, lpPassword);
		StringCchCopy(lpDiskInfo[4].lpLocalName, LOCAL_NAME_MAX_LENGTH, TEXT("T:"));
		StringCchCopy(lpDiskInfo[4].lpRemoteName, REMOTE_NAME_MAX_LENGTH,
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
	UINT uRetVal = 0;

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

/******************************************************************************/
VOID ShowWarningMessage(BOOL bVisible)
{
	int iCmd = (TRUE == bVisible) ? SW_SHOW : SW_HIDE;

	ShowWindow(GetDlgItem(g_hwndMain, IDC_WARNS), iCmd);
	ShowWindow(GetDlgItem(g_hwndMain, IDC_ERR_LIST), iCmd);
}

/******************************************************************************/
VOID DisplayErrMessage(HWND hwndParent)
{
	static TCHAR lpErrMessage[ERROR_MESSAGE_MAX_LENGTH];
	UINT_PTR uPos = 0;
	UINT uDisk;
	static LPTSTR lpDisks[] = {
	TEXT("P: "),
	TEXT("Q: "), 
	TEXT("R: "), 
	TEXT("S: "), 
	TEXT("T: ") };

	uPos += FormatMessage(FORMAT_MESSAGE_FROM_STRING,
		TEXT("Disk mapping finished with following results:\r\n\r\n"),
		0, 0, (LPTSTR)(((UINT_PTR)lpErrMessage) + (uPos * sizeof(TCHAR))), 
		(ERROR_MESSAGE_MAX_LENGTH - uPos), NULL);

	for (uDisk = 0; uDisk < g_uErrorsCnt; uDisk++)
	{
		uPos += FormatMessage(FORMAT_MESSAGE_FROM_STRING,
			lpDisks[uDisk],
			0, 0, (LPTSTR)(((UINT_PTR)lpErrMessage) + (uPos * sizeof(TCHAR))),
			(ERROR_MESSAGE_MAX_LENGTH - uPos), NULL);

		uPos += FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL,
			g_lpdwErrors[uDisk], 0, 
			(LPTSTR)(((UINT_PTR)lpErrMessage) + (uPos * sizeof(TCHAR))),
			(ERROR_MESSAGE_MAX_LENGTH - uPos), NULL);
	}

	MessageBox(hwndParent, lpErrMessage, g_lpCaption, MB_OK);
}

/******************************************************************************/
INT_PTR CALLBACK DialogProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
	switch (uMsg)
	{
	case WM_DRAWITEM:
		if (IDC_LOGO == wParam)
		{
			HDC hdc = ((LPDRAWITEMSTRUCT)lParam)->hDC;
			RECT rc = ((LPDRAWITEMSTRUCT)lParam)->rcItem;

			int left = 0;
			int height = rc.bottom;

			/* If Image is correctly loaded, draw it */
			if (NULL != g_hLogoImage)
			{
				HDC hdcMem = CreateCompatibleDC(hdc);
				BITMAP bitmap;
				HGDIOBJ oldBitmap = SelectObject(hdcMem, g_hLogoImage);

				GetObject(g_hLogoImage, sizeof(bitmap), &bitmap);
				BitBlt(hdc, 0, 0, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, SRCCOPY);

				left = bitmap.bmWidth;
				height = bitmap.bmHeight;

				SelectObject(hdcMem, oldBitmap);
				DeleteDC(hdcMem);
			}

			/* Draw white background */
			if (left < rc.right)
			{
				SelectObject(hdc, GetStockObject(DC_PEN));
				SelectObject(hdc, GetStockObject(DC_BRUSH));
				SetDCBrushColor(hdc, RGB(255, 255, 255));
				SetDCPenColor(hdc, RGB(255, 255, 255));
				Rectangle(hdc, left, 0, rc.right, height);
			}

			return TRUE;
		}
		return FALSE;

	case WM_MAP_NOTIFY:

		if (wParam < ((LPMAPPARAM)lParam)->uDisksCnt)
		{
			DWORD dwError;

			SendDlgItemMessage(hwndDlg, IDC_PROGRESS,
				PBM_SETPOS, (wParam + 1), 0);

			dwError = ((LPMAPPARAM)lParam)->lpDisks[wParam].dwResult;
			if (dwError != NO_ERROR)
			{
				ShowWarningMessage(TRUE);
				g_bErrors = TRUE;
			}

			g_lpdwErrors[g_uErrorsCnt++] = dwError;
		}
		else if (MAX_DISKS == wParam)
		{
			HWND hwndToNotify = g_lpMapParam->hwndToNotify;

			EndMapDisks(((LPMAPPARAM)lParam), INFINITE);
			if (((LPMAPPARAM)lParam) == g_lpMapParam)
			{
				g_lpMapParam = NULL;
			}

			/* Update UI */
			EnableWindow(GetDlgItem(hwndToNotify, IDOK), TRUE);
			EnableWindow(GetDlgItem(hwndToNotify, IDC_LOGIN), TRUE);
			EnableWindow(GetDlgItem(hwndToNotify, IDC_ID), TRUE);
			EnableWindow(GetDlgItem(hwndToNotify, IDC_PASSWD), TRUE);
			EnableWindow(GetDlgItem(hwndToNotify, IDCANCEL), TRUE);
			EnableWindow(GetDlgItem(hwndToNotify, IDC_SAVE_PASS), TRUE);

			if (TRUE == g_bIsCancelling)
			{
				g_bIsCancelling = FALSE;
				ProgressBarMarquee(GetDlgItem(hwndToNotify, IDC_PROGRESS), 
					FALSE);


				if (TRUE == g_bCloseAfterCancel)
				{
					DestroyWindow(g_hwndMain);
				}
			}

			if (FALSE == g_bErrors)
			{
				DestroyWindow(hwndToNotify);
			}

			SetDlgItemText(hwndToNotify, IDCANCEL, TEXT("Close"));

		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code)
		{
		case NM_CLICK:
		case NM_RETURN:			
			switch (((LPNMHDR)lParam)->idFrom)
			{
			case IDC_MAILTO:			
				ShellExecute(NULL, TEXT("open"),
					TEXT("mailto:xmisik00@stud.feec.vutbr.cz"), NULL,
					NULL, SW_SHOW);
				return TRUE;
			
			case IDC_ERR_LIST:
				DisplayErrMessage(hwndDlg);
				return TRUE;
			}
			return FALSE;			
		}
		return FALSE;

	case WM_COMMAND:
	{		
		switch (HIWORD(wParam))
		{
		case 0:
			switch (LOWORD(wParam))
			{
			case ID_REMOVEREGISTRYDATA:

				if (IDYES == MessageBox(hwndDlg,
					TEXT("This will remove Login, Id and Password ")\
					TEXT("stored in Registry database.\r\n")\
					TEXT("\r\nDo you wish to continue?"),
					g_lpCaption, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2))
				{
					if (TRUE == g_bMyRegKeyValid)
					{
						RegCloseKey(g_hMyRegKey);
						g_bMyRegKeyValid = FALSE;
					}
					DeleteMyRegKey();
				}
				
				return TRUE;

			case ID_RUNATSTARTUP:
				if (NULL != g_hMainMenu)
				{
					if (GetMenuState(g_hMainMenu, ID_RUNATSTARTUP, MF_BYCOMMAND) &
						MF_CHECKED)
					{
						if (TRUE == RunAtStartup(FALSE))
						{
							CheckMenuItem(g_hMainMenu, ID_RUNATSTARTUP,
								MF_BYCOMMAND | MF_UNCHECKED);
						}
					}
					else
					{
						if (TRUE == RunAtStartup(TRUE))
						{
							CheckMenuItem(g_hMainMenu, ID_RUNATSTARTUP,
								MF_BYCOMMAND | MF_CHECKED);
						}
					}
				}
				return TRUE;
			
			case IDCANCEL:
				if (NULL == g_lpMapParam)
				{
					DestroyWindow(hwndDlg);
				}
				else
				{
					if (FALSE == RequestStopMapDisks(g_lpMapParam))
					{
						/* Fatal Error */
						PostQuitMessage(1);
					}
					else
					{
						g_bIsCancelling = TRUE;

						EnableWindow(
							GetDlgItem(g_lpMapParam->hwndToNotify, IDCANCEL), 
							FALSE);

						ProgressBarMarquee(
							GetDlgItem(hwndDlg, IDC_PROGRESS), TRUE);

						SendDlgItemMessage(hwndDlg, IDC_PROGRESS, 
							PBM_SETMARQUEE, TRUE, 0);
					}
				}
				return TRUE;

			case IDOK:
				if (NULL == g_lpMapParam)
				{
					UINT cnt;

					ShowWarningMessage(FALSE);
					g_uErrorsCnt = 0;

					/* Read values from input fields */
					cnt = GetDlgItemText(hwndDlg, IDC_LOGIN, g_lpLogin, LOGIN_MAX_LENGTH);
					cnt *= GetDlgItemText(hwndDlg, IDC_ID, g_lpId, LOGIN_MAX_LENGTH);
					cnt *= GetDlgItemText(hwndDlg, IDC_PASSWD, g_lpPassword, 
						PASSWORD_MAX_LENGTH);

					/* If all fields were filled-in */
					if (cnt > 0)
					{
						if (TRUE == g_bMyRegKeyValid)
						{
							WriteRegistry(g_hMyRegKey, hwndDlg);
						}

						/* Begin Disk Mapping */
						g_lpMapParam = BeginMapDisks(hwndDlg, g_lpLogin,
							g_lpId, g_lpPassword);

						if (NULL != g_lpMapParam)
						{
							/* Update UI */
							EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);
							EnableWindow(GetDlgItem(hwndDlg, IDC_LOGIN), FALSE);
							EnableWindow(GetDlgItem(hwndDlg, IDC_ID), FALSE);
							EnableWindow(GetDlgItem(hwndDlg, IDC_PASSWD), FALSE);
							EnableWindow(GetDlgItem(hwndDlg, IDC_SAVE_PASS), FALSE);

							ProgressBarMarquee(GetDlgItem(hwndDlg, IDC_PROGRESS), FALSE);
							SendDlgItemMessage(hwndDlg, IDC_PROGRESS, PBM_SETSTATE, PBST_NORMAL, 0);
							SendDlgItemMessage(hwndDlg, IDC_PROGRESS, PBM_SETRANGE, 0,
								MAKELPARAM(0, (g_lpMapParam->uDisksCnt)));
							SendDlgItemMessage(hwndDlg, IDC_PROGRESS,
								PBM_SETPOS, 0, 0);

							SetDlgItemText(hwndDlg, IDCANCEL, TEXT("Cancel"));
							g_bErrors = FALSE;
						}
						else
						{
							MessageBox(hwndDlg, TEXT("Could not map disks."), g_lpCaption, MB_OK | MB_ICONHAND);
						}

						/* Zero password memory */
						SecureZeroMemory(g_lpPassword, PASSWORD_MAX_LENGTH * sizeof(TCHAR));
					}
					else
					{
						MessageBox(hwndDlg, TEXT("Please fill in all of the required fields"),
							g_lpCaption, MB_OK | MB_ICONINFORMATION);
					}
				}
				return TRUE;

			}
			break;

		case EN_CHANGE:
			switch (LOWORD(wParam))
			{
			case IDC_LOGIN:
			case IDC_ID:

				/* Clear Password field */
				SetDlgItemText(hwndDlg, IDC_PASSWD, NULL);

			case IDC_PASSWD:
				/* Uncheck 'Save Password' */
				SendDlgItemMessage(hwndDlg, IDC_SAVE_PASS, BM_SETCHECK,
					(WPARAM)BST_UNCHECKED, 0);
				return TRUE;
			}
			return FALSE;

		default:
			break;
		}
		return FALSE;
	}

	case WM_CLOSE:
		if (NULL != g_lpMapParam)
		{
			if (TRUE == g_bIsCancelling)
			{
				EndMapDisks(g_lpMapParam, 5000);
				g_lpMapParam = NULL;
				DestroyWindow(hwndDlg);
			}
			else
			{
				if (FALSE == RequestStopMapDisks(g_lpMapParam))
				{
					/* Fatal Error */
					PostQuitMessage(1);
				}
				else
				{
					g_bIsCancelling = TRUE;
					g_bCloseAfterCancel = TRUE;

					EnableWindow(
						GetDlgItem(g_lpMapParam->hwndToNotify, IDCANCEL),
						FALSE);

					ProgressBarMarquee(
						GetDlgItem(hwndDlg, IDC_PROGRESS), TRUE);

					SendDlgItemMessage(hwndDlg, IDC_PROGRESS,
						PBM_SETMARQUEE, TRUE, 0);
				}
			}
		}
		else
		{
			DestroyWindow(hwndDlg);
		}
		return TRUE;


	case WM_DESTROY:
		PostQuitMessage(0);
		return TRUE;

	case WM_INITDIALOG:
	{
		MONITORINFO mi;


		InitCommonControls();

		/* Move windows to the center of monitor */
		mi.cbSize = sizeof(MONITORINFO);
		if (FALSE != GetMonitorInfo(
			MonitorFromWindow(hwndDlg, MONITOR_DEFAULTTOPRIMARY), &mi))
		{
			RECT rc;
			if (TRUE == GetWindowRect(hwndDlg, &rc))
			{
				LONG lWidth = rc.right - rc.left;
				LONG lHeight = rc.bottom - rc.top;

				MoveWindow(hwndDlg,
					((mi.rcWork.right - mi.rcWork.left) / 2) + mi.rcWork.left - (lWidth / 2),
					((mi.rcWork.bottom - mi.rcWork.top) / 2) + mi.rcWork.top - (lHeight / 2),
					lWidth,
					lHeight,
					TRUE);
			}
		}

		/* Set Small Warning icon */
		if (NULL != g_hSmallWarnIcon)
		{
			SendDlgItemMessage(hwndDlg, IDC_WARNS, STM_SETICON,
				(WPARAM)g_hSmallWarnIcon, 0);
		}		

		/* Set Edit Controls Limits */
		SendDlgItemMessage(hwndDlg, IDC_LOGIN, EM_SETLIMITTEXT,
			(WPARAM)(LOGIN_MAX_LENGTH - 1), 0);
		SendDlgItemMessage(hwndDlg, IDC_ID, EM_SETLIMITTEXT,
			(WPARAM)(LOGIN_MAX_LENGTH - 1), 0);
		SendDlgItemMessage(hwndDlg, IDC_PASSWD, EM_SETLIMITTEXT,
			(WPARAM)(PASSWORD_MAX_LENGTH - 1), 0);

		/* Store handle to the Menu */
		g_hMainMenu = GetMenu(hwndDlg);

		/* If already registered to run at startup, chceck the menu */
		CheckMenuItem(g_hMainMenu, ID_RUNATSTARTUP,
			MF_BYCOMMAND | (MF_CHECKED * IsRegisteredToRunAtStartup()));
		

		if (TRUE == g_bMyRegKeyValid)
		{
			ReadRegistry(g_hMyRegKey, hwndDlg);
		}
	}

		return TRUE;
	default:
		return FALSE;		
	}    
	return FALSE;
}

/******************************************************************************/
HWND CreateToolTip(int toolID, HWND hDlg, PTSTR pszText, INT iMaxWidth)
{	
	// Get the window of the tool.
	HWND hwndTool = GetDlgItem(hDlg, toolID);

	// Create the tooltip. g_hInst is the global instance handle.
	HWND hwndTip = CreateWindowEx(0, TOOLTIPS_CLASS, NULL,
		WS_POPUP | TTS_ALWAYSTIP ,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		hDlg, NULL,
		g_hMyInstance, NULL);

	if (NULL == hwndTool || NULL == hwndTip)
	{
		return (HWND)NULL;
	}

	// Associate the tooltip with the tool.
	TOOLINFO toolInfo = { 0 };
	toolInfo.cbSize = sizeof(toolInfo);	
	toolInfo.hwnd = hDlg;
	toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
	toolInfo.uId = (UINT_PTR)hwndTool;
	toolInfo.lpszText = pszText;
	SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);

	SendMessage(hwndTip, TTM_SETMAXTIPWIDTH, 0, iMaxWidth);

	return hwndTip;
}


/******************************************************************************/
INT WINAPI WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow
	)
{
	MSG msg;
	HICON hIcon;
	HWND hwndPassTooltip;
		
	g_hMyInstance = hInstance;

#ifdef _DEBUG
	InitMemDbg();
#endif

	/* Get Process heap */
	g_hHeap = GetProcessHeap();

	/* Open Registry Key */
	g_bMyRegKeyValid = OpenMyRegKey(&g_hMyRegKey);

	g_hSmallWarnIcon = LoadImage(g_hMyInstance, MAKEINTRESOURCE(IDI_WARN),
		IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

	/* Create main Window */
	g_hwndMain = CreateDialogParam(
		hInstance,
		MAKEINTRESOURCE(IDD_MAIN_WND),
		NULL,
		DialogProc,
		(LPARAM)NULL
	);

	if (NULL == g_hwndMain)
	{
		MessageBox(NULL, TEXT("Application Initialization Failed!"),
			TEXT("Error!"), MB_OK | MB_ICONHAND);
		return -1;
	}	

	/* Create Tooltip */
	hwndPassTooltip = CreateToolTip(IDC_SAVE_PASS, g_hwndMain, 
		TEXT("Password will be stored in encrypted form inside\r\n")\
		TEXT("Windows Registry database. Encryption will be\r\n")\
		TEXT("performed using WIN32 Crypto API.\r\n\r\n")\
		TEXT("WARNING\r\n")\
		TEXT("Stored Password can be decrypted by any application \r\n")\
		TEXT("running as current Windows user on this computer."),
		800);
	if (NULL != hwndPassTooltip)
	{
		hIcon = LoadIcon(NULL, IDI_SHIELD);
		if (NULL == hIcon)
		{
			hIcon = LoadIcon(NULL, IDI_INFORMATION);
		}
		SendMessage(hwndPassTooltip, TTM_SETTITLE, (WPARAM)hIcon, (LPARAM)TEXT("Security Considerations"));		
		
		DestroyIcon(hIcon);
		SendMessage(hwndPassTooltip, TTM_SETDELAYTIME, (WPARAM)TTDT_AUTOPOP, 25000);
	}

	/* Set caption of main window */
	SetWindowText(g_hwndMain, g_lpCaption);

	/* Load and set the icon */
	hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN));
	if (NULL != hIcon)
	{
		SendMessage(g_hwndMain, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		SendMessage(g_hwndMain, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

		DestroyIcon(hIcon);
	}
	
	/* Load Logo image */
	g_hLogoImage = LoadImage(hInstance, MAKEINTRESOURCE(IDB_LOGO), IMAGE_BITMAP, 0, 0, LR_SHARED);

	    
	/* Draw Window */
	ShowWindow(g_hwndMain, nCmdShow);
    
    
	/* Enter the message loop */
	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		/* If message is a Map Thread notification, dispatch the message */
		if (WM_MAP_NOTIFY == msg.message)
		{
			msg.hwnd = ((LPMAPPARAM)(msg.lParam))->hwndToNotify;
			DispatchMessage(&msg);
		}
		else
		{
			if (FALSE == IsDialogMessage(g_hwndMain, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
    }

	/* Delete logo image object*/
	DeleteObject(g_hLogoImage);

	/* Destroy password tooltip window */
	if (NULL != hwndPassTooltip)
	{
		DestroyWindow(hwndPassTooltip);
	}

	/* Close My Reg Key */
	if (TRUE == g_bMyRegKeyValid)
	{
		RegCloseKey(g_hMyRegKey);
		g_bMyRegKeyValid = FALSE;
	}

	/* Destroy Warn Icon */
	if (NULL != g_hSmallWarnIcon)
	{
		DestroyIcon(g_hSmallWarnIcon);
	}

#ifdef _DEBUG
	WriteMemDbgMessage();
#endif
	
	ExitProcess((UINT)(msg.wParam));

   return (INT)(msg.wParam);
}
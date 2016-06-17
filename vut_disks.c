/* ========================================================================== */
/*                                                                            */
/*   vut_disks.c                                                              */
/*   (c) 2014 Ing. Stefan Misik                                               */
/*                                                                            */
/*   Description                                                              */
/*   Tool to connecting new VUT disks.                                        */
/*                                                                            */
/* ========================================================================== */

#include "vut_disks.h"
#include "resource.h"

#include "disk_mapper.h"
#include "registry.h"
#include "win_tile_manifest.h"



HINSTANCE g_hMyInstance;
HWND g_hwndMain;
HANDLE g_hHeap;
LPTSTR g_lpCaption = TEXT("VUT Disk Mapper");


TCHAR g_lpLogin[LOGIN_MAX_LENGTH];
TCHAR g_lpId[LOGIN_MAX_LENGTH];
TCHAR g_lpPassword[PASSWORD_MAX_LENGTH];

HICON g_hSmallWarnIcon;
HMENU g_hMainMenu = NULL;
HANDLE g_hLogoImage;

LPMAPPARAM g_lpMapParam = NULL;
BOOL g_bIsCancelling = FALSE;
BOOL g_bCloseAfterCancel = FALSE;



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
					TEXT("mailto:stefan.misik@phd.feec.vutbr.cz"), NULL,
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
                
            case ID_VISUALMANIFEST:
                if (IDYES == MessageBox(hwndDlg,
					TEXT("This will create VisualElementsManifest.xml file ")
					TEXT("which graphically integrates application tile ")
                    TEXT("in Windows 8.1/10 start menu, when it is ")
                    TEXT("created.\r\n")
					TEXT("\r\nDo you wish to continue?"),
					g_lpCaption, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2))
				{					
					/* Create Windows 8.1/10 Tile style Manifest if 
                     * there  is none */
                    CreateWinTileManifest();
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
						WriteRegistry(hwndDlg);

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
						ZeroMemory(g_lpPassword, PASSWORD_MAX_LENGTH * sizeof(TCHAR));
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
                INITCOMMONCONTROLSEX icex;
        
                /* Initialize common controls */        
                icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
                icex.dwICC = ICC_STANDARD_CLASSES;
                InitCommonControlsEx(&icex);
		
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

		/* If already registered to run at startup, check the menu */
		CheckMenuItem(g_hMainMenu, ID_RUNATSTARTUP,
			MF_BYCOMMAND | (MF_CHECKED * IsRegisteredToRunAtStartup()));		

		/* Read registry data */
        ReadRegistry(hwndDlg);
		
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
    
    
	/* Get Process heap */
	g_hHeap = GetProcessHeap();
    
    /* Open Registry Key */
	OpenMyRegKey();

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
        
        /* Invalidate Whole Window */
        InvalidateRect(g_hwndMain, NULL, FALSE);
	    
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
	CloseMyRegKey();

	/* Destroy Warn Icon */
	if (NULL != g_hSmallWarnIcon)
	{
		DestroyIcon(g_hSmallWarnIcon);
	}

	
	ExitProcess((UINT)(msg.wParam));

   return (INT)(msg.wParam);
}
/* ========================================================================== */
/*                                                                            */
/*   vut_disks.c                                                              */
/*   (c) 2014 Ing. Stefan Misik                                               */
/*                                                                            */
/*   Description                                                              */
/*   Tool to connecting new VUT Brno disks.                                   */
/*                                                                            */
/* ========================================================================== */
#include <Shlwapi.h>

#include "vut_disks.h"
#include "resource.h"
#include "defs.h"
#include "disk_mapper.h"
#include "registry.h"
#include "win_tile_manifest_gen.h"
#include "about_dialog.h"

#define DEFAULT_PASSWD_CHAR 0x25CF

#define LOGO_IMAGE_NAME PROJECT_NAME ".Logo.png"

const LPTSTR g_lpDisks[VUT_DISK_NUM] = {
    TEXT("P:"),
    TEXT("Q:"), 
    TEXT("R:"), 
    TEXT("S:"), 
    TEXT("T:"),
    TEXT("V:")
};

HINSTANCE g_hMyInstance;
HWND g_hwndMain;
HANDLE g_hHeap;
UINT g_uErrorsCnt;

LPTSTR g_lpCaption = TEXT(PROJECT_DESC);

TCHAR g_lpLogin[LOGIN_MAX_LENGTH];
TCHAR g_lpId[LOGIN_MAX_LENGTH];
TCHAR g_lpPassword[PASSWORD_MAX_LENGTH];
TCHAR g_lpDiskSelect[DISKSELECT_MAX_LENGTH];
BOOL g_bDiskSelectChanged = FALSE;
DISKSELECTION g_ds;

HICON g_hSmallWarnIcon;
HICON g_hRevIcon;
HMENU g_hMainMenu = NULL;
HANDLE g_hLogoImage;

LPMAPPARAM g_lpMapParam = NULL;
BOOL g_bIsCancelling = FALSE;
BOOL g_bCloseAfterCancel = FALSE;

/******************************************************************************/
/*                               Private                                      */
/******************************************************************************/

/******************************************************************************/
static BOOL WriteResourceToFile(
    HANDLE hFile,
    HRSRC hRsrc
)
{
    BOOL bRet = FALSE;    
    
    /* Check passed values */
    if(INVALID_HANDLE_VALUE != hFile && NULL != hRsrc)
    {
        DWORD dwSize;
        LPVOID lpData;
        DWORD dwPos;
        DWORD dwWritten;
        HGLOBAL hgRsrc;
        
        /* Get Resource size */
        dwSize = SizeofResource(g_hMyInstance, hRsrc);
        /* Get Resource data */
        hgRsrc = LoadResource(g_hMyInstance, hRsrc);
        lpData = LockResource(hgRsrc);
        
        if(NULL != lpData && 0 != dwSize)
        {
            dwPos = 0;
            while(dwPos < dwSize)
            {
                dwWritten = 0;
                /* Write to file */
                if(TRUE == WriteFile(
                    hFile,
                    (LPVOID)((UINT_PTR)lpData + dwPos),
                    dwSize - dwPos,
                    &dwWritten,
                    NULL
                ))
                {
                    dwPos += dwWritten;
                }
                else
                {
                    break;
                }
            }
            bRet = TRUE;
        }        
    }
    
    return bRet;
}

/******************************************************************************/
static VOID DoWindows10Integration(VOID)
{
    win_tile_manifest_t tile;
    char * file_name;
    HANDLE hLogoImage;
    
    tile.tile_color.red = 0xdc;
    tile.tile_color.green = 0x00;
    tile.tile_color.blue = 0x21;
    tile.flags = WIN_TILE_FLAGS_SHOW_NAME;
    tile.logo150 = NULL;
    tile.logo70 = NULL;
    
    /* Create DLL file */
    hLogoImage = CreateFile(
        TEXT(LOGO_IMAGE_NAME),
        GENERIC_WRITE,
        0, NULL,
        CREATE_ALWAYS, 
        FILE_ATTRIBUTE_NORMAL, 
        NULL);

    if(INVALID_HANDLE_VALUE != hLogoImage)
    {
        /* Write resource data */               
        if(WriteResourceToFile(hLogoImage,
            FindResource(
                g_hMyInstance,
                MAKEINTRESOURCE(IDI_LOGO_IMAGE),
                RT_RCDATA
            )))
        {
            tile.logo150 = LOGO_IMAGE_NAME;
        }
        
        /* Close file */
        CloseHandle(hLogoImage);
    }
    

    file_name = (char *)HeapAlloc(g_hHeap, 
            0, sizeof(char) * MAX_PATH);                                   

    if(NULL != file_name)
    {
        GetModuleFileNameA(NULL, file_name, 
            MAX_PATH);
        /* Create Windows 8.1/10 Tile style Manifest if 
         * there  is none */
        generate_win_tile_manifest(file_name,
                &tile);

        HeapFree(g_hHeap, 0, file_name);
    }
}

/******************************************************************************/
VOID VUTDisksEnableSavePassword(
    HWND hwndDlg,
    BOOL bEnable
)
{
    if(bEnable)
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_SAVE_PASS), 
            TRUE);
    }
    else
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_SAVE_PASS), 
            FALSE);
        SendDlgItemMessage(hwndDlg, IDC_SAVE_PASS, 
            BM_SETCHECK, (WPARAM)BST_UNCHECKED,(LPARAM)0);
    }
}

/******************************************************************************/
static BOOL ProgressBarMarquee(HWND hWnd, BOOL bState)
{
	DWORD dwStyle;
	BOOL bOld;

	dwStyle = GetWindowLong(hWnd, GWL_STYLE);

	bOld = ((dwStyle & PBS_MARQUEE) != 0);

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
static VOID ShowWarningMessage(BOOL bVisible)
{
	int iCmd = (TRUE == bVisible) ? SW_SHOW : SW_HIDE;

	ShowWindow(GetDlgItem(g_hwndMain, IDC_WARNS), iCmd);
	ShowWindow(GetDlgItem(g_hwndMain, IDC_ERR_LIST), iCmd);
}

/******************************************************************************/
static VOID DisplayErrMessage(HWND hwndParent)
{
	TCHAR * lpErrMessage;
	UINT_PTR uPos = 0;
	UINT uDisk;
	    
    /* Allocate memory */
    lpErrMessage = (TCHAR *)HeapAlloc(g_hHeap, 0, sizeof(TCHAR) *
            ERROR_MESSAGE_BUFFER_LENGTH);
    if(NULL == lpErrMessage)
    {
        return;
    }

	uPos += FormatMessage(FORMAT_MESSAGE_FROM_STRING,
		TEXT("Disk mapping finished with following results:\r\n\r\n"),
		0, 0, (LPTSTR)(((UINT_PTR)lpErrMessage) + (uPos * sizeof(TCHAR))), 
		(ERROR_MESSAGE_BUFFER_LENGTH - uPos), NULL);

	for (uDisk = 0; uDisk < g_uErrorsCnt; uDisk++)
	{
		uPos += FormatMessage(FORMAT_MESSAGE_FROM_STRING,
			g_lpDisks[uDisk],
			0, 0, (LPTSTR)(((UINT_PTR)lpErrMessage) + (uPos * sizeof(TCHAR))),
			(ERROR_MESSAGE_BUFFER_LENGTH - uPos), NULL);

		uPos += FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL,
			g_lpdwErrors[uDisk], 0, 
			(LPTSTR)(((UINT_PTR)lpErrMessage) + (uPos * sizeof(TCHAR))),
			(ERROR_MESSAGE_BUFFER_LENGTH - uPos), NULL);
	}

	MessageBox(hwndParent, lpErrMessage, g_lpCaption, MB_OK);
    
    /* Free memory */
    HeapFree(g_hHeap, 0, lpErrMessage);
}

/******************************************************************************/
static HMONITOR MonitorFromCursor(
    VOID
)
{
    POINT pt;
    if(!GetCursorPos(&pt))
    {
        pt.x = 0;
        pt.y = 0;
    }
    
    return MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
}

/******************************************************************************/
/*                         Windows Messages                                   */
/******************************************************************************/

/**
 * @brief Process WM_INITDIALOG message
 * 
 * @param hwndInitialControl A handle to the control to receive the default
 *        keyboard focus
 * @param[in] lpData Additional initialization data
 * @return Set the keyboard focus
 */
static BOOL OnInitDialog(
    HWND hwnd,
    HWND hwndInitialControl,
    LPVOID lpAdditionalData
)
{
    MONITORINFO mi;
    INITCOMMONCONTROLSEX icex;

    /* Disk selection has not been changed yet */
    g_bDiskSelectChanged = FALSE;

    /* Initialize common controls */        
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    /* Move windows to the center of monitor */
    mi.cbSize = sizeof(MONITORINFO);
    if(FALSE != GetMonitorInfo(MonitorFromCursor(), &mi))
    {
        RECT rc;
        if(TRUE == GetWindowRect(hwnd, &rc))
        {
            LONG lWidth = rc.right - rc.left;
            LONG lHeight = rc.bottom - rc.top;

            MoveWindow(hwnd,
                ((mi.rcWork.right - mi.rcWork.left) / 2) +
                    mi.rcWork.left - (lWidth / 2),
                ((mi.rcWork.bottom - mi.rcWork.top) / 2) +
                    mi.rcWork.top - (lHeight / 2),
                lWidth,
                lHeight,
                TRUE);
        }
    }

    /* Set Small Warning icon */
    if(NULL != g_hSmallWarnIcon)
    {
        SendDlgItemMessage(hwnd, IDC_WARNS, STM_SETICON,
            (WPARAM)g_hSmallWarnIcon, 0);
    }

    if(NULL != g_hRevIcon)
    {
        SendDlgItemMessage(hwnd, IDC_SHOWP, BM_SETIMAGE,
            (WPARAM)IMAGE_ICON, (LPARAM)g_hRevIcon);
    }

    /* Set Edit Controls Limits */
    SendDlgItemMessage(hwnd, IDC_LOGIN, EM_SETLIMITTEXT,
        (WPARAM)(LOGIN_MAX_LENGTH - 1), 0);
    SendDlgItemMessage(hwnd, IDC_ID, EM_SETLIMITTEXT,
        (WPARAM)(LOGIN_MAX_LENGTH - 1), 0);
    SendDlgItemMessage(hwnd, IDC_PASSWD, EM_SETLIMITTEXT,
        (WPARAM)(PASSWORD_MAX_LENGTH - 1), 0);

    /* Store handle to the Menu */
    g_hMainMenu = GetMenu(hwnd);

    /* If already registered to run at startup, check the menu */
    CheckMenuItem(g_hMainMenu, ID_RUNATSTARTUP,
        MF_BYCOMMAND | (MF_CHECKED * IsRegisteredToRunAtStartup()));		

    /* Read registry data */
    ReadRegistry(hwnd);
    return TRUE;
}

/**
 * @brief called when window is to be closed
 * 
 * @param hwnd Main window handle
 * 
 * @return TRUE if message is processed
 */
static INT_PTR OnClose(
    HWND hwnd
)
{
    if (NULL != g_lpMapParam)
    {
        if (TRUE == g_bIsCancelling)
        {
            EndMapDisks(g_lpMapParam, 5000);
            g_lpMapParam = NULL;
            DestroyWindow(hwnd);
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
                    GetDlgItem(hwnd, IDC_PROGRESS), TRUE);

                SendDlgItemMessage(hwnd, IDC_PROGRESS,
                    PBM_SETMARQUEE, TRUE, 0);
            }
        }
    }
    else
    {
        DestroyWindow(hwnd);
    }

    return TRUE;
}

/**
 * @brief Called when window is to be destroyed
 * 
 * @param hwnd Main window handle
 * 
 * @return TRUE if message is processed
 */
static INT_PTR OnDestroy(
    HWND hwnd
)
{
    /* Store disk selection settings */
    if(g_bDiskSelectChanged)
        WriteDiskSelectionRegistry();

    /* Quit application */
    PostQuitMessage(0);

    return TRUE;
}

/**
 * @brief Command sent by a control
 * 
 * @param hwnd Main window handle
 * @param wNotifCode Control-specific notification code
 * @param wControlID Dialog Control ID
 * @param hwndC COntrol window handle
 * 
 * @return TRUE if message is processed
 */
static INT_PTR OnControlCommand(
    HWND hwnd,
    WORD wNotifCode,
    WORD wControlID,
    HWND hwndC
)
{
    switch (wNotifCode)
    {
    case BN_CLICKED:
        switch (wControlID)
        {
        case IDCANCEL:
            if (NULL == g_lpMapParam)
            {
                DestroyWindow(hwnd);
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
                        GetDlgItem(hwnd, IDC_PROGRESS), TRUE);

                    SendDlgItemMessage(hwnd, IDC_PROGRESS, PBM_SETMARQUEE, TRUE,
                        0);
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
                cnt = GetDlgItemText(hwnd, IDC_LOGIN, g_lpLogin,
                    LOGIN_MAX_LENGTH);
                cnt *= GetDlgItemText(hwnd, IDC_ID, g_lpId, LOGIN_MAX_LENGTH);
                cnt *= GetDlgItemText(hwnd, IDC_PASSWD, g_lpPassword, 
                    PASSWORD_MAX_LENGTH);

                /* If all fields were filled-in */
                if (cnt > 0)
                {
                    WriteLoginRegistry(hwnd);

                    /* Begin Disk Mapping */
                    g_lpMapParam = BeginMapDisks(hwnd, g_lpLogin, g_lpId,
                        g_lpPassword);

                    if (NULL != g_lpMapParam)
                    {
                        /* Update UI */
                        EnableWindow(GetDlgItem(hwnd, IDOK), FALSE);
                        EnableWindow(GetDlgItem(hwnd, IDC_LOGIN), FALSE);
                        EnableWindow(GetDlgItem(hwnd, IDC_ID), FALSE);
                        EnableWindow(GetDlgItem(hwnd, IDC_PASSWD), FALSE);
                        EnableWindow(GetDlgItem(hwnd, IDC_SAVE_LOGIN), FALSE);
                        EnableWindow(GetDlgItem(hwnd, IDC_SAVE_PASS), FALSE);
                        EnableWindow(GetDlgItem(hwnd, IDC_SHOWP), FALSE);
                        EnableWindow(GetDlgItem(hwnd, IDC_DISK_SELECT), FALSE);

                        ProgressBarMarquee(GetDlgItem(hwnd, IDC_PROGRESS),
                            FALSE);
                        SendDlgItemMessage(hwnd, IDC_PROGRESS, PBM_SETSTATE,
                            PBST_NORMAL, 0);
                        SendDlgItemMessage(hwnd, IDC_PROGRESS, PBM_SETRANGE, 0,
                            MAKELPARAM(0, (g_lpMapParam->uDisksCnt)));
                        SendDlgItemMessage(hwnd, IDC_PROGRESS, PBM_SETPOS, 0,
                            0);

                        SetDlgItemText(hwnd, IDCANCEL, TEXT("Cancel"));
                        g_bErrors = FALSE;
                    }
                    else
                    {
                        MessageBox(hwnd, TEXT("Could not map disks."),
                            g_lpCaption, MB_OK | MB_ICONHAND);
                    }

                    /* Zero password memory */
                    ZeroMemory(g_lpPassword,
                        PASSWORD_MAX_LENGTH * sizeof(TCHAR));
                }
                else
                {
                    MessageBox(hwnd,
                        TEXT("Please fill in all of the required fields"),
                        g_lpCaption, MB_OK | MB_ICONINFORMATION);
                }
            }
            return TRUE;

        case IDC_SHOWP:
        {
            HWND hPassWnd;

            /* Get the Password window handle */
            hPassWnd = GetDlgItem(hwnd, IDC_PASSWD);                        

            if(BST_CHECKED == 
                IsDlgButtonChecked(hwnd, IDC_SHOWP))
            {
                SendMessage(hPassWnd, EM_SETPASSWORDCHAR,
                    (WPARAM)DEFAULT_PASSWD_CHAR, 0);

                SendDlgItemMessage(hwnd, IDC_SHOWP, BM_SETCHECK,
                    (WPARAM)BST_UNCHECKED, 0);                        
            }
            else
            {
                SendMessage(hPassWnd, EM_SETPASSWORDCHAR, 0, 0);

                SendDlgItemMessage(hwnd, IDC_SHOWP, BM_SETCHECK,
                    (WPARAM)BST_CHECKED, 0);
            }

            InvalidateRect(hPassWnd, NULL, TRUE);                        
            return TRUE;
        }

        case IDC_SAVE_LOGIN:
            VUTDisksEnableSavePassword(hwnd, BST_CHECKED ==
                SendDlgItemMessage(hwnd, IDC_SAVE_LOGIN, BM_GETCHECK, 0, 0));
            return TRUE;

        case IDC_DISK_SELECT:
            if(IDOK == ShowDiskSelectDialog(hwnd, &g_ds))
            {
                if(0 != DiskSelectToString(&g_ds, g_lpDiskSelect,
                    DISKSELECT_MAX_LENGTH))
                    SetDlgItemText(hwnd, IDC_DISK_SELECTED,
                        (LPTSTR)g_lpDiskSelect);

                g_bDiskSelectChanged = TRUE;
            }
            return TRUE;

        }
        break;

    case EN_CHANGE:
        switch (wControlID)
        {
        case IDC_LOGIN:
        case IDC_ID:

            /* Clear Password field */
            SetDlgItemText(hwnd, IDC_PASSWD, NULL);

        case IDC_PASSWD:
            /* Uncheck 'Save Password' */
            SendDlgItemMessage(hwnd, IDC_SAVE_PASS, BM_SETCHECK,
                (WPARAM)BST_UNCHECKED, 0);
            return TRUE;
        }
        return FALSE;            
    }
    return FALSE;
}

/**
 * @brief Command sent by a menu or an accelerator
 * 
 * @param hwnd Main window handle
 * @param wID Menu item or accelerator ID
 * @param bIsMenu TRUE if sent by menu item
 * 
 * @return TRUE if message is processed
 */
static INT_PTR OnMenuAccCommand(
    HWND hwnd,
    WORD wID,
    BOOL bIsMenu
)
{
    switch (wID)
    {
        case ID_REMOVEREGISTRYDATA:

            if (IDYES == MessageBox(hwnd,
                TEXT("This will remove Login, ID, Password and other ")\
                TEXT("data stored in the Registry database.\r\n")\
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
            if (IDYES == MessageBox(hwnd,
                TEXT("This will create VisualElementsManifest.xml file ")
                TEXT("which graphically integrates application tile ")
                TEXT("in Windows 8.1/10 start menu, when it is ")
                TEXT("created.\r\n")
                TEXT("\r\nDo you wish to continue?"),
                g_lpCaption, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2))
            {
                DoWindows10Integration();
            }
            return TRUE;

        case ID_MENU_ABOUT:
            ShowAboutDialog(hwnd);
            return TRUE;

        case ID_EXIT:
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            return TRUE;
    }
    return FALSE;
}

/**
 * @brief Sent to parent window of an owner-drawn control when aspect of the
 *        control has changed
 *
 * @param hwnd Main window handle
 * @param wControlID Identifier of the control to be drawn
 * @param lpDIS Information about the item to be drawn
 *
 * @return TRUE if message is processed
 */
static INT_PTR OnDrawItem(
    HWND hwnd,
    WPARAM wControlID,
    LPDRAWITEMSTRUCT lpDIS
)
{
    if (IDC_LOGO == wControlID)
    {
        /* If Image is correctly loaded, draw it */
        if (NULL != g_hLogoImage)
        {
            int left;
            int height;

            HDC hdcMem = CreateCompatibleDC(lpDIS->hDC);
            BITMAP bitmap;
            HGDIOBJ oldBitmap = SelectObject(hdcMem, g_hLogoImage);

            GetObject(g_hLogoImage, sizeof(bitmap), &bitmap);
            BitBlt(lpDIS->hDC, 0, 0, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0,
                0, SRCCOPY);

            left = bitmap.bmWidth;
            height = bitmap.bmHeight;

            SelectObject(hdcMem, oldBitmap);
            DeleteDC(hdcMem);

            /* Draw white background */
            if (left < lpDIS->rcItem.right)
            {
                SelectObject(lpDIS->hDC, GetStockObject(DC_PEN));
                SelectObject(lpDIS->hDC, GetStockObject(DC_BRUSH));
                SetDCBrushColor(lpDIS->hDC, RGB(255, 255, 255));
                SetDCPenColor(lpDIS->hDC, RGB(255, 255, 255));
                Rectangle(lpDIS->hDC, left, 0, lpDIS->rcItem.right, height);
            }
        }

        return TRUE;
    }
    return FALSE;
}

/**
 * @brief Sent by a common control to its parent window when an event has
 *        occurred or control requires some information
 *
 * @param hwnd Main window handle
 * @param wControlID Identifier of the control sending the notification
 * @param lpNM Pointer to notification information
 *
 * @return TRUE if message is processed
 */
static INT_PTR OnNotify(
    HWND hwnd,
    WPARAM wControlID,
    LPNMHDR lpNM
)
{
    switch (lpNM->code)
    {
    case NM_CLICK:
    case NM_RETURN:			
        switch (lpNM->idFrom)
        {		
        case IDC_ERR_LIST:
            DisplayErrMessage(hwnd);
            return TRUE;
        }
        break;			
    }

    return FALSE;
}

/**
 * @brief Disk mapping progress notification
 *
 * @param hwnd Main window handle
 * @param cDisks Number of processed disks
 * @param lpMP Pointer to disk mapping parameters structure
 *
 * @return TRUE if message is processed
 */
static INT_PTR OnMapNotify(
    HWND hwnd,
    UINT cDisks,
    LPMAPPARAM lpMP
)
{
    if (cDisks < lpMP->uDisksCnt)
    {
        DWORD dwError;

        SendDlgItemMessage(hwnd, IDC_PROGRESS, PBM_SETPOS, (cDisks + 1), 0);

        dwError = lpMP->lpDisks[cDisks].dwResult;
        if (dwError != NO_ERROR)
        {
            ShowWarningMessage(TRUE);
            g_bErrors = TRUE;
        }

        g_lpdwErrors[g_uErrorsCnt++] = dwError;
    }
    else if (INVALID_DISK_NUMBER == cDisks)
    {
        /* End disk mapping and free mapping parameters structure */
        EndMapDisks(lpMP, INFINITE);
        if (lpMP == g_lpMapParam)
        {
            g_lpMapParam = NULL;
        }
        lpMP = NULL;

        /* Update UI */
        EnableWindow(GetDlgItem(hwnd, IDOK), TRUE);
        EnableWindow(GetDlgItem(hwnd, IDC_LOGIN), TRUE);
        EnableWindow(GetDlgItem(hwnd, IDC_ID), TRUE);
        EnableWindow(GetDlgItem(hwnd, IDC_PASSWD), TRUE);
        EnableWindow(GetDlgItem(hwnd, IDCANCEL), TRUE);                        
        EnableWindow(GetDlgItem(hwnd, IDC_SAVE_LOGIN), TRUE);
        VUTDisksEnableSavePassword(hwnd, BST_CHECKED ==
            SendDlgItemMessage(hwnd, IDC_SAVE_LOGIN, BM_GETCHECK, 0, 0));
        EnableWindow(GetDlgItem(hwnd, IDC_SHOWP), TRUE);
        EnableWindow(GetDlgItem(hwnd, IDC_DISK_SELECT), TRUE);

        if (TRUE == g_bIsCancelling)
        {
            g_bIsCancelling = FALSE;
            ProgressBarMarquee(GetDlgItem(hwnd, IDC_PROGRESS), 
                FALSE);


            if (TRUE == g_bCloseAfterCancel)
            {
                DestroyWindow(g_hwndMain);
            }
        }

        if (FALSE == g_bErrors)
        {
            DestroyWindow(hwnd);
        }

        SetDlgItemText(hwnd, IDCANCEL, TEXT("Close"));
    }
    return TRUE;
}

/**
 * @brief Main window dialog procedure
 * 
 * @param hwnd Main Window handle
 * @param uMsg Message to process
 * @param wParam WParam
 * @param lParam LParam
 * 
 * @return Depending on message
 */
INT_PTR CALLBACK DialogProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    switch(uMsg)
    {
    case WM_INITDIALOG:
        return OnInitDialog(hwnd, (HWND)wParam, (LPVOID)lParam);
            
    case WM_COMMAND:
        /* Control command */
        if(0 != lParam)
        {
            return OnControlCommand(hwnd, HIWORD(wParam),
                    LOWORD(wParam), (HWND)lParam);
        }
        /* Menu or accelerator command */
        else
        {
            return OnMenuAccCommand(hwnd, LOWORD(wParam),
                    (HIWORD(wParam) == 0));
        }

    case WM_CLOSE:
        return OnClose(hwnd);

    case WM_DESTROY:
        return OnDestroy(hwnd);

    case WM_DRAWITEM:
        return OnDrawItem(hwnd, wParam, (LPDRAWITEMSTRUCT)lParam);

    case WM_NOTIFY:
        return OnNotify(hwnd, wParam, (LPNMHDR)lParam);

    case WM_MAP_NOTIFY:
        return OnMapNotify(hwnd, (UINT)wParam, (LPMAPPARAM)lParam);
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
INT WINAPI wWinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPWSTR lpCmdLine,
	int nCmdShow
	)
{
	MSG msg;
	HICON hIcon;
	HWND hwndPassTooltip, hwndShowPTooltip;      
        
        
	g_hMyInstance = hInstance;
    
    
	/* Get Process heap */
	g_hHeap = GetProcessHeap();
    
    /* Open Registry Key */
	OpenMyRegKey();

	g_hSmallWarnIcon = LoadImage(g_hMyInstance, MAKEINTRESOURCE(IDI_WARN),
		IMAGE_ICON, 24, 24, LR_DEFAULTCOLOR);
    
    g_hRevIcon = LoadImage(g_hMyInstance, MAKEINTRESOURCE(IDI_REVEAL),
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
        DWORD dwErr = GetLastError();
		MessageBox(NULL, TEXT("Application Initialization Failed!"),
			TEXT("Error!"), MB_OK | MB_ICONHAND);
		return dwErr;
	}	

	/* Create Tooltips */
    hwndShowPTooltip = CreateToolTip(IDC_SHOWP, g_hwndMain,
            TEXT("Show / hide password"), 400);
    
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
		SendMessage(hwndPassTooltip, TTM_SETTITLE, (WPARAM)hIcon,
            (LPARAM)TEXT("Security Considerations"));		
		
		DestroyIcon(hIcon);
		SendMessage(hwndPassTooltip, TTM_SETDELAYTIME, (WPARAM)TTDT_AUTOPOP,
            25000);
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
	g_hLogoImage = LoadImage(hInstance, MAKEINTRESOURCE(IDB_LOGO), IMAGE_BITMAP,
        0, 0, LR_SHARED);
        
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

	/* Destroy tooltip windows */
	if (NULL != hwndPassTooltip)
	{
		DestroyWindow(hwndPassTooltip);
	}
    
    if (NULL != hwndShowPTooltip)
    {
        DestroyWindow(hwndShowPTooltip);
    }

	/* Close My Reg Key */
	CloseMyRegKey();

    if(NULL != g_hRevIcon)
    {
        DestroyIcon(g_hRevIcon);
    }
    
	/* Destroy Warn Icon */
	if (NULL != g_hSmallWarnIcon)
	{
		DestroyIcon(g_hSmallWarnIcon);
	}

	
	ExitProcess((UINT)(msg.wParam));

   return (INT)(msg.wParam);
}

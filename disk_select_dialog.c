#include <basetsd.h>
#include <minwindef.h>

#include "disk_select_dialog.h"
#include "resource.h"

/**
 * @brief List of all considered disk letters
 */
const TCHAR g_aDiskLetters[] = 
{
    TEXT('A'), TEXT('B'), TEXT('C'), TEXT('D'), TEXT('E'), TEXT('F'), TEXT('G'),
    TEXT('H'), TEXT('I'), TEXT('J'), TEXT('K'), TEXT('L'), TEXT('M'), TEXT('N'),
    TEXT('O'), TEXT('P'), TEXT('Q'), TEXT('R'), TEXT('S'), TEXT('T'), TEXT('U'),
    TEXT('V'), TEXT('W'), TEXT('X'), TEXT('Y'), TEXT('Z')
};

/**
 * @brief Default disk letters
 */
const TCHAR g_aDefaultDiskLetters[VUT_DISK_NUM] = 
{
    TEXT('P'), TEXT('Q'), TEXT('R'), TEXT('S'), TEXT('T'), TEXT('V')
};

/**
 * @brief Add disk letters to combo boxes
 * 
 * @param hwndDlg Disk select dialog window handle
 */
static VOID SetDiskLetters(
    HWND hwndDlg
)
{
    INT iDisk, iLetter;
    TCHAR aDrive[4];
    DWORD dwAvailableDiskLetters;
    
    /* Get available disks */
    dwAvailableDiskLetters = 0;
    aDrive[1] = TEXT(':'); aDrive[2] = TEXT('\\'); aDrive[3] = TEXT('\0');
    for(iLetter = 0; iLetter < (sizeof(g_aDiskLetters)/sizeof(TCHAR));
        iLetter ++)
    {
        aDrive[0] = g_aDiskLetters[iLetter];
        
        if(DRIVE_NO_ROOT_DIR == GetDriveType(aDrive))
            dwAvailableDiskLetters |= (1 << iLetter);
    }
    
    
    for(iDisk = 0; iDisk < VUT_DISK_NUM; iDisk ++)
    {
        HWND hwndComboBox;
        hwndComboBox = GetDlgItem(hwndDlg, IDC_DISK_LETTER_BASE + iDisk);
        
        if(NULL != hwndComboBox)
        {
                        
            aDrive[1] = TEXT('\0');
            for(iLetter = 0; iLetter < (sizeof(g_aDiskLetters)/sizeof(TCHAR));
                iLetter ++)
            {
                if((1 << iLetter) & dwAvailableDiskLetters)
                {
                    aDrive[0] = g_aDiskLetters[iLetter];
                    ComboBox_AddString(hwndComboBox, aDrive);
                }
            }
        }
    }    
}

/**
 * @brief Load disk selection information into dialog
 * 
 * @param hwndDlg Disk selection dialog
 * @param lpDiskSelection Disk selection
 */
static VOID LoadSelection(
    HWND hwndDlg,
    LPDISKSELECTION lpDiskSelection
)
{
    int iDisk;
    
    for(iDisk = 0; iDisk < VUT_DISK_NUM; iDisk ++)
    {
        HWND hwnd;
        TCHAR lptstrLetter[2];
        lptstrLetter[1] = TEXT('\0');

        /* Load disk enable */
        hwnd = GetDlgItem(hwndDlg, IDC_DISK_ENABLE_BASE + iDisk);        
        if(NULL != hwnd)
        {
            Button_SetCheck(hwnd,
                ((lpDiskSelection->dwDiskEnable >> iDisk) & 0x1) ?
                BST_CHECKED : BST_UNCHECKED);
        }
        
        /* Load disk letter */
        hwnd = GetDlgItem(hwndDlg, IDC_DISK_LETTER_BASE + iDisk);        
        if(NULL != hwnd)
        {
            lptstrLetter[0] = lpDiskSelection->aDiskLetters[iDisk];
            ComboBox_SelectString(hwnd, -1, lptstrLetter);
        }
    }
}

/**
 * @brief Store disk selection information from dialog
 * 
 * @param hwndDlg Disk selection dialog
 * @param lpDiskSelection Disk selection
 */
static VOID StoreSelection(
    HWND hwndDlg,
    LPDISKSELECTION lpDiskSelection
)
{
    int iDisk;
    DWORD dwDiskEnable = 0;

    for(iDisk = 0; iDisk < VUT_DISK_NUM; iDisk ++)
    {
        HWND hwnd;
        TCHAR lptstrLetter[2];
        
        /* Store disk enable */
        hwnd = GetDlgItem(hwndDlg, IDC_DISK_ENABLE_BASE + iDisk);        
        if(NULL != hwnd)
        {
            /* Set enable flag */
            if(BST_CHECKED == Button_GetCheck(hwnd))
                dwDiskEnable |= (1 << iDisk);
        }
        
        /* Store disk letter */
        hwnd = GetDlgItem(hwndDlg, IDC_DISK_LETTER_BASE + iDisk);        
        if(NULL != hwnd)
        {
            if(0 != ComboBox_GetText(hwnd, lptstrLetter,
                    sizeof(lptstrLetter)/sizeof(TCHAR)))
                lpDiskSelection->aDiskLetters[iDisk] = lptstrLetter[0];
            else
            {
                lpDiskSelection->aDiskLetters[iDisk] =
                    g_aDefaultDiskLetters[iDisk];
                dwDiskEnable &= ~(1 << iDisk);
            }
        }
    }
    /* Store enable flags */
    lpDiskSelection->dwDiskEnable = dwDiskEnable;
}

/**
 * @brief Is specified disk letter already selected
 *  
 * @param hwndDlg Disk select dialog
 * @param hwndCurrSelector Selector to check
 * 
 * @return TRUE if letter is already taken 
 */
static BOOL IsAlreadySelected(HWND hwndDlg, HWND hwndCurrSelector)
{
    int iDisk;
    TCHAR lptstrLetter[16];
    TCHAR tchrLetter;
    
    if(0 != ComboBox_GetText(hwndCurrSelector, lptstrLetter,
        sizeof(lptstrLetter)/sizeof(TCHAR)))
    {
        tchrLetter = lptstrLetter[0];
    }
    else
    {
        return FALSE;
    }

    for(iDisk = 0; iDisk < VUT_DISK_NUM; iDisk ++)
    {
        HWND hwnd;
        
        /* Store disk letter */
        hwnd = GetDlgItem(hwndDlg, IDC_DISK_LETTER_BASE + iDisk);        
        if(NULL != hwnd && hwnd != hwndCurrSelector)
        {            
            if(0 != ComboBox_GetText(hwnd, lptstrLetter,
                    sizeof(lptstrLetter)/sizeof(TCHAR)))
            {
                if(tchrLetter == lptstrLetter[0])
                    return TRUE;
            }
        }
    }
    return FALSE;
}

/******************************************************************************/
INT_PTR static CALLBACK DiskSelectProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        /* Set focus to OK button */
        SetFocus(GetDlgItem(hwndDlg, IDOK));
        /* Add disk letters */
        SetDiskLetters(hwndDlg);
        /* Load disk selection */
        LoadSelection(hwndDlg, (LPDISKSELECTION)lParam);

        /* Store disk selection pointer */
        SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)lParam);

        return FALSE;
    }
    
    case WM_COMMAND:
    {
        if(0 != lParam)
        {
            WORD wControlId, wCode;
            wControlId = LOWORD(wParam);
            wCode = HIWORD(wParam);

            switch (wControlId)
            {
            case IDOK:
            {
                LPDISKSELECTION lpds;
                lpds = (LPDISKSELECTION)GetWindowLongPtr(hwndDlg,
                    GWLP_USERDATA);
                if(NULL != lpds)
                    StoreSelection(hwndDlg, lpds);
            }
            case IDCANCEL:
                EndDialog(hwndDlg, LOWORD(wParam));
                return TRUE;

            default:
                if(wControlId >= IDC_DISK_LETTER_BASE &&
                    wControlId < (IDC_DISK_LETTER_BASE + VUT_DISK_NUM))
                {
                    if(CBN_SELCHANGE == wCode)
                    {
                        if(IsAlreadySelected(hwndDlg, (HWND)lParam))
                        {
                            MessageBox(hwndDlg,
                                TEXT("Disk letter already used."),
                                TEXT("Disk Selection"), MB_OK);

                            SendMessage((HWND)lParam, CB_SETCURSEL, -1, 0);
                        }
                    }
                }
                break;
            }            
        }
    }
    }
    return FALSE;    
}

/******************************************************************************/
VOID DiskSelectGetDefaults(
    LPDISKSELECTION lpDs
)
{
    UINT uDisk;
    
    for(uDisk = 0; uDisk < (sizeof(g_aDefaultDiskLetters) / sizeof(TCHAR));
        uDisk ++)
    {
        lpDs->aDiskLetters[uDisk] = g_aDefaultDiskLetters[uDisk];
    }

    lpDs->dwDiskEnable = ((DWORD)0xFFFFFFFF) >> (32 - VUT_DISK_NUM);
}

/******************************************************************************/
UINT DiskSelectToString(
    LPDISKSELECTION lpDs,
    LPTSTR lpBuffer,
    UINT uBufferLength    
)
{
    UINT uDisk, uPos;
    DWORD dwDiskEnable = lpDs->dwDiskEnable;
            
    for(uDisk = 0, uPos = 0; uDisk < VUT_DISK_NUM && (uPos + 3) < uBufferLength;
        uDisk ++)
    {
        lpBuffer[uPos ++] = (1 & dwDiskEnable) ? lpDs->aDiskLetters[uDisk] :
            TEXT('-');

        lpBuffer[uPos ++] = TEXT(',');
        dwDiskEnable >>= 1;
    }

    if(uPos > 0)
        uPos --;

    if(uPos < uBufferLength)
        lpBuffer[uPos ++] = TEXT('\0');
    return uPos;            
}

/******************************************************************************/
INT_PTR ShowDiskSelectDialog(
    HWND hwndParent,
    LPDISKSELECTION lpDiskSelection
)
{
    return DialogBoxParam(g_hMyInstance, MAKEINTRESOURCE(IDD_DISK_SELECT),
        hwndParent, DiskSelectProc, (LPARAM)lpDiskSelection);
}

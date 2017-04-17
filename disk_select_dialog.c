#include <basetsd.h>

#include "disk_select_dialog.h"
#include "resource.h"

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
    int iDisk;
    
    for(iDisk = 0; iDisk < VUT_DISK_NUM; iDisk ++)
    {
        HWND hwndComboBox;
        hwndComboBox = GetDlgItem(hwndDlg, IDC_DISK_LETTER_BASE + iDisk);
        
        if(NULL != hwndComboBox)
        {
            TCHAR tchrLetter[2];
            
            tchrLetter[1] = TEXT('\0');
            for(tchrLetter[0] = TEXT('A'); tchrLetter[0] <= TEXT('Z');
                    tchrLetter[0] ++)
            {
                ComboBox_AddString(hwndComboBox, tchrLetter);
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
        }
    }
    /* Store enable flags */
    lpDiskSelection->dwDiskEnable = dwDiskEnable;
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
        switch (HIWORD(wParam))
        {
        case 0:
            switch (LOWORD(wParam))
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
            
            case IDC_GET_LATEST:
                return TRUE;
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
INT_PTR ShowDiskSelectDialog(
    HWND hwndParent,
    LPDISKSELECTION lpDiskSelection
)
{
    return DialogBoxParam(g_hMyInstance, MAKEINTRESOURCE(IDD_DISK_SELECT),
        hwndParent, DiskSelectProc, (LPARAM)lpDiskSelection);
}

#include "about_dialog.h"
#include "resource.h"

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
        
        SetFocus(GetDlgItem(hwndDlg, IDOK));

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
INT_PTR ShowDiskSelectDialog(
    HWND hwndParent,
    INT * lpDisksSelector 
)
{
    return DialogBoxParam(g_hMyInstance, MAKEINTRESOURCE(IDD_DISK_SELECT),
        hwndParent, DiskSelectProc, (LPARAM)lpDisksSelector);
}

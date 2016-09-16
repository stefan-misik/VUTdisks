#include "reveal_button.h"


#define REVB_STATE 0

/******************************************************************************/
static VOID RevButtonNotifyParent(
    HWND hWnd,
    WORD wEvent
)
{
    int iID;    
    HWND hwndParent;
    
    /* Get the ID of the control */
    iID = GetDlgCtrlID(hWnd);
    
    /* Get parent window */
    hwndParent = GetParent(hWnd);
    
    /* Notify Parent */
    SendMessage(hwndParent, WM_COMMAND,
        MAKEWPARAM(iID, (WORD)wEvent),
        (LPARAM)hWnd);
}

/******************************************************************************/
static LRESULT CALLBACK RevButtonWindowProc(
    HWND   hWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    switch(uMsg)
    {
    
    /* Create window */
    case WM_CREATE:
        /* Init state variable */
        SetWindowLong(hWnd, REVB_STATE, 0);
        break; 
        
    /* Mouse button down */
    case WM_LBUTTONDOWN:
        /* Notify Parent */
        RevButtonNotifyParent(hWnd, RB_REVEAL);
        
        /* Capture the mouse */
        SetCapture(hWnd);         
        return 0;
    
    /* Mouse button up */    
    case WM_LBUTTONUP:
        {
            //LONG lState = GetWindowLong(hWnd, REVB_STATE);
            /* If mouse is captured by this window, release it */
            if(hWnd == GetCapture())
            {
                /* Notify Parent */
                RevButtonNotifyParent(hWnd, RB_HIDE);
                
                ReleaseCapture();
            }        
            return 0;
        }
    
    default:
        break;          
    }
    
    return DefWindowProc(hWnd, uMsg, wParam, lParam);  
}


/******************************************************************************/
ATOM RevButtonRegisterClass(
    VOID
)
{
    WNDCLASSEX wce;
    
    wce.cbSize = sizeof(WNDCLASSEX);
    wce.style = 0;
    wce.lpfnWndProc = RevButtonWindowProc;
    wce.cbClsExtra = 0;
    wce.cbWndExtra = sizeof(LONG);
    wce.hInstance = g_hMyInstance;
    wce.hIcon = 0;
    wce.hCursor = LoadCursor(NULL, IDC_ARROW);
    wce.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wce.lpszMenuName = NULL;
    wce.lpszClassName = TEXT(REVEAL_BUTTON_CLASS);
    wce.hIconSm = NULL;
    
    return RegisterClassEx(&wce);
}

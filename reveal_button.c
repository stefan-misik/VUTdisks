#include "reveal_button.h"
#include "resource.h"

#define REVB_EXTRA_STATE 0
#define REVB_EXTRA_ICON REVB_EXTRA_STATE + sizeof(LONG)

#define REVB_EXTRA_TOTAL REVB_EXTRA_ICON + sizeof(HICON)



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
static VOID RevButtonLoadIcon(
    HWND hWnd,
    INT iCx, 
    INT iCy
)
{
    HICON hIcon;
    INT iDim;
    UINT uIconID;
    static const UINT arruIconSizes[] = {16, 24, 32, 48, 64, 72};
    
    /* Destroy icon, if there is one already */
    hIcon = (HICON)GetWindowLongPtr(hWnd, REVB_EXTRA_ICON);
    if(NULL != hIcon)
    {
        DestroyIcon(hIcon);
    }
    
    hIcon = NULL;
    
    /* Chose icon of appropriate size */
    iDim = iCx < iCy ? iCx : iCy;
    
    for(uIconID = 0; uIconID < (sizeof(arruIconSizes) / sizeof(UINT)); 
            uIconID++)
    {
        if(iDim <= arruIconSizes[uIconID])
        {
            if(iDim == arruIconSizes[uIconID])
            {
                uIconID ++;
            }
            break;
        }
    } 
    
    /* Load that icon */
    if(uIconID != 0)
    {
        iDim = arruIconSizes[uIconID - 1];
        
        hIcon = (HICON)LoadImage(g_hMyInstance, MAKEINTRESOURCE(IDI_REVEAL), 
                IMAGE_ICON, iDim, iDim, LR_DEFAULTCOLOR);
    }
    
    SetWindowLongPtr(hWnd, REVB_EXTRA_ICON, (LONG_PTR)hIcon);
}

/******************************************************************************/
static LRESULT RevButtonCreate(
    HWND hWnd,
    LPCREATESTRUCT lpCs
)
{  
    /* Init extra fields */
    SetWindowLong(hWnd, REVB_EXTRA_STATE, (LONG)0);    
    SetWindowLongPtr(hWnd, REVB_EXTRA_ICON, (LONG_PTR)NULL);
    
    return 0;
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
        return RevButtonCreate(hWnd, (LPCREATESTRUCT)lParam);
    
    /* Destroy */
    case WM_DESTROY:
        {
            HICON hIcon;
            
            /* Destroy icon, if there is one */
            hIcon = (HICON)GetWindowLongPtr(hWnd, REVB_EXTRA_ICON);
            if(NULL != hIcon)
            {
                DestroyIcon(hIcon);
            }
            
            break;
        }
            
        
    /* Size Window */
    case WM_SIZE:
        /* Load Icon of appropriate size */
        RevButtonLoadIcon(hWnd, (INT)LOWORD(lParam), (INT)LOWORD(lParam));
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
    wce.cbWndExtra = REVB_EXTRA_TOTAL;
    wce.hInstance = g_hMyInstance;
    wce.hIcon = NULL;
    wce.hCursor = LoadCursor(NULL, IDC_ARROW);
    wce.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wce.lpszMenuName = NULL;
    wce.lpszClassName = TEXT(REVEAL_BUTTON_CLASS);
    wce.hIconSm = NULL;
    
    return RegisterClassEx(&wce);
}

#include "reveal_button.h"
#include "resource.h"

#define REVB_EXTRA_STATE 0
#define REVB_EXTRA_ICON REVB_EXTRA_STATE + sizeof(LONG)
#define REVB_EXTRA_ICON_SIZE REVB_EXTRA_ICON + sizeof(HICON)

#define REVB_EXTRA_TOTAL REVB_EXTRA_ICON_SIZE + sizeof(LONG)

/* Reveal button state flags */
#define RBS_DEFAULT 0
#define RBS_ACTIVE 1
#define RBS_MOUSE_OVER 2


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
    else
    {
        iDim = 0;
    }
    
    SetWindowLongPtr(hWnd, REVB_EXTRA_ICON, (LONG_PTR)hIcon);
    SetWindowLong(hWnd, REVB_EXTRA_ICON_SIZE, (LONG)iDim);
}

/******************************************************************************/
static LRESULT RevButtonCreate(
    HWND hWnd,
    LPCREATESTRUCT lpCs
)
{  
    /* Init extra fields */
    SetWindowLong(hWnd, REVB_EXTRA_STATE, (LONG)RBS_DEFAULT);    
    SetWindowLongPtr(hWnd, REVB_EXTRA_ICON, (LONG_PTR)NULL);
    SetWindowLong(hWnd, REVB_EXTRA_ICON_SIZE, (LONG)0);  
    
    return 0;
}

/******************************************************************************/
static LRESULT RevButtonPaint(
    HWND hWnd
)
{
    HICON hIcon;
    PAINTSTRUCT ps;
    
    if(NULL != BeginPaint(hWnd, &ps))
    {   
        /* Get Icon */
        hIcon = (HICON)GetWindowLongPtr(hWnd, REVB_EXTRA_ICON);
        if(NULL != hIcon)
        {
            INT iDim, iOffset;
            RECT rc;
            LONG lState = GetWindowLong(hWnd, REVB_EXTRA_STATE);
            
            GetWindowRect(hWnd, &rc);
            
            /* Get Icon size */
            iDim = (int)GetWindowLong(hWnd, REVB_EXTRA_ICON_SIZE);  
            
            iOffset = RBS_MOUSE_OVER & lState ? -1 : 0;
            
            /* Draw Icon */
            DrawIconEx(ps.hdc, 
                (rc.right - rc.left - iDim) / 2 + iOffset, 
                (rc.bottom - rc.top - iDim) / 2 + iOffset, 
                hIcon, iDim, iDim, 0, NULL, DI_NORMAL);
        }        
        EndPaint(hWnd, &ps);
    }
    return 0;
}

/******************************************************************************/
static LRESULT RevButtonMouseMove(
    HWND hWnd,
    WPARAM wParam,
    WORD wX,
    WORD wY
)
{
    LONG lState;
    RECT rc;
    POINT p;

    p.x = wX;
    p.y = wY;
    GetClientRect(hWnd, &rc);
            
    /* Get current state */
    lState = GetWindowLong(hWnd, REVB_EXTRA_STATE);

    if(!(lState & RBS_MOUSE_OVER))
    {
        if(PtInRect(&rc, p))
        {
            TRACKMOUSEEVENT tme;

            tme.cbSize = sizeof(TRACKMOUSEEVENT);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hWnd;
            tme.dwHoverTime = 0;

            if(FALSE != TrackMouseEvent(&tme))
            {
                /* Change State */
                SetWindowLong(hWnd, REVB_EXTRA_STATE, lState | RBS_MOUSE_OVER);

                /* Redraw window */
                InvalidateRect(hWnd, NULL, TRUE);
            }
        }
    }
    else
    {       
        if(!PtInRect(&rc, p))
        {
            /* Change State */
            SetWindowLong(hWnd, REVB_EXTRA_STATE, lState & (~RBS_MOUSE_OVER));

            /* Redraw window */
            InvalidateRect(hWnd, NULL, TRUE);
        }
        
    }

    
    return 0;
}

static LRESULT RevButtonMouseLeave(
    HWND hWnd
)
{
    LONG lState;

    /* Get current state */
    lState = GetWindowLong(hWnd, REVB_EXTRA_STATE);

    if(lState & RBS_MOUSE_OVER)
    {
        /* Change State */
        SetWindowLong(hWnd, REVB_EXTRA_STATE, 
                lState & (~RBS_MOUSE_OVER));

        /* Redraw window */
        InvalidateRect(hWnd, NULL, TRUE);
    }
         
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
        
    /* Paint the button */
    case WM_PAINT:
        return RevButtonPaint(hWnd);
        
    /* Mouse button down */
    case WM_LBUTTONDOWN:
        {
            LONG lState;
            
            /* Get current state */
            lState = GetWindowLong(hWnd, REVB_EXTRA_STATE);
            
            /* Notify Parent */
            RevButtonNotifyParent(hWnd, RB_REVEAL);

            /* Change State */
            SetWindowLong(hWnd, REVB_EXTRA_STATE, lState | RBS_ACTIVE);

            /* Redraw window */
            InvalidateRect(hWnd, NULL, TRUE);

            /* Capture the mouse */
            SetCapture(hWnd);         
            return 0;
        }
    
    /* Mouse button up */    
    case WM_LBUTTONUP:
        {
            LONG lState = GetWindowLong(hWnd, REVB_EXTRA_STATE);
            /* If mouse is captured by this window, release it */
            if(RBS_ACTIVE & lState)
            {
                /* Notify Parent */
                RevButtonNotifyParent(hWnd, RB_HIDE);
                
                /* Change State */
                SetWindowLong(hWnd, REVB_EXTRA_STATE, lState & (~RBS_ACTIVE));
                
                /* Redraw window */
                InvalidateRect(hWnd, NULL, TRUE);
            } 
            
            if(hWnd == GetCapture())
            {
                ReleaseCapture();
            }
            return 0;
        }
    /* Mouse move over window */
    case WM_MOUSEMOVE:
        return RevButtonMouseMove(hWnd, wParam, LOWORD(lParam), HIWORD(lParam));

    /* Mouse leave form window */
    case WM_MOUSELEAVE:
        return RevButtonMouseLeave(hWnd);
    
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
    wce.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wce.lpszMenuName = NULL;
    wce.lpszClassName = TEXT(REVEAL_BUTTON_CLASS);
    wce.hIconSm = NULL;
    
    return RegisterClassEx(&wce);
}



#include "reveal_button.h"

#include <gdiplus.h>
#include <new>

#include "resource.h"


#define REVB_EXTRA_STATE 0


#define REVB_EXTRA_TOTAL REVB_EXTRA_STATE + sizeof(LONG)

typedef enum tagREVBUTSTATE
{
    RBS_DEFAULT = 0,
    RBS_ACTIVE
} REVBUTSTATE, *LPREVBUTSTATE;


static ULONG_PTR g_pGDIPlusToken;
static ATOM g_aRevBurtonAtom = 0;


/******************************************************************************/
static VOID RevButtonSetState(
    HWND hWnd,
    LONG lState
)
{
    /* Change state variable */
    SetWindowLong(hWnd, REVB_EXTRA_STATE, lState);
    
    /* Invalidate the window to be redrawn */
    InvalidateRect(hWnd, NULL, TRUE);
}

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
static LRESULT RevButtonCreate(
    HWND hWnd,
    LPCREATESTRUCT lpCs
)
{ 
    /* Init extra fields */
    SetWindowLong(hWnd, REVB_EXTRA_STATE, (LONG)RBS_DEFAULT);    
        
    return 0;
}

/******************************************************************************/
static LRESULT RevButtonDestroy(
    HWND hWnd    
)
{   
    return 0;
}

/******************************************************************************/
static LRESULT RevButtonPaint(
    HWND hWnd
)
{
    PAINTSTRUCT ps;
    LONG lState;
    
    lState = GetWindowLong(hWnd, REVB_EXTRA_STATE);
    
    if(NULL != BeginPaint(hWnd, &ps))
    {    
        Gdiplus::Graphics g(ps.hdc); 
        Gdiplus::Color crBg, crFg;    
        Gdiplus::SolidBrush sb(Gdiplus::Color::Black);
        Gdiplus::GraphicsPath gp;
        Gdiplus::Matrix m;
        RECT rc;
        INT iDim, iW, iH;
        DOUBLE dOffsetX, dOffsetY;
        
        if(lState != RBS_ACTIVE)
        {
            crBg.SetFromCOLORREF((COLORREF)::GetSysColor(COLOR_BTNFACE));
            crFg.SetFromCOLORREF((COLORREF)::GetSysColor(COLOR_BTNTEXT)); 
        }
        else
        {
            crBg.SetFromCOLORREF((COLORREF)::GetSysColor(COLOR_BTNTEXT));
            crFg.SetFromCOLORREF((COLORREF)::GetSysColor(COLOR_BTNFACE));
        }
        
        GetClientRect(hWnd, &rc);
        iW = rc.right - rc.left;
        iH = rc.bottom - rc.top;
                      
        if(iW < iH)
        {
            iDim = iW;
            dOffsetX = 0;
            dOffsetY = (iH - iDim) / 2.0;
        }
        else
        {
            iDim = iH; 
            dOffsetX = (iW - iDim) / 2.0;
            dOffsetY = 0;
        }        

        /* Enable Smoothing */
        g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias); 
        
        /* Draw background */
        sb.SetColor(crBg);
        g.FillRectangle(&sb, (Gdiplus::REAL)dOffsetX, (Gdiplus::REAL)dOffsetY,
                (Gdiplus::REAL)iDim, (Gdiplus::REAL)iDim);
        
        /* Draw Foreground */
        sb.SetColor(crFg);
        /* Center of the eye symbol */
        gp.AddEllipse((Gdiplus::REAL)0.3, (Gdiplus::REAL)0.3, 
                (Gdiplus::REAL)0.40, (Gdiplus::REAL)0.40);
        /* Eyelid symbol */    
        gp.AddArc((Gdiplus::REAL)0.05, (Gdiplus::REAL)0.05, 
                (Gdiplus::REAL)0.9, (Gdiplus::REAL)0.9,
                (Gdiplus::REAL)180, (Gdiplus::REAL)180);
        gp.AddArc((Gdiplus::REAL)0.2, (Gdiplus::REAL)0.2,
                (Gdiplus::REAL)0.6, (Gdiplus::REAL)0.6,
                (Gdiplus::REAL)180, (Gdiplus::REAL)180);
        
        /* Transform and draw */
        m.Scale((Gdiplus::REAL)iDim, (Gdiplus::REAL)iDim);
        m.Translate((Gdiplus::REAL)dOffsetX, (Gdiplus::REAL)dOffsetY,
                Gdiplus::MatrixOrderAppend);
        gp.Transform(&m);
        g.FillPath(&sb, &gp);
        

        /* Flush Graphics object */
        g.Flush();            
        
        EndPaint(hWnd, &ps);
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
        return RevButtonDestroy(hWnd);   
        
    /* Size Window */
    case WM_SIZE:        
        InvalidateRect(hWnd, NULL, TRUE);
        break;
        
    /* Paint the button */
    case WM_PAINT:
        return RevButtonPaint(hWnd);
        
    /* Mouse button down */
    case WM_LBUTTONDOWN:
        /* Notify Parent */
        RevButtonNotifyParent(hWnd, RB_REVEAL);
        
        /* Change State */
        RevButtonSetState(hWnd, RBS_ACTIVE);
        
        /* Capture the mouse */
        SetCapture(hWnd);         
        return 0;
    
    /* Mouse button up */    
    case WM_LBUTTONUP:
        {
            LONG lState = GetWindowLong(hWnd, REVB_EXTRA_STATE);
            /* If mouse is captured by this window, release it */
            if(RBS_ACTIVE == lState)
            {
                /* Notify Parent */
                RevButtonNotifyParent(hWnd, RB_HIDE);
                
                /* Change State */
                RevButtonSetState(hWnd, RBS_DEFAULT);
            } 
            
            if(hWnd == GetCapture())
            {
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
ATOM RevButtonInit(
    VOID
)
{
    WNDCLASSEX wce;
    ATOM aReturn = 0;    
    Gdiplus::GdiplusStartupInput gdisi;
    
    gdisi.GdiplusVersion = 1;
    gdisi.DebugEventCallback = NULL;
    gdisi.SuppressBackgroundThread = FALSE;
    gdisi.SuppressExternalCodecs = FALSE;   
    
            
    Gdiplus::Status status = Gdiplus::GdiplusStartup(&g_pGDIPlusToken, 
            &gdisi, NULL);
    if(status == Gdiplus::Ok)
    {  
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

        aReturn = RegisterClassEx(&wce);    
    }
    g_aRevBurtonAtom = aReturn;
    return aReturn;
}

/******************************************************************************/
VOID RevButtonDeinit(
    VOID
)
{
    if(0 != g_aRevBurtonAtom)
    {
        UnregisterClass((LPTSTR)(INT_PTR)g_aRevBurtonAtom, g_hMyInstance);
        
        Gdiplus::GdiplusShutdown(g_pGDIPlusToken);
    }
}
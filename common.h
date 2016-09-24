#ifndef COMMON_H
#define	COMMON_H

#undef _WIN32_WINNT
#undef _WIN32_IE
#define _WIN32_WINNT 0x0600
#define _WIN32_IE 0x0900
#ifndef _UNICODE
    #define _UNICODE
#endif
#ifndef UNICODE
    #define UNICODE
#endif
#define OEMRESOURCE

#include <windows.h>
#include <commctrl.h>

extern HINSTANCE g_hMyInstance;
extern HANDLE g_hHeap;

#ifndef PBM_SETMARQUEE
    #define PBM_SETMARQUEE WM_USER + 10
#endif
#ifndef PBM_SETSTATE
    #define PBM_SETSTATE WM_USER + 16
#endif
#ifndef PBST_NORMAL
    #define PBST_NORMAL 0x0001
#endif
#ifndef TTM_SETMAXTIPWIDTH
    #define TTM_SETMAXTIPWIDTH 0x0418
#endif
#ifndef TTM_SETTITLE
    #ifdef UNICODE
        #define TTM_SETTITLE 0x0421
    #else 
        #define TTM_SETTITLE 0x0420
    #endif
#endif
#ifndef IDI_SHIELD
    #define IDI_SHIELD MAKEINTRESOURCE(32518)
#endif
#ifndef PBS_MARQUEE
    #define PBS_MARQUEE 0x08
#endif


#endif	/* COMMON_H */
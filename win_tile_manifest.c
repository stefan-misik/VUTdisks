#include "win_tile_manifest.h"
#include <Shlwapi.h>

/******************************************************************************/
static const char g_lpManifest[] = 
    "<Application xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n"
    "<VisualElements \n"
    "BackgroundColor=\"#c20e1a\"\n"
    "ShowNameOnSquare150x150Logo=\"on\"\n"
    "ForegroundText=\"light\"/>\n"
    "</Application>";

/******************************************************************************/
VOID static GetManifestFileName(LPTSTR lpFileName, DWORD dwLen)
{
    /* Get module filename */
    GetModuleFileName(NULL, lpFileName, dwLen);
    
    PathRemoveExtension(lpFileName);
    
    lstrcat(lpFileName, TEXT(".VisualElementsManifest.xml"));  
}

/******************************************************************************/
VOID CheckWinTileManifest(VOID)
{    
    LPTSTR lpFileName;
    
    /* Allocate memory for the file name */
    lpFileName = HeapAlloc(g_hHeap, 0, sizeof(TCHAR) * MAX_PATH);
    
    if(NULL != lpFileName)
    {
        HANDLE hFile;
        
        GetManifestFileName(lpFileName, MAX_PATH);

        hFile = CreateFile(lpFileName, GENERIC_WRITE, 0, NULL, CREATE_NEW,
                FILE_ATTRIBUTE_NORMAL, NULL);

        if(INVALID_HANDLE_VALUE != hFile)
        {
            DWORD dwPos, dwLen, dwWritten;

            dwLen = sizeof(g_lpManifest) / sizeof(char);
            dwPos = 0;

            while(dwPos < dwLen)
            {
                if(!WriteFile(hFile, (LPCVOID)(((UINT_PTR)g_lpManifest) + dwPos), 
                    dwLen - dwPos, &dwWritten, NULL))
                {
                    break;
                }
                dwPos += dwWritten;
            }

            CloseHandle(hFile);

        }
        
        HeapFree(g_hHeap, 0, lpFileName);
    }
}
#include "MemDbg.h"

#undef HeapAlloc
#undef HeapFree

MEMDBGSTRUCT mdFirst = {0};
CRITICAL_SECTION csMemDbg;

VOID InitMemDbg()
{
	InitializeCriticalSection(&csMemDbg);
}

LPVOID WINAPI HeapAllocDbg(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes, INT iLine, char lpFile[])
{
	LPMEMDBGSTRUCT	lpMds	= &mdFirst;
	LPVOID			lpVoid	= HeapAlloc(hHeap, dwFlags, dwBytes);
	INT				cStrLen = 0;

	
	EnterCriticalSection(&csMemDbg);
	
	while(lpMds->lpNext != NULL)
		lpMds = (LPMEMDBGSTRUCT)(lpMds->lpNext);
	
	lpMds->lpNext = malloc(sizeof(MEMDBGSTRUCT));

	lpMds = (LPMEMDBGSTRUCT)(lpMds->lpNext);

	lpMds->lpNext = NULL;
	lpMds->iLine = iLine;
	lpMds->lpAddress = lpVoid;

	CopyMemory(lpMds->lpFileName, lpFile, __FILENAME_MAX_LENGTH__*sizeof(char));
	LeaveCriticalSection(&csMemDbg);

	return lpVoid;
}
BOOL WINAPI HeapFreeDbg(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem)
{
	LPMEMDBGSTRUCT	lpMds = &mdFirst;
	LPMEMDBGSTRUCT  lpPrevMds = NULL;

	EnterCriticalSection(&csMemDbg);

	while(lpMds->lpNext != NULL && lpMds->lpAddress != lpMem)
	{
		lpPrevMds = lpMds;
		lpMds = (LPMEMDBGSTRUCT)(lpMds->lpNext);
	}

	if(lpMds->lpAddress == lpMem)
	{
		lpPrevMds->lpNext = lpMds->lpNext;
		free(lpMds);
	}
	else
		OutputDebugString(TEXT("---=== SPRÁVA PAMATE: Zadaný Pointer nebol nájdený\n"));
	LeaveCriticalSection(&csMemDbg);

	return HeapFree(hHeap, dwFlags, lpMem);
}

VOID WriteMemDbgMessage()
{
	LPMEMDBGSTRUCT	lpMds = &mdFirst;
	LPMEMDBGSTRUCT	lpPrevMds;

	TCHAR			lpszMessage[1024];
	TCHAR			lpszFileName[__FILENAME_MAX_LENGTH__];
	INT				cLeaks		= 1;

	OutputDebugString(TEXT("------=== ZAÈIATOK VÝPISU SPRÁVY PAMATE ===------\n"));

	if(lpMds->lpNext == NULL)
		OutputDebugString(TEXT("Všetka pamä bola uvolnená.\n"));
	else
	{
		OutputDebugString(TEXT("Boli nájdené chyby správy pamäte:\n"));
		while(lpMds->lpNext != NULL)
		{
			lpPrevMds = lpMds;
			lpMds = (LPMEMDBGSTRUCT)(lpMds->lpNext);

			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
			lpMds->lpFileName, -1,
			lpszFileName, __FILENAME_MAX_LENGTH__);

			StringCchPrintf(lpszMessage, 1024, L"\t%i. Pama (%#010x) alokovaná na riadku è. %i v súbore \"%s\" nebola uvolnená.\n\0", cLeaks++, lpMds->lpAddress, lpMds->iLine, lpszFileName);
			OutputDebugString(lpszMessage);
			if(lpPrevMds != &mdFirst)
				free(lpPrevMds);
		}		
	}
	OutputDebugString(TEXT("-------=== KONIEC VÝPISU SPRÁVY PAMATE ===-------\n"));
	DeleteCriticalSection(&csMemDbg);
}
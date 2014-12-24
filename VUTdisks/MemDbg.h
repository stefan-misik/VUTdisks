#ifndef __MEMDBG
#define __MEMDBG

#ifndef UNICODE
	#define UNICODE
#endif	//UNICODE

#include <Windows.h>
#include <strsafe.h>

#define __FILENAME_MAX_LENGTH__		512


#define HeapAlloc(hHeap, dwFlags, dwBytes)		HeapAllocDbg((hHeap), (dwFlags), (dwBytes), __LINE__, __FILE__)
#define HeapFree								HeapFreeDbg

typedef struct MEMDBGSTRUCTtag
{
	LPVOID		lpAddress;
	INT			iLine;
	char		lpFileName[__FILENAME_MAX_LENGTH__];
	LPVOID		lpNext;	
} MEMDBGSTRUCT, *LPMEMDBGSTRUCT;

VOID InitMemDbg();
LPVOID WINAPI HeapAllocDbg(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes, INT iLine, char lpFile[]);
BOOL WINAPI HeapFreeDbg(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem);
VOID WriteMemDbgMessage();

#endif


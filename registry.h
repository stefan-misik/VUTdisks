#ifndef REGISTRY_H
#define REGISTRY_H

#include "common.h"


BOOL RunAtStartup(BOOL bEnable);

BOOL IsRegisteredToRunAtStartup(VOID);

BOOL DeleteMyRegKey(VOID);

VOID OpenMyRegKey(VOID);

VOID CloseMyRegKey(VOID);

VOID ReadRegistry(HWND hwnd);

VOID WriteRegistry(HWND hwnd);

#endif /* REGISTRY_H */


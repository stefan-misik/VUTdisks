#include "registry.h"
#include "resource.h"
#include "vut_disks.h"
#include "disk_mapper.h"
#include <Wincrypt.h>
#include <winnt.h>


static HKEY g_hMyRegKey;
static BOOL g_bMyRegKeyValid = FALSE;

/******************************************************************************/
BOOL RunAtStartup(BOOL bEnable)
{
	HKEY hKey;
	static TCHAR lpExeName[1024];

	/* Open Windows/Run key */
	if (RegOpenKeyEx(HKEY_CURRENT_USER,
		TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 0,
		KEY_SET_VALUE | KEY_WOW64_32KEY, &hKey) != 
		ERROR_SUCCESS)
	{
		return FALSE;
	}

	/* If id enabling */
	if (TRUE == bEnable)
	{
		DWORD dwRes;

		/* Get path to current EXE file */
		dwRes = GetModuleFileName(NULL, lpExeName, 1024);

		/* Return, if filename was nor received correctly */
		if (dwRes == 0 && dwRes >= 1024)
		{
            RegCloseKey(hKey);
			return FALSE;
		}	
		
		/* Ty to write filename of current executable to registry */
		if (ERROR_SUCCESS != RegSetValueEx(hKey, g_lpCaption, 0,
			REG_SZ, (LPBYTE)lpExeName, (dwRes + 1) * sizeof(TCHAR)))
		{
			RegCloseKey(hKey);			
			return FALSE;
		}				
	}
	else
	{
		/* Delete registry value holding filename of VUT Disk Mapper */
		RegDeleteValue(hKey, g_lpCaption);
	}

	/* Close all registry keys */
	RegCloseKey(hKey);

	return TRUE;
}

/******************************************************************************/
BOOL IsRegisteredToRunAtStartup(VOID)
{
	HKEY hKey;
	LONG lRes;

	
	/* Open Windows/Run key */
	if (RegOpenKeyEx(HKEY_CURRENT_USER,
		TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 0,
		KEY_QUERY_VALUE | KEY_WOW64_32KEY, &hKey) != 
		ERROR_SUCCESS)
	{		
		return FALSE;
	}

	/* Check if specified registry value exists */
	lRes = RegQueryValueEx(hKey, g_lpCaption, NULL,
		NULL, NULL, NULL);

	/* Close all registry keys */
	RegCloseKey(hKey);	

	/* If value exists */
	if (ERROR_SUCCESS == lRes)
	{		
		return TRUE;		
	}

	return FALSE;
}

/******************************************************************************/
BOOL DeleteMyRegKey(VOID)
{
    /* Close the registry Key */
    CloseMyRegKey();

    /* Delete the registry data */
	if (RegDeleteKey(HKEY_CURRENT_USER, TEXT("Software\\VUTdisks")) != 
	    ERROR_SUCCESS)
	{
		return FALSE;
	}

    /* Open the registry key */
    OpenMyRegKey();

	return TRUE;
}

/******************************************************************************/
VOID OpenMyRegKey(VOID)
{	
	DWORD	dwDisposition;
	
	if (RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("Software\\VUTdisks"), 0,
		NULL, 0, KEY_WRITE | KEY_READ | KEY_WOW64_32KEY,
		NULL, &g_hMyRegKey, &dwDisposition) != ERROR_SUCCESS)
	{
		g_bMyRegKeyValid = FALSE;
	}
    else
    {
        g_bMyRegKeyValid = TRUE;
    }

	if (dwDisposition == REG_CREATED_NEW_KEY)
	{
		RegSetValueEx(g_hMyRegKey, TEXT("szLogin"), 0,
			REG_SZ, (LPBYTE)NULL, 0);

		RegSetValueEx(g_hMyRegKey, TEXT("szId"), 0,
			REG_SZ, (LPBYTE)NULL, 0);

		RegSetValueEx(g_hMyRegKey, TEXT("pbPassword"), 0,
			REG_BINARY, (LPBYTE)NULL, 0);

        RegSetValueEx(g_hMyRegKey, TEXT("dwDiskEnable"), 0,
			REG_SZ, (LPBYTE)NULL, 0);

        RegSetValueEx(g_hMyRegKey, TEXT("szDiskLetters"), 0,
			REG_SZ, (LPBYTE)NULL, 0);
	}
}

/******************************************************************************/
VOID CloseMyRegKey(VOID)
{
     if (TRUE == g_bMyRegKeyValid)
    {
        RegCloseKey(g_hMyRegKey);
        g_bMyRegKeyValid = FALSE;
    }
}

/******************************************************************************/
VOID ReadRegistry(HWND hwnd)
{	
	DWORD dwLength;
	DWORD dwType;
    BOOL bLoadedLogin = FALSE;
    BOOL bLoadedDiskSelection = FALSE;
    
    /* Check key */
    if(!g_bMyRegKeyValid)
    {
        return;
    }
    
    /* Read disk selection */
    dwLength = sizeof(DWORD);
    RegQueryValueEx(g_hMyRegKey, TEXT("dwDiskEnable"), 0,
        &dwType, (LPBYTE)(&(g_ds.dwDiskEnable)), &dwLength);
    if (REG_DWORD != dwType)
	{
		dwLength = 0;
	}
    if(0 != dwLength)
    {
        dwLength = VUT_DISK_NUM * sizeof(TCHAR);
        RegQueryValueEx(g_hMyRegKey, TEXT("szDiskLetters"), 0,
            &dwType, (LPBYTE)(&(g_ds.aDiskLetters)), &dwLength);
        if (REG_SZ == dwType && (VUT_DISK_NUM * sizeof(TCHAR)) == dwLength)
        {
            bLoadedDiskSelection = TRUE;
        }
    }
    if(FALSE == bLoadedDiskSelection)
    {
        DiskSelectGetDefaults(&g_ds);
    }
    if(0 != DiskSelectToString(&g_ds, g_lpDiskSelect, DISKSELECT_MAX_LENGTH))
        SetDlgItemText(hwnd, IDC_DISK_SELECTED, (LPTSTR)g_lpDiskSelect);

	/* Read login */
	dwLength = (LOGIN_MAX_LENGTH - 1) * sizeof(TCHAR);
	RegQueryValueEx(g_hMyRegKey, TEXT("szLogin"), 0,
		&dwType, (LPBYTE)g_lpLogin, &dwLength);

	if (REG_SZ != dwType)
	{
		dwLength = 0;
	}	
	g_lpLogin[dwLength] = TEXT('\0');	
	SetDlgItemText(hwnd, IDC_LOGIN, g_lpLogin);
    
    if(0 != dwLength)
    {
        bLoadedLogin = TRUE;
    }

	/* Read ID */
	dwLength = (LOGIN_MAX_LENGTH - 1) * sizeof(TCHAR);
	RegQueryValueEx(g_hMyRegKey, TEXT("szId"), 0,
		&dwType, (LPBYTE)g_lpLogin, &dwLength);

	if (REG_SZ != dwType)
	{
		dwLength = 0;
	}
	g_lpLogin[dwLength] = TEXT('\0');
	SetDlgItemText(hwnd, IDC_ID, g_lpLogin);
    
    if(0 != dwLength && bLoadedLogin)
    {
        SendDlgItemMessage(hwnd, IDC_SAVE_LOGIN, 
            BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
    }
    VUTDisksEnableSavePassword(hwnd, 0 != dwLength && bLoadedLogin);

	/* Read Password length*/
	if (ERROR_SUCCESS ==
		RegQueryValueEx(g_hMyRegKey, TEXT("pbPassword"), NULL,
		&dwType, NULL, &dwLength))
	{
		DATA_BLOB dbInput, dbOutput;
		PBYTE pbData = HeapAlloc(g_hHeap, 0, dwLength);

		/* Read Password */
		if (ERROR_SUCCESS ==
			RegQueryValueEx(g_hMyRegKey, TEXT("pbPassword"), NULL,
			&dwType, pbData, &dwLength))
		{
			dbInput.pbData = pbData;
			dbInput.cbData = dwLength;

			/* Try to decrypt Password */
			if (TRUE == CryptUnprotectData(
				&dbInput, NULL, NULL, NULL, NULL, 0, &dbOutput))
			{
				/* Set Password */
				CopyMemory(g_lpPassword, dbOutput.pbData, dbOutput.cbData);
				g_lpPassword[dbOutput.cbData] = TEXT('\0');

				SetDlgItemText(hwnd, IDC_PASSWD, 
                                    (LPTSTR)g_lpPassword);
				ZeroMemory(dbOutput.pbData, dbOutput.cbData);
				ZeroMemory(g_lpPassword, PASSWORD_MAX_LENGTH);
				LocalFree(dbOutput.pbData);

				SendDlgItemMessage(hwnd, IDC_SAVE_PASS, 
                    BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
			}
			else
			{
				SetDlgItemText(hwnd, IDC_PASSWD, NULL);
			}

		}
		else
		{
			SetDlgItemText(hwnd, IDC_PASSWD, NULL);
		}

		HeapFree(g_hHeap, 0, pbData);
	}
	else
	{
		SetDlgItemText(hwnd, IDC_PASSWD, NULL);
	}
}

/******************************************************************************/
VOID WriteDiskSelectionRegistry(VOID)
{
    /* Check key */
    if(!g_bMyRegKeyValid)
    {
        return;
    }

    /* Write disk selection */
    RegSetValueEx(g_hMyRegKey, TEXT("dwDiskEnable"), 0,
        REG_DWORD, (LPBYTE)(&(g_ds.dwDiskEnable)), sizeof(DWORD));
    RegSetValueEx(g_hMyRegKey, TEXT("szDiskLetters"), 0,
        REG_SZ, (LPBYTE)(&(g_ds.aDiskLetters)), VUT_DISK_NUM * sizeof(TCHAR));
}

/******************************************************************************/
VOID WriteLoginRegistry(HWND hwnd)
{	
	DWORD dwLength;
    
    /* Check key */
    if(!g_bMyRegKeyValid)
    {
        return;
    }

    /* If 'Save Login data' is checked */	
	if (BST_CHECKED ==
		SendDlgItemMessage(hwnd, IDC_SAVE_LOGIN, BM_GETCHECK, 0, 0))
	{    
        /* Save Login */
        dwLength = GetDlgItemText(hwnd, IDC_LOGIN, g_lpLogin, LOGIN_MAX_LENGTH);
        RegSetValueEx(g_hMyRegKey, TEXT("szLogin"), 0,
            REG_SZ, (LPBYTE)g_lpLogin, dwLength * sizeof(TCHAR));

        /* Save Id */
        dwLength = GetDlgItemText(hwnd, IDC_ID, g_lpId, LOGIN_MAX_LENGTH);
        RegSetValueEx(g_hMyRegKey, TEXT("szId"), 0,
            REG_SZ, (LPBYTE)g_lpId, dwLength * sizeof(TCHAR));

        /* If 'Save Password' is checked */	
        if (BST_CHECKED ==
            SendDlgItemMessage(hwnd, IDC_SAVE_PASS, BM_GETCHECK, 0, 0))
        {
            DATA_BLOB dbInput, dbOutput;

            dbInput.cbData = GetDlgItemText(hwnd, IDC_PASSWD, g_lpPassword,
                PASSWORD_MAX_LENGTH) * sizeof(TCHAR);
            dbInput.pbData = (PBYTE)g_lpPassword;

            /* Encrypt Password */
            if (FALSE ==
                CryptProtectData(&dbInput, NULL, NULL, NULL,
                NULL, 0, &dbOutput))
            {
                dbOutput.cbData = 0;
                dbOutput.pbData = NULL;
            }		

            /* Store Password */
            RegSetValueEx(g_hMyRegKey, TEXT("pbPassword"), 0,
                REG_BINARY, dbOutput.pbData, dbOutput.cbData);

            if (dbOutput.cbData > 0)
            {
                LocalFree(dbOutput.pbData);
            }
        }
        else
        {
            /* Clear Password */
            RegSetValueEx(g_hMyRegKey, TEXT("pbPassword"), 0,
                REG_BINARY, (LPBYTE)NULL, 0);
        }
    }    
}

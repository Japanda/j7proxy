#include "main.h"

bool RemoteLoadLibW ( DWORD dwProcessId, PCWSTR pszLibFile )
{
     bool bInjected		= false;
     
     HANDLE	hProcess	= NULL,
            hThread		= NULL;
     
     PWSTR pszLibFileRemote = NULL;

     int iChars = 1 + lstrlenW(pszLibFile);
     int iSize  = iChars * sizeof(WCHAR);	// calculate the size we need in the foreign process
     
     // open a handle to the foreign process 
     hProcess = OpenProcess( PROCESS_ACCESS, FALSE, dwProcessId );
     
     if (hProcess == NULL)
        return false;
        
     // allocate space for the string in the foreign process
     pszLibFileRemote = (PWSTR)VirtualAllocEx(hProcess, NULL, iSize, MEM_COMMIT, PAGE_READWRITE);
     
     if (pszLibFileRemote == NULL)
        return false;
        
     // Copy the DLL path in to the foreign process
     if (!WriteProcessMemory(hProcess, pszLibFileRemote,(PVOID) pszLibFile, iSize, NULL))
        return false;
        
     // all DLLs are mapped in the local address space of a process and in 99% of all cases
     // it's nt.dll and then the kernel so the relative address of kernel functions is equal in
     // all processes
     LPTHREAD_START_ROUTINE pfnThreadRtn = (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("Kernel32"), "LoadLibraryW");
     
     if (pfnThreadRtn == NULL)
        return false;
        
     hThread = CreateRemoteThread(hProcess, NULL, 0, pfnThreadRtn, pszLibFileRemote, 0, NULL);
     
     // unable to create foreign thread
     if (hThread == NULL)
        return false;
        
     // wait for the remote-thread to finish
     WaitForSingleObject(hThread, INFINITE);
     
     // everything went smoothly till here so we assume that the injection was successful
     bInjected = true;
     
     
     if (pszLibFileRemote != NULL) 
        VirtualFreeEx(hProcess, pszLibFileRemote, iSize, MEM_RELEASE);
     
     if (hThread != NULL) 
        CloseHandle(hThread);
        
     if (hProcess != NULL) 
        CloseHandle(hProcess);
        
     return bInjected;
}

bool RemoteLoadLibA ( DWORD dwProcessId, PCSTR pszLibFile )
{
	PWSTR pszLibFileW = NULL;	// pointer to the UNICODE string
    
    // allocate space for the converted UNICODE string
    pszLibFileW = (PWSTR) _alloca((lstrlenA(pszLibFile) + 1) * sizeof(WCHAR));

	// convert from ANSI to UNICODE
	wsprintfW(pszLibFileW, L"%S", pszLibFile);

	// call the UNICODE version
	return RemoteLoadLibW ( dwProcessId, pszLibFileW );
}

int iGetDebugPrivilege ( void )
{
	HANDLE             hToken;
	TOKEN_PRIVILEGES   CurrentTPriv;
	LUID               luidVal;

	if ( OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken) == FALSE )
		return 0;

	if ( LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luidVal) == FALSE )
	{
		CloseHandle( hToken );
		return 0;
	}

	CurrentTPriv.PrivilegeCount            = 1;
	CurrentTPriv.Privileges[0].Attributes  = SE_PRIVILEGE_ENABLED;
	CurrentTPriv.Privileges[0].Luid        = luidVal;

	int iRet = AdjustTokenPrivileges(hToken, FALSE, &CurrentTPriv, sizeof( TOKEN_PRIVILEGES ), NULL, NULL);

	CloseHandle(hToken);

	return iRet;
}

#include "main.h"

#define ERROR_MSG "Error - j7proxy"

char szWorkingDirectory[MAX_PATH];
char szAppName[] = "CLIENT";
char szIniFileName[MAX_PATH];
char szGTASAMPParam[2048];
char szNickName[256], szHost[256], szPort[256], szPassword[256];
char szGTAPath[MAX_PATH], szGTAPathEXE[MAX_PATH];

void WriteRegistryStringValue ( HKEY hkRoot, LPCSTR szSubKey, LPCSTR szValue, char* szBuffer )
{
    HKEY hkTemp;
    RegCreateKeyEx ( hkRoot, szSubKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkTemp, NULL );
    if ( hkTemp )
    {
        RegSetValueEx ( hkTemp, szValue, NULL, REG_SZ, (LPBYTE)szBuffer, strlen(szBuffer) + 1 );
        RegCloseKey ( hkTemp );
    }
}

bool ReadRegistryStringValue ( HKEY hkRoot, LPCSTR szSubKey, LPCSTR szValue, char* szBuffer=NULL, DWORD dwBufferSize=0 )
{
    // Clear output
    if ( szBuffer && dwBufferSize )
        szBuffer[0] = 0;

    bool bResult = false;
    HKEY hkTemp = NULL;
    if ( RegOpenKeyEx ( hkRoot, szSubKey, 0, KEY_READ, &hkTemp ) == ERROR_SUCCESS ) 
    {
        if ( RegQueryValueEx ( hkTemp, szValue, NULL, NULL, (LPBYTE)szBuffer, &dwBufferSize ) == ERROR_SUCCESS )
        {
            bResult = true;
        }
        RegCloseKey ( hkTemp );
    }
    return bResult;
}

int GetGamePath ( char * szBuffer, size_t sizeBufferSize )
{
    WIN32_FIND_DATA fdFileInfo;
    char szRegBuffer[MAX_PATH];
    ReadRegistryStringValue ( HKEY_LOCAL_MACHINE, "SOFTWARE\\Rockstar Games\\GTA San Andreas\\Installation", "ExePath", szRegBuffer, MAX_PATH - 1 );

    if ( ( GetAsyncKeyState ( VK_CONTROL ) & 0x8000 ) == 0 )
    {
        if ( strlen( szRegBuffer ) )
        {
			// Check for replacement characters (?), to see if there are any (unsupported) unicode characters
			if ( strchr ( szRegBuffer, '?' ) > 0 )
				return -1;

            char szExePath[MAX_PATH];
			sprintf(szGTAPath, szRegBuffer);
            sprintf ( szExePath, "%s\\%s", szRegBuffer, "gta_sa.exe" );
            if ( INVALID_HANDLE_VALUE != FindFirstFile( szExePath, &fdFileInfo ) )
            {
                _snprintf ( szBuffer, sizeBufferSize, "%s", szExePath );
                return 1;
            }
        }
    }
    BROWSEINFO bi = { 0 };
    bi.lpszTitle = "Select your Grand Theft Auto: San Andreas Installation Directory";
    LPITEMIDLIST pidl = SHBrowseForFolder ( &bi );

    if ( pidl != 0 )
    {
        // get the name of the  folder
        if ( !SHGetPathFromIDList ( pidl, szBuffer ) )
        {
            szBuffer = NULL;
        }

        // free memory used
        IMalloc * imalloc = 0;
        if ( SUCCEEDED( SHGetMalloc ( &imalloc )) )
        {
            imalloc->Free ( pidl );
            imalloc->Release ( );
        }
    
        char szExePath[MAX_PATH];
		sprintf(szGTAPath, szBuffer);
        sprintf ( szExePath, "%s\\gta_sa.exe", szBuffer );
        if ( INVALID_HANDLE_VALUE != FindFirstFile( szExePath, &fdFileInfo ) )
        {
            WriteRegistryStringValue ( HKEY_LOCAL_MACHINE, "SOFTWARE\\Rockstar Games\\GTA San Andreas\\Installation", "ExePath", szBuffer );
        }
        else
        {
            if ( MessageBox ( NULL, "Could not find gta_sa.exe at the path you have selected. Choose another folder?", "Error", MB_OKCANCEL ) == IDOK )
            {
                return GetGamePath ( szBuffer, sizeBufferSize );
            }
            else
            {
                return 0;
            }
        }
        return 1;
    }
    else
    {
        return 0;
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	if(GetModuleFileName(NULL, szWorkingDirectory, sizeof(szWorkingDirectory) - 32) != 0)
	{
		if(strrchr(szWorkingDirectory, '\\') != NULL)
			*strrchr(szWorkingDirectory, '\\') = 0;
		else
			strcpy(szWorkingDirectory, ".");
	}
	else
		strcpy(szWorkingDirectory, ".");
	sprintf(szIniFileName, "%s\\j7proxy.ini", szWorkingDirectory);
	

	if(GetGamePath(szGTAPathEXE, MAX_PATH) == 0)
	{
		MessageBox(0, "Could not get the gta path", ERROR_MSG, 0);
		ExitProcess(0);
	}

	GetPrivateProfileString(szAppName, "Nickname", NULL, szNickName, sizeof(szNickName), szIniFileName);
	GetPrivateProfileString(szAppName, "Proxy_server", NULL, szHost, sizeof(szHost), szIniFileName);
	GetPrivateProfileString(szAppName, "Proxy_port", NULL, szPort, sizeof(szPort), szIniFileName);
	GetPrivateProfileString(szAppName, "Target_SAMPServer_password", NULL, szPassword, sizeof(szPassword), szIniFileName);

	strcpy(szGTASAMPParam, "-c -n ");
	strcat(szGTASAMPParam, szNickName);
	strcat(szGTASAMPParam, " -h ");
	strcat(szGTASAMPParam, szHost);
	strcat(szGTASAMPParam, " -p ");
	strcat(szGTASAMPParam, szPort);
	strcat(szGTASAMPParam, " -w ");
	strcat(szGTASAMPParam, szPassword);


	PROCESS_INFORMATION piLoadee;
	STARTUPINFO siLoadee;
	memset(&piLoadee, 0, sizeof (PROCESS_INFORMATION));
	memset(&siLoadee, 0, sizeof (STARTUPINFO));
	siLoadee.cb = sizeof(STARTUPINFO);
	if(!CreateProcess(szGTAPathEXE, szGTASAMPParam, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, szGTAPath, &siLoadee, &piLoadee))
	{
		MessageBox(NULL, "Could not start GTA", ERROR_MSG, MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	if(!RemoteLoadLib(piLoadee.dwProcessId, "samp.dll"))
	{
		MessageBox(NULL, "Could not inject SA-MP", ERROR_MSG, MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	strcat(szWorkingDirectory, "\\j7proxy.dll");
	if(!RemoteLoadLib(piLoadee.dwProcessId, szWorkingDirectory))
	{
		MessageBox(NULL, "Could not inject j7proxy", ERROR_MSG, MB_ICONEXCLAMATION | MB_OK);
		CloseHandle(piLoadee.hThread);
		return 0;
	}

	ResumeThread(piLoadee.hThread);

    return 0;
}

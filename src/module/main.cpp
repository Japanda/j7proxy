#include "main.h"

HMODULE dwSAMPAddr = NULL;
char szWorkingDirectory[MAX_PATH];
char szIniFileName[MAX_PATH];
char szAppName[] = "CLIENT";
DWORD iIniPort;
DWORD dwPort;

DWORD HOOK_PORTSHIFT_CONTINUE;
uint8_t _declspec(naked) hook_portshift(void)
{
	_asm
	{
		push ecx
		push eax
		mov edx, dwPort
		push edx
		push 0

		push ebx
		mov ebx, dwSAMPAddr
		add ebx, 0x16A3A
		mov HOOK_PORTSHIFT_CONTINUE, ebx
		pop ebx

		jmp HOOK_PORTSHIFT_CONTINUE
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch(ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:

		if(GetModuleFileName(NULL, szWorkingDirectory, sizeof(szWorkingDirectory) - 32) != 0)
		{
			if(strrchr(szWorkingDirectory, '\\') != NULL)
				*strrchr(szWorkingDirectory, '\\') = 0;
			else
				strcpy(szWorkingDirectory, ".");
		}
		else
			strcpy(szWorkingDirectory, ".");
		sprintf(szIniFileName, "%s\\j7proxy\\j7proxy.ini", szWorkingDirectory);

		iIniPort = GetPrivateProfileInt(szAppName, "Target_SAMPServer_port", 0, szIniFileName);
		if(iIniPort == 0)
		{
			MessageBox(0, "Error reading the INI", "Error - j7proxy", 0);
			ExitProcess(0);
		}
		dwPort = iIniPort;

		dwSAMPAddr = GetModuleHandle("samp");
		
		CDetour api;
		if(api.Create((uint8_t *)((uint32_t )dwSAMPAddr) + 0x16A35, (uint8_t *)hook_portshift, DETOUR_TYPE_JMP, 5) == 0)
			MessageBox(0, "Failed to hook hook_portshift.", "j7proxy - Error", MB_ICONEXCLAMATION | MB_OK);

		break;
	
	case DLL_PROCESS_DETACH:

		break;
	}
	
	return TRUE;
}
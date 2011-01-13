#ifndef _REMOTELOADLIB_H_
#define _REMOTELOADLIB_H_
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <malloc.h>  
#include <TlHelp32.h>

#define PROCESS_ACCESS PROCESS_QUERY_INFORMATION |PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE

#ifdef UNICODE
#define RemoteLoadLib RemoteLoadLibW
#else
#define RemoteLoadLib RemoteLoadLibA
#endif
bool RemoteLoadLibW ( DWORD dwProcessId, PCWSTR pszLibFile );
bool RemoteLoadLibA ( DWORD dwProcessId, PCSTR  pszLibFile );

#endif

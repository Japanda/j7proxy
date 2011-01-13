/*
	CDetour 2.0 by Sinner, don't forget to credit me!

	Credits:
	LanceVorgin (his CDetour class) + z0mbie (ADE32)
*/

#pragma once

#pragma warning(disable: 4311)
#pragma warning(disable: 4312)

#include "ADE32.h"

typedef enum detour_types_s
{
	DETOUR_TYPE_NOT_SET = -1,
	DETOUR_TYPE_JMP,
	DETOUR_TYPE_PUSH_RET,
	DETOUR_TYPE_PUSH_FUNC,
	DETOUR_TYPE_CALL_FUNC,

} detour_types_t;

#define DetourRandTypeLow		DETOUR_TYPE_OBS_ADD
#define DetourRandTypeHigh		DETOUR_TYPE_OBS_ADDNOT

class CDetour
{
public:
	void* memcpy_s( void* pvAddress, const void* pvBuffer, size_t stLen );
	void *Create(BYTE *orig, const BYTE *det, int iPatchType, int len=0);
	void *Create(char *dllName, char *apiName, const BYTE *det, int iPatchType, int len=0);
	bool Remove(BYTE *orig, BYTE *jmp, int iPatchType, int len=0);
	bool Remove(char *dllName, char *apiName, BYTE *jmp, int iPatchType, int len=0);
	bool BackupFunction( BYTE* func, int len );
	bool RestoreFunction( BYTE* func, int len );
	BYTE bBackup[20];

private:
	bool Detour(BYTE *&jmp, BYTE *&orig, const BYTE *&det, int iPatchType, int len);
	int GetDetourLen(int iPatchType);
	int GetDetourLenAuto(BYTE *&orig, int iMinLen);

private:
	HMODULE m_hModule;
	DWORD m_dwAddress;
};


#pragma once
#include <Windows.h>

class CPatch
{
public:
	inline static void Patch(void* address, void* data, int size)
	{
		unsigned long protect[2];
		VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &protect[0]);
		memcpy(address, data, size);
		VirtualProtect(address, size, protect[0], &protect[1]);
	}

	inline static void Patch2(int address, void* data, int size)
	{
		unsigned long protect[2];
		VirtualProtect((void *)address, size, PAGE_EXECUTE_READWRITE, &protect[0]);
		memcpy((void *)address, data, size);
		VirtualProtect((void *)address, size, protect[0], &protect[1]);
	}
public:
	inline static void Unprotect(int address, int size)
	{
		unsigned long protect[2];
		VirtualProtect((void *)address, size, PAGE_EXECUTE_READWRITE, &protect[0]);
	}
	inline static void Nop(int address, int size)
	{
		unsigned long protect[2];
		VirtualProtect((void *)address, size, PAGE_EXECUTE_READWRITE, &protect[0]);
		memset((void *)address, 0x90, size);
		VirtualProtect((void *)address, size, protect[0], &protect[1]);
	}
	inline static void FillWithZeroes(int address, int size)
	{
		unsigned long protect[2];
		VirtualProtect((void *)address, size, PAGE_EXECUTE_READWRITE, &protect[0]);
		memset((void *)address, 0x00, size);
		VirtualProtect((void *)address, size, protect[0], &protect[1]);
	}
	inline static void RedirectCall(int address, void *func)
	{
		int temp = 0xE8;
		Patch((void *)address, &temp, 1);
		temp = (int)func - ((int)address + 5);
		Patch((void *)((int)address + 1), &temp, 4);
	}
	inline static void RedirectJump(int address, void *func)
	{
		int temp = 0xE9;
		Patch((void *)address, &temp, 1);
		temp = (int)func - ((int)address + 5);
		Patch((void *)((int)address + 1), &temp, 4);
	}
	inline static void SetChar(int address, char value)
	{
		Patch((void *)address, &value, 1);
	}
	inline static void SetShort(int address, short value)
	{
		Patch((void *)address, &value, 2);
	}
	inline static void SetInt(int address, int value)
	{
		Patch((void *)address, &value, 4);
	}
	inline static void SetFloat(int address, float value)
	{
		Patch((void *)address, &value, 4);
	}
	inline static void SetPointer(int address, void *value)
	{
		Patch((void *)address, &value, 4);
	}

	inline static void AdjustPointer(int address, void *value, int offset)
	{
		int result;
		if (((char)*(DWORD*)(address-0x3) == (char)0x8B)  || 
			((char)*(DWORD*)(address-0x3) == (char)0x83)  || 
			((char)*(DWORD*)(address-0x3) == (char)0x39)  || 
			((char)*(DWORD*)(address-0x3) == (char)0xC7)  ||
			((char)*(DWORD*)(address-0x1) == (char)0xA1)  ||
			((char)*(DWORD*)(address-0x2) == (char)0x8B)  ||
			((char)*(DWORD*)(address-0x3) == (char)0x89)  ||
			((char)*(DWORD*)(address-0x2) == (char)0x89)  
			) {

				if (
			((char)*(DWORD*)(address-0x2) != (char)0xE9)  &&
			((char)*(DWORD*)(address-0x1) != (char)0x00)  
			) {
		result = (DWORD)value + (DWORD)*(DWORD*)address - (DWORD)offset;
		Patch((void *)address, &result, 4);
				}
		}
	}
	
	inline static bool FileExists(const TCHAR *fileName)
	{
    DWORD       fileAttr;
    fileAttr = GetFileAttributes(fileName);
    if (0xFFFFFFFF == fileAttr && GetLastError()==ERROR_FILE_NOT_FOUND)
        return false;
    return true;
	}
};
#pragma once
//#include "printf_messagebox.h"
#include <Windows.h>

////////////// Patching functions //////////////////

struct tForbiddenMemoryRegion
{
	DWORD start;
	DWORD end;
};

class CPatch
{
private:
	static bool PatchDebugMode;

	static bool SavedPatchDebugModeStack[32];
	static int SavedPatchDebugModeStackID;

	static tForbiddenMemoryRegion m_forbiddenMemoryRegionArray[16];
	static int m_numOfForbiddenMemoryRegions;
	static unsigned int m_numberOfMemoryChanges;
	static bool m_bDumpMemoryAddressesToLogFile;

	// Checks if address is not forbidden
	static void CheckIfNotForbiddenAddress(DWORD dwAddress, DWORD size);

	// Performs task for memory address
	static void DoTasksForMemoryAddress(DWORD dwAddress, int iSize);

	static void _PatchMemoryDataWithoutDebugCheck(DWORD dwAddress, const void* bData, int iSize);

public:

	// Initializer
	static void Init();

	// Returns number of memory changes
	static unsigned int GetNumberOfMemoryChanges();

	// Adds a forbidden memory region
	static void AddForbiddenMemoryRegion(DWORD start, DWORD end);

	// Enables/disables dumping memory addresses to log file
	static void EnableDumpingMemoryAddressesToLogFile(bool bEnable);

	// Saves and disables patch debug state
	static void SaveAndDisablePatchDebugState();

	// Restores saved patch debug mode
	static void RestoreSavedPatchDebugState();

	// Disables debug mode
	static void DisableDebugMode();

	// Enables debug mode
	static void EnableDebugMode();

	// Checks if debug mode is active
	static bool IsDebugModeActive();

	// Patches memory data
	static void PatchMemoryData(DWORD dwAddress, const void* bData, int iSize);

	// Patches BYTE value
	static void PatchBYTE(DWORD dwAddress, BYTE to);

	// Patches WORD value
	static void PatchWORD(DWORD dwAddress, WORD to);

	// Patches pointer
	static void PatchPointer(DWORD dwAddress, const void* to);

	// Patches DWORD value
	static void PatchDWORD(DWORD dwAddress, DWORD to);

	// Patches float value
	static void PatchFloat(DWORD dwAddress, float to);

	static void PutCallFunction(DWORD dwAddress, const void* to);

	// NOPs instruction (0x90)
	static void NOPinstructions(DWORD dwAddress, int iSize);

	static void RedirectFunction(DWORD dwAddress, void* to);

	static void PutOnAddressRelative4byteAddressToAddress(DWORD sourceAddress, DWORD destination);

	static void RedirectCode(DWORD dwAddress, DWORD to);

	static void RedirectCode(DWORD dwAddress, void* to);

	static void RedirectCodeOneByte(DWORD dwAddress, DWORD to);

	static void RedirectCodeOneByte(DWORD dwAddress, void* to);

	// Redirects method
	template<typename T>static void RedirectMethod(DWORD dwAddress, T method)
	{
		auto _ptr = method;
		void*& ptr = (void*&)_ptr;
		RedirectCode(dwAddress, ptr);
	}
};
#include "CPatch.h"
//#include "CGenericLogStorage.h"
//#include "ProjectConstants.h"

bool CPatch::PatchDebugMode;

bool CPatch::SavedPatchDebugModeStack[32];
int CPatch::SavedPatchDebugModeStackID;

tForbiddenMemoryRegion CPatch::m_forbiddenMemoryRegionArray[16];
int CPatch::m_numOfForbiddenMemoryRegions;
unsigned int CPatch::m_numberOfMemoryChanges;
bool CPatch::m_bDumpMemoryAddressesToLogFile;

// Initializer
void CPatch::Init()
{
	CPatch::PatchDebugMode = false;
	CPatch::SavedPatchDebugModeStackID = 0;
	CPatch::m_numOfForbiddenMemoryRegions = 0;
	CPatch::m_numberOfMemoryChanges = 0;
	CPatch::m_bDumpMemoryAddressesToLogFile = false;
}

// Returns number of memory changes
unsigned int CPatch::GetNumberOfMemoryChanges()
{
	return CPatch::m_numberOfMemoryChanges;
}

// Adds a forbidden memory region
void CPatch::AddForbiddenMemoryRegion(DWORD start, DWORD end)
{
	tForbiddenMemoryRegion* pRegion = &CPatch::m_forbiddenMemoryRegionArray[CPatch::m_numOfForbiddenMemoryRegions++];
	pRegion -> start = start;
	pRegion -> end = end;
}

// Enables/disables dumping memory addresses to log file
void CPatch::EnableDumpingMemoryAddressesToLogFile(bool bEnable)
{
	CPatch::m_bDumpMemoryAddressesToLogFile = bEnable;
}

// Saves and disables patch debug state
void CPatch::SaveAndDisablePatchDebugState()
{
	SavedPatchDebugModeStack[SavedPatchDebugModeStackID++] = PatchDebugMode;

	PatchDebugMode = false;
}

// Restores saved patch debug mode
void CPatch::RestoreSavedPatchDebugState()
{
	SavedPatchDebugModeStackID--;
	PatchDebugMode = SavedPatchDebugModeStack[SavedPatchDebugModeStackID];
}

// Disables debug mode
void CPatch::DisableDebugMode()
{
	CPatch::PatchDebugMode = false;
}

// Checks if debug mode is active
bool CPatch::IsDebugModeActive()
{
	return CPatch::PatchDebugMode;
}

// Patches memory data
void CPatch::PatchMemoryData(DWORD dwAddress, const void* bData, int iSize)
{
	_PatchMemoryDataWithoutDebugCheck(dwAddress, bData, iSize);
}

// Patches BYTE value
void CPatch::PatchBYTE(DWORD dwAddress, BYTE to)
{		
	_PatchMemoryDataWithoutDebugCheck(dwAddress, &to, sizeof(to));
}

// Patches WORD value
void CPatch::PatchWORD(DWORD dwAddress, WORD to)
{
	_PatchMemoryDataWithoutDebugCheck(dwAddress, &to, sizeof(to));
}

// Patches pointer
void CPatch::PatchPointer(DWORD dwAddress, const void* to)
{
	_PatchMemoryDataWithoutDebugCheck(dwAddress, &to, sizeof(to));
}

// Patches DWORD value
void CPatch::PatchDWORD(DWORD dwAddress, DWORD to)
{
	_PatchMemoryDataWithoutDebugCheck(dwAddress, &to, sizeof(to));
}

// Patches float value
void CPatch::PatchFloat(DWORD dwAddress, float to)
{
	_PatchMemoryDataWithoutDebugCheck(dwAddress, &to, sizeof(to));
}

void CPatch::PutCallFunction(DWORD dwAddress, const void* to)
{
	DWORD movement = (DWORD)to - dwAddress - 5;

	SaveAndDisablePatchDebugState();

	PatchBYTE(dwAddress, 0xE8);		// call

	PatchDWORD(dwAddress + 1, movement);

	RestoreSavedPatchDebugState();
}

// NOPs instruction (0x90)
void CPatch::NOPinstructions(DWORD dwAddress, int iSize)
{
	DWORD dwProtect[2];
	VirtualProtect((PVOID)dwAddress, iSize, PAGE_EXECUTE_READWRITE, &dwProtect[0]);
	memset((PVOID)dwAddress, 0x90, iSize);
	VirtualProtect((PVOID)dwAddress, iSize, dwProtect[0], &dwProtect[1]);
}

void CPatch::RedirectFunction(DWORD dwAddress, void* to)
{
	RedirectCode(dwAddress, (void*)to);
}

void CPatch::PutOnAddressRelative4byteAddressToAddress(DWORD sourceAddress, DWORD destination)
{
	DWORD movement = (DWORD)destination - sourceAddress - 4;

	SaveAndDisablePatchDebugState();

	PatchDWORD(sourceAddress, movement);

	RestoreSavedPatchDebugState();
}

void CPatch::RedirectCode(DWORD dwAddress, DWORD to)
{
	DWORD movement = (DWORD)to - dwAddress - 5;

	SaveAndDisablePatchDebugState();

	unsigned __int8 data[5];
	data[0] = 0xE9;		// jmp
	*(DWORD*)(data+1) = movement;

	PatchMemoryData(dwAddress, data, 5);

	RestoreSavedPatchDebugState();		
}

void CPatch::RedirectCode(DWORD dwAddress, void* to)
{
	RedirectCode(dwAddress, (DWORD)to);
}

void CPatch::RedirectCodeOneByte(DWORD dwAddress, DWORD to)
{
	BYTE movement = (BYTE)((DWORD)to - dwAddress - 2);

	SaveAndDisablePatchDebugState();

	PatchBYTE(dwAddress, 0xEB);		// jmp

	PatchBYTE(dwAddress + 1, movement);
		
	RestoreSavedPatchDebugState();		
}

void CPatch::RedirectCodeOneByte(DWORD dwAddress, void* to)
{
	RedirectCodeOneByte(dwAddress, (DWORD)to);
}

// Performs task for memory address
void CPatch::DoTasksForMemoryAddress(DWORD dwAddress, int iSize)
{
	CPatch::CheckIfNotForbiddenAddress(dwAddress, iSize);

	CPatch::m_numberOfMemoryChanges++;
}

void CPatch::_PatchMemoryDataWithoutDebugCheck(DWORD dwAddress, const void* bData, int iSize)
{
	CPatch::DoTasksForMemoryAddress(dwAddress, iSize);

	DWORD dwProtect[2];
	VirtualProtect((PVOID)dwAddress, iSize, PAGE_EXECUTE_READWRITE, &dwProtect[0]);
	memcpy((PVOID)dwAddress, bData, iSize);
	VirtualProtect((PVOID)dwAddress, iSize, dwProtect[0], &dwProtect[1]);
}

// Checks if address is not forbidden
void CPatch::CheckIfNotForbiddenAddress(DWORD dwAddress, DWORD size)
{
	// return;

	for(int i = 0; i < CPatch::m_numOfForbiddenMemoryRegions; i++)
	{
		tForbiddenMemoryRegion* pRegion = &CPatch::m_forbiddenMemoryRegionArray[i];
	}

	// printf_MessageBox("address: 0x%X", dwAddress);
}
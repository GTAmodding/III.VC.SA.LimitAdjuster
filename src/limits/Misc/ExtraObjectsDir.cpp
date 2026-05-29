#include "LimitAdjuster.h"

using namespace injector;


struct CDirectory
{
    struct DirectoryInfo {
        uint32_t offset;
        uint32_t size;
        char name[24];
    };
    DirectoryInfo *entries;
    int32_t maxFiles;
    int32_t numFiles;

    void AddItem(DirectoryInfo *dirinfo, int imgid);
    int FindItem(char *name);
};
int
CDirectory::FindItem(char *name)
{
    int i;
    for (i = 0; i < numFiles; i++)
        if (strcmpi(entries[i].name, name) == 0)
            return i;
    return -1;
}
void
CDirectory::AddItem(DirectoryInfo *dirinfo, int imgid)
{
    int i;
    DirectoryInfo dirinfo2 = *dirinfo;
    dirinfo2.offset |= imgid << 24;
    i = FindItem(dirinfo->name);
    if (i < 0)
        entries[numFiles++] = dirinfo2;
}

void __declspec(naked) dirEntryHookVC()
{
    _asm
    {
        mov     edx, [esp + 40h + 0x8]	// fileID
        lea	eax, [esp + 40h - 0x34]	// dir entry
        mov	ecx, ds:0xA10730	// CStreaming::ms_pExtraObjectsDir
        push	edx
        push	eax
        call	CDirectory::AddItem

        push	dword ptr 0x40FCA2
        retn
    }
}

void __declspec(naked) dirEntryHookIII()
{
    _asm
    {
        mov     edx, [esp + 48h + 0x8]	// fileID
        lea	eax, [esp + 48h - 0x3C]	// dir entry
        mov	ecx, ds:0x95CB90	// CStreaming::ms_pExtraObjectsDir
        push	edx
        push	eax
        call	CDirectory::AddItem

        push	dword ptr 0x406F30
        retn
    }
}

class ExtraObjectsDirIII : public SimpleAdjuster
{
public:
    const char* GetLimitName() { return GetGVM().IsIII() ? "ExtraObjectsDir" : nullptr; }
    void ChangeLimit(int, const std::string& value)
    {
        int n = std::stoi(value);
        WriteMemory(0x4069D2 + 1, n, true);

        //for skin and bones mod
        injector::MakeJMP(0x406F20, dirEntryHookIII, true);

        // ignore txd.img
        injector::MakeJMP(0x48C12E, 0x48C14C, true);
    }
} ExtraObjectsDirIII;

class ExtraObjectsDirVC : public SimpleAdjuster
{
public:
    const char* GetLimitName() { return GetGVM().IsVC() ? "ExtraObjectsDir" : nullptr; }
    void ChangeLimit(int, const std::string& value)
    {
        int n = std::stoi(value);
        WriteMemory(0x4106E9 + 1, n, true);

        injector::MakeJMP(0x40FC92, dirEntryHookVC, true);
    }
} ExtraObjectsDirVC;

class ExtraObjectsDirSA : public SimpleAdjuster
{
public:
    const char* GetLimitName() { return GetGVM().IsSA() ? "ExtraObjectsDir" : nullptr; }
    void ChangeLimit(int, const std::string& value)
    {
        int n = std::stoi(value);
        WriteMemory<uint32_t>(0x5B8DDF + 1, n, true);
    }
} ExtraObjectsDirSA;

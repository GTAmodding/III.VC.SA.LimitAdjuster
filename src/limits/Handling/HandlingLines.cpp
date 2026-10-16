/*
 *  Handling Lines Adjuster (San Andreas 1.0 US)
 *  Licensed under the MIT License (http://opensource.org/licenses/MIT)
 *
 *  The game keeps the standard lines of handling.cfg in a static array of 210 entries
 *  (cHandlingDataMgr::m_aVehicleHandling) and finds a line by its name in a static table of the
 *  210 names of the original vehicles (VehicleNames, char[210][14]). A line with any other name
 *  is not loaded, and a vehicle that names it gets the out of range id 210.
 *
 *  This adjuster moves the array to a bigger one and lets handling.cfg define new names: a
 *  standard line whose name is not in the table gets the next free id (210, 211, ...), which the
 *  vehicles.ide line of a new vehicle can then use (handling.cfg is read before the IDE files).
 *  The game stores the handling id of a vehicle model in a byte, so there can be at most 255.
 *
 *  [SALIMITS]
 *  HandlingLines = 255         ; number of standard lines (210 in the game, at most 255)
 *
 *  The references to the array were found by disassembling gta_sa.exe 1.0 US: 15 instructions
 *  with the absolute address (one of them in the check of the mod garages, which reads the model
 *  flags of the car), and four methods that address it through 'this' (the line loader,
 *  GetVehiclePointer, HasFrontWheelDrive and HasRearWheelDrive), which are replaced. The bike
 *  array behind it is reached with a base inside the old array (0xC3491C + id * 64) and is left
 *  alone. The adjuster may be loaded before the game has constructed cHandlingDataMgr, so the
 *  old array is copied when the game calls LoadHandlingData (from 0x5BFA9A).
 */
#include "LimitAdjuster.h"
#include <cstring>
#include <cstdint>
#include <string>
#include <vector>

using namespace injector;

namespace
{
    const uintptr_t kOldArray   = 0xC2B9DC;     // gHandlingDataMgr.m_aVehicleHandling
    const size_t    kEntrySize  = 0xE0;         // sizeof(tHandlingData)
    const size_t    kOldCount   = 210;
    const size_t    kMaxCount   = 255;          // CVehicleModelInfo::m_nHandlingId is a byte
    const char*     kNames      = (const char*)0x8D3978;    // char VehicleNames[210][14]
    const size_t    kNameSize   = 14;

    uint8_t*                 g_array    = nullptr;   // the new array, kOldCount..capacity-1 are new, capacity is a spare entry
    size_t                   g_capacity = kOldCount;
    std::vector<std::string> g_names;               // names of the new ids, from 210 on

    // instructions holding an absolute address in the array (address of the 4 bytes)
    const uintptr_t kRefs[] = {
        0x447742, 0x478518, 0x478565, 0x47AEB6, 0x4C7C27, 0x4C89CE, 0x4C8E7F, 0x6A2FD4,
        0x6B0BA4, 0x6B2D28, 0x6B2D4A, 0x6BF526, 0x6F0D58, 0x6F2A2D, 0x6F6097,
    };

    // first word of a handling.cfg line
    std::string FirstWord(const char* line)
    {
        const char* p = line;
        while (*p == ' ' || *p == '\t') ++p;
        const char* e = p;
        while (*e && *e != ' ' && *e != '\t' && *e != '\r' && *e != '\n') ++e;
        return std::string(p, e - p);
    }

    int FindNewName(const std::string& name)
    {
        for (size_t i = 0; i < g_names.size(); ++i)
            if (g_names[i] == name)
                return int(kOldCount + i);
        return -1;
    }

    // id for a standard line whose name the game does not know; the spare entry when full
    int __cdecl FindOrAddName(const char* line)
    {
        std::string name = FirstWord(line);
        int id = FindNewName(name);
        if (id >= 0)
            return id;
        if (kOldCount + g_names.size() < g_capacity)
        {
            g_names.push_back(name);
            return int(kOldCount + g_names.size() - 1);
        }
        static bool told = false;
        if (!told)
        {
            told = true;
            char buf[256];
            sprintf(buf, "handling.cfg has more standard lines than HandlingLines (%u).\nThe line of '%s' and further new ones are ignored.",
                    unsigned(g_capacity), name.c_str());
            MessageBoxA(0, buf, "Limit Adjuster", MB_ICONWARNING);
        }
        return int(g_capacity);
    }

    // cHandlingDataMgr::GetHandlingId, thiscall (this, name)
    int __fastcall GetHandlingId(void*, int, const char* name)
    {
        for (size_t i = 0; i < kOldCount; ++i)          // as the game: the name starts with the table entry
            if (strncmp(name, kNames + i * kNameSize, kNameSize) == 0)
                return int(i);
        int id = FindNewName(name);
        return id >= 0 ? id : 0;                        // the game would return 210, outside of its array
    }

    // cHandlingDataMgr::GetVehiclePointer, HasFrontWheelDrive, HasRearWheelDrive, thiscall (this, id)
    void* __fastcall GetVehiclePointer(void*, int, uint8_t id)
    {
        return g_array + id * kEntrySize;
    }

    bool __fastcall HasFrontWheelDrive(void*, int, uint8_t id)
    {
        return g_array[id * kEntrySize + 0x74] != 'R';  // tHandlingData::m_transmissionData.m_nDriveType
    }

    bool __fastcall HasRearWheelDrive(void*, int, uint8_t id)
    {
        return g_array[id * kEntrySize + 0x74] != 'F';
    }

    // the new array as a copy of the old one, made when the game loads handling.cfg
    void* g_loadHandlingData = (void*)0x5BD830;

    void __fastcall LoadHandlingData(void* mgr, int)
    {
        memcpy(g_array, (void*)kOldArray, kOldCount * kEntrySize);
        for (size_t i = kOldCount; i <= g_capacity; ++i)                   // new ones start as a copy of the first
            memcpy(g_array + i * kEntrySize, (void*)kOldArray, kEntrySize);
        ((void(__thiscall*)(void*))g_loadHandlingData)(mgr);
    }

    // cHandlingDataMgr::LoadHandlingData, standard line (0x5BE1D3): eax = line, edi = this.
    // Finds the id as the game does, else a new one, then continues at 0x5BE1F6 with
    // ebp = handling data and m_nVehicleId set, as the original code.
    void* g_findExactWord = (void*)0x6F4F30;
    void* g_lineDone      = (void*)0x5BE430;

    void __declspec(naked) LoadStandardLine()
    {
        __asm
        {
            push    eax                     // the line, for FindOrAddName
            push    0xD2
            push    0xE
            push    0x8D3978
            push    eax
            mov     ecx, edi
            call    g_findExactWord         // returns 210 when not found
            pop     ecx
            cmp     eax, 0xD2
            jb      known
            push    ecx
            call    FindOrAddName
            add     esp, 4
        known:
            mov     edx, eax
            imul    edx, edx, 0xE0
            add     edx, g_array
            mov     ebp, edx
            mov     dword ptr [ebp], eax    // m_nVehicleId
            jmp     g_lineDone
        }
    }
}

class HandlingLinesSA : public SimpleAdjuster
{
public:
    // the addresses are those of 1.0 US
    const char* GetLimitName()
    {
        auto& gvm = GetGVM();
        return gvm.IsSA() && gvm.IsUS() && gvm.GetMajorVersion() == 1 && gvm.GetMinorVersion() == 0 ? "HandlingLines" : nullptr;
    }

    void ChangeLimit(int, const std::string& value)
    {
        size_t n = IsUnlimited(value) ? kMaxCount : std::stoul(value);
        if (n < kOldCount) n = kOldCount;
        if (n > kMaxCount) n = kMaxCount;
        if (g_array || n == kOldCount) return;

        g_capacity = n;
        g_array = new uint8_t[(n + 1) * kEntrySize]();

        for (uintptr_t field : kRefs)
        {
            uintptr_t old = ReadMemory<uintptr_t>(field, true);
            WriteMemory<uintptr_t>(field, uintptr_t(g_array) + (old - kOldArray), true);
        }
        MakeJMP(0x446FC0, GetVehiclePointer);
        MakeJMP(0x6A0480, HasFrontWheelDrive);
        MakeJMP(0x6A04B0, HasRearWheelDrive);
        MakeJMP(0x6F4FD0, GetHandlingId);
        MakeJMP(0x5BE1D3, LoadStandardLine);
        MakeCALL(0x5BFA9A, LoadHandlingData);
    }

    bool GetUsage(int, std::string& output)
    {
        if (!g_array) return false;
        return Adjuster::GetUsage(output, kOldCount + g_names.size(), g_capacity);
    }
} HandlingLinesSA;

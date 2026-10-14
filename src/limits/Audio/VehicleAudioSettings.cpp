/*
 *  Vehicle Audio Settings Loader (San Andreas 1.0 US)
 *  Licensed under the MIT License (http://opensource.org/licenses/MIT)
 *
 *  The engine sound, horn, doors and radio of a vehicle come from a static table in the game
 *  (0x860AF0) with one entry for each of the vehicle models 400 to 611. A vehicle added with a
 *  new model id reads past its end.
 *
 *  This adjuster replaces the table with one for the model ids 400 to 19999 and fills it from a
 *  text file in the format of the vehicle audio loader of the fastman92 limit adjuster
 *  (data\gtasa_vehicleAudioSettings.cfg): one line per vehicle, its IDE model name and the 14
 *  values of the entry. Models without a line keep the entry of the game, new ones get the
 *  entry of the first vehicle (Landstalker). The file is read when the first vehicle is created,
 *  after the IDE files, so that the names can be turned into model ids.
 *
 *  [SALIMITS]
 *  VehicleAudioSettings = data\gtasa_vehicleAudioSettings.cfg    ; or 0 to leave the game's table
 *
 *  The table is read in two places in gta_sa.exe 1.0 US: CAEVehicleAudioEntity::
 *  GetVehicleAudioSettings (0x4F5C10, replaced) and a copy of it inlined in
 *  CAEVehicleAudioEntity::Initialise (0x4F77C1, redirected).
 */
#include "LimitAdjuster.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

using namespace injector;

namespace
{
#pragma pack(push, 1)
    struct tVehicleAudioSettings            // 0x24 bytes
    {
        int16_t VehicleAudioType;           // 0x00
        int16_t PlayerBank;                 // 0x02
        int16_t DummyBank;                  // 0x04
        int8_t  BassSetting;                // 0x06
        int8_t  _pad7;
        float   BassFactor;                 // 0x08
        float   EnginePitch;                // 0x0C
        int8_t  HornType;                   // 0x10
        int8_t  _pad11[3];
        float   HornPitch;                  // 0x14
        int8_t  DoorType;                   // 0x18
        int8_t  EngineUpgrade;              // 0x19
        int8_t  RadioStation;               // 0x1A
        int8_t  RadioType;                  // 0x1B
        int8_t  VehicleAudioTypeForName;    // 0x1C
        int8_t  _pad1D[3];
        float   EngineVolumeOffset;         // 0x20
    };
#pragma pack(pop)
    static_assert(sizeof(tVehicleAudioSettings) == 0x24, "tVehicleAudioSettings");

    const uintptr_t kOldTable = 0x860AF0;
    const int       kFirstId  = 400;
    const int       kOldCount = 212;            // 400..611
    const int       kNewCount = 20000 - 400;    // 400..19999

    tVehicleAudioSettings* g_table  = nullptr;
    std::string            g_path;
    bool                   g_loaded = false;

    // CModelInfo::GetModelInfo(const char* name, int* index)
    void* (*GetModelInfo)(const char*, int*) = (void* (*)(const char*, int*))0x4C5940;

    void Load()
    {
        g_loaded = true;
        FILE* f = fopen(g_path.c_str(), "r");
        if (!f)
        {
            char buf[512];
            sprintf(buf, "Could not open the vehicle audio settings file\n%s", g_path.c_str());
            MessageBoxA(0, buf, "Limit Adjuster", MB_ICONWARNING);
            return;
        }
        char line[512];
        int lineNo = 0, unknown = 0;
        std::string unknownNames;
        while (fgets(line, sizeof(line), f))
        {
            ++lineNo;
            char* p = line;
            while (*p == ' ' || *p == '\t') ++p;
            if (*p == ';' || *p == '#' || *p == '\r' || *p == '\n' || *p == 0)
                continue;
            char name[64];
            int type, player, dummy, bass, horn, door, upgrade, radio, radioType, forName;
            float bassFactor, pitch, hornPitch, volume;
            int n = sscanf(p, "%63s %d %d %d %d %f %f %d %f %d %d %d %d %d %f", name, &type, &player, &dummy, &bass,
                           &bassFactor, &pitch, &horn, &hornPitch, &door, &upgrade, &radio, &radioType, &forName, &volume);
            if (n != 15)
                continue;
            int id = -1;
            if (!GetModelInfo(name, &id) || id < kFirstId || id >= kFirstId + kNewCount)
            {
                if (++unknown <= 8) unknownNames.append(" ").append(name);
                continue;
            }
            tVehicleAudioSettings& s = g_table[id - kFirstId];
            s.VehicleAudioType = int16_t(type);
            s.PlayerBank = int16_t(player);
            s.DummyBank = int16_t(dummy);
            s.BassSetting = int8_t(bass);
            s.BassFactor = bassFactor;
            s.EnginePitch = pitch;
            s.HornType = int8_t(horn);
            s.HornPitch = hornPitch;
            s.DoorType = int8_t(door);
            s.EngineUpgrade = int8_t(upgrade);
            s.RadioStation = int8_t(radio);
            s.RadioType = int8_t(radioType);
            s.VehicleAudioTypeForName = int8_t(forName);
            s.EngineVolumeOffset = volume;
        }
        fclose(f);
        (void)unknownNames;     // vehicles of the file that this game does not have are skipped quietly
    }

    const tVehicleAudioSettings* Entry(int modelIndex)
    {
        if (!g_loaded)
            Load();
        int i = modelIndex - kFirstId;
        return (i >= 0 && i < kNewCount) ? &g_table[i] : &g_table[0];
    }

    // static tVehicleAudioSettings CAEVehicleAudioEntity::GetVehicleAudioSettings(int16 modelIndex), returned through a pointer
    tVehicleAudioSettings* __stdcall GetVehicleAudioSettings(tVehicleAudioSettings* out, int modelIndex)
    {
        *out = *Entry(int16_t(modelIndex));
        return out;
    }

    // CAEVehicleAudioEntity::Initialise at 0x4F77C1: eax = model index - 400, ebp = this.
    // Replaces 'lea esi, [eax+eax*8]; lea edi, [ebp+80h]; lea esi, [esi*4+860AF0h]' (16 bytes)
    // before the copy (rep movsd of 9 dwords from esi to edi); keeps ecx and edx.
    const void* __cdecl EntryForInitialise(int index)
    {
        return Entry(index + kFirstId);
    }

    void __declspec(naked) InitialiseCopy()
    {
        __asm
        {
            push    ecx
            push    edx
            push    eax
            call    EntryForInitialise
            add     esp, 4
            mov     esi, eax
            pop     edx
            pop     ecx
            lea     edi, [ebp + 0x80]
            ret
        }
    }
}

class VehicleAudioSettingsSA : public SimpleAdjuster
{
public:
    // the addresses are those of 1.0 US
    const char* GetLimitName()
    {
        auto& gvm = GetGVM();
        return gvm.IsSA() && gvm.IsUS() && gvm.GetMajorVersion() == 1 && gvm.GetMinorVersion() == 0 ? "VehicleAudioSettings" : nullptr;
    }

    void ChangeLimit(int, const std::string& value)
    {
        if (value.empty() || value == "0" || g_table)
            return;
        g_path = value;
        g_table = new tVehicleAudioSettings[kNewCount];
        const tVehicleAudioSettings* old = (const tVehicleAudioSettings*)kOldTable;   // constant data of the exe
        for (int i = 0; i < kNewCount; ++i)
            g_table[i] = old[i < kOldCount ? i : 0];

        MakeJMP(0x4F5C10, GetVehicleAudioSettings);
        MakeCALL(0x4F77C1, InitialiseCopy);
        MakeNOP(0x4F77C1 + 5, 16 - 5);
    }
} VehicleAudioSettingsSA;

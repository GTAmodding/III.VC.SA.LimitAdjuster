#include "Core.h"
#include "Adjusters.h"
#include <fstream>

// SA: the engine sound, horn, doors and radio of a vehicle come from a table with one entry for each of the models 400
// to 611 (0x860AF0). A vehicle added with a new model id reads past its end. The table moves to one that covers every
// model id; new vehicles start with the first vehicle's entry, and data\gtasa_vehicleAudioSettings.cfg (one line per
// vehicle: its IDE name and the 14 values of the entry) can set any of them. The file is read when the first vehicle
// sound is set up, after the IDE files, so its names resolve to model ids.

namespace
{
#pragma pack(push, 1)
    struct AudioSettings
    {
        int16_t audioType;
        int16_t playerBank;
        int16_t dummyBank;
        int8_t bassSetting;
        int8_t pad7;
        float bassFactor;
        float enginePitch;
        int8_t hornType;
        int8_t pad11[3];
        float hornPitch;
        int8_t doorType;
        int8_t engineUpgrade;
        int8_t radioStation;
        int8_t radioType;
        int8_t audioTypeForName;
        int8_t pad1D[3];
        float engineVolumeOffset;
    };
#pragma pack(pop)
    static_assert(sizeof(AudioSettings) == 0x24);

    constexpr uintptr_t VanillaTable = 0x860AF0;
    constexpr int FirstId = 400;
    constexpr int VanillaCount = 212;
    constexpr int Count = 20000 - FirstId; // every model id the game's indices can hold

    std::vector<AudioSettings> table;
    std::once_flag loaded;
    std::vector<SafetyHookMid> hooks;

    void LoadFile()
    {
        std::ifstream file("data\\gtasa_vehicleAudioSettings.cfg");
        if (!file)
            return;

        auto getModelInfo = (void* (__cdecl*)(const char*, int*))0x4C5940;
        int applied = 0;
        std::string line;
        while (std::getline(file, line))
        {
            auto start = line.find_first_not_of(" \t");
            if (start == std::string::npos || line[start] == ';' || line[start] == '#')
                continue;

            char name[64]{};
            int type, player, dummy, bass, horn, door, upgrade, radio, radioType, forName;
            float bassFactor, pitch, hornPitch, volume;
            if (sscanf_s(line.c_str() + start, "%63s %d %d %d %d %f %f %d %f %d %d %d %d %d %f", name, (unsigned)sizeof(name), &type, &player,
                &dummy, &bass, &bassFactor, &pitch, &horn, &hornPitch, &door, &upgrade, &radio, &radioType, &forName, &volume) != 15)
                continue;

            int id = -1;
            if (!getModelInfo(name, &id) || id < FirstId || id >= FirstId + Count)
                continue;

            auto& s = table[id - FirstId];
            s.audioType = (int16_t)type;
            s.playerBank = (int16_t)player;
            s.dummyBank = (int16_t)dummy;
            s.bassSetting = (int8_t)bass;
            s.bassFactor = bassFactor;
            s.enginePitch = pitch;
            s.hornType = (int8_t)horn;
            s.hornPitch = hornPitch;
            s.doorType = (int8_t)door;
            s.engineUpgrade = (int8_t)upgrade;
            s.radioStation = (int8_t)radio;
            s.radioType = (int8_t)radioType;
            s.audioTypeForName = (int8_t)forName;
            s.engineVolumeOffset = volume;
            applied++;
        }
        core::Log("VehicleAudioSettings: {} entries from data\\gtasa_vehicleAudioSettings.cfg", applied);
    }

    void EnsureLoaded(SafetyHookContext&)
    {
        std::call_once(loaded, LoadFile);
    }
}

namespace limits
{
    void InstallVehicleAudio()
    {
        if (!core::IsSA())
            return;
        auto setting = core::GetSetting("VehicleAudioSettings", "unlimited");
        if (!setting.apply)
            return;

        // CAEVehicleAudioEntity::GetVehicleAudioSettings and the copy inlined in CAEVehicleAudioEntity::Initialise
        const mem::Expect expected[] = { { 0x4F5C26, VanillaTable }, { 0x4F77CD, VanillaTable } };
        if (!mem::CheckValues(expected, "VehicleAudioSettings") || *(uint32_t*)0x4F5C10 != 0x2444BF0F || *(uint32_t*)0x4F77C1 != 0x8DC0348D)
            return;

        table.resize(Count);
        auto vanilla = (const AudioSettings*)VanillaTable;
        for (int i = 0; i < Count; i++)
            table[i] = vanilla[i < VanillaCount ? i : 0];

        hooks.push_back(safetyhook::create_mid(0x4F5C10, EnsureLoaded));
        hooks.push_back(safetyhook::create_mid(0x4F77C1, EnsureLoaded));
        mem::WriteDword(0x4F5C26, (uint32_t)(uintptr_t)table.data());
        mem::WriteDword(0x4F77CD, (uint32_t)(uintptr_t)table.data());
        core::RegisterLimit({ "VehicleAudioSettings", [] { return -1; }, [] { return Count + FirstId; }, [] { return Count + FirstId; } });
    }
}

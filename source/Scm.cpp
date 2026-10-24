#include "Core.h"
#include "Adjusters.h"

namespace
{
#include "ScmRefs.h"

    constexpr uint32_t NativeBase{ 0xA49960 };
    constexpr uint32_t NativeMain{ 200000 };
    constexpr uint32_t NativeMission{ 69000 };
    constexpr uint32_t MaxScriptBytes{ 0x7FFFFFFC };
    core::Setting mainSetting, missionSetting;
    uint8_t* scriptSpace{ (uint8_t*)NativeBase };
    uint32_t mainCapacity{ NativeMain }, missionCapacity{ NativeMission };
    uint32_t mainUsed{}, missionUsed{}, saveMissionOffset{ NativeMain };
    std::vector<mem::Expect> scriptRefs;

    [[noreturn]] void ScriptError(const char* message)
    {
        core::Log("SCM: {}", message);
        MessageBoxA(nullptr, message, "Open Limit Adjuster - SCM", MB_OK | MB_ICONERROR);
        ExitProcess(ERROR_BAD_FORMAT);
    }

    uint32_t Align(uint32_t size)
    {
        if (size > MaxScriptBytes)
            ScriptError("The script size exceeds the game's signed 32-bit offset range.");
        return (size + 3) & ~3u;
    }

    template<typename T> T ReadAt(int file, uint32_t offset, uint32_t length)
    {
        T value{};
        if (offset > length || sizeof(T) > length - offset)
            ScriptError("The SCM header extends beyond the script file.");
        if (injector::cstd<bool(int, int, int)>::call<0x538990>(file, offset, SEEK_SET))
            ScriptError("Cannot seek within the script file.");
        if (injector::cstd<int(int, void*, int)>::call<0x538950>(file, &value, sizeof(value)) != sizeof(value))
            ScriptError("Cannot read the SCM header.");
        return value;
    }

    uint32_t NextHeader(int file, uint32_t offset, uint32_t length)
    {
        if (ReadAt<uint16_t>(file, offset, length) != 2 || ReadAt<uint8_t>(file, offset + 2, length) != 1)
            ScriptError("Unsupported SCM header: expected a jump with a 32-bit offset.");
        auto next = ReadAt<uint32_t>(file, offset + 3, length);
        if (next <= offset || next >= length)
            ScriptError("Invalid SCM header offset.");
        return next;
    }

    // Streamed scripts have their own native allocations. Include their sector sizes in the mission allowance too.
    uint32_t LargestImgScript()
    {
        auto file = injector::cstd<int(const char*, const char*)>::call<0x538900>("script.img", "rb");
        if (!file)
            return 0;
        auto length = injector::cstd<int(int)>::call<0x5389E0>(file);
        uint32_t largest{};
        if (length >= 8 && ReadAt<uint32_t>(file, 0, length) == 0x32524556)
        {
            auto count = ReadAt<uint32_t>(file, 4, length);
            if (count <= ((uint32_t)length - 8) / 32)
            {
                struct Entry
                {
                    uint32_t offset;
                    uint16_t streamingSize, archiveSize;
                    char name[24];
                };
                for (uint32_t i{}; i < count; i++)
                {
                    auto entry = ReadAt<Entry>(file, 8 + i * 32, length);
                    auto end = (const char*)memchr(entry.name, 0, sizeof(entry.name));
                    if (!end || end - entry.name < 4 || _strnicmp(end - 4, ".scm", 4))
                        continue;
                    auto sectors = entry.streamingSize ? entry.streamingSize : entry.archiveSize;
                    auto bytes = (uint32_t)sectors * 2048;
                    if ((uint64_t)entry.offset * 2048 + bytes <= (uint32_t)length)
                        largest = std::max(largest, bytes);
                }
            }
        }
        injector::cstd<int(int)>::call<0x5389D0>(file);
        return largest;
    }

    uint32_t ChooseSize(const core::Setting& setting, uint32_t native, uint32_t minimum, uint32_t required)
    {
        auto size = !setting.apply ? native : setting.unlimited ? std::max(minimum, required) : setting.value;
        size = Align(size);
        if (size < required)
            ScriptError("The SCM exceeds the configured MAINsegmentSize or MissionSize. Use unlimited or a larger size.");
        return size;
    }

    void ConfigureSpace(uint32_t mainSize, uint32_t missionSize)
    {
        if ((uint64_t)mainSize + missionSize > MaxScriptBytes)
            ScriptError("The combined MAIN and mission size exceeds the game's offset range.");
        if (mainSize == mainCapacity && missionSize == missionCapacity)
            return;
        auto data = (uint8_t*)VirtualAlloc(nullptr, (size_t)mainSize + missionSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (!data)
            ScriptError("Cannot allocate SCM script space.");
        // Init has already stopped and reset all script threads before reading the selected SCM.
        for (auto ref : scriptRefs)
        {
            auto offset = ref.value == NativeBase + NativeMain ? mainSize : ref.value - NativeBase;
            mem::WriteDword(ref.operand, (uint32_t)data + offset);
        }
        mem::WriteDword(core::SA(0x468D59, 0x401CF9), (mainSize + missionSize) / 4);
        for (auto operand : { 0x468E75, 0x468ECF })
            mem::WriteDword(operand, mainSize);
        for (auto operand : { 0x4899D2, 0x489A5B })
            mem::WriteDword(operand, missionSize);
        if (scriptSpace != (uint8_t*)NativeBase)
            VirtualFree(scriptSpace, 0, MEM_RELEASE);
        scriptSpace = data;
        mainCapacity = mainSize;
        missionCapacity = missionSize;
        core::Log("SCM: MAIN {} bytes, mission {} bytes at {:08X}", mainCapacity, missionCapacity, (uintptr_t)scriptSpace);
    }

    int __cdecl ReadMain(int file, void*, int)
    {
        auto fileSize = injector::cstd<int(int)>::call<0x5389E0>(file);
        if (fileSize <= 0)
            ScriptError("Cannot read the selected main.scm.");
        auto globalsEnd = NextHeader(file, 0, fileSize);
        auto objectsEnd = NextHeader(file, globalsEnd, fileSize);
        auto missionsEnd = NextHeader(file, objectsEnd, fileSize);
        mainUsed = ReadAt<uint32_t>(file, objectsEnd + 8, fileSize);
        missionUsed = ReadAt<uint32_t>(file, objectsEnd + 12, fileSize);
        if (mainUsed < missionsEnd || mainUsed > (uint32_t)fileSize || missionUsed > (uint32_t)fileSize - mainUsed)
            ScriptError("Invalid MAIN or largest-mission size in the SCM header.");
        auto imgSize = LargestImgScript();
        auto mainSize = ChooseSize(mainSetting, NativeMain, 1024 * 1024, mainUsed);
        auto missionSize = ChooseSize(missionSetting, NativeMission, 256 * 1024, missionUsed);
        if (missionSetting.apply && missionSetting.unlimited)
            missionSize = std::max(missionSize, imgSize);
        saveMissionOffset = Align(std::max(NativeMain, mainUsed));
        ConfigureSpace(mainSize, missionSize);
        core::Log("SCM header: MAIN {}, mission {}, IMG largest {}", mainUsed, missionUsed, imgSize);
        injector::cstd<int(int, int, int)>::call<0x538990>(file, 0, SEEK_SET);
        // Read only MAIN, so a large allowance never copies missions over unused MAIN storage.
        return injector::cstd<int(int, void*, int)>::call<0x538950>(file, scriptSpace, mainUsed);
    }

    uint32_t EncodeOffset(uint32_t offset)
    {
        if (offset >= mainCapacity && offset - mainCapacity < missionCapacity)
            return saveMissionOffset + offset - mainCapacity;
        if (offset >= mainUsed)
            ScriptError("Cannot save an instruction pointer outside the loaded SCM.");
        return offset;
    }

    uint32_t DecodeOffset(uint32_t offset)
    {
        if (offset >= saveMissionOffset && offset - saveMissionOffset < missionCapacity)
            return mainCapacity + offset - saveMissionOffset;
        if (offset >= mainUsed)
            ScriptError("The save contains an instruction pointer outside the loaded SCM.");
        return offset;
    }

    int __cdecl SaveOffset(uint32_t* source, int size)
    {
        auto offset = EncodeOffset(*source);
        return injector::cstd<int(void*, int)>::call<0x5D1270>(&offset, size);
    }

    int __cdecl LoadOffset(uint32_t* target, int size)
    {
        auto result = injector::cstd<int(void*, int)>::call<0x5D1300>(target, size);
        *target = DecodeOffset(*target);
        return result;
    }

    int __cdecl SaveThread(void* source, int size)
    {
        std::array<uint8_t, 224> copy;
        memcpy(copy.data(), source, copy.size());
        for (auto offset : { 0x10, 0x14, 0x18, 0x1C, 0x20, 0x24, 0x28, 0x2C, 0x30, 0x34 })
        {
            auto& pointer = *(uint32_t*)(copy.data() + offset);
            if (pointer)
                pointer = NativeBase + EncodeOffset(pointer - (uint32_t)scriptSpace);
        }
        return injector::cstd<int(void*, int)>::call<0x5D1270>(copy.data(), size);
    }

    int __cdecl LoadThread(void* target, int size)
    {
        auto result = injector::cstd<int(void*, int)>::call<0x5D1300>(target, size);
        auto& base = *(uint32_t*)((uint8_t*)target + 0x10);
        if (base)
            base = (uint32_t)scriptSpace + DecodeOffset(base - NativeBase);
        return result;
    }

    int __cdecl LoadGlobalsSize(uint32_t* target, int size)
    {
        auto result = injector::cstd<int(void*, int)>::call<0x5D1300>(target, size);
        if (*target > mainUsed || *target != *(uint32_t*)(scriptSpace + 3))
            ScriptError("The save's script globals do not match the selected SCM.");
        return result;
    }

    bool CheckCall(uintptr_t address, uintptr_t target)
    {
        if (*(uint8_t*)address == 0xE8 && address + 5 + *(int32_t*)(address + 1) == target)
            return true;
        core::Log("SCM: unsupported call at {:08X}", address);
        return false;
    }

    void InstallScriptSpace()
    {
        mainSetting = core::GetSetting("MAINsegmentSize", "unlimited");
        missionSetting = core::GetSetting("MissionSize", "unlimited");
        if (!mainSetting.apply && !missionSetting.apply)
            return;
        scriptRefs.assign(std::begin(ScriptShared), std::end(ScriptShared));
        std::span<const mem::Expect> refs = core::hoodlum ? std::span(ScriptHoodlum) : std::span(ScriptCompact);
        scriptRefs.insert(scriptRefs.end(), refs.begin(), refs.end());
        const mem::Expect sizes[] =
        {
            { core::SA(0x468D59, 0x401CF9), (NativeMain + NativeMission) / 4 },
            { 0x468E75, NativeMain }, { 0x468ECF, NativeMain },
            { 0x4899D2, NativeMission }, { 0x489A5B, NativeMission },
        };
        if (!mem::CheckValues(scriptRefs, "SCM") || !mem::CheckValues(sizes, "SCM"))
            return;
        for (auto call : { 0x468E7F, 0x468EDB })
            if (!CheckCall(call, 0x538950))
                return;
        for (auto call : { 0x5D4F5E, 0x5D4F76, 0x5D4FA2 })
            if (!CheckCall(call, 0x5D1270))
                return;
        for (auto call : { 0x5D4FE3, 0x5D5335, 0x5D5346, 0x5D536A })
            if (!CheckCall(call, 0x5D1300))
                return;
        for (auto call : { 0x468E7F, 0x468EDB })
            injector::MakeCALL(call, ReadMain);
        injector::MakeCALL(0x5D4F5E, SaveThread);
        injector::MakeCALL(0x5D4F76, SaveOffset);
        injector::MakeCALL(0x5D4FA2, SaveOffset);
        injector::MakeCALL(0x5D4FE3, LoadGlobalsSize);
        injector::MakeCALL(0x5D5335, LoadThread);
        injector::MakeCALL(0x5D5346, LoadOffset);
        injector::MakeCALL(0x5D536A, LoadOffset);
        core::RegisterLimit({ "MAINsegmentSize", [] { return (int)mainUsed; }, [] { return (int)mainCapacity; },
            [] { return (int)(mainSetting.unlimited ? MaxScriptBytes - missionCapacity : mainCapacity); } });
        core::RegisterLimit({ "MissionSize", [] { return (int)missionUsed; }, [] { return (int)missionCapacity; },
            [] { return (int)(missionSetting.unlimited ? MaxScriptBytes - mainCapacity : missionCapacity); } });
    }

    constexpr uint32_t LightSize{ 0x7C }, NativeLights{ 8 }, MaxLights{ 65535 };
    mem::Reserve searchlights;
    uint32_t lightCapacity{ NativeLights }, lightMaximum{ NativeLights };
    SafetyHookInline addLightHook, lightIndexHook;

    bool GrowLights(uint32_t count)
    {
        if (!searchlights.Commit((size_t)count * LightSize))
            return false;
        for (auto i = lightCapacity; i < count; i++)
        {
            auto light = searchlights.data() + (size_t)i * LightSize;
            light[2] = 1;
            *(uint16_t*)(light + 4) = 1;
        }
        lightCapacity = count;
        mem::WriteDword(core::SA(0x493013, 0x15692D3), (count - 1) * LightSize);
        mem::WriteDword(0x469322, (uint32_t)searchlights.data() + count * LightSize + 16);
        mem::WriteDword(0x4939BD, (uint32_t)searchlights.data() + count * LightSize);
        mem::WriteDword(0x493A00, count);
        mem::WriteDword(0x493E42, count);
        return true;
    }

    int __cdecl AddLight(float x, float y, float z, void* entity, float tx, float ty, float tz, float radius, float baseRadius)
    {
        bool free{};
        for (uint32_t i{}; i < lightCapacity; i++)
        {
            if (!searchlights.data()[(size_t)i * LightSize])
            {
                free = true;
                break;
            }
        }
        if (!free)
        {
            if (lightCapacity >= lightMaximum || !GrowLights(std::min(lightMaximum, lightCapacity + std::max(lightCapacity / 2, 8u))))
                return -1;
            core::Log("ScriptSearchLights: grown to {}", lightCapacity);
        }
        return addLightHook.ccall<int>(x, y, z, entity, tx, ty, tz, radius, baseRadius);
    }

    int __cdecl LightIndex(uint32_t handle, uint8_t type)
    {
        if (type == 2 && (handle & 0xFFFF) >= lightCapacity)
            return -1;
        return lightIndexHook.ccall<int>(handle, type);
    }

    void InstallSearchlights()
    {
        auto setting = core::GetSetting("ScriptSearchLights", "unlimited");
        if (!setting.apply)
            return;
        std::vector<mem::Expect> refs(std::begin(SearchlightShared), std::end(SearchlightShared));
        std::span<const mem::Expect> specific = core::hoodlum ? std::span(SearchlightHoodlum) : std::span(SearchlightCompact);
        refs.insert(refs.end(), specific.begin(), specific.end());
        const mem::Expect bounds[] =
        {
            { core::SA(0x493013, 0x15692D3), NativeLights * LightSize },
            { 0x469322, 0xA95158 }, { 0x4939BD, 0xA95148 }, { 0x493A00, NativeLights }, { 0x493E42, NativeLights },
        };
        if (!mem::CheckValues(refs, "ScriptSearchLights") || !mem::CheckValues(bounds, "ScriptSearchLights"))
            return;
        if (*(uint8_t*)core::SA(0x493000, 0x15692C0) != 0xA0 ||
            mem::ReadDword(core::SA(0x493001, 0x15692C1)) != 0xA94D68 ||
            *(uint8_t*)0x4839A0 != 0x57 || mem::ReadDword(0x4839A1) != 0x08247C8B)
        {
            core::Log("ScriptSearchLights: unsupported allocator or handle lookup");
            return;
        }
        lightMaximum = setting.unlimited ? MaxLights : std::clamp(setting.value, 1u, MaxLights);
        auto initial = setting.unlimited ? NativeLights : lightMaximum;
        if (!searchlights.Create((size_t)lightMaximum * LightSize, (size_t)initial * LightSize))
            return;
        for (auto ref : refs)
            mem::WriteDword(ref.operand, (uint32_t)searchlights.data() + ref.value - 0xA94D68);
        lightCapacity = 0;
        GrowLights(initial);
        addLightHook = safetyhook::create_inline(core::SA(0x493000, 0x15692C0), AddLight);
        lightIndexHook = safetyhook::create_inline(0x4839A0, LightIndex);
        core::RegisterLimit({ "ScriptSearchLights", [] { return (int)*(uint16_t*)0xA90830; },
            [] { return (int)lightCapacity; }, [] { return (int)lightMaximum; } });
    }
}

void limits::InstallScm()
{
    if (!core::IsSA())
        return;
    InstallScriptSpace();
    InstallSearchlights();
}

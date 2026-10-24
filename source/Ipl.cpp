#include "Core.h"
#include "Adjusters.h"

// SA IPL loading: instances of the IPL file being read, and one index array per IPL file with instances. The game
// checks neither bound.

namespace
{
    const uintptr_t loadedBuildingsRefs[] = { 0x5B51F3, 0x5B5202, 0x5B524C, 0x5B525B, 0x5B531D, 0x5B5343, 0x5B893B, 0x5B8A51, 0x5B8A72 };
    const uintptr_t indexArraysRefs[] = { 0x40479A, 0x4047B7, 0x405C3D, 0x406033, 0x40619B };
    const uintptr_t indexArraysRefsHoodlum[] = { 0x1561160, 0x15649FA, 0x1569777, 0x405C3D, 0x40619B };

    limits::MovedArray* entities = nullptr;
    limits::MovedArray* arrays = nullptr;
    std::vector<SafetyHookMid> boundsHooks;

    [[noreturn]] void Reject(const char* key)
    {
        auto message = std::format("{}: IPL capacity exceeded. Check the map and configured limit.", key);
        core::Log("{}", message);
        MessageBoxA(nullptr, message.c_str(), "Open Limit Adjuster - IPL", MB_OK | MB_ICONERROR);
        ExitProcess(ERROR_BAD_FORMAT);
    }

    void EntityBound(SafetyHookContext& context)
    {
        if (context.ecx >= entities->capacity)
            Reject("EntitiesPerIpl");
    }

    void ArrayBound(SafetyHookContext& context)
    {
        if (context.ecx >= arrays->capacity)
            Reject("EntityIpl");
    }

    limits::MovedArray* Move(const char* key, uintptr_t base, uint32_t vanilla, uintptr_t counter, uint32_t ceiling, std::span<const uintptr_t> refs)
    {
        auto setting = core::GetSetting(key, "unlimited");
        if (!setting.apply)
            return nullptr;

        auto capacity = std::max(core::MaxCapacity(setting, ceiling), vanilla);
        auto moved = limits::MoveArray(key, base, 4, vanilla, capacity, refs, false);
        if (moved)
            core::RegisterLimit({ key, [counter] { return *(int*)counter; }, [moved] { return (int)moved->capacity; }, [moved] { return (int)moved->capacity; } });
        return moved;
    }
}

namespace limits
{
    void InstallIpl()
    {
        if (!core::IsSA())
            return;

        auto arrayWrite = core::SA(0x404797, 0x15649F7);
        if (*(uint16_t*)0x5B8938 != 0x0489 || *(uint8_t*)0x5B893A != 0x8D ||
            *(uint16_t*)arrayWrite != 0x0489 || *(uint8_t*)(arrayWrite + 2) != 0x8D)
            return;
        entities = Move("EntitiesPerIpl", 0xBCC0E0, 4096, 0xBCC0D8, 1000000, loadedBuildingsRefs);
        arrays = Move("EntityIpl", 0x8E3F08, 40, 0x8E3F00, 65536, core::hoodlum ? std::span<const uintptr_t>(indexArraysRefsHoodlum) : std::span<const uintptr_t>(indexArraysRefs));
        if (entities)
            boundsHooks.push_back(safetyhook::create_mid(0x5B8938, EntityBound));
        if (arrays)
            boundsHooks.push_back(safetyhook::create_mid(arrayWrite, ArrayBound));
        for (auto& hook : boundsHooks)
        {
            if (!hook)
                Reject("IPL bound check unavailable");
        }
    }
}

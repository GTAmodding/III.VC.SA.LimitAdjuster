#include "Core.h"
#include "Adjusters.h"
#include <cstdio>

// Model info stores (IDE definitions). A store is { int count; T objects[N]; } and the game writes past it when full.
// Each store moves into space for as many entries as there are model ids. New entries get a copy of an untouched
// constructed element of the original store, as the game only constructs the static arrays once.

namespace
{
    struct StoreDef
    {
        const char* key;
        uintptr_t base;
        uint32_t elemSize;
        uint32_t vanilla;
        uint32_t ceiling;
        uintptr_t addFn; // CModelInfo::Add*Model, 0 when entries need no construction
        std::span<const uintptr_t> refs;
    };

    struct StoreState
    {
        const StoreDef* def;
        limits::MovedArray* moved;
    };

    std::vector<StoreState> stores;
    std::vector<SafetyHookMid> hooks;
    StoreState* effectStore = nullptr;

    void CheckEffect(SafetyHookContext& context);

    [[noreturn]] void Reject(const StoreState& state, const char* reason)
    {
        auto message = std::format("{}: {}. Check the IDE data and configured limit.", state.def->key, reason);
        core::Log("{}", message);
        MessageBoxA(nullptr, message.c_str(), "Open Limit Adjuster - model definitions", MB_OK | MB_ICONERROR);
        ExitProcess(ERROR_BAD_FORMAT);
    }

    // Element `index` of a moved store gets the bytes of the original store's element 0.
    void Construct(const StoreState& state, uint32_t index)
    {
        auto def = state.def;
        if (!def->addFn || index >= state.moved->capacity)
            return;

        auto slot = state.moved->data + 4 + (size_t)index * def->elemSize;
        if (!state.moved->Commit(4 + ((size_t)index + 1) * def->elemSize))
            Reject(state, "not enough memory for another definition");
        if (*(uintptr_t*)slot == 0)
            memcpy(slot, (void*)(def->base + 4), def->elemSize);
    }

    void CheckEffect(SafetyHookContext& context)
    {
        auto& state = *effectStore;
        if (*(uint32_t*)state.moved->data >= state.moved->capacity)
            Reject(state, "definition capacity exceeded");
        auto line = *(const char**)(context.esp + 4);
        int id = -1;
        auto ids = core::IsIII() ? 5500 : 6500;
        if (sscanf(line, "%d", &id) != 1 || id < 0 || id >= ids)
            Reject(state, "invalid effect model ID");
        auto table = (uint8_t**)(core::IsIII() ? 0x83D408 : 0x92D4C8);
        auto model = table[id];
        if (!model || model[core::IsIII() ? 0x2B : 0x1A] == 255)
            Reject(state, "missing model or more than 255 effects on one model");
    }

    template <size_t I>
    void AddHook(SafetyHookContext& context)
    {
        auto& state = stores[I];
        auto id = *(int32_t*)(context.esp + 4);
        auto ids = core::IsIII() ? 5500 : core::IsVC() ? 6500 : 20000;
        if (id < 0 || id >= ids)
            Reject(state, "model ID is outside the game's model table");
        auto count = *(uint32_t*)state.moved->data;
        if (count >= state.moved->capacity)
            Reject(state, "definition capacity exceeded");
        Construct(state, count);
    }

    template <size_t... I>
    constexpr auto MakeAddHooks(std::index_sequence<I...>)
    {
        return std::array<void (*)(SafetyHookContext&), sizeof...(I)>{ &AddHook<I>... };
    }

    constexpr size_t MaxStores = 16;
    const auto addHooks = MakeAddHooks(std::make_index_sequence<MaxStores>());

    bool NativeAdd(uintptr_t address)
    {
        switch (address)
        {
            case 0x50B920: return mem::ReadDword(address) == 0x5BB43D81;
            case 0x50B9C0: return mem::ReadDword(address) == 0x076C3D83;
            case 0x50BA10: return mem::ReadDword(address) == 0xA1083D83;
            case 0x50BAD0: return mem::ReadDword(address) == 0x16003D83;
            case 0x50BA60: return mem::ReadDword(address) == 0x2DE03D83;
            case 0x55F730: return mem::ReadDword(address) == 0x9EAC3D81;
            case 0x55F6E0: return mem::ReadDword(address) == 0xA6B03D81;
            case 0x55F690: return mem::ReadDword(address) == 0x1F103D83;
            case 0x55F640: return mem::ReadDword(address) == 0x29883D83;
            case 0x55F5D0: return mem::ReadDword(address) == 0x2A883D83;
            case 0x55F580: return mem::ReadDword(address) == 0xCA703D81;
            case 0x4C6620: return mem::ReadDword(address) == 0xAAE950A1;
            case 0x4C6650: return mem::ReadDword(address) == 0xB1BF58A1;
            case 0x4C66B0: return mem::ReadDword(address) == 0xB1C960A1;
            case 0x4C6710: return mem::ReadDword(address) == 0xB1E158A1;
            case 0x4C6740: return mem::ReadDword(address) == 0xB1E958A1;
            case 0x4C6770: return mem::ReadDword(address) == 0xB1F650A1;
            case 0x4C67A0: return mem::ReadDword(address) == 0xB478F8A1;
            default: return false;
        }
    }

    void AddStores(std::span<const StoreDef> defs, uintptr_t initialise)
    {
        stores.reserve(MaxStores);
        for (auto& def : defs)
        {
            auto setting = core::GetSetting(def.key, "unlimited");
            if (!setting.apply || stores.size() >= MaxStores)
                continue;

            if (def.addFn)
            {
                if (!NativeAdd(def.addFn))
                {
                    core::Log("{}: add function changed, left as it is", def.key);
                    continue;
                }
            }
            auto effectLoader = core::IsIII() ? 0x4780E0u : 0x48BA80u;
            if (!def.addFn && !core::IsSA() && mem::ReadDword(effectLoader) != 0x55575653)
                continue;
            auto capacity = std::max(core::MaxCapacity(setting, def.ceiling), def.vanilla);
            auto moved = limits::MoveArray(def.key, def.base, 1, 4 + def.vanilla * def.elemSize, 4 + capacity * def.elemSize, def.refs, false);
            if (!moved)
                continue;
            moved->capacity = capacity;
            *(uint32_t*)moved->data = 0;

            auto index = stores.size();
            stores.push_back({ &def, moved });
            if (def.addFn)
                hooks.push_back(safetyhook::create_mid(def.addFn, addHooks[index]));
            else if (!core::IsSA())
            {
                effectStore = &stores.back();
                hooks.push_back(safetyhook::create_mid(effectLoader, CheckEffect));
            }
            if (!hooks.empty() && !hooks.back())
                Reject(stores.back(), "could not install definition bound check");

            core::RegisterLimit({ def.key, [moved] { return *(int*)moved->data; }, [moved] { return (int)moved->capacity; }, [moved] { return (int)moved->capacity; } });
        }

        // VC/SA CModelInfo::Initialise adds a few models with the add inlined: construct the first entries before it runs.
        if (!initialise)
            return;
        hooks.push_back(safetyhook::create_mid(initialise, [](SafetyHookContext&)
        {
            for (auto& state : stores)
            {
                for (uint32_t i = 0; i < 32; i++)
                    Construct(state, i);
            }
        }));
    }

    // GTA III 1.0
    const uintptr_t simpleIII[] = { 0x50B3DA, 0x50B5CD, 0x50B5D5, 0x50B5F2, 0x50B7C8, 0x50B922, 0x50B945, 0x50B94B, 0x50B954 };
    const uintptr_t timeIII[] = { 0x50B3F8, 0x50B63D, 0x50B645, 0x50B663, 0x50B7F0, 0x50B9C2, 0x50B9DF, 0x50B9E5, 0x50B9EE };
    const uintptr_t clumpIII[] = { 0x50B3E4, 0x50B679, 0x50B681, 0x50B698, 0x50B804, 0x50BA12, 0x50BA2D, 0x50BA35, 0x50BA3E };
    const uintptr_t pedIII[] = { 0x50B3BB, 0x50B6EC, 0x50B6F4, 0x50B70B, 0x50B7FA, 0x50BAD2, 0x50BAED, 0x50BAF5, 0x50BAFE };
    const uintptr_t vehicleIII[] = { 0x50B3D0, 0x50B6AE, 0x50B6B6, 0x50B6D6, 0x50B80E, 0x50BA62, 0x50BA7D, 0x50BA83, 0x50BA8E };
    const uintptr_t effectsIII[] = { 0x50B37C, 0x50B582, 0x50B791, 0x50B799, 0x50B7B6, 0x50B7BE, 0x50BB31 };
    const StoreDef storesIII[] =
    {
        { "AtomicModels",  0x885BB4, 0x4C,  5000, 5500,  0x50B920, simpleIII },
        { "TimeModels",    0x94076C, 0x58,  30,   5500,  0x50B9C0, timeIII },
        { "ClumpModels",   0x6FA108, 0x34,  5,    5500,  0x50BA10, clumpIII },
        { "PedModels",     0x941600, 0x48,  90,   5500,  0x50BAD0, pedIII },
        { "VehicleModels", 0x8E2DE0, 0x1F8, 120,  5500,  0x50BA60, vehicleIII },
        { "2dEffects",     0x9434F8, 0x34,  2000, 32767, 0,        effectsIII },
    };

    // GTA Vice City 1.0
    const uintptr_t simpleVC[] = { 0x55F732, 0x55F755, 0x55F75B, 0x55F764, 0x55F83A, 0x55F842, 0x55F858, 0x55F9BA, 0x55FB24, 0x55FB2E, 0x55FBB3,
                                   0x55FBB9, 0x55FBC2, 0x55FBFA, 0x55FC17, 0x55FC1D, 0x55FC26, 0x55FC5E, 0x55FC7B, 0x55FC81, 0x55FC8A, 0x55FCC2,
                                   0x55FCDF, 0x55FCE5, 0x55FCEE, 0x55FD26, 0x55FD43, 0x55FD49, 0x55FD52, 0x55FD8A, 0x55FDA7, 0x55FDAD, 0x55FDB6,
                                   0x55FDEE, 0x55FE0B, 0x55FE11, 0x55FE1A, 0x55FE52, 0x55FE6F, 0x55FE75, 0x55FE7E };
    const uintptr_t timeVC[] = { 0x55F6E2, 0x55F705, 0x55F70B, 0x55F714, 0x55F86E, 0x55F876, 0x55F893, 0x55F9CE, 0x55FB96 };
    const uintptr_t weaponVC[] = { 0x55F692, 0x55F6AF, 0x55F6B5, 0x55F6BE, 0x55F8A9, 0x55F8B1, 0x55F8C8, 0x55F9E2, 0x55FB8C };
    const uintptr_t clumpVC[] = { 0x55F642, 0x55F65D, 0x55F665, 0x55F66E, 0x55F8DE, 0x55F8E6, 0x55F903, 0x55F9EC, 0x55FB77 };
    const uintptr_t vehicleVC[] = { 0x55F5D2, 0x55F5ED, 0x55F5F3, 0x55F5FE, 0x55F919, 0x55F921, 0x55F93B, 0x55F9D8, 0x55FB62 };
    const uintptr_t pedVC[] = { 0x55F582, 0x55F5A3, 0x55F5AB, 0x55F5B4, 0x55F951, 0x55F959, 0x55F973, 0x55F9C4, 0x55FB4D };
    const uintptr_t effectsVC[] = { 0x55F571, 0x55F989, 0x55F991, 0x55F9A8, 0x55F9B0, 0x55FB38 };
    const StoreDef storesVC[] =
    {
        { "AtomicModels",  0x709EAC, 0x44,  3885, 6500,  0x55F730, simpleVC },
        { "TimeModels",    0x74A6B0, 0x50,  385,  6500,  0x55F6E0, timeVC },
        { "WeaponModels",  0x751F10, 0x48,  37,   6500,  0x55F690, weaponVC },
        { "ClumpModels",   0x752988, 0x30,  5,    6500,  0x55F640, clumpVC },
        { "VehicleModels", 0x752A88, 0x174, 110,  6500,  0x55F5D0, vehicleVC },
        { "PedModels",     0x75CA70, 0x48,  130,  6500,  0x55F580, pedVC },
        { "2dEffects",     0x75EF10, 0x34,  1210, 32767, 0,        effectsVC },
    };

    // GTA San Andreas 1.0 US
    const uintptr_t atomicSA[] = { 0x4C63E1, 0x4C63FE, 0x4C65DA, 0x4C6621, 0x4C6633, 0x4C6865, 0x4C689A, 0x4C68B5, 0x4C68E8, 0x4C68F9, 0x4C6927,
                                   0x4C6938, 0x4C6966, 0x4C6977, 0x4C69A5, 0x4C69B6, 0x4C69E4, 0x4C69F5, 0x4C6A23, 0x4C6A34,
                                   0x4C63F2, 0x4C662D, 0x4C6822, 0x4C6829, 0x4C6877, 0x4C6881, 0x4C6890, 0x4C68A5, 0x4C68F3, 0x4C6932, 0x4C6971,
                                   0x4C69B0, 0x4C69EF, 0x4C6A2E, 0x4C68AC };
    const uintptr_t damageSA[] = { 0x4C640B, 0x4C6428, 0x4C65E0, 0x4C6651, 0x4C6662, 0x4C682F, 0x4C6416, 0x4C665D };
    const uintptr_t timeSA[] = { 0x4C6459, 0x4C6470, 0x4C65EC, 0x4C66B1, 0x4C66C2, 0x4C683B, 0x4C6464, 0x4C66BD };
    const uintptr_t weaponSA[] = { 0x4C64A5, 0x4C64BC, 0x4C65F8, 0x4C6711, 0x4C6722, 0x4C6847, 0x4C64B0, 0x4C671D };
    const uintptr_t clumpSA[] = { 0x4C64C9, 0x4C64E0, 0x4C65FE, 0x4C6741, 0x4C6752, 0x4C684D, 0x4C64D4, 0x4C674D };
    const uintptr_t vehicleSA[] = { 0x4C64ED, 0x4C6508, 0x4C6604, 0x4C6771, 0x4C6786, 0x4C6853, 0x4C64F8, 0x4C6780 };
    const uintptr_t pedSA[] = { 0x4C6518, 0x4C652F, 0x4C660A, 0x4C67A1, 0x4C67B3, 0x4C6859, 0x4C6523, 0x4C67AD };
    const uintptr_t effectsSA[] = { 0x4C5A61, 0x4C63B3, 0x4C653C, 0x4C65C6, 0x4C6610, 0x4C685F, 0x4C654B };
    const StoreDef storesSA[] =
    {
        { "AtomicModels",       0xAAE950, 0x20,  14000, 20000, 0x4C6620, atomicSA },
        { "DamageAtomicModels", 0xB1BF58, 0x24,  70,    20000, 0x4C6650, damageSA },
        { "TimeModels",         0xB1C960, 0x24,  169,   20000, 0x4C66B0, timeSA },
        { "WeaponModels",       0xB1E158, 0x28,  51,    20000, 0x4C6710, weaponSA },
        { "ClumpModels",        0xB1E958, 0x24,  92,    20000, 0x4C6740, clumpSA },
        { "VehicleModels",      0xB1F650, 0x308, 212,   20000, 0x4C6770, vehicleSA },
        { "PedModels",          0xB478F8, 0x44,  278,   20000, 0x4C67A0, pedSA },
        { "2dEffects",          0xB4C2D8, 0x40,  100,   32767, 0,        effectsSA },
    };
}

namespace limits
{
    void InstallModelStores()
    {
        if (core::IsIII())
            AddStores(storesIII, 0);
        else if (core::IsVC())
            AddStores(storesVC, 0x55FA40);
        else if (core::IsSA())
            AddStores(storesSA, 0x4C6810);
    }
}

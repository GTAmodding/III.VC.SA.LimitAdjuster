#include "Core.h"
#include "GameMemory.h"
#include "Adjusters.h"

// Pools grow when they are full. Each pool gets address space for its ceiling, its storage never moves, so pointers the
// game or other plugins keep stay valid, and the pool keeps the game's own layout.

namespace
{
    struct CPool
    {
        uint8_t* objects;
        uint8_t* flags;
        int32_t size;
        int32_t firstFree;
    };

    // Space in front of the storage: the game looks up handle -1 (index -1) and expects readable memory there.
    constexpr size_t Front = 0x1000;

    struct PoolDef
    {
        const char* key;
        uintptr_t global;      // CPool* variable
        uint32_t elemSize;
        uint32_t vanilla;
        uint32_t ceiling;
        uintptr_t newFn;       // CPool<T>::New(), thiscall
        uintptr_t newAtFn;     // New(handle) / SetNotFreeAt(handle), thiscall, or 0
        const char* partner = nullptr; // pool that always has the same size (SA peds and ped intelligence)
    };

    struct PoolState
    {
        const PoolDef* def = nullptr;
        bool active = false;
        uint32_t maximum = 0;
        int32_t sizeValue = 0;  // the current size, compared against by code that hardcodes the vanilla size
        mem::Reserve objects;
        mem::Reserve flags;
        uint8_t* originalObjects = nullptr;
        uint8_t* originalFlags = nullptr;
        int32_t originalSize = 0;
        SafetyHookInline newHook;
        SafetyHookInline newAtHook;
    };

    constexpr size_t MaxPools = 32;
    std::array<PoolState, MaxPools> pools;
    size_t poolCount = 0;

    CPool* GetPool(const PoolState& state)
    {
        return *(CPool**)state.def->global;
    }

    uintptr_t replayMode = 0;
    bool replayGuardReady = false;

    PoolState* Partner(PoolState& state)
    {
        if (state.def->partner)
        {
            for (size_t i = 0; i < poolCount; i++)
            {
                if (std::string_view(pools[i].def->key) == state.def->partner)
                    return &pools[i];
            }
        }
        return nullptr;
    }

    bool OwnsStorage(const PoolState& state)
    {
        auto pool = GetPool(state);
        return state.active && pool && state.originalObjects && pool->objects == state.objects.data() + Front;
    }

    bool CommitSize(PoolState& state, int32_t size)
    {
        return state.objects.Commit(Front + (size_t)size * state.def->elemSize) && state.flags.Commit(Front + size);
    }

    void PublishSize(PoolState& state, int32_t size)
    {
        auto pool = GetPool(state);
        memset(pool->flags + pool->size, 0x80, size - pool->size);
        pool->size = size;
        state.sizeValue = size;
        core::Log("{}: grown to {}", state.def->key, size);
    }

    bool Grow(PoolState& state, int32_t minimum)
    {
        auto pool = GetPool(state);
        if (!OwnsStorage(state) || minimum < 0 || (uint32_t)minimum > state.maximum)
            return false;
        if (minimum <= pool->size)
            return true;
        // Replay snapshots allocate exactly the current pool size. Keep that size until CopyBack is finished.
        if (!replayGuardReady || (replayMode && *(uint8_t*)replayMode))
            return false;

        auto partner = Partner(state);
        if (state.def->partner && (!partner || !OwnsStorage(*partner) || GetPool(*partner)->size != pool->size))
            return false;
        auto size = std::min<int32_t>(state.maximum, std::max(minimum, pool->size + std::max(pool->size / 2, 64)));
        if (!CommitSize(state, size) || (partner && !CommitSize(*partner, size)))
            return false;
        // Neither logical size changes until both commitments succeed.
        PublishSize(state, size);
        if (partner)
            PublishSize(*partner, size);
        return true;
    }

    // CPool<T>::New, the same in all three games: the next free slot after the last one handed out, wrapping once.
    void* Allocate(CPool* pool, uint32_t elemSize)
    {
        bool wrapped = false;
        for (;;)
        {
            if (++pool->firstFree >= pool->size)
            {
                if (wrapped)
                    return nullptr;
                wrapped = true;
                pool->firstFree = 0;
            }

            auto& flag = pool->flags[pool->firstFree];
            if (flag & 0x80)
            {
                flag = (flag + 1) & 0x7F; // used, with the next reference number
                return pool->objects + (size_t)pool->firstFree * elemSize;
            }
        }
    }

    // Replaces New: the game's own loop jumps back into its first bytes, so it can't be hooked and called.
    template <size_t I>
    void* __fastcall NewHook(CPool* pool, void*)
    {
        auto& state = pools[I];
        auto result = Allocate(pool, state.def->elemSize);
        if (!result && state.active && pool == GetPool(state))
        {
            auto oldSize = pool->size;
            if (Grow(state, oldSize + 1))
            {
                pool->firstFree = oldSize - 1;
                result = Allocate(pool, state.def->elemSize);
            }
        }
        return result;
    }

    template <size_t I>
    void* __fastcall NewAtHook(CPool* pool, void*, int32_t handle)
    {
        auto& state = pools[I];
        if (handle < 0 || (handle & 0x80))
            return nullptr;
        auto index = (uint32_t)handle >> 8;
        if (index >= (uint32_t)pool->size)
        {
            if (!state.active || pool != GetPool(state) || !Grow(state, (int32_t)index + 1))
                return nullptr;
        }
        if (!(pool->flags[index] & 0x80))
            return nullptr;
        return state.newAtHook.thiscall<void*>(pool, handle);
    }

    template <size_t... I>
    constexpr auto MakeNewHooks(std::index_sequence<I...>)
    {
        return std::array<void*, sizeof...(I)>{ (void*)&NewHook<I>... };
    }

    template <size_t... I>
    constexpr auto MakeNewAtHooks(std::index_sequence<I...>)
    {
        return std::array<void*, sizeof...(I)>{ (void*)&NewAtHook<I>... };
    }

    const auto newHooks = MakeNewHooks(std::make_index_sequence<MaxPools>());
    const auto newAtHooks = MakeNewAtHooks(std::make_index_sequence<MaxPools>());

    // Prepare storage before changing any pointer in the native pool.
    bool PrepareAdoption(PoolState& state)
    {
        auto pool = GetPool(state);
        if (!state.active || !pool)
            return false;

        if (pool->size != (int32_t)state.def->vanilla)
        {
            core::Log("{}: the pool has {} slots instead of {} (changed by another mod?), left as it is", state.def->key, pool->size, state.def->vanilla);
            state.active = false;
            return false;
        }

        if (!state.objects)
        {
            if (!state.objects.Create(Front + (size_t)state.maximum * state.def->elemSize, Front) || !state.flags.Create(Front + state.maximum, Front))
            {
                state.active = false;
                return false;
            }
        }

        auto size = pool->size;
        if (!CommitSize(state, size))
        {
            state.active = false;
            return false;
        }
        memcpy(state.objects.data() + Front, pool->objects, (size_t)size * state.def->elemSize);
        memcpy(state.flags.data() + Front, pool->flags, size);

        return true;
    }

    void PublishAdoption(PoolState& state)
    {
        auto pool = GetPool(state);
        auto size = pool->size;
        state.originalObjects = pool->objects;
        state.originalFlags = pool->flags;
        state.originalSize = size;
        pool->objects = state.objects.data() + Front;
        pool->flags = state.flags.data() + Front;
        state.sizeValue = size;
    }

    void Adopt(PoolState& state)
    {
        if (PrepareAdoption(state))
            PublishAdoption(state);
    }

    // Before the game frees a pool: give it back its own blocks.
    void Release(PoolState& state)
    {
        auto pool = GetPool(state);
        if (!state.active || !pool || !state.originalObjects || pool->objects != state.objects.data() + Front)
            return;

        pool->objects = state.originalObjects;
        pool->flags = state.originalFlags;
        pool->size = state.originalSize;
        state.originalObjects = nullptr;
        state.originalFlags = nullptr;
    }

    bool NativeEntry(uintptr_t address)
    {
        switch (address)
        {
            case 0x4A3E80: return mem::ReadDword(address) == 0x5653C030;
            case 0x475AF0: return mem::ReadDword(address) == 0x5653C030;
            case 0x4D80D0: return mem::ReadDword(address) == 0x5653C030;
            case 0x4D8140: return mem::ReadDword(address) == 0x0424448B;
            case 0x554CF0: return mem::ReadDword(address) == 0x5653C030;
            case 0x554D60: return mem::ReadDword(address) == 0x0424448B;
            case 0x405980: return mem::ReadDword(address) == 0x5653C030;
            case 0x405AE0: return mem::ReadDword(address) == 0x5653C030;
            case 0x4BC020: return mem::ReadDword(address) == 0x5653C030;
            case 0x4BC090: return mem::ReadDword(address) == 0x0424448B;
            case 0x473BC0: return mem::ReadDword(address) == 0x5653C030;
            case 0x57C6A0: return mem::ReadDword(address) == 0x5653C030;
            case 0x57C710: return mem::ReadDword(address) == 0x0424448B;
            case 0x4C1560: return mem::ReadDword(address) == 0x5653C030;
            case 0x489C10: return mem::ReadDword(address) == 0x5653C030;
            case 0x50F000: return mem::ReadDword(address) == 0x5653C030;
            case 0x50F070: return mem::ReadDword(address) == 0x0424448B;
            case 0x5BB490: return mem::ReadDword(address) == 0x5653C030;
            case 0x5BB500: return mem::ReadDword(address) == 0x0424448B;
            case 0x407F80: return mem::ReadDword(address) == 0x5653C030;
            case 0x4E43D0: return mem::ReadDword(address) == 0x5653C030;
            case 0x4E4440: return mem::ReadDword(address) == 0x0424448B;
            case 0x4878C0: return mem::ReadDword(address) == 0x5653C030;
            case 0x5F9540: return mem::ReadDword(address) == 0x5653C030;
            case 0x5F95B0: return mem::ReadDword(address) == 0x0424448B;
            case 0x4171A0: return mem::ReadDword(address) == 0x5653C030;
            case 0x552240: return mem::ReadDword(address) == 0x5608518B;
            case 0x5522E0: return mem::ReadDword(address) == 0x5608518B;
            case 0x536D10: return mem::ReadDword(address) == 0x5608518B;
            case 0x5E45E0: return mem::ReadDword(address) == 0x5608518B;
            case 0x5E0540: return mem::ReadDword(address) == 0x5304518B;
            case 0x6E2A50: return mem::ReadDword(address) == 0x5608518B;
            case 0x6D5DD0: return mem::ReadDword(address) == 0x5304518B;
            case 0x403FA0: return mem::ReadDword(address) == 0x5608518B;
            case 0x5A1C20: return mem::ReadDword(address) == 0x5608518B;
            case 0x59F610: return mem::ReadDword(address) == 0x5304518B;
            case 0x532630: return mem::ReadDword(address) == 0x5608518B;
            case 0x40FB80: return mem::ReadDword(address) == 0x5608518B;
            case 0x61A500: return mem::ReadDword(address) == 0x5608518B;
            case 0x4B5570: return mem::ReadDword(address) == 0x5608518B;
            case 0x41B5B0: return mem::ReadDword(address) == 0x5608518B;
            case 0x41B660: return mem::ReadDword(address) == 0x5608518B;
            case 0x41B710: return mem::ReadDword(address) == 0x5608518B;
            case 0x69D8E0: return mem::ReadDword(address) == 0x5608518B;
            case 0x605EC0: return mem::ReadDword(address) == 0x5608518B;
            case 0x602A90: return mem::ReadDword(address) == 0x5304518B;
            case 0x5EA9F0: return mem::ReadDword(address) == 0x5608518B;
            case 0x4C94C0: return mem::ReadDword(address) == 0x5608518B;
            case 0x552B50: return mem::ReadDword(address) == 0x5608518B;
            case 0x5D9110: return mem::ReadDword(address) == 0x5608518B;
            case 0x5D9260: return mem::ReadDword(address) == 0x5608518B;
            case 0x5D93B0: return mem::ReadDword(address) == 0x5608518B;
            default: return false;
        }
    }

    core::Setting PoolSetting(const PoolDef& def)
    {
        auto setting = core::GetSetting(def.key, "unlimited");
        if (def.partner)
        {
            auto other = core::GetSetting(def.partner, "unlimited");
            setting.apply = setting.apply && other.apply;
            if (!other.unlimited && (setting.unlimited || other.value < setting.value))
            {
                setting.unlimited = false;
                setting.value = other.value;
            }
        }
        return setting;
    }

    // Registers a pool. Returns its index or -1.
    int AddPool(const PoolDef& def)
    {
        auto setting = PoolSetting(def);
        if (!setting.apply || poolCount >= MaxPools)
            return -1;

        if (!NativeEntry(def.newFn) || (def.newAtFn && !NativeEntry(def.newAtFn)))
        {
            core::Log("{}: allocator changed, left as it is", def.key);
            return -1;
        }
        if (def.partner)
        {
            // Both partners must pass preflight before either allocator is installed.
            auto peds = std::string_view(def.key) == "Peds";
            bool partnerInstalled = false;
            for (size_t i = 0; i < poolCount; i++)
                partnerInstalled |= pools[i].active && pools[i].newHook && std::string_view(pools[i].def->key) == def.partner;
            if (!partnerInstalled && (!NativeEntry(peds ? 0x605EC0 : 0x5E45E0) || !NativeEntry(peds ? 0x602A90 : 0x5E0540)))
                return -1;
        }
        auto index = (int)poolCount++;
        auto& state = pools[index];
        state.def = &def;
        state.maximum = std::max(core::MaxCapacity(setting, def.ceiling), def.vanilla);
        state.active = true;
        state.sizeValue = def.vanilla;
        state.newHook = safetyhook::create_inline(def.newFn, newHooks[index]);
        if (def.newAtFn)
            state.newAtHook = safetyhook::create_inline(def.newAtFn, newAtHooks[index]);

        if (!state.newHook || (def.newAtFn && !state.newAtHook))
        {
            state.newHook.reset();
            state.newAtHook.reset();
            state.active = false;
            return -1;
        }

        core::RegisterLimit({ def.key,
            [&state]
            {
                auto pool = GetPool(state);
                if (!pool)
                    return -1;
                int used = 0;
                for (int32_t i = 0; i < pool->size; i++)
                    used += (pool->flags[i] & 0x80) == 0;
                return used;
            },
            [&state] { auto pool = GetPool(state); return pool ? pool->size : 0; },
            [&state] { return (int)state.maximum; } });
        return index;
    }

    // A fixed (not unlimited) setting larger than vanilla: grow right after the pool is created.
    void ApplyFixedSize(PoolState& state)
    {
        auto setting = core::GetSetting(state.def->key, "unlimited");
        auto partnerSetting = state.def->partner ? core::GetSetting(state.def->partner, "unlimited") : setting;
        if (state.active && (!setting.unlimited || !partnerSetting.unlimited))
            Grow(state, (int32_t)state.maximum);
    }

    struct Group
    {
        std::vector<int> members;
        SafetyHookInline createHook;
        SafetyHookInline destroyHook;
    };

    std::vector<std::unique_ptr<Group>> groups;

    void AdoptGroup(Group& group)
    {
        for (auto i : group.members)
        {
            auto& state = pools[i];
            if (!state.def->partner)
            {
                Adopt(state);
                continue;
            }
            auto partner = Partner(state);
            if (!partner)
            {
                state.active = false;
                continue;
            }
            if (&state > partner)
                continue;
            if (!PrepareAdoption(state) || !PrepareAdoption(*partner))
            {
                state.active = false;
                partner->active = false;
                continue;
            }
            PublishAdoption(state);
            PublishAdoption(*partner);
        }
        for (auto i : group.members)
            ApplyFixedSize(pools[i]);
    }

    void ReleaseGroup(Group& group)
    {
        for (auto i : group.members)
            Release(pools[i]);
    }

    template <size_t G>
    int __cdecl CreateHook()
    {
        auto& group = *groups[G];
        auto result = group.createHook.ccall<int>();
        AdoptGroup(group);
        return result;
    }

    template <size_t G>
    int __cdecl DestroyHook()
    {
        auto& group = *groups[G];
        ReleaseGroup(group);
        return group.destroyHook.ccall<int>();
    }

    template <size_t... G>
    constexpr auto MakeCreateHooks(std::index_sequence<G...>)
    {
        return std::array<void*, sizeof...(G)>{ (void*)&CreateHook<G>... };
    }

    template <size_t... G>
    constexpr auto MakeDestroyHooks(std::index_sequence<G...>)
    {
        return std::array<void*, sizeof...(G)>{ (void*)&DestroyHook<G>... };
    }

    constexpr size_t MaxGroups = 8;
    const auto createHooks = MakeCreateHooks(std::make_index_sequence<MaxGroups>());
    const auto destroyHooks = MakeDestroyHooks(std::make_index_sequence<MaxGroups>());

    // Pools created together by one function (`create`, cdecl, no arguments) and freed by another (`destroy`, or 0
    // when the game never frees them).
    void AddGroup(std::span<const PoolDef> defs, uintptr_t create, uintptr_t destroy)
    {
        if (groups.size() >= MaxGroups)
            return;

        auto group = std::make_unique<Group>();
        std::vector<int> indices;
        for (auto& def : defs)
            indices.push_back(AddPool(def));

        for (size_t i = 0; i < defs.size(); i++)
        {
            if (indices[i] < 0)
                continue;
            group->members.push_back(indices[i]);
        }

        if (group->members.empty())
            return;

        auto index = groups.size();
        group->createHook = safetyhook::create_inline(create, createHooks[index]);
        if (destroy)
            group->destroyHook = safetyhook::create_inline(destroy, destroyHooks[index]);
        groups.push_back(std::move(group));
    }

    PoolState* FindPool(std::string_view key)
    {
        for (size_t i = 0; i < poolCount; i++)
        {
            if (pools[i].def->key == key)
                return &pools[i];
        }
        return nullptr;
    }

    SafetyHookMid objectLoadHook;

    void CheckSavedObject(int32_t handle, int32_t model)
    {
        auto state = FindPool("Objects");
        if (!state || !OwnsStorage(*state))
            return;
        auto ids = core::IsIII() ? 5500 : core::IsVC() ? 6500 : 20000;
        if (handle < 0 || (handle & 0x80) || model < 0 || model >= ids || !state || !Grow(*state, (handle >> 8) + 1))
        {
            core::Log("Objects: invalid or unavailable saved object handle {} / model {}", handle, model);
            MessageBoxA(nullptr, "The saved object handle or model is outside the available limits.",
                "Open Limit Adjuster - object pool", MB_OK | MB_ICONERROR);
            ExitProcess(ERROR_BAD_FORMAT);
        }
    }

    void BeforeObjectLookup(SafetyHookContext& context)
    {
        auto handle = *(int32_t*)(context.esp + (core::IsIII() ? 0x14 : 0x10));
        auto model = *(int16_t*)(context.esp + (core::IsIII() ? 0x1A : 0x16));
        CheckSavedObject(handle, model);
    }

    // SA: the call that reads a saved object's model. Only this call is replaced: the instruction after it
    // (the object pool load) is rewritten by other plugins.
    constexpr uintptr_t SavedObjectModelCallSA = 0x5D4A83;
    uintptr_t loadSavedObjectModel = 0;

    void __cdecl LoadSavedObjectModel(int32_t* model, uint32_t size)
    {
        ((void(__cdecl*)(void*, uint32_t))loadSavedObjectModel)(model, size);
        // The handle was read into the variable right before it.
        CheckSavedObject(model[-1], model[0]);
    }

    bool HookSavedObjectModelSA()
    {
        // lea edx, [esp+1Ch]; push 4; push edx; call
        static const uint8_t expected[] = { 0x8D, 0x54, 0x24, 0x1C, 0x6A, 0x04, 0x52, 0xE8 };
        auto address = SavedObjectModelCallSA - 7;
        if (memcmp((const void*)address, expected, sizeof(expected)) != 0)
            return false;
        loadSavedObjectModel = injector::GetBranchDestination(SavedObjectModelCallSA).as_int();
        injector::MakeCALL(SavedObjectModelCallSA, LoadSavedObjectModel, true);
        return true;
    }

    void GuardObjectLoading()
    {
        auto state = FindPool("Objects");
        if (!state)
            return;
        bool guarded = false;
        if (core::IsSA())
            guarded = HookSavedObjectModelSA();
        else
        {
            auto address = core::IsIII() ? 0x4A27AFu : 0x4BF203u;
            auto expected = core::IsIII() ? 0x0E280D8Bu : 0xDBE00D8Bu;
            if (mem::ReadDword(address) == expected)
                objectLoadHook = safetyhook::create_mid(address, BeforeObjectLookup);
            guarded = (bool)objectLoadHook;
        }
        if (!guarded)
        {
            state->active = false;
            core::Log("Objects: loader preflight unavailable, pool left as it is");
        }
    }

    uintptr_t replayUpdate = 0;
    uintptr_t replayTrigger = 0;

    bool ExpandedReplayIndices()
    {
        for (auto key : { "Peds", "Vehicles" })
        {
            auto state = FindPool(key);
            if (state && OwnsStorage(*state) && GetPool(*state)->size > (int32_t)state->def->vanilla)
                return true;
        }
        return false;
    }

    std::array<uintptr_t, 3> replayCalls{};

    bool __cdecl SkipReplay()
    {
        return !*(uint8_t*)replayMode && ExpandedReplayIndices();
    }

    __declspec(naked) void UpdateReplayBridge()
    {
        __asm
        {
            pushfd
            pushad
            call SkipReplay
            test al, al
            jz ForwardUpdate
            popad
            popfd
            mov eax, 0
            ret
        ForwardUpdate:
            popad
            popfd
            jmp [replayUpdate]
        }
    }

    __declspec(naked) void TriggerReplayBridge()
    {
        __asm
        {
            pushfd
            pushad
            call SkipReplay
            test al, al
            jz ForwardTrigger
            popad
            popfd
            mov eax, 0
            ret
        ForwardTrigger:
            popad
            popfd
            jmp [replayTrigger]
        }
    }

    void InstallReplayGuard()
    {
        uintptr_t updateCall = 0;
        std::array<uintptr_t, 2> triggerCalls{};
        if (core::IsIII())
        {
            replayMode = 0x95CD5B;
            replayUpdate = 0x593170;
            replayTrigger = 0x596030;
            updateCall = 0x48C975;
            triggerCalls = { 0x593210, 0x597526 };
        }
        else if (core::IsVC())
        {
            replayMode = 0xA10AF6;
            replayUpdate = 0x624EC0;
            replayTrigger = 0x6209E0;
            updateCall = 0x4A45C3;
            triggerCalls = { 0x61F1C5, 0x624F86 };
        }
        else if (core::IsSA())
        {
            replayMode = 0xA43088;
            replayUpdate = 0x460500;
            replayTrigger = 0x4600F0;
            updateCall = 0x53C090;
            triggerCalls = { 0x460470, 0x4605BD };
        }
        replayCalls = { updateCall, triggerCalls[0], triggerCalls[1] };
        for (size_t i = 0; i < replayCalls.size(); i++)
        {
            auto call = replayCalls[i];
            auto target = i == 0 ? replayUpdate : replayTrigger;
            if (!call || *(uint8_t*)call != 0xE8 || call + 5 + *(int32_t*)(call + 1) != target)
            {
                core::Log("Pools: replay call at {:X} changed, pool growth disabled", call);
                return;
            }
        }
        // Keep CALL opcodes intact for startup code scanners and preserve native register arguments.
        injector::MakeCALL(replayCalls[0], UpdateReplayBridge, true);
        injector::MakeCALL(replayCalls[1], TriggerReplayBridge, true);
        injector::MakeCALL(replayCalls[2], TriggerReplayBridge, true);
        replayGuardReady = true;
    }

    // Code outside the pool that compares a handle index against the vanilla size compares against the live size.
    void FollowSize(std::string_view key, std::initializer_list<uintptr_t> compares)
    {
        auto state = FindPool(key);
        if (!state)
            return;
        for (auto address : compares)
        {
            if (mem::IsCompare(address, state->def->vanilla))
                mem::PatchCompare(address, &state->sizeValue);
            else
                core::Log("{}: the game code at {:X} is not the expected one, left as it is", key, address);
        }
    }

    // SA: a handle asked for a pointer that isn't in the pool (a script asking for a missing second player's ped
    // passes null) indexes the flags with pointer / size, far in front of the storage. That read lands in the game's
    // heap with the native pools, but faults in front of the reserved storage: such pointers get handle -1.
    struct RefFunction
    {
        const char* pool;
        uintptr_t compact;
        uintptr_t hoodlumAddress;
        bool member; // CPool<T>::GetIndex (thiscall) or CPools::Get*Ref (cdecl)
    };

    const RefFunction refFunctionsSA[] =
    {
        { "Vehicles", 0x424160, 0x15667F0, true },
        { "Peds",     0x4442D0, 0x156CD40, true },
        { "Objects",  0x465070, 0x1561010, true },
        { "Peds",     0x54FF60, 0x54FF60,  false },
        { "Vehicles", 0x54FFC0, 0x54FFC0,  false },
        { "Objects",  0x550020, 0x550020,  false },
    };

    constexpr size_t RefCount = std::size(refFunctionsSA);
    std::array<SafetyHookInline, RefCount> refHooks;
    std::array<PoolState*, RefCount> refPools{};

    bool OutsidePool(PoolState& state, CPool* pool, uintptr_t object)
    {
        if (!pool || pool != GetPool(state) || !OwnsStorage(state))
            return false;
        auto begin = (uintptr_t)pool->objects;
        return object < begin || object >= begin + (size_t)pool->size * state.def->elemSize;
    }

    template <size_t I>
    int __fastcall MemberRefHook(CPool* pool, void*, uintptr_t object)
    {
        if (OutsidePool(*refPools[I], pool, object))
            return -1;
        return refHooks[I].thiscall<int>(pool, object);
    }

    template <size_t I>
    int __cdecl StaticRefHook(uintptr_t object)
    {
        auto& state = *refPools[I];
        if (OutsidePool(state, GetPool(state), object))
            return -1;
        return refHooks[I].ccall<int>(object);
    }

    template <size_t... I>
    constexpr auto MakeRefHooks(std::index_sequence<I...>, bool member)
    {
        return member ? std::array<void*, sizeof...(I)>{ (void*)&MemberRefHook<I>... } : std::array<void*, sizeof...(I)>{ (void*)&StaticRefHook<I>... };
    }

    bool NativeRef(const RefFunction& ref, uintptr_t address, const PoolState& state)
    {
        if (ref.member)
        {
            // mov eax, [ecx]; mov edx, [esp+4]; mov ecx, [ecx+4]; sub edx, eax
            static const uint8_t expected[] = { 0x8B, 0x01, 0x8B, 0x54, 0x24, 0x04, 0x8B, 0x49, 0x04, 0x2B, 0xD0 };
            return memcmp((const void*)address, expected, sizeof(expected)) == 0;
        }
        // mov ecx, [esp+4]; push esi; mov esi, [pool]
        static const uint8_t expected[] = { 0x8B, 0x4C, 0x24, 0x04, 0x56, 0x8B, 0x35 };
        return memcmp((const void*)address, expected, sizeof(expected)) == 0 && mem::ReadDword(address + 7) == state.def->global;
    }

    void GuardPoolRefsSA()
    {
        static const auto memberHooks = MakeRefHooks(std::make_index_sequence<RefCount>(), true);
        static const auto staticHooks = MakeRefHooks(std::make_index_sequence<RefCount>(), false);
        for (size_t i = 0; i < RefCount; i++)
        {
            auto& ref = refFunctionsSA[i];
            auto state = FindPool(ref.pool);
            auto address = core::SA(ref.compact, ref.hoodlumAddress);
            if (!state || !state->active)
                continue;
            if (!NativeRef(ref, address, *state))
            {
                core::Log("{}: handle lookup at {:X} changed, left as it is", ref.pool, address);
                continue;
            }
            refPools[i] = state;
            refHooks[i] = safetyhook::create_inline(address, ref.member ? memberHooks[i] : staticHooks[i]);
        }
    }

    // GTA III 1.0
    const PoolDef poolsIII[] =
    {
        { "PtrNode",        0x943044, 0x0C,  30000, 1000000, 0x4A3E80, 0 },
        { "EntryInfoNode",  0x941448, 0x14,  5400,  200000,  0x475AF0, 0 },
        { "Peds",           0x8F2C60, 0x5F0, 140,   1000,    0x4D80D0, 0x4D8140 },
        { "Vehicles",       0x9430DC, 0x5A8, 110,   1000,    0x554CF0, 0x554D60 },
        { "Buildings",      0x8F2C04, 0x64,  5500,  100000,  0x405980, 0 },
        { "Treadables",     0x8F2568, 0x94,  1214,  20000,   0x405AE0, 0 },
        { "Objects",        0x880E28, 0x19C, 450,   20000,   0x4BC020, 0x4BC090 },
        { "Dummys",         0x8F2C18, 0x70,  2802,  100000,  0x473BC0, 0 },
        { "AudioScriptObj", 0x8F1B6C, 0x14,  256,   4096,    0x57C6A0, 0x57C710 },
    };

    // GTA Vice City 1.0
    const PoolDef poolsVC[] =
    {
        { "PtrNode",        0xA0FD38, 0x0C,  50000, 1000000, 0x4C1560, 0 },
        { "EntryInfoNode",  0xA0D8BC, 0x14,  3200,  200000,  0x489C10, 0 },
        { "Peds",           0x97F2AC, 0x6D8, 140,   1000,    0x50F000, 0x50F070 },
        { "Vehicles",       0xA0FDE4, 0x5DC, 110,   1000,    0x5BB490, 0x5BB500 },
        { "Buildings",      0x97F240, 0x64,  7000,  100000,  0x407F80, 0 },
        { "Objects",        0x94DBE0, 0x1A0, 460,   20000,   0x4E43D0, 0x4E4440 },
        { "Dummys",         0x97F25C, 0x68,  2340,  100000,  0x4878C0, 0 },
        { "AudioScriptObj", 0x978778, 0x14,  192,   4096,    0x5F9540, 0x5F95B0 },
        { "ColModel",       0x9753C0, 0x4C,  4400,  100000,  0x4171A0, 0 },
    };

    // GTA San Andreas 1.0 US. Building and dummy indices are stored as 16-bit numbers in the IPL records, hence 32767.
    const PoolDef poolsSA[] =
    {
        { "PtrNodeSingle",   0xB74484, 0x08,  70000, 1000000, 0x552240, 0 },
        { "PtrNodeDouble",   0xB74488, 0x0C,  3200,  200000,  0x5522E0, 0 },
        { "EntryInfoNode",   0xB7448C, 0x14,  500,   200000,  0x536D10, 0 },
        { "Peds",            0xB74490, 0x7C4, 140,   1000,    0x5E45E0, 0x5E0540, "PedIntelligence" },
        { "Vehicles",        0xB74494, 0xA18, 110,   1000,    0x6E2A50, 0x6D5DD0 },
        { "Buildings",       0xB74498, 0x38,  13000, 32767,   0x403FA0, 0 },
        { "Objects",         0xB7449C, 0x19C, 350,   20000,   0x5A1C20, 0x59F610 },
        { "Dummys",          0xB744A0, 0x38,  2500,  32767,   0x532630, 0 },
        { "ColModel",        0xB744A4, 0x30,  10150, 100000,  0x40FB80, 0 },
        { "Task",            0xB744A8, 0x80,  500,   50000,   0x61A500, 0 },
        { "Event",           0xB744AC, 0x44,  200,   20000,   0x4B5570, 0 },
        { "PointRoute",      0xB744B0, 0x64,  64,    4096,    0x41B5B0, 0 },
        { "PatrolRoute",     0xB744B4, 0x1A4, 32,    4096,    0x41B660, 0 },
        { "NodeRoute",       0xB744B8, 0x24,  64,    4096,    0x41B710, 0 },
        { "TaskAllocator",   0xB744BC, 0x20,  16,    4096,    0x69D8E0, 0 },
        { "PedIntelligence", 0xB744C0, 0x294, 140,   1000,    0x605EC0, 0x602A90, "Peds" },
        { "PedAttractors",   0xB744C4, 0xC4,  64,    4096,    0x5EA9F0, 0 },
    };

    const PoolDef vehicleStructsSA[] =
    {
        { "VehicleStructs", 0xB4E680, 0x314, 50, 2000, 0x4C94C0, 0 },
    };

    const PoolDef quadTreeSA[] =
    {
        { "QuadTreeNodes", 0xB745BC, 0x28, 400, 100000, 0x552B50, 0 },
    };

    const PoolDef envMapSA[] =
    {
        { "EnvMapMaterials",  0xC02D28, 0x0C, 4096, 200000, 0x5D9110, 0 },
        { "EnvMapAtomics",    0xC02D2C, 0x0C, 1024, 100000, 0x5D9260, 0 },
        { "SpecMapMaterials", 0xC02D30, 0x08, 4096, 200000, 0x5D93B0, 0 },
    };
}

namespace limits
{
    void InstallPools()
    {
        InstallReplayGuard();
        if (core::IsIII())
        {
            AddGroup(poolsIII, 0x4A1770, 0x4A1880);
            FollowSize("Peds", { 0x4D4946 });
        }
        else if (core::IsVC())
        {
            AddGroup(poolsVC, 0x4C0270, 0x4C0070);
            FollowSize("Peds", { 0x4F4876, 0x4F48C5, 0x50D7F5 });
            FollowSize("Vehicles", { 0x5B7E52 });
        }
        else if (core::IsSA())
        {
            AddGroup(poolsSA, 0x550F10, 0x5519F0);
            AddGroup(vehicleStructsSA, 0x5B8F00, 0);
            AddGroup(quadTreeSA, 0x552C00, 0);
            AddGroup(envMapSA, 0x5DA020, 0x5DA130);
            FollowSize("Peds", { 0x5E4252 });
            GuardPoolRefsSA();
        }
        GuardObjectLoading();
    }
}

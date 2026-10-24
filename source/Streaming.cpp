#include "Core.h"
#include "Adjusters.h"
#include "StreamingReferences.h"
#include "ArchiveReferences.h"
#include "ArchiveReferencesHoodlum.h"

namespace
{
    uint32_t streamingBudget = 0;

    bool IsLargeAddressAware()
    {
        SYSTEM_INFO info{};
        GetSystemInfo(&info);
        return (uintptr_t)info.lpMaximumApplicationAddress > 0x80000000u;
    }

    // Streaming memory: the budget for models and textures kept loaded. Too much runs a 32-bit game out of address
    // space (Direct3D keeps a second copy of managed textures), so the automatic value depends on whether the exe can
    // use more than 2 GB.
    void SetMemoryAvailable(uintptr_t operand, uint32_t vanilla, uintptr_t forceJump)
    {
        auto setting = core::GetSetting("MemoryAvailable", "auto");
        if (!setting.apply || mem::ReadDword(operand) != vanilla)
        {
            if (setting.apply)
                core::Log("MemoryAvailable: the game's value was changed by another mod, left as it is");
            return;
        }

        auto largeAddressAware = IsLargeAddressAware();
        uint32_t limitMB = largeAddressAware ? 1536 : 512;
        uint32_t mb = 0;
        if (setting.automatic || setting.unlimited)
        {
            if (core::IsSA())
                mb = largeAddressAware ? 1024 : 384;
            else
                mb = largeAddressAware ? 512 : 256;
        }
        else if (setting.percent)
        {
            MEMORYSTATUSEX status{ sizeof(status) };
            GlobalMemoryStatusEx(&status);
            mb = (uint32_t)std::min<uint64_t>(status.ullTotalPhys / 100 * setting.value / (1024 * 1024), limitMB);
        }
        else
        {
            mb = std::min(setting.value, limitMB);
        }

        auto bytes = std::max(mb * 1024 * 1024, vanilla);
        mem::WriteDword(operand, bytes);
        if (forceJump)
            injector::MakeNOP(forceJump, 2, true);
        streamingBudget = bytes;
        core::Log("MemoryAvailable: {} MB{}", bytes / (1024 * 1024), largeAddressAware ? " (large address aware exe)" : "");
    }

    // CDirectory grows when an entry is added to a full one (extra objects: IMG models without an IDE definition).
    struct CDirectory
    {
        uint8_t* entries;
        int32_t capacity;
        int32_t count;
        bool ownsEntries; // SA only
    };

    SafetyHookInline addItemHook;
    void* (__cdecl* gameMalloc)(size_t) = nullptr;
    void (__cdecl* gameFree)(void*) = nullptr;
    bool hasOwnsFlag = false;
    struct DirectoryPolicy
    {
        const char* key;
        uintptr_t global;
        uint32_t maximum;
    };
    std::array<DirectoryPolicy, 2> directoryPolicies;

    bool GrowDirectory(CDirectory* dir, uint32_t maximum)
    {
        if (dir->count < 0 || dir->capacity < 0 || dir->count > dir->capacity)
            return false;
        if (dir->count < dir->capacity)
            return true;
        if ((uint32_t)dir->capacity >= maximum)
            return false;

        auto newCapacity = std::min<uint32_t>(dir->capacity * 2u + 64, maximum);
        auto entries = (uint8_t*)gameMalloc((size_t)newCapacity * 0x20);
        if (!entries)
            return false;
        if (dir->count)
            memcpy(entries, dir->entries, (size_t)dir->count * 0x20);
        if (!hasOwnsFlag || dir->ownsEntries)
            gameFree(dir->entries);
        dir->entries = entries;
        dir->capacity = newCapacity;
        if (hasOwnsFlag)
            dir->ownsEntries = true;
        return true;
    }

    void __fastcall AddItemHook(CDirectory* dir, void*, const void* info)
    {
        for (auto& policy : directoryPolicies)
        {
            if (policy.maximum && dir == *(CDirectory**)policy.global)
            {
                if (!GrowDirectory(dir, policy.maximum))
                {
                    core::Log("{}: directory full, entry ignored", policy.key);
                    return;
                }
                break;
            }
        }
        addItemHook.thiscall(dir, info);
    }

    void HookDirectories(uintptr_t addItem, uintptr_t mallocFn, uintptr_t freeFn, bool ownsFlag,
                         uintptr_t extraGlobal, uintptr_t cutsceneGlobal, uintptr_t cutsceneOperand)
    {
        auto extra = core::GetSetting("ExtraObjectsDir", "unlimited");
        auto cutscene = core::GetSetting("CutsceneDir", "unlimited");
        if (!extra.apply && !cutscene.apply)
            return;
        const uint8_t saBytes[] = { 0x8B, 0xC1, 0x8B, 0x48, 0x04 };
        const uint8_t earlyBytes[] = { 0x53, 0x56, 0x8B, 0x71, 0x08 };
        if (memcmp((void*)addItem, ownsFlag ? saBytes : earlyBytes, 5) != 0 ||
            (cutscene.apply && mem::ReadDword(cutsceneOperand) != 512))
        {
            core::Log("Directories: unexpected code, left as it is");
            return;
        }

        gameMalloc = (decltype(gameMalloc))mallocFn;
        gameFree = (decltype(gameFree))freeFn;
        hasOwnsFlag = ownsFlag;
        directoryPolicies = {{ { "ExtraObjectsDir", extraGlobal, extra.apply ? std::max(core::IsIII() ? 128u : core::IsVC() ? 256u : 550u, core::MaxCapacity(extra, 1000000)) : 0 },
                               { "CutsceneDir", cutsceneGlobal, cutscene.apply ? std::max(512u, core::MaxCapacity(cutscene, 1000000)) : 0 } }};
        addItemHook = safetyhook::create_inline(addItem, AddItemHook);
        if (!addItemHook)
            return;
        if (cutscene.apply && !cutscene.unlimited)
            mem::WriteDword(cutsceneOperand, directoryPolicies[1].maximum);
        for (auto& policy : directoryPolicies)
        {
            if (!policy.maximum)
                continue;
            auto p = &policy;
            core::RegisterLimit({ p->key,
                [p] { auto dir = *(CDirectory**)p->global; return dir ? dir->count : 0; },
                [p] { auto dir = *(CDirectory**)p->global; return dir ? dir->capacity : 0; },
                [p] { return (int)p->maximum; } });
        }
    }

    SafetyHookInline imageOpenHook;
    std::array<HANDLE, 128> imageHandles{};
    std::array<std::array<char, 64>, 128> imageNames{};
    std::array<std::array<uint8_t, 48>, 128> imageDescriptors{};
    uint32_t imageMaximum{};
    uintptr_t imageCountAddress{};
    uintptr_t imageFlagsAddress{};

    void RelocateChecked(std::span<const mem::Expect> refs, uintptr_t oldBase, uintptr_t newBase)
    {
        for (auto& ref : refs)
            mem::WriteDword(ref.operand, newBase + ref.value - oldBase);
    }

    int __cdecl OpenImageHook(const char* name)
    {
        if (!name || strlen(name) >= 64)
            return 0;
        uint32_t slot = core::IsSA() ? 0 : *(uint32_t*)imageCountAddress;
        if (core::IsSA())
            while (slot < imageMaximum && imageHandles[slot])
                ++slot;
        if (slot >= imageMaximum)
        {
            core::Log("ImgArchives: archive limit reached, ignored {}", name);
            return 0;
        }
        SetLastError(0);
        auto file = CreateFileA(name, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                               *(DWORD*)imageFlagsAddress | 0x10000001, nullptr);
        if (file == INVALID_HANDLE_VALUE)
            return 0;
        imageHandles[slot] = file;
        strcpy_s(imageNames[slot].data(), imageNames[slot].size(), name);
        *(uint32_t*)imageCountAddress = std::max(*(uint32_t*)imageCountAddress, slot + 1);
        return core::IsSA() ? (int)(slot << 24) : 1;
    }

    // Mod Loader streams IMG files itself through the game's own archive arrays, and it loads after this plugin
    bool ModLoaderInstalled()
    {
        return GetModuleHandleW(L"modloader.asi") || GetFileAttributesW(L"scripts\\modloader.asi") != INVALID_FILE_ATTRIBUTES ||
            GetFileAttributesW(L"modloader.asi") != INVALID_FILE_ATTRIBUTES || GetFileAttributesW(L"plugins\\modloader.asi") != INVALID_FILE_ATTRIBUTES;
    }

    void SetImgArchives()
    {
        auto setting = core::GetSetting("ImgArchives", "unlimited");
        if (!setting.apply)
            return;
        if (ModLoaderInstalled())
        {
            core::Log("ImgArchives: Mod Loader found, archives are left to it");
            return;
        }
        // CdStream encodes the archive in the high byte; SA also sign-extends that byte.
        imageMaximum = std::max(core::IsSA() ? 32u : 8u, core::MaxCapacity(setting, 128));
        auto handles = core::IsIII() ? std::span<const mem::Expect>(imageHandlesIII) : core::IsVC() ? std::span<const mem::Expect>(imageHandlesVC) : (core::hoodlum ? std::span<const mem::Expect>(imageHandlesHoodlum) : std::span<const mem::Expect>(imageHandlesSA));
        auto names = core::IsIII() ? std::span<const mem::Expect>(imageNamesIII) : core::IsVC() ? std::span<const mem::Expect>(imageNamesVC) : (core::hoodlum ? std::span<const mem::Expect>(imageNamesHoodlum) : std::span<const mem::Expect>(imageNamesSA));
        auto descriptors = core::hoodlum ? std::span<const mem::Expect>(imageDescriptorsHoodlum) : std::span<const mem::Expect>(imageDescriptorsSA);
        auto ends = core::hoodlum ? std::span<const mem::Expect>(imageDescriptorEndsHoodlum) : std::span<const mem::Expect>(imageDescriptorEndsSA);
        auto open = core::IsIII() ? 0x406270u : core::IsVC() ? 0x4081E0u : core::SA(0x4067B0, 0x1564A90);
        const uint8_t earlyBytes[] = { 0x53, 0x8B, 0x5C, 0x24, 0x08 };
        const uint8_t saBytes[] = { 0x56, 0x57, 0x33, 0xF6, 0x8B };
        const mem::Expect initSA[] = { { 0x406B75, 32 }, { 0x406B98, 0x8E4898 } };
        if (core::IsSA() && core::hoodlum && (*(uint8_t*)0x4067B0 != 0xE9 ||
            0x4067B5u + mem::ReadDword(0x4067B1) != open))
        {
            core::Log("ImgArchives: native opener entry was redirected, left as it is");
            return;
        }
        auto openMatches = memcmp((void*)open, core::IsSA() ? saBytes : earlyBytes, 5) == 0;
        // Accept the equivalent register XOR encoding.
        if (core::IsSA() && *(uint16_t*)open == 0x5756 && *(uint16_t*)(open + 2) == 0xF631 && *(uint8_t*)(open + 4) == 0x8B)
            openMatches = true;
        if (!openMatches)
        {
            core::Log("ImgArchives: unexpected open prologue {:08X} {:02X}, left as it is", mem::ReadDword(open), *(uint8_t*)(open + 4));
            return;
        }
        if (!mem::CheckValues(handles, "ImgArchives") || !mem::CheckValues(names, "ImgArchives") ||
            (core::IsSA() && (!mem::CheckValues(descriptors, "ImgArchives") ||
                              !mem::CheckValues(ends, "ImgArchives") ||
                              !mem::CheckValues(initSA, "ImgArchives"))))
            return;
        imageCountAddress = core::IsIII() ? 0x621270 : core::IsVC() ? 0x6F76C8 : 0x8E4094;
        imageFlagsAddress = core::IsIII() ? 0x6212C4 : core::IsVC() ? 0x6F771C : 0x8E3FE0;
        imageOpenHook = safetyhook::create_inline(open, OpenImageHook);
        if (!imageOpenHook)
        {
            core::Log("ImgArchives: cannot hook the native archive opener, left as it is");
            return;
        }
        auto oldHandles = core::IsIII() ? 0x621278 : core::IsVC() ? 0x6F76D0 : 0x8E4010;
        auto oldNames = core::IsIII() ? 0x621030 : core::IsVC() ? 0x6F7488 : 0x8E4098;
        RelocateChecked(handles, oldHandles, (uintptr_t)imageHandles.data());
        RelocateChecked(names, oldNames, (uintptr_t)imageNames.data());
        if (core::IsSA())
        {
            mem::WriteDword(0x406B75, imageMaximum);
            mem::WriteDword(0x406B98, (uintptr_t)imageNames.data() + imageMaximum * 64);
            RelocateChecked(descriptors, 0x8E48D8, (uintptr_t)imageDescriptors.data());
            for (auto& ref : ends)
                mem::WriteDword(ref.operand, (uintptr_t)imageDescriptors.data() + imageMaximum * 48 + ref.value - 0x8E4A58);
        }
        core::RegisterLimit({ "ImgArchives", [] { return *(int*)imageCountAddress; },
            [] { return (int)imageMaximum; }, [] { return (int)imageMaximum; } });
    }

    SafetyHookInline txdAddVC;

    int __cdecl AddTxdVC(const char* name)
    {
        auto pool = *(uint32_t**)0x9B6A4C;
        if (!pool || !name || strlen(name) >= 20 || !pool[2])
            return -1;
        auto flags = (uint8_t*)pool[1];
        for (uint32_t n = 0; n < pool[2]; n++)
        {
            auto index = (pool[3] + 1 + n) % pool[2];
            if (flags[index] & 0x80)
                return txdAddVC.ccall<int>(name);
        }
        core::Log("TxdStore: no free texture dictionary slot");
        return -1;
    }

    void SetTxdStoreVC()
    {
        auto setting = core::GetSetting("TxdStore", "unlimited");
        if (!setting.apply)
            return;
        const mem::Expect counts[] =
        {
            { 0x41065F, 1385 }, { 0x580FD6, 1385 }, { 0x58102D, 1385 },
            { 0x61D98C, 1385 }, { 0x61D9E6, 1385 }, { 0x61DAA6, 1385 },
            { 0x61DBA6, 1385 }, { 0x61DCFC, 1385 }, { 0x61DD3A, 1385 },
        };
        const uint8_t addBytes[] = { 0x8B, 0x0D, 0x4C, 0x6A, 0x9B, 0x00 };
        if (!mem::CheckValues(counts, "TxdStore") || !mem::CheckValues(streamingRefsVC, "TxdStore") ||
            !mem::CheckValues(streamingRangesVC, "TxdStore") || memcmp((void*)0x580F00, addBytes, sizeof(addBytes)))
            return;
        auto slots = std::max(1385u, core::MaxCapacity(setting, 20000));
        auto extra = slots - 1385;
        // COL and IFP streaming IDs follow TXDs; their own slot indices and model IDs remain unchanged.
        static mem::Reserve records;
        if (!records.Create((7951u + extra + 16) * 20, (7951u + extra + 16) * 20))
            return;
        auto add = safetyhook::create_inline(0x580F00, AddTxdVC);
        if (!add)
            return;
        txdAddVC = std::move(add);
        for (auto& ref : streamingRefsVC)
        {
            auto offset = ref.value - 0x94DDD0;
            if (offset >= 7885 * 20)
                offset += extra * 20;
            mem::WriteDword(ref.operand, (uintptr_t)records.data() + offset);
        }
        for (auto& ref : streamingRangesVC)
        {
            auto negative = (int32_t)ref.value < 0;
            mem::WriteDword(ref.operand, negative ? ref.value - extra : ref.value + extra);
        }
        for (auto& ref : counts)
            mem::WriteDword(ref.operand, slots);
        core::RegisterLimit({ "TxdStore", []
            {
                auto pool = *(uint32_t**)0x9B6A4C;
                if (!pool)
                    return 0;
                int used = 0;
                auto flags = (uint8_t*)pool[1];
                for (uint32_t i = 0; i < pool[2]; i++)
                    used += (flags[i] & 0x80) == 0;
                return used;
            }, [slots] { return (int)slots; }, [slots] { return (int)slots; } });
    }

    // GTA III: txd slots are the last streaming ids (5500 + slot), so more slots need a bigger streaming info array.
    const uintptr_t streamingInfoRefsIII[] =
    {
        0x406444, 0x40644C, 0x406456, 0x406461, 0x406468, 0x406472, 0x40648C, 0x406493, 0x40649D, 0x4064A8, 0x4064AF, 0x4064B9, 0x4064D1, 0x4064D8,
        0x4064E2, 0x4064ED, 0x4064F4, 0x4064FE, 0x406515, 0x40651C, 0x406526, 0x406531, 0x406538, 0x406542, 0x406559, 0x406560, 0x40656A, 0x406575,
        0x40657C, 0x406586, 0x40659D, 0x4065A4, 0x4065AE, 0x4065B9, 0x4065C0, 0x4065CA, 0x4065E1, 0x4065E8, 0x4065F2, 0x4065FD, 0x406604, 0x40660E,
        0x406629, 0x406630, 0x40663A, 0x406645, 0x406651, 0x40665B, 0x406681, 0x406688, 0x406692, 0x40669D, 0x4066A4, 0x4066AE, 0x4066D6, 0x4066DD,
        0x4066E7, 0x4066F2, 0x4066F9, 0x406703, 0x406738, 0x40673F, 0x406749, 0x406754, 0x40675B, 0x406765, 0x40679A, 0x4067A1, 0x4067AB, 0x4067B6,
        0x4067BD, 0x4067C7, 0x4067F5, 0x4067FC, 0x406806, 0x406811, 0x406818, 0x406822, 0x406847, 0x40684E, 0x406858, 0x406863, 0x40686A, 0x406874,
        0x4068FA, 0x406901, 0x40696B, 0x406EAB, 0x406EF2, 0x406F0D, 0x406FD3, 0x407027, 0x4077AF, 0x407E8A, 0x407EB9, 0x407ECB, 0x407EDC, 0x407EEC,
        0x407EF2, 0x407EFD, 0x407F28, 0x407F30, 0x407F40, 0x407F96, 0x407FBB, 0x407FC2, 0x4082A4, 0x4082B6, 0x4082D1, 0x4082E3, 0x408300, 0x408312,
        0x408376, 0x408846, 0x40887E, 0x408894, 0x40889D, 0x4088A7, 0x4088B8, 0x4088C4, 0x4088D3, 0x40890A, 0x408934, 0x408962, 0x40896C, 0x40898B,
        0x4089C9, 0x40937D, 0x409406, 0x40946E, 0x4094DD, 0x40958E, 0x409698, 0x4096BA, 0x40975B, 0x4098B8, 0x40991D, 0x409948, 0x4099F4, 0x409A37,
        0x409AB3, 0x409C93, 0x409CDE, 0x409D07, 0x409D68, 0x409E8E, 0x409ED9, 0x409EF4, 0x409F16, 0x409F69, 0x409F7A, 0x40A0AC, 0x40A0E9, 0x40A123,
        0x40A198, 0x40A1A4, 0x40A1C7, 0x40A214, 0x40A2B4, 0x40A2C5, 0x40A2D3, 0x40A4AD, 0x40A4C9, 0x40A4DC, 0x40A699, 0x40A709, 0x40A7A3, 0x40A7C0,
        0x40A7CB, 0x40A7D4, 0x40A7DC, 0x40A833, 0x40A850, 0x40A85B, 0x40A864, 0x40A86C, 0x40A9D7, 0x40ADCA, 0x40AEE9, 0x40AF2D, 0x40AF84, 0x40AF8E,
        0x40B01B, 0x40B0CB, 0x40B1C6, 0x40B1DF, 0x40B225, 0x40B22E, 0x40B262, 0x40B272, 0x4180F7, 0x418206, 0x41820F, 0x41824C, 0x418255, 0x418278,
        0x418281, 0x41828A, 0x4182CB, 0x4182D4, 0x41FD2A, 0x41FD35, 0x41FD65, 0x41FDF7, 0x41FE02, 0x41FE32, 0x4276A7, 0x4355A8, 0x4371F2, 0x43790D,
        0x43A77A, 0x444609, 0x490F13, 0x491175, 0x4A9ED3, 0x542729, 0x54A5FD, 0x54DD66, 0x54FF19, 0x585C2A, 0x5860E1, 0x5872D3, 0x587404, 0x5953B1,
        0x5955B8,
    };
    const uintptr_t txdStoreSizesIII[] = { 0x406979, 0x527458, 0x5274D1, 0x592C99, 0x592CE5, 0x592D3E, 0x592E33, 0x592F24, 0x592F5A };

    struct TxdPoolIII
    {
        uint8_t* objects;
        uint8_t* flags;
        int32_t size;
        int32_t firstFree;
    };
    mem::Reserve txdObjectsIII, txdFlagsIII;
    uint32_t txdMaximumIII{};
    uint32_t txdInitialIII{};
    SafetyHookInline txdConstructorIII, txdFlushIII, txdAddIII;

    TxdPoolIII* __fastcall ConstructTxdIII(TxdPoolIII* pool, void*, uint32_t)
    {
        memset(txdFlagsIII.data() + 4096, 0x80, txdMaximumIII);
        pool->objects = txdObjectsIII.data() + 4096;
        pool->flags = txdFlagsIII.data() + 4096;
        pool->size = txdInitialIII;
        pool->firstFree = 0;
        return pool;
    }

    void __fastcall FlushTxdIII(TxdPoolIII* pool, void*)
    {
        if (pool->objects != txdObjectsIII.data() + 4096)
        {
            txdFlushIII.thiscall(pool);
            return;
        }
        pool->objects = nullptr;
        pool->flags = nullptr;
        pool->size = 0;
        pool->firstFree = 0;
    }

    int __cdecl AddTxdIII(const char* name)
    {
        auto pool = *(TxdPoolIII**)0x8F5FB8;
        if (!pool || !name || strlen(name) >= 20)
            return -1;
        if (pool->objects != txdObjectsIII.data() + 4096)
            return txdAddIII.ccall<int>(name);
        for (;;)
        {
            for (int32_t n = 0; n < pool->size; n++)
            {
                auto index = (pool->firstFree + 1 + n) % pool->size;
                if ((pool->flags[index] & 0x80) == 0)
                    continue;
                pool->flags[index] = (pool->flags[index] + 1) & 0x7F;
                pool->firstFree = index;
                auto record = pool->objects + (size_t)index * 28;
                memset(record, 0, 28);
                strcpy_s((char*)record + 8, 20, name);
                return index;
            }
            auto size = std::min(txdMaximumIII, (uint32_t)pool->size + std::max(64u, (uint32_t)pool->size / 2));
            if (size <= (uint32_t)pool->size || !txdObjectsIII.Commit(4096 + (size_t)size * 28))
            {
                core::Log("TxdStore: no free texture dictionary slot");
                return -1;
            }
            // All flags, including not-yet-active slots, are free for fixed-bound native scans.
            pool->firstFree = pool->size - 1;
            pool->size = size;
        }
    }

    void SetTxdStoreIII()
    {
        auto setting = core::GetSetting("TxdStore", "unlimited");
        if (!setting.apply)
            return;

        std::vector<mem::Expect> expected;
        for (auto operand : txdStoreSizesIII)
            expected.push_back({ operand, 850 });
        expected.push_back({ 0x40664B, 6342 });
        if (!mem::CheckValues(expected, "TxdStore"))
            return;

        const uint8_t constructorBytes[] = { 0x53, 0x56, 0x57, 0x55, 0x83, 0xEC, 0x08 };
        const uint8_t flushBytes[] = { 0x53, 0x89, 0xCB, 0x83, 0x7B, 0x08, 0x00 };
        const uint8_t addBytes[] = { 0x8B, 0x0D, 0xB8, 0x5F, 0x8F, 0x00 };
        if (memcmp((void*)0x527B50, constructorBytes, sizeof(constructorBytes)) ||
            memcmp((void*)0x527A50, flushBytes, sizeof(flushBytes)) ||
            memcmp((void*)0x5274E0, addBytes, sizeof(addBytes)))
            return;
        auto slots = std::max<uint32_t>(core::MaxCapacity(setting, 4096), 850);
        txdMaximumIII = slots;
        txdInitialIII = setting.unlimited ? 850u : slots;
        if (!txdObjectsIII.Create(4096 + (size_t)slots * 28, 4096 + (size_t)txdInitialIII * 28) ||
            !txdFlagsIII.Create(4096 + slots, 4096 + slots))
            return;
        auto constructor = safetyhook::create_inline(0x527B50, ConstructTxdIII);
        auto flush = safetyhook::create_inline(0x527A50, FlushTxdIII);
        auto add = safetyhook::create_inline(0x5274E0, AddTxdIII);
        if (!constructor || !flush || !add)
            return;
        auto total = 5500 + slots;
        // CStreaming::Init initialises the array in steps of 8 and then 6 more, so keep room for them.
        auto moved = limits::MoveArray("TxdStore", 0x6C7088, 0x14, 6350, total + 14, streamingInfoRefsIII, false);
        if (!moved)
            return;

        mem::WriteDword(0x40664B, total - 8);
        for (auto operand : txdStoreSizesIII)
            mem::WriteDword(operand, slots);

        txdConstructorIII = std::move(constructor);
        txdFlushIII = std::move(flush);
        txdAddIII = std::move(add);
        core::RegisterLimit({ "TxdStore", []
            {
                auto pool = *(TxdPoolIII**)0x8F5FB8;
                if (!pool)
                    return 0;
                int used = 0;
                for (int32_t i = 0; i < pool->size; i++)
                    used += (pool->flags[i] & 0x80) == 0;
                return used;
            }, [] { auto pool = *(TxdPoolIII**)0x8F5FB8; return pool ? pool->size : 0; },
            [slots] { return (int)slots; } });
    }
}

namespace limits
{
    uint32_t StreamingBudget()
    {
        return streamingBudget;
    }

    void InstallStreaming()
    {
        SetImgArchives();
        if (core::IsIII())
        {
            SetMemoryAvailable(0x406ADA, 0x3200000, 0x406AD2);
            HookDirectories(0x473600, 0x5BD500, 0x5BD530, false, 0x95CB90, 0x8F5F88, 0x40460C);
            SetTxdStoreIII();
        }
        else if (core::IsVC())
        {
            SetMemoryAvailable(0x41079F, 0x2D00000, 0);
            HookDirectories(0x4873F0, 0x640250, 0x645BB0, false, 0xA10730, 0x9B5F88, 0x407483);
            SetTxdStoreVC();
        }
        else if (core::IsSA())
        {
            SetMemoryAvailable(0x5B8E6A, 0x3200000, 0);
            HookDirectories(0x532310, 0x82119A, 0x8214BD, true, 0x8E48D0, 0xB5F848, 0x4D5A7A);
        }
    }
}

#include "Core.h"
#include "Adjusters.h"
#include "PopulationCycle.inl"

// The lists the renderer fills every frame. They are what a longer draw distance runs out of first.

namespace
{
    // An older skygfx for III/VC draws the world from the game's own visible entity array
    bool OlderSkyGfxInstalled()
    {
        if (!core::OlderSkyGfxInstalled())
            return false;
        core::Log("VisibleEntityPtrs: an older skygfx was found, the game's own list is kept");
        return true;
    }

    // Visible entity lists: the game never checks their bound. Moved into space that commits itself as it fills.
    void MoveVisibleList(const char* key, uintptr_t base, uint32_t vanilla, uintptr_t counter, std::span<const uintptr_t> refs)
    {
        auto setting = core::GetSetting(key, "unlimited");
        if (!setting.apply)
            return;

        auto capacity = std::max(core::MaxCapacity(setting, 262144), vanilla);
        auto moved = limits::MoveArray(key, base, 4, vanilla, capacity, refs, false);
        if (!moved)
            return;

        core::RegisterLimit({ key, [counter] { return *(int*)counter; }, [moved] { return (int)moved->capacity; }, [moved] { return (int)moved->capacity; } });
    }

    // Invisible entity list: bounded by compares against capacity - 1.
    void MoveInvisibleList(uintptr_t base, uintptr_t counter, std::span<const uintptr_t> refs, std::span<const uintptr_t> bounds)
    {
        auto setting = core::GetSetting("InvisibleEntityPtrs", "unlimited");
        if (!setting.apply)
            return;

        std::vector<mem::Expect> expected;
        for (auto operand : bounds)
            expected.push_back({ operand, 149 });
        if (!mem::CheckValues(expected, "InvisibleEntityPtrs"))
            return;

        auto capacity = std::max<uint32_t>(core::MaxCapacity(setting, 16384), 150);
        auto moved = limits::MoveArray("InvisibleEntityPtrs", base, 4, 150, capacity, refs, false);
        if (!moved)
            return;

        for (auto operand : bounds)
            mem::WriteDword(operand, capacity - 1);

        core::RegisterLimit({ "InvisibleEntityPtrs", [counter] { return *(int*)counter; }, [capacity] { return (int)capacity; }, [capacity] { return (int)capacity; } });
    }

    // CLinkList free lists grow by another block of links when they run empty. Links are only reached through the
    // list pointers, so the blocks don't have to be next to each other.
    struct LinkList
    {
        const char* key;
        uintptr_t list;
        uint32_t linkSize;
        uint32_t prevOffset;  // in a link
        uint32_t nextOffset;  // in a link
        uint32_t freeHead;    // offset of the free list head link in the list
        uint32_t freeTail;
        uint32_t vanilla;
        uint32_t ceiling;
    };

    struct LinkListState
    {
        const LinkList* def;
        uint32_t total;
        uint32_t maximum;
    };

    std::vector<LinkListState> linkLists;

    bool IsFreeListEmpty(const LinkList& def)
    {
        auto head = def.list + def.freeHead;
        return *(uintptr_t*)(head + def.nextOffset) == def.list + def.freeTail;
    }

    void GrowLinkList(LinkListState& state)
    {
        auto& def = *state.def;
        if (!IsFreeListEmpty(def) || state.total >= state.maximum)
            return;

        auto count = std::min(std::max<uint32_t>(state.total / 2, 32), state.maximum - state.total);
        auto block = (uint8_t*)calloc(count, def.linkSize);
        if (!block)
            return;

        auto head = def.list + def.freeHead;
        for (uint32_t i = 0; i < count; i++)
        {
            auto link = (uintptr_t)(block + i * def.linkSize);
            auto next = *(uintptr_t*)(head + def.nextOffset);
            *(uintptr_t*)(link + def.prevOffset) = head;
            *(uintptr_t*)(link + def.nextOffset) = next;
            *(uintptr_t*)(next + def.prevOffset) = link;
            *(uintptr_t*)(head + def.nextOffset) = link;
        }
        state.total += count;
        core::Log("{}: grown to {}", def.key, state.total);
    }

    LinkListState* FindLinkList(uintptr_t list)
    {
        for (auto& state : linkLists)
        {
            if (state.def->list == list)
                return &state;
        }
        return nullptr;
    }

    void AddLinkLists(std::span<const LinkList> defs)
    {
        linkLists.reserve(linkLists.size() + defs.size());
        for (auto& def : defs)
        {
            auto setting = core::GetSetting(def.key, "unlimited");
            if (!setting.apply)
                continue;
            auto& state = linkLists.emplace_back(LinkListState{ &def, def.vanilla, std::max(core::MaxCapacity(setting, def.ceiling), def.vanilla) });
            core::RegisterLimit({ def.key, [] { return -1; }, [&state] { return (int)state.total; }, [&state] { return (int)state.maximum; } });
        }
    }

    std::vector<SafetyHookMid> linkListHooks;

    // Hooks a CLinkList insert where ecx is the list (thiscall entry).
    void HookLinkListInsert(uintptr_t address)
    {
        linkListHooks.push_back(safetyhook::create_mid(address, [](SafetyHookContext& regs)
        {
            if (auto state = FindLinkList(regs.ecx))
                GrowLinkList(*state);
        }));
    }

    // Hooks code that always inserts into the same list.
    template <uintptr_t List>
    void HookFixedLinkListInsert(uintptr_t address)
    {
        linkListHooks.push_back(safetyhook::create_mid(address, [](SafetyHookContext&)
        {
            if (auto state = FindLinkList(List))
                GrowLinkList(*state);
        }));
    }

    // III 1.0
    const uintptr_t visibleRefsIII[] = { 0x4A7853, 0x4A78D6, 0x4A7953, 0x4A9336, 0x4A9C6B, 0x4A9EF8, 0x4AA10F };
    const uintptr_t invisibleRefsIII[] = { 0x4A7873, 0x4A9D7A, 0x4AA00A, 0x4AA186 };
    const uintptr_t invisibleBoundsIII[] = { 0x4A9D62, 0x4A9FF2, 0x4AA170 };
    const LinkList linkListsIII[] =
    {
        { "AlphaList",             0x8F42E4, 0x10, 0x08, 0x0C, 0x20, 0x30, 20,  65536 },
        { "AlphaEntityList",       0x943084, 0x10, 0x08, 0x0C, 0x20, 0x30, 150, 65536 },
        { "SortedVehiclesAndPeds", 0x629AC0, 0x10, 0x08, 0x0C, 0x20, 0x30, 40,  65536 },
    };

    // VC 1.0
    const uintptr_t visibleRefsVC[] = { 0x4C7621, 0x4C77FB, 0x4C85BB, 0x4C9F83, 0x4CA1BB, 0x4CA203 };
    const uintptr_t invisibleRefsVC[] = { 0x4C76E8, 0x4C78C2, 0x4CA223 };
    const uintptr_t invisibleBoundsVC[] = { 0x4C76D2, 0x4C78AC };
    const LinkList linkListsVC[] =
    {
        { "AlphaList",                 0x983B3C, 0x10, 0x08, 0x0C, 0x20, 0x30, 20,  65536 },
        { "BoatAlphaList",             0x97F2F8, 0x10, 0x08, 0x0C, 0x20, 0x30, 20,  65536 },
        { "AlphaEntityList",           0xA0FD84, 0x10, 0x08, 0x0C, 0x20, 0x30, 200, 65536 },
        { "AlphaUnderwaterEntityList", 0x978D44, 0x10, 0x08, 0x0C, 0x20, 0x30, 30,  65536 },
        { "SortedVehiclesAndPeds",     0x704858, 0x10, 0x08, 0x0C, 0x20, 0x30, 40,  65536 },
    };

    // SA 1.0 US
    const uintptr_t visibleRefsSA[] = { 0x553529, 0x553944, 0x553A53, 0x553B03 };
    const uintptr_t visibleLodRefsSA[] = { 0x5534F5, 0x553923, 0x553CB3 };
    const uintptr_t invisibleRefsSA[] = { 0x554AC7, 0x553986 };
    const uintptr_t invisibleBoundsSA[] = { 0x554ABE };
    const uintptr_t lodRenderListRefsSA[] = { 0x5536D1, 0x5536F6, 0x55377C, 0x5537B1, 0x553844, 0x553876, 0x5558C9 };
    const uintptr_t lodDontRenderListRefsSA[] = { 0x5536E1, 0x553700, 0x5558D9 };
    const LinkList linkListsSA[] =
    {
        { "AlphaList",                    0xC88070, 0x14, 0x0C, 0x10, 0x28, 0x3C, 20,    65536 },
        { "BoatAlphaList",                0xC880C8, 0x14, 0x0C, 0x10, 0x28, 0x3C, 20,    65536 },
        { "AlphaEntityList",              0xC88120, 0x14, 0x0C, 0x10, 0x28, 0x3C, 200,   65536 },
        { "AlphaUnderwaterEntityList",    0xC88178, 0x14, 0x0C, 0x10, 0x28, 0x3C, 100,   65536 },
        { "AlphaReallyDrawLastList",      0xC881D0, 0x14, 0x0C, 0x10, 0x28, 0x3C, 50,    65536 },
        { "WeaponPedsForPC",              0xC88224, 0x0C, 0x04, 0x08, 0x18, 0x24, 100,   65536 },
        { "StreamingObjectInstancesList", 0x9654F0, 0x0C, 0x04, 0x08, 0x18, 0x24, 1000,  1000000 },
        { "MatrixList",                   0xB74288, 0x54, 0x4C, 0x50, 0x150, 0x1A4, 900, 200000 },
    };

    // SA keeps its LOD render lists in a 16 KB scratch buffer with no bound check.
    void MoveLodListsSA()
    {
        auto setting = core::GetSetting("LodRenderList", "unlimited");
        if (!setting.apply)
            return;

        std::vector<mem::Expect> expected;
        for (auto operand : lodRenderListRefsSA)
            expected.push_back({ operand, 0xC8E0E0 });
        for (auto operand : lodDontRenderListRefsSA)
            expected.push_back({ operand, 0xC900C8 });
        if (!mem::CheckValues(expected, "LodRenderList"))
            return;

        auto capacity = std::max<uint32_t>(core::MaxCapacity(setting, 262144), 1024);
        static mem::Reserve renderList;
        static mem::Reserve dontRenderList;
        if (!renderList.Create(capacity * 8, 0x4000) || !dontRenderList.Create(capacity * 8, 0x2000))
            return;

        mem::InstallCommitHandler();
        for (auto operand : lodRenderListRefsSA)
            mem::WriteDword(operand, (uint32_t)(uintptr_t)renderList.data());
        for (auto operand : lodDontRenderListRefsSA)
            mem::WriteDword(operand, (uint32_t)(uintptr_t)dontRenderList.data());

        core::RegisterLimit({ "LodRenderList", [] { return (int)((*(uintptr_t*)0xB745D0 - (uintptr_t)renderList.data()) / 8); }, [capacity] { return (int)capacity; }, [capacity] { return (int)capacity; } });
    }
}

namespace limits
{
    void InstallRenderLists()
    {
        if (core::IsIII())
        {
            if (!OlderSkyGfxInstalled())
                MoveVisibleList("VisibleEntityPtrs", 0x6E9920, 2000, 0x940730, visibleRefsIII);
            MoveInvisibleList(0x880B50, 0x8F1B78, invisibleRefsIII, invisibleBoundsIII);
            AddLinkLists(linkListsIII);
            HookLinkListInsert(0x529290); // CLinkList<AlphaObjectInfo>::InsertSorted
            HookLinkListInsert(0x4AABF0); // CLinkList<CEntityInfo>::InsertSorted
        }
        else if (core::IsVC())
        {
            if (!OlderSkyGfxInstalled())
                MoveVisibleList("VisibleEntityPtrs", 0x7D54F8, 2000, 0xA0D1E4, visibleRefsVC);
            MoveInvisibleList(0x94D8E8, 0x978790, invisibleRefsVC, invisibleBoundsVC);
            AddLinkLists(linkListsVC);
            HookLinkListInsert(0x582BD0);
            HookLinkListInsert(0x4CA540);
        }
        else if (core::IsSA())
        {
            population::Install();
            MoveVisibleList("VisibleEntityPtrs", 0xB75898, 1000, 0xB76844, visibleRefsSA);
            MoveVisibleList("VisibleLodPtrs", 0xB748F8, 1000, 0xB76840, visibleLodRefsSA);
            MoveInvisibleList(0xB745D8, 0xB7683C, invisibleRefsSA, invisibleBoundsSA);
            MoveLodListsSA();
            AddLinkLists(linkListsSA);
            HookLinkListInsert(0x733910);           // CLinkList<AlphaObjectInfo>::InsertSorted
            HookFixedLinkListInsert<0xC88224>(0x5E46D0); // ped weapon list, inlined insert
            HookFixedLinkListInsert<0x9654F0>(0x409650); // CStreaming::AddEntity
            HookFixedLinkListInsert<0xB74288>(0x54F560); // CPlaceable::AllocateMatrix
            HookFixedLinkListInsert<0xB74288>(0x54F4C0); // CPlaceable::AllocateStaticMatrix
        }
    }
}

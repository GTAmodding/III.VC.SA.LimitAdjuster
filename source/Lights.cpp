#include "Core.h"
#include "Adjusters.h"

// Coronas, shadows and lights. The slot arrays (coronas, static shadows) are searched and drawn by loops over every
// slot, so those loops run to a "high-water mark" (the highest slot in use, rounded up to 8 because some loops are
// unrolled 8 times) instead of the capacity. Only the search for a free slot goes up to the capacity.

namespace
{
    int32_t RoundUp8(int32_t value)
    {
        return std::max<int32_t>((value + 7) & ~7, 8);
    }

    struct SlotArray
    {
        const char* key = nullptr;
        uint32_t elemSize = 0;
        uint32_t usedOffset = 0;   // a dword that's non-zero while the slot is used
        limits::MovedArray* moved = nullptr;
        int32_t capacity = 0;      // compared against by the free-slot search
        int32_t highWater = 0;     // slots in use are below this
        int32_t highWater8 = 8;    // compared against by every other loop

        bool IsUsed(int32_t index) const
        {
            return *(uint32_t*)(moved->data + (size_t)index * elemSize + usedOffset) != 0;
        }

        // A new slot is always the first free one, so it is at most at the old mark: walk up past newly used slots,
        // then down past freed ones.
        void Recount()
        {
            auto mark = highWater;
            while (mark < capacity && IsUsed(mark))
                mark++;
            while (mark > 0 && !IsUsed(mark - 1))
                mark--;
            highWater = mark;
            highWater8 = std::min(RoundUp8(mark), capacity);
        }

        int Used() const
        {
            int used = 0;
            for (int32_t i = 0; i < highWater; i++)
                used += IsUsed(i);
            return used;
        }
    };

    struct SlotArrayDef
    {
        const char* key;
        uintptr_t base;
        uint32_t elemSize;
        uint32_t vanilla;
        uint32_t usedOffset;
        uint32_t ceiling;
        std::span<const uintptr_t> refs;
        std::span<const uintptr_t> loops;     // compares against the vanilla count that become the high-water mark
        std::span<const uintptr_t> freeSlot;  // compares against the vanilla count that become the capacity
    };

    // Moves the array and redirects its bounds. Returns nullptr when anything isn't as expected.
    SlotArray* MoveSlotArray(const SlotArrayDef& def)
    {
        auto setting = core::GetSetting(def.key, "unlimited");
        if (!setting.apply)
            return nullptr;
        if (!mem::CheckCompares(def.loops, def.vanilla, def.key) || !mem::CheckCompares(def.freeSlot, def.vanilla, def.key))
            return nullptr;

        auto capacity = std::max<uint32_t>(core::MaxCapacity(setting, def.ceiling), def.vanilla) & ~7u;
        auto moved = limits::MoveArray(def.key, def.base, def.elemSize, def.vanilla, capacity, def.refs);
        if (!moved)
            return nullptr;

        auto array = new SlotArray();
        array->key = def.key;
        array->elemSize = def.elemSize;
        array->usedOffset = def.usedOffset;
        array->moved = moved;
        array->capacity = (int32_t)capacity;
        array->highWater = (int32_t)def.vanilla;
        array->Recount();

        for (auto address : def.loops)
            mem::PatchCompare(address, &array->highWater8);
        for (auto address : def.freeSlot)
            mem::PatchCompare(address, &array->capacity);

        core::RegisterLimit({ def.key, [array] { return array->Used(); }, [array] { return array->capacity; }, [array] { return array->capacity; } });
        return array;
    }

    std::vector<SafetyHookMid> hooks;

    // Recounts at the start of a function that runs once per frame (the array's own update).
    template <SlotArray** Array, void (*After)() = nullptr>
    void RecountHook(SafetyHookContext&)
    {
        if (*Array)
        {
            (*Array)->Recount();
            if constexpr (After != nullptr)
                After();
        }
    }

    // Per-frame lists (stored shadows, point lights, bright lights, shiny texts): filled up to a counter that is
    // reset every frame, so only the "full?" compare changes.
    void MoveList(const char* key, uintptr_t base, uint32_t elemSize, uint32_t vanilla, uint32_t ceiling, uintptr_t counter, bool counter16,
                  std::span<const uintptr_t> refs, std::span<const uintptr_t> fullChecks)
    {
        auto setting = core::GetSetting(key, "unlimited");
        if (!setting.apply || !mem::CheckCompares(fullChecks, vanilla, key))
            return;

        auto capacity = std::max(core::MaxCapacity(setting, ceiling), vanilla);
        auto moved = limits::MoveArray(key, base, elemSize, vanilla, capacity, refs);
        if (!moved)
            return;

        auto limit = new int32_t((int32_t)capacity);
        for (auto address : fullChecks)
            mem::PatchCompare(address, limit);

        core::RegisterLimit({ key, [counter, counter16] { return counter16 ? (int)*(uint16_t*)counter : *(int*)counter; }, [limit] { return *limit; }, [limit] { return *limit; } });
    }

    // Shadow poly bunches come from a free list. When it runs empty another block is linked in.
    struct PolyBunches
    {
        uintptr_t head;
        uint32_t elemSize;
        uint32_t nextOffset;
        uint32_t total;
        uint32_t maximum;
    };
    PolyBunches polyBunches{};

    void GrowPolyBunches(SafetyHookContext&)
    {
        auto& p = polyBunches;
        if (*(uintptr_t*)p.head != 0 || p.total >= p.maximum)
            return;

        auto count = std::min<uint32_t>(std::max<uint32_t>(p.total / 2, 128), p.maximum - p.total);
        auto block = (uint8_t*)calloc(count, p.elemSize);
        if (!block)
            return;
        for (uint32_t i = 0; i < count; i++)
            *(uintptr_t*)(block + i * p.elemSize + p.nextOffset) = i + 1 < count ? (uintptr_t)(block + (i + 1) * p.elemSize) : 0;
        *(uintptr_t*)p.head = (uintptr_t)block;
        p.total += count;
    }

    void HookPolyBunches(uintptr_t head, uint32_t elemSize, uint32_t nextOffset, uint32_t vanilla, uintptr_t emptyCheck)
    {
        auto setting = core::GetSetting("PolyBunches", "unlimited");
        if (!setting.apply)
            return;
        polyBunches = { head, elemSize, nextOffset, vanilla, std::max(core::MaxCapacity(setting, 200000), vanilla) };
        hooks.push_back(safetyhook::create_mid(emptyCheck, GrowPolyBunches));
        core::RegisterLimit({ "PolyBunches", [] { return -1; }, [] { return (int)polyBunches.total; }, [] { return (int)polyBunches.maximum; } });
    }

    SlotArray* coronas = nullptr;
    SlotArray* staticShadows = nullptr;

    // ---------------------------------------------------------------------------------------------------------------
    // GTA III 1.0

    const uintptr_t coronaRefsIII[] =
    {
        0x4F8F8C, 0x4F8F92, 0x4F9027, 0x4F906F, 0x4F907F, 0x4F9086, 0x4F908D, 0x4F9099, 0x4F90A0, 0x4F90A9, 0x4F90B2, 0x4F90BB, 0x4F90C4, 0x4F90CD,
        0x4F90E4, 0x4F90F0, 0x4F90F7, 0x4F9104, 0x4F910D, 0x4F91D2, 0x4F91F9, 0x4F9265, 0x4F926F, 0x4F9275, 0x4F927B, 0x4F9281, 0x4F9287, 0x4F928D,
        0x4F9293, 0x4F9299, 0x4F92A0, 0x4F92A9, 0x4F92AF, 0x4F92D6, 0x4F92DC, 0x4F9303, 0x4F930A, 0x4F931A, 0x4F93E7, 0x4F9421, 0x4F946A, 0x4F947E,
        0x4F94DD, 0x4F9516, 0x4F9550, 0x4F9591, 0x4F95F9, 0x4F962E, 0x4F9667, 0x4F96A8, 0x4F96F8, 0x4F9770, 0x4F9784, 0x4F9792, 0x4F98CC, 0x4F990D,
        0x4F9925, 0x4F99BF, 0x4F99CC, 0x4F99DD, 0x4F99EA, 0x4F99F7, 0x4F9A22, 0x4F9A2B, 0x4F9A46, 0x4F9A52, 0x4F9A7D, 0x4F9B6B, 0x4F9B72, 0x4F9B79,
        0x4F9B80, 0x4F9B87, 0x4F9B8E, 0x4F9B95, 0x4F9B9C, 0x4F9C0B, 0x4F9C19, 0x4F9C26, 0x4F9C2F, 0x4F9C3C, 0x4F9C49, 0x4F9C93, 0x4F9C9D, 0x4F9CD3,
        0x4F9CDD, 0x4F9CE3, 0x4F9CEA, 0x4F9CF7, 0x4F9D19, 0x4F9D40, 0x4F9D92, 0x4F9E87, 0x4F9EBB, 0x4F9ECB, 0x4F9EEC, 0x4F9FE7, 0x4F9FF1, 0x4F9FFB,
        0x4FA005, 0x4FA00F, 0x4FA019, 0x4FA023, 0x4FA02D, 0x4FA134, 0x4FA163, 0x4FA17A, 0x4FA185, 0x4FA19F, 0x4FA1A6, 0x4FA1B6, 0x4FA1BD, 0x4FA1C4,
        0x4FA1CB, 0x4FA1D2, 0x4FA1D9, 0x4FA1E8, 0x4FA1FB, 0x4FA20E, 0x4FA217, 0x4FA21D, 0x4FA229, 0x4FA233, 0x4FA23D, 0x4FA249, 0x4FA252, 0x4FA25B,
        0x4FA265, 0x4FA26F, 0x4FA277, 0x4FA281, 0x4FA287, 0x4FA291, 0x4FA29B, 0x4FA32C, 0x4FA340, 0x4FA34E, 0x4FA359, 0x4FA362, 0x4FA36B, 0x4FA375,
    };
    const uintptr_t coronaLoopsIII[] = { 0x4F8FA7, 0x4F9860, 0x4F9ABD, 0x4F9BA7, 0x4F9F2A, 0x4FA03B, 0x4FA12C, 0x4FA13A, 0x4FA324, 0x4FA332 };
    const uintptr_t coronaFreeIII[] = { 0x4FA15B, 0x4FA16A };

    const uintptr_t staticShadowRefsIII[] =
    {
        0x512CBA, 0x512CC4, 0x512CCE, 0x512CD8, 0x512CE2, 0x512CEC, 0x512CF6, 0x512D00, 0x512D0A, 0x512D14, 0x512D1E, 0x512D28, 0x512D32, 0x512D3C,
        0x512D46, 0x512D50, 0x51321C, 0x513224, 0x51323F, 0x513277, 0x5132E2, 0x51331F, 0x513360, 0x513378, 0x513390, 0x5133A8, 0x5133BC, 0x5133C3,
        0x5133CD, 0x5133D9, 0x5133E7, 0x5133F1, 0x5133F7, 0x5133FD, 0x51340E, 0x513418, 0x513424, 0x513435, 0x513454, 0x513482, 0x513488, 0x513492,
        0x51349E, 0x5134A8, 0x5134B2, 0x5134B8, 0x5134BE, 0x5134C8, 0x5134D0, 0x5134D9, 0x5134E2, 0x5134EC, 0x5134F6, 0x513504, 0x51350E, 0x513514,
        0x51351B, 0x513526, 0x514635, 0x51463C, 0x514643, 0x51464A, 0x514651, 0x514658, 0x51465F, 0x514666, 0x51468C, 0x514699, 0x5146A6, 0x5146B7,
        0x5146F6, 0x514708, 0x514712, 0x514724, 0x51472E, 0x514778, 0x5147D4, 0x5147DB, 0x5147E2, 0x5148BA, 0x514931, 0x51495A, 0x514988, 0x5149B0,
        0x5149D4, 0x5149E2, 0x5149F0, 0x514B5B, 0x514BB5, 0x514BBB, 0x514BC9, 0x514BD5, 0x514BDB, 0x514BE1, 0x514C0C, 0x514C12, 0x514C20, 0x514C26,
        0x514C2C, 0x514C32, 0x516BE8, 0x516BF2, 0x516BFB, 0x516C04, 0x516C0D, 0x516C28,
    };
    const uintptr_t staticShadowLoopsIII[] = { 0x512D5E, 0x513214, 0x51322B, 0x514671, 0x5146CA, 0x5148C8, 0x5148E1, 0x516C33 };
    const uintptr_t staticShadowFreeIII[] = { 0x51344C, 0x51345B };

    const uintptr_t storedShadowRefsIII[] =
    {
        0x513772, 0x51377C, 0x513781, 0x5137AB, 0x5137B5, 0x5137BF, 0x5137C9, 0x5137D0, 0x5137DA, 0x5137E8, 0x5137F2, 0x5137FC, 0x513802, 0x51381A,
        0x513820, 0x51405B, 0x51407A, 0x514084, 0x514093, 0x51409D, 0x5140AC, 0x5140B6, 0x5140C5, 0x5140CF, 0x5140DE, 0x5140E8, 0x5140F7, 0x514101,
        0x514113, 0x51411F, 0x514137, 0x514165, 0x514187, 0x514198, 0x5141AE, 0x5141D6, 0x5141E0, 0x5141F8, 0x514202, 0x514214, 0x51423C, 0x514270,
        0x51429A, 0x5142BE, 0x5142CC, 0x5142DA,
    };
    const uintptr_t storedShadowFullIII[] = { 0x513750 };

    const uintptr_t pointLightRefsIII[] =
    {
        0x510878, 0x510882, 0x510887, 0x5108A8, 0x5108D3, 0x5108E1, 0x5108F5, 0x5108FF, 0x510909, 0x510934, 0x510940, 0x51094C, 0x510988, 0x5109A8,
        0x5109B0, 0x5109E1, 0x510A0D, 0x510A65, 0x510AF1, 0x510AF9, 0x510B03, 0x510B49, 0x510B55, 0x510B61, 0x510CA7, 0x510CB6, 0x510D08, 0x510D20,
        0x510D26, 0x510D3C, 0x510D42, 0x510EE2, 0x510EF2, 0x510EFC, 0x510F04, 0x510F9D, 0x511009, 0x511019, 0x511028, 0x511032, 0x51103A, 0x511044,
        0x511208, 0x51123A, 0x511273, 0x511344, 0x511378, 0x51138E, 0x5114FC, 0x511510, 0x5116D8, 0x51170D, 0x511749, 0x51191C, 0x516FCA, 0x516FE0,
        0x516FF6, 0x51700A, 0x517025, 0x517038, 0x51704A, 0x517089, 0x517224, 0x517231, 0x517244, 0x51725A, 0x517270, 0x517295, 0x5172AE, 0x5172C9,
        0x51730E,
    };
    const uintptr_t pointLightFullIII[] = { 0x510819 };

    const uintptr_t brightLightRefsIII[] =
    {
        0x51988A, 0x5199E8, 0x5199F2, 0x5199FC, 0x519A0A, 0x519B21, 0x519B2E, 0x519B34, 0x519B4F, 0x519B5C, 0x519B62, 0x519B77, 0x519B84, 0x519B8E,
        0x519C5A, 0x519C67, 0x519C73, 0x519C82, 0x519C9D, 0x519CAA, 0x519CB0, 0x519CBF, 0x519CD4, 0x519CE1, 0x519CE7, 0x519CF6, 0x519DD9, 0x519DE6,
        0x519DF2, 0x519E01, 0x519E1C, 0x519E29, 0x519E2F, 0x519E3E, 0x519E53, 0x519E60, 0x519E66, 0x519E75, 0x519F59, 0x519F66, 0x519F72, 0x519F81,
        0x519F9C, 0x519FA9, 0x519FAF, 0x519FBE, 0x519FD3, 0x519FE0, 0x519FE6, 0x519FF5, 0x51A0D9, 0x51A0E6, 0x51A0F2, 0x51A101, 0x51A11C, 0x51A129,
        0x51A12F, 0x51A13E, 0x51A153, 0x51A160, 0x51A166, 0x51A175, 0x51A27A, 0x51A2A4, 0x51A2D1, 0x51A4B5, 0x51A4BB, 0x51A4DD, 0x51A500, 0x51A524,
        0x51A548, 0x51A573, 0x51A57D, 0x51A587, 0x51A591,
    };
    const uintptr_t brightLightFullIII[] = { 0x51A43B };

    const uintptr_t shinyTextRefsIII[] =
    {
        0x51A636, 0x51A63C, 0x51A646, 0x51A651, 0x51A6B6, 0x51A6D0, 0x51A6DC, 0x51A6E8, 0x51A6F4, 0x51A700, 0x51A712, 0x51A71E, 0x51A72A, 0x51A736,
        0x51A742, 0x51A754, 0x51A760, 0x51A76C, 0x51A778, 0x51A784, 0x51A79C, 0x51A7A8, 0x51A7B4, 0x51A7CD, 0x51A870, 0x51A88A, 0x51A896, 0x51A8A2,
        0x51A8AE, 0x51A8BA, 0x51A8CC, 0x51A8D8, 0x51A8E4, 0x51A8F0, 0x51A8FC, 0x51A90E, 0x51A91A, 0x51A926, 0x51A932, 0x51A93E, 0x51A956, 0x51A962,
        0x51A96E, 0x51A987, 0x51AB56, 0x51AB5C, 0x51AB7F, 0x51ABA2, 0x51ABC6, 0x51ABEA, 0x51AC14, 0x51AC1E, 0x51AC28, 0x51AC32, 0x51AC3C, 0x51AC46,
        0x51AC50, 0x51AC5A, 0x51AC6D, 0x51AC7A, 0x51AC87, 0x51AC94, 0x51AC9C, 0x51ACBF, 0x51ACF3, 0x51ACF9, 0x51AD2F, 0x51AD35, 0x51AD6F,
    };
    const uintptr_t shinyTextFullIII[] = { 0x51AADB };

    // ---------------------------------------------------------------------------------------------------------------
    // GTA Vice City 1.0

    const uintptr_t coronaRefsVC[] =
    {
        0x54243C, 0x542450, 0x54245E, 0x542469, 0x542472, 0x54247B, 0x542485, 0x5425BF, 0x5425FC, 0x54261A, 0x542625, 0x542645, 0x542655, 0x54265C,
        0x542663, 0x54266A, 0x542671, 0x542678, 0x542688, 0x54269F, 0x5426C3, 0x5426C9, 0x5426D3, 0x5426DD, 0x5426E7, 0x5426F4, 0x5426FD, 0x542706,
        0x542710, 0x54271A, 0x542722, 0x542729, 0x54272F, 0x542739, 0x542743, 0x542748, 0x542786, 0x542886, 0x5428A7, 0x5428C4, 0x5428E1, 0x5428FE,
        0x54291B, 0x542938, 0x54295C, 0x54301D, 0x543042, 0x54304C, 0x543062, 0x54306C, 0x543082, 0x54308C, 0x5430A2, 0x5430AC, 0x5430C2, 0x5430CC,
        0x5430E2, 0x5430EC, 0x543105, 0x543112, 0x54317F, 0x543192, 0x54319F, 0x5431A8, 0x5431B5, 0x543208, 0x543212, 0x543245, 0x54324F, 0x54326D,
        0x54328F, 0x5432B6, 0x543308, 0x5433F7, 0x54342B, 0x54343B, 0x54345C, 0x543577, 0x5435BB, 0x5435C8, 0x5435D9, 0x5435E1, 0x5435EA, 0x5435F3,
        0x5435FC, 0x543605, 0x54360E, 0x543617, 0x543628, 0x543638, 0x543644, 0x54364B, 0x543658, 0x543661, 0x543722, 0x543743, 0x5437F8, 0x543813,
        0x543819, 0x54381F, 0x543825, 0x54382B, 0x543831, 0x543837, 0x54383D, 0x543844, 0x543852, 0x543859, 0x54387D, 0x543884, 0x5438AF, 0x5438B6,
        0x5438C4, 0x543993, 0x5439CD, 0x543A0B, 0x543A16, 0x543A2A, 0x543A89, 0x543AC2, 0x543AFC, 0x543B3D, 0x543BA5, 0x543BDA, 0x543C13, 0x543C54,
        0x543CA4, 0x543D17, 0x543D2B, 0x543D39, 0x543E78, 0x543EB2, 0x543ECA, 0x543F65, 0x543F75, 0x543F85, 0x543F9B, 0x543FB0, 0x543FBD, 0x543FF8,
        0x54400F, 0x54401B, 0x544046, 0x5441FC, 0x544202,
    };
    const uintptr_t coronaLoopsVC[] = { 0x542434, 0x542442, 0x5425B7, 0x5425C5, 0x542955, 0x54310C, 0x54349A, 0x543E0C, 0x544086, 0x544217 };
    const uintptr_t coronaFreeVC[] = { 0x5425F4, 0x542603 };

    const uintptr_t staticShadowRefsVC[] =
    {
        0x569618, 0x569622, 0x56962B, 0x569634, 0x569664, 0x56966E, 0x569679, 0x56C371, 0x56C387, 0x56C3A3, 0x56C3B9, 0x56C3D3, 0x56C3E1, 0x56C3EF,
        0x56C56B, 0x56C5B5, 0x56C5BB, 0x56C5C9, 0x56C5D5, 0x56C5DB, 0x56C5E1, 0x56C60C, 0x56C612, 0x56C620, 0x56C626, 0x56C62C, 0x56C632, 0x56C6E5,
        0x56C6EC, 0x56C6F3, 0x56C6FA, 0x56C701, 0x56C708, 0x56C70F, 0x56C716, 0x56C73C, 0x56C749, 0x56C757, 0x56C7AA, 0x56C7E6, 0x56C7F8, 0x56C802,
        0x56C814, 0x56C81E, 0x56C931, 0x56C9B7, 0x56C9C2, 0x56C9D0, 0x56CB9A, 0x56E92C, 0x56E934, 0x56E94F, 0x56E972, 0x56E998, 0x56E9C3, 0x56E9EE,
        0x56EA1A, 0x56EA32, 0x56EA4A, 0x56EA62, 0x56EA76, 0x56EA7D, 0x56EA87, 0x56EA93, 0x56EAA1, 0x56EAAB, 0x56EAB1, 0x56EAB7, 0x56EAC6, 0x56EAD0,
        0x56EADD, 0x56EAEA, 0x56EB13, 0x56EB1D, 0x56EB3C, 0x56EB71, 0x56EB77, 0x56EB81, 0x56EB8D, 0x56EB97, 0x56EBA1, 0x56EBA7, 0x56EBAD, 0x56EBB5,
        0x56EBBD, 0x56EBC6, 0x56EBCF, 0x56EBD9, 0x56EBE3, 0x56EBED, 0x56EBFB, 0x56EC05, 0x56EC0C, 0x56EC19, 0x56EC24, 0x56EFFD, 0x56F007, 0x56F011,
        0x56F01B, 0x56F025, 0x56F02F, 0x56F039, 0x56F043, 0x56F04D, 0x56F057, 0x56F061, 0x56F06B, 0x56F075, 0x56F07F, 0x56F089, 0x56F093,
    };
    const uintptr_t staticShadowLoopsVC[] = { 0x569681, 0x56C721, 0x56C7BD, 0x56CBA8, 0x56CC10, 0x56E924, 0x56E93B, 0x56F0A1 };
    const uintptr_t staticShadowFreeVC[] = { 0x56EB34, 0x56EB43 };

    const uintptr_t storedShadowRefsVC[] =
    {
        0x56CCA5, 0x56CCD1, 0x56CCDB, 0x56CCF1, 0x56CCFB, 0x56CD11, 0x56CD1B, 0x56CD31, 0x56CD3B, 0x56CD51, 0x56CD5B, 0x56CD71, 0x56CD7B, 0x56CD94,
        0x56CDA0, 0x56CDB8, 0x56CDE5, 0x56CE08, 0x56CE53, 0x56CE69, 0x56CE96, 0x56CEA0, 0x56CEB8, 0x56CEC2, 0x56CED4, 0x56CEEA, 0x56CF06, 0x56CF1C,
        0x56CF36, 0x56CF44, 0x56CF52, 0x56E6E2,
    };
    const uintptr_t storedShadowFullVC[] = { 0x56E6C0 };

    const uintptr_t pointLightRefsVC[] =
    {
        0x5666CC, 0x56671A, 0x566732, 0x566738, 0x56674E, 0x566754, 0x5668F2, 0x5668FE, 0x566908, 0x566910, 0x56699E, 0x566A0D, 0x566A19, 0x566A28,
        0x566A32, 0x566A3A, 0x566A44, 0x566C01, 0x566C33, 0x566C6C, 0x566D1D, 0x566E66, 0x566E7C, 0x567013, 0x567023, 0x5671E2, 0x567217, 0x567253,
        0x567462, 0x567482, 0x56748A, 0x5674BB, 0x5674E7, 0x56753F, 0x5675D1, 0x5675D9, 0x5675E3, 0x567629, 0x567635, 0x567641, 0x5677E6, 0x5677F0,
        0x5677F5, 0x567814, 0x56783E, 0x56784C, 0x567860, 0x56786A, 0x567874, 0x5678A4, 0x5678B0, 0x5678BC, 0x567AAC, 0x568FB2, 0x568FBF, 0x568FD2,
        0x568FE8, 0x568FFE, 0x569020, 0x569033, 0x569045, 0x569084,
    };
    const uintptr_t pointLightFullVC[] = { 0x567789 };

    const uintptr_t brightLightRefsVC[] =
    {
        0x5718C5, 0x5718CB, 0x5718ED, 0x571910, 0x571934, 0x571958, 0x571983, 0x57198D, 0x571997, 0x5719A1, 0x571ACC, 0x571C2A, 0x571C34, 0x571C3E,
        0x571C4C, 0x571D6F, 0x571D7C, 0x571D82, 0x571D9D, 0x571DAA, 0x571DB0, 0x571DC5, 0x571DD2, 0x571DDC, 0x571EA9, 0x571EB6, 0x571EC2, 0x571ED1,
        0x571EEC, 0x571EF9, 0x571EFF, 0x571F0E, 0x571F23, 0x571F30, 0x571F36, 0x571F45, 0x572029, 0x572036, 0x572042, 0x572051, 0x57206C, 0x572079,
        0x57207F, 0x57208E, 0x5720A3, 0x5720B0, 0x5720B6, 0x5720C5, 0x5721A9, 0x5721B6, 0x5721C2, 0x5721D1, 0x5721EC, 0x5721F9, 0x5721FF, 0x57220E,
        0x572223, 0x572230, 0x572236, 0x572245, 0x572329, 0x572336, 0x572342, 0x572351, 0x57236C, 0x572379, 0x57237F, 0x57238E, 0x5723A3, 0x5723B0,
        0x5723B6, 0x5723C5, 0x5724C8, 0x5724F1, 0x57251A, 0x57505C,
    };
    const uintptr_t brightLightFullVC[] = { 0x57184B };

    // ---------------------------------------------------------------------------------------------------------------
    // GTA San Andreas 1.0 US

    const uintptr_t coronaRefsSA[] =
    {
        0x6FAACF, 0x6FAEA0, 0x6FAF42, 0x6FB648, 0x6FB6CF, 0x6FC2E8, 0x6FC318, 0x6FC341, 0x6FC34A, 0x6FC351, 0x6FC358, 0x6FC365, 0x6FC36B, 0x6FC37A,
        0x6FC384, 0x6FC38E, 0x6FC398, 0x6FC3A1, 0x6FC3B9, 0x6FC3C3, 0x6FC3CD, 0x6FC3D7, 0x6FC3E1, 0x6FC3EB, 0x6FC3F1, 0x6FC3FB, 0x6FC403, 0x6FC40D,
        0x6FC417, 0x6FC432, 0x6FC44A, 0x6FC454, 0x6FC45A, 0x6FC478, 0x6FC496, 0x6FC4AC, 0x6FC4B2, 0x6FC538, 0x6FC555, 0x6FC56D,
    };
    const uintptr_t coronaLoopsSA[] = { 0x6FC2EF, 0x6FC2F5, 0x6FC53F, 0x6FC545 };
    const uintptr_t coronaFreeSA[] = { 0x6FC321, 0x6FC327 };
    constexpr uintptr_t coronaBaseSA = 0xC3E058;

    // SA's corona and static shadow loops also use end pointers and counts; they follow the high-water mark.
    int32_t coronaUpdateEndSA = 0;
    int32_t coronaReflectionsClearEndSA = 0;
    int32_t coronaReflectionsEndSA = 0;

    void AfterCoronaRecountSA()
    {
        auto base = (int32_t)(uintptr_t)coronas->moved->data;
        auto count = coronas->highWater8;
        coronaUpdateEndSA = base + count * 0x3C;
        coronaReflectionsClearEndSA = base + count * 0x3C + 0x36;
        coronaReflectionsEndSA = base + count * 0x3C + 0x30;
        mem::WriteDword(0x6FAF4A, count); // CCoronas::Render loop count
    }

    void PublishCoronaSA(SafetyHookContext& regs)
    {
        // Registration also runs after Update. Publish a new slot immediately so another registration finds its
        // identifier and Render includes it in this frame, including the sun's visibility and fade state.
        auto count = (int32_t)(regs.ecx / 0x3C + 1);
        if (count > coronas->highWater && count <= coronas->capacity)
        {
            coronas->highWater = count;
            coronas->highWater8 = std::min(RoundUp8(count), coronas->capacity);
            AfterCoronaRecountSA();
        }
    }

    const uintptr_t staticShadowRefsSA[] =
    {
        0x706E7B, 0x707F42, 0x708362, 0x70837E, 0x70B740, 0x70B748, 0x70B753, 0x70B76D, 0x70B77C, 0x70B933, 0x70B939, 0x70B968, 0x70B976, 0x70B97D,
        0x70B98C, 0x70B993, 0x70B99A, 0x70BB28, 0x70BB30, 0x70BB58, 0x70BB86, 0x70BB90, 0x70BB9B, 0x70BBA5, 0x70BBAF, 0x70BBB9, 0x70BBBF, 0x70BBC5,
        0x70BBCB, 0x70BBD4, 0x70BBF4, 0x70BBFE, 0x70BC08, 0x70BC15, 0x70BC1B, 0x70BC22, 0x70BC28, 0x70BC33, 0x70BC5E, 0x70BC74, 0x70BC9B, 0x70BCB3,
        0x70BCCC, 0x70BCDD, 0x70BCEE, 0x70BCFF, 0x70BD0C, 0x70BD26, 0x70BD30, 0x70BD3B, 0x70BD46, 0x70BD50, 0x70BD5B, 0x70BD65, 0x70BD71, 0x70BD7C,
        0x70BD82, 0x70BD89,
    };
    const uintptr_t staticShadowLoopsSA[] = { 0x70BB3D, 0x70BC49 };
    const uintptr_t staticShadowFreeSA[] = { 0x70BB61, 0x70BB67 };

    int32_t staticShadowEnd4SA = 0;
    int32_t staticShadowEnd3BSA = 0;
    int32_t staticShadowEnd36SA = 0;

    void AfterStaticShadowRecountSA()
    {
        auto end = (int32_t)(uintptr_t)staticShadows->moved->data + staticShadows->highWater8 * 0x40;
        staticShadowEnd4SA = end + 0x04;
        staticShadowEnd3BSA = end + 0x3B;
        staticShadowEnd36SA = end + 0x36;
    }

    const uintptr_t storedShadowRefsSA[] =
    {
        0x7073B1, 0x70A9EF, 0x70AA12, 0x70AA1A, 0x70B0F2, 0x70B169, 0x70B17D, 0x70B18F, 0x70B23D, 0x70B2B4, 0x70B2C5, 0x70B2D3, 0x70B391, 0x70B407,
        0x70B411, 0x70B427, 0x70B4ED, 0x70B56C, 0x70B57B, 0x70B596,
    };
    const uintptr_t storedShadowFullSA[] = { 0x707398 };

    const uintptr_t pointLightRefsSA[] =
    {
        0x6FFBE8, 0x6FFC00, 0x6FFC06, 0x6FFC47, 0x6FFC76, 0x6FFCFE, 0x6FFD10, 0x6FFD22, 0x6FFD7A, 0x6FFDA8, 0x6FFDB5, 0x6FFDBF, 0x6FFDCB, 0x6FFE0B,
        0x6FFE24, 0x6FFE32, 0x6FFEA8, 0x6FFEC0, 0x6FFEC6, 0x6FFF06, 0x6FFF35, 0x6FFF9A, 0x6FFFAA, 0x6FFFB8, 0x7001E6, 0x7001F4, 0x7001FF, 0x700219,
        0x700233, 0x70023D, 0x700247, 0x700259, 0x700263, 0x70026E, 0x7002A5, 0x7002B1, 0x7002BB, 0x700362, 0x70039F, 0x7003AD, 0x7003B9, 0x7003CC,
        0x7003D9, 0x7003E3, 0x7003F0, 0x700412, 0x700424, 0x700435, 0x70043D, 0x70051A, 0x70052C, 0x70053A, 0x700544, 0x7005A3, 0x700625, 0x700633,
        0x700641, 0x70064B, 0x700655, 0x700661, 0x7007B7, 0x7007C7, 0x7007D7, 0x70086D, 0x7008AA, 0x7008BC, 0x700979, 0x700987, 0x700B05, 0x700B15,
        0x700B25, 0x70801E,
    };
    const uintptr_t pointLightFullSA[] = { 0x70017C };

    const uintptr_t brightLightRefsSA[] = { 0x72424A, 0x7247D8, 0x7247F4, 0x72480E, 0x724828, 0x724842, 0x72485C, 0x724862, 0x72486C, 0x724877 };
    const uintptr_t brightLightFullSA[] = { 0x724783 };

    const uintptr_t shinyTextRefsSA[] =
    {
        0x7248FB, 0x724BB8, 0x724BDF, 0x724BF9, 0x724C13, 0x724C2D, 0x724C4B, 0x724C55, 0x724C5F, 0x724C69, 0x724C73, 0x724C7D, 0x724C87, 0x724C91,
        0x724C9B, 0x724CA5, 0x724CAB, 0x724CB1, 0x724CB7, 0x724CC4, 0x724CCB, 0x724CEA, 0x724CF1, 0x724D06, 0x724D0D, 0x724D22,
    };
    const uintptr_t shinyTextFullSA[] = { 0x724B68 };

    void InstallSA()
    {
        // Corona loops in Init/Update/Render/RenderReflections use counts and end pointers.
        const uintptr_t endPointers[] = { 0x6FAEB5, 0x6FB656, 0x6FB9B6 };
        const uint32_t endValues[] = { coronaBaseSA + 64 * 0x3C, coronaBaseSA + 64 * 0x3C + 0x36, coronaBaseSA + 64 * 0x3C + 0x30 };
        bool endsOk = true;
        for (size_t i = 0; i < std::size(endPointers); i++)
            endsOk = endsOk && mem::IsCompare(endPointers[i], endValues[i]);
        const mem::Expect counts[] = { { 0x6FAAD4, 64 }, { 0x6FAF4A, 64 }, { 0x6FC370, 0xC3E038 } };

        if (endsOk && mem::CheckValues(counts, "Coronas") && *(uint8_t*)0x6FC36F == 0xA3)
        {
            coronas = MoveSlotArray({ "Coronas", coronaBaseSA, 0x3C, 64, 0xC, 32760, coronaRefsSA, coronaLoopsSA, coronaFreeSA });
            if (coronas)
            {
                AfterCoronaRecountSA();
                mem::PatchCompare(0x6FAEB5, &coronaUpdateEndSA);
                mem::PatchCompare(0x6FB656, &coronaReflectionsClearEndSA);
                mem::PatchCompare(0x6FB9B6, &coronaReflectionsEndSA);
                mem::WriteDword(0x6FAAD4, coronas->capacity); // Clear all identifiers on New Game and reload as well.
                hooks.push_back(safetyhook::create_mid(0x6FC36F, PublishCoronaSA));
                hooks.push_back(safetyhook::create_mid(0x6FADF0, RecountHook<&coronas, AfterCoronaRecountSA>));
            }
        }
        else
        {
            core::Log("Coronas: the game code is not the expected one (another mod changed it?), left as it is");
        }

        // Static shadow loops use end pointers into the next array.
        constexpr uint32_t shadowEnd = 0xC4A030 + 48 * 0x40;
        const uintptr_t shadowEnds4[] = { 0x706E88, 0x707F96, 0x708614 };
        const uintptr_t shadowEnds3B[] = { 0x708376 };
        const uintptr_t shadowEnds36[] = { 0x7083BC, 0x70866E };
        if (mem::CheckCompares(shadowEnds4, shadowEnd + 0x04, "StaticShadows") && mem::CheckCompares(shadowEnds3B, shadowEnd + 0x3B, "StaticShadows") &&
            mem::CheckCompares(shadowEnds36, shadowEnd + 0x36, "StaticShadows"))
        {
            staticShadows = MoveSlotArray({ "StaticShadows", 0xC4A030, 0x40, 48, 0x4, 32760, staticShadowRefsSA, staticShadowLoopsSA, staticShadowFreeSA });
            if (staticShadows)
            {
                AfterStaticShadowRecountSA();
                for (auto address : shadowEnds4)
                    mem::PatchCompare(address, &staticShadowEnd4SA);
                for (auto address : shadowEnds3B)
                    mem::PatchCompare(address, &staticShadowEnd3BSA);
                for (auto address : shadowEnds36)
                    mem::PatchCompare(address, &staticShadowEnd36SA);
                hooks.push_back(safetyhook::create_mid(0x707F40, RecountHook<&staticShadows, AfterStaticShadowRecountSA>));
            }
        }

        HookPolyBunches(0xC403D8, 0x68, 0x54, 360, 0x709902);
        MoveList("ShadowsStored", 0xC40430, 0x34, 48, 4096, 0xC403DC, true, storedShadowRefsSA, storedShadowFullSA);
        // An older skygfx colours the point light fog from the game's own array
        if (core::OlderSkyGfxInstalled())
            core::Log("PointLights: an older skygfx was found, the game's own list is kept");
        else
            MoveList("PointLights", 0xC3F0E0, 0x30, 32, 1024, 0xC3F0D0, false, pointLightRefsSA, pointLightFullSA);
        MoveList("BrightLights", 0xC7CB58, 0x38, 32, 4096, 0xC7C6FC, false, brightLightRefsSA, brightLightFullSA);
        MoveList("ShinyTexts", 0xC7D258, 0x58, 32, 4096, 0xC7C6F8, false, shinyTextRefsSA, shinyTextFullSA);
    }
}

namespace limits
{
    void InstallLights()
    {
        if (core::IsIII())
        {
            coronas = MoveSlotArray({ "Coronas", 0x72E518, 0x80, 56, 0x0, 32760, coronaRefsIII, coronaLoopsIII, coronaFreeIII });
            if (coronas)
                hooks.push_back(safetyhook::create_mid(0x4F8EC0, RecountHook<&coronas>));
            staticShadows = MoveSlotArray({ "StaticShadows", 0x773BE8, 0x40, 64, 0x4, 32760, staticShadowRefsIII, staticShadowLoopsIII, staticShadowFreeIII });
            if (staticShadows)
                hooks.push_back(safetyhook::create_mid(0x516BE0, RecountHook<&staticShadows>));
            HookPolyBunches(0x8F435C, 0x6C, 0x68, 300, 0x5168B5);
            MoveList("ShadowsStored", 0x779058, 0x30, 48, 4096, 0x95CCEE, true, storedShadowRefsIII, storedShadowFullIII);
            MoveList("PointLights", 0x7096D0, 0x2C, 32, 1024, 0x95CC3E, true, pointLightRefsIII, pointLightFullIII);
            MoveList("BrightLights", 0x733A68, 0x38, 32, 4096, 0x8F2C70, false, brightLightRefsIII, brightLightFullIII);
            MoveList("ShinyTexts", 0x87B2D8, 0x58, 32, 4096, 0x8F1AAC, false, shinyTextRefsIII, shinyTextFullIII);
        }
        else if (core::IsVC())
        {
            coronas = MoveSlotArray({ "Coronas", 0x815128, 0x68, 56, 0xC, 32760, coronaRefsVC, coronaLoopsVC, coronaFreeVC });
            if (coronas)
                hooks.push_back(safetyhook::create_mid(0x544130, RecountHook<&coronas>));
            staticShadows = MoveSlotArray({ "StaticShadows", 0x8615A0, 0x40, 48, 0x4, 32760, staticShadowRefsVC, staticShadowLoopsVC, staticShadowFreeVC });
            if (staticShadows)
                hooks.push_back(safetyhook::create_mid(0x569610, RecountHook<&staticShadows>));
            HookPolyBunches(0x9B48D4, 0x68, 0x54, 380, 0x56B389);
            MoveList("ShadowsStored", 0x868C70, 0x34, 48, 4096, 0xA10AAC, true, storedShadowRefsVC, storedShadowFullVC);
            MoveList("PointLights", 0x7E4FE0, 0x2C, 32, 1024, 0x9751B0, false, pointLightRefsVC, pointLightFullVC);
            MoveList("BrightLights", 0x818848, 0x38, 32, 4096, 0x97F2BC, false, brightLightRefsVC, brightLightFullVC);
        }
        else if (core::IsSA())
        {
            InstallSA();
        }
    }
}

#include "Core.h"
#include "Adjusters.h"

// handling.cfg: every line names a vehicle from a fixed table, and an unknown name writes past the handling data.
// The handling manager moves into a block with room for every id a byte can hold (the game reads handling ids as
// bytes), names are matched exactly, and new names in handling.cfg get new ids. Bike, flying and boat data keep their
// "id - first id" indexing; their arrays now reach up to the last id.

namespace
{
    constexpr uint32_t MaxIds = 256;
    constexpr uint32_t ScratchId = MaxIds - 1; // where lines go once every id is taken

    uintptr_t vanillaTable = 0;
    std::vector<std::string> names;
    uint32_t maxIds = MaxIds;
    SafetyHookInline findExactWordHook;
    SafetyHookInline getHandlingIdHook;

    std::string Token(const char* text)
    {
        std::string token;
        while (*text && *text != ' ' && *text != '\t' && *text != '\r' && *text != '\n' && token.size() < 13)
            token.push_back(*text++);
        return token;
    }

    int Find(const std::string& name)
    {
        for (size_t i = 0; i < names.size(); i++)
        {
            if (names[i] == name)
                return (int)i;
        }
        return -1;
    }

    int __fastcall FindExactWordHook(void* mgr, void*, const char* word, const char* table, int entrySize, int count)
    {
        if ((uintptr_t)table != vanillaTable)
            return findExactWordHook.thiscall<int>(mgr, word, table, entrySize, count);

        auto name = Token(word);
        if (auto id = Find(name); id >= 0)
            return id;

        if (names.size() < std::min(maxIds, ScratchId))
        {
            names.push_back(name);
            core::Log("Handling: '{}' gets id {}", name, names.size() - 1);
            return (int)names.size() - 1;
        }

        core::Log("Handling: no free id for '{}', line ignored", name);
        return (int)ScratchId;
    }

    int __fastcall GetHandlingIdHook(void*, void*, const char* name)
    {
        auto id = Find(Token(name));
        if (id < 0)
        {
            core::Log("Handling: vehicle handling '{}' is not in handling.cfg, using the first entry", Token(name));
            return 0;
        }
        return id;
    }

    void LoadNames(uintptr_t table, uint32_t count)
    {
        vanillaTable = table;
        for (uint32_t i = 0; i < count; i++)
            names.emplace_back(Token((const char*)(table + i * 14)));
    }

    // A sub-array (bike, flying, boat data) indexed by id - first.
    struct SubArray
    {
        uint32_t oldOffset;
        uint32_t elemSize;
        uint32_t first;
        uint32_t vanillaCount;
        uint32_t newOffset = 0;
    };

    struct HandlingDef
    {
        uintptr_t manager;
        uint32_t managerSize;
        uint32_t stdOffset;
        uint32_t stdSize;
        uint32_t vanillaCount;
        uintptr_t table;
        uintptr_t findExactWord;
        uintptr_t getHandlingId;
        std::span<const uintptr_t> refs;
    };

    // Moves the manager. Returns the new block (header and standard entries at the same offsets), or nullptr.
    uint8_t* MoveManager(const HandlingDef& def, std::span<SubArray> subArrays, uint32_t& newSize)
    {
        if (!mem::CheckRefs(def.refs, def.manager, def.managerSize, "Handling"))
            return nullptr;

        newSize = def.stdOffset + MaxIds * def.stdSize;
        for (auto& sub : subArrays)
        {
            sub.newOffset = newSize;
            newSize += (MaxIds - sub.first) * sub.elemSize;
        }

        auto block = (uint8_t*)calloc(1, newSize + 16);
        if (!block)
            return nullptr;

        memcpy(block, (void*)def.manager, def.stdOffset + def.vanillaCount * def.stdSize);
        for (auto& sub : subArrays)
            memcpy(block + sub.newOffset, (void*)(def.manager + sub.oldOffset), sub.vanillaCount * sub.elemSize);

        mem::RelocateRefs(def.refs, def.manager, def.managerSize, (uintptr_t)block);
        return block;
    }

    void HookLookups(const HandlingDef& def)
    {
        LoadNames(def.table, def.vanillaCount);
        findExactWordHook = safetyhook::create_inline(def.findExactWord, FindExactWordHook);
        getHandlingIdHook = safetyhook::create_inline(def.getHandlingId, GetHandlingIdHook);
        core::RegisterLimit({ "HandlingEntries", [] { return (int)names.size(); }, [] { return (int)maxIds; }, [] { return (int)maxIds; } });
    }

    // handling.cfg is read into a shared 55000-byte buffer; give it its own.
    void MoveFileBuffer(std::span<const uintptr_t> refs, uintptr_t oldBuffer, uintptr_t sizeOperand)
    {
        static std::vector<uint8_t> buffer(2 * 1024 * 1024);
        if (!mem::CheckRefs(refs, oldBuffer, 1, "Handling"))
            return;
        for (auto operand : refs)
            mem::WriteDword(operand, (uint32_t)(uintptr_t)buffer.data());
        if (sizeOperand)
            mem::WriteDword(sizeOperand, (uint32_t)buffer.size() - 1);
    }

    // GTA III 1.0
    const uintptr_t refsIII[] =
    {
        0x477B8D, 0x48BD78, 0x5203D9, 0x52C811, 0x53E493, 0x547266, 0x54B1B6, 0x54E303, 0x5335ED, 0x533754, 0x5338DB, 0x5339B0, 0x533AD4, 0x533C4F,
        0x533E02, 0x533EF5, 0x5515EF, 0x551602,
    };

    void InstallIII()
    {
        const HandlingDef def{ 0x728060, 0x3030, 0x14, 0xD8, 57, 0x6013E8, 0x546AA0, 0x546B70, refsIII };
        if (!mem::CheckValues(std::array<mem::Expect, 1>{ { { 0x546DD4, 0xD6D8 } } }, "Handling"))
            return;
        uint32_t size = 0;
        if (!MoveManager(def, {}, size))
            return;
        MoveFileBuffer(std::array<uintptr_t, 2>{ 0x546DD9, 0x546DF4 }, 0x715C28, 0x546DD4);
        HookLookups(def);
    }

    // GTA Vice City 1.0
    const uintptr_t refsVC[] =
    {
        0x48447E, 0x484B99, 0x48C0D3, 0x4A5035, 0x4AE591, 0x4AE59C, 0x4AE5A2, 0x4AE5A8, 0x4AE5B4, 0x4AE5BA, 0x4AE5C6, 0x4AE5CC, 0x4AE5D8, 0x4AE5DE,
        0x4AE5EA, 0x4AE5F0, 0x4AE5FC, 0x4AE602, 0x4AE60E, 0x4AE614, 0x4AE620, 0x4AE626, 0x4AE632, 0x4AE63F, 0x4AE64A, 0x4AE650, 0x4AE656, 0x4AE662,
        0x4AE66D, 0x4AE680, 0x4AE686, 0x4AE699, 0x4AE69F, 0x4AE6AB, 0x4AE6B1, 0x4AE6BD, 0x4AE6C3, 0x4AE6CF, 0x4AE6D5, 0x4AE6E1, 0x4AE6E7, 0x4AE6F3,
        0x4AE6F9, 0x4AE705, 0x4AE70B, 0x4AE717, 0x4AE71D, 0x4AE729, 0x4AE72F, 0x4AE73B, 0x4AE741, 0x4AE74D, 0x4AE753, 0x4AE75F, 0x4AE765, 0x4AE771,
        0x4AE777, 0x4AE783, 0x579E99, 0x57A85E, 0x595EB7, 0x596226, 0x5965B3, 0x59668D, 0x59684D, 0x596927, 0x596B8F, 0x596DE2, 0x596EDD, 0x596FD2,
        0x5970CB, 0x597437, 0x5977C3, 0x59789D, 0x59E7DA, 0x59E7E8, 0x5A6562, 0x5A6572, 0x5A657D, 0x5ABDD0, 0x5ABE14, 0x5AF839, 0x5B2B99, 0x5B9AD4,
        0x5B9B05, 0x5B9B27, 0x5B9B32, 0x5BA394, 0x5BA3C5, 0x5BA3E7, 0x5BA3F2, 0x615873, 0x61587F, 0x615891,
    };

    void InstallVC()
    {
        // Offsets of the sub-arrays from the manager, as immediates in the loader, the constructors and the getters.
        const mem::Expect offsets[] =
        {
            { 0x5AAF86, 0x5B2C }, { 0x615879, 0x5B2C },
            { 0x5AB264, 0x5C6C }, { 0x5ABA88, 0x5C6C }, { 0x5ABA92, 0x5C6C },
            { 0x5AB4A6, 0x5F64 }, { 0x5ABA58, 0x5F64 }, { 0x5ABA62, 0x5F64 },
        };
        if (!mem::CheckValues(offsets, "Handling") || *(uint8_t*)0x5AB48D != 0x60 || *(uint8_t*)0x5ABA49 != 0x60 || *(uint8_t*)0x5ABA79 != 0x69)
            return;

        SubArray subs[] = { { 0x5B2C, 0x40, 81, 5 }, { 0x5C6C, 0x4C, 96, 10 }, { 0x5F64, 0x3C, 86, 11 } };
        const HandlingDef def{ 0x978E58, 0x61FC, 0x14, 0xDC, 106, 0x69B4A4, 0x5ABD30, 0x5ABCC0, refsVC };
        uint32_t size = 0;
        if (!MoveManager(def, subs, size))
            return;

        for (auto operand : { 0x5AAF86, 0x615879 })
            mem::WriteDword(operand, subs[0].newOffset);
        for (auto operand : { 0x5AB264, 0x5ABA88, 0x5ABA92 })
            mem::WriteDword(operand, subs[1].newOffset);
        for (auto operand : { 0x5AB4A6, 0x5ABA58, 0x5ABA62 })
            mem::WriteDword(operand, subs[2].newOffset);
        // Upper id bounds of the boat and flying data (unsigned byte compares).
        mem::WriteByte(0x5AB48D, 0xFF);
        mem::WriteByte(0x5ABA49, 0xFF);
        mem::WriteByte(0x5ABA79, 0xFF);

        MoveFileBuffer(std::array<uintptr_t, 2>{ 0x5AAE49, 0x5AAE64 }, 0x7FFEE0, 0);
        HookLookups(def);
    }

    // GTA San Andreas 1.0 US
    const uintptr_t refsSA[] =
    {
        0x447742, 0x478518, 0x478565, 0x47AEB6, 0x4C7C27, 0x4C89CE, 0x4C8E7F, 0x509CBB, 0x5B7273, 0x5BFA96, 0x5BFAA6, 0x5BFAB0, 0x5BFABA, 0x5BFAC4,
        0x5BFACE, 0x6A2FD4, 0x6A4EEB, 0x6A4F19, 0x6A50A0, 0x6B0BA4, 0x6B0BE2, 0x6B21A2, 0x6B2D28, 0x6B2D4A, 0x6BF526, 0x6BF546, 0x6CC3EE, 0x6D6E6B,
        0x6D6EAA, 0x6D7681, 0x6D76AD, 0x6D76CF, 0x6F0D58, 0x6F2A2D, 0x6F2A42, 0x6F2A53, 0x6F6097,
    };
    const uintptr_t refsHoodlumSA[] =
    {
        0x1565282, 0x478518, 0x478565, 0x47AEB6, 0x4C7C27, 0x4C89CE, 0x4C8E7F, 0x509CBB, 0x5B7273, 0x5BFA96, 0x5BFAA6, 0x5BFAB0, 0x5BFABA, 0x5BFAC4,
        0x5BFACE, 0x6A2FD4, 0x6A4EEB, 0x6A4F19, 0x6A50A0, 0x6B0BA4, 0x6B0BE2, 0x6B21A2, 0x6B2D28, 0x6B2D4A, 0x6BF526, 0x6BF546, 0x6CC3EE, 0x6D6E6B,
        0x6D6EAA, 0x6D7681, 0x6D76AD, 0x6D76CF, 0x6F0D58, 0x6F2A2D, 0x6F2A42, 0x6F2A53, 0x6F6097,
    };

    void InstallSA()
    {
        constexpr uintptr_t manager = 0xC2B9C8;
        auto flyingBias = core::SA(0x6F52E5, 0x403754);
        const mem::Expect expected[] =
        {
            { 0x5BD918, 0x8F54 }, { 0x6BF52B, manager + 0x8F54 }, { 0x6CE3CF, manager + 0x8F54 },
            { 0x5BDA60, 0x7B24 }, { flyingBias, 0x7B24 }, { 0x6F52EE, 0xBB14 },
            { 0x6F5315, 0x9A50 }, { 0x6F531E, 0xC354 },
        };
        if (!mem::CheckValues(expected, "Handling") || *(uint8_t*)0x6F52D9 != 0xD1 || *(uint8_t*)0x6F5309 != 0xBA)
            return;

        SubArray subs[] = { { 0xB7D4, 0x40, 162, 13 }, { 0xBB14, 0x58, 186, 24 }, { 0xC354, 0x3C, 175, 12 } };
        const HandlingDef def{ manager, 0xC624, 0x14, 0xE0, 210, 0x8D3978, 0x6F4F30, 0x6F4FD0, core::hoodlum ? std::span<const uintptr_t>(refsHoodlumSA) : std::span<const uintptr_t>(refsSA) };
        uint32_t size = 0;
        auto block = MoveManager(def, subs, size);
        if (!block)
            return;

        // The loader and the bike constructors use "array offset - first id * size".
        auto bikeBias = subs[0].newOffset - 162 * 0x40;
        auto flyingBiasValue = subs[1].newOffset - 186 * 0x58;
        auto boatBias = subs[2].newOffset - 175 * 0x3C;
        mem::WriteDword(0x5BD918, bikeBias);
        mem::WriteDword(0x6BF52B, (uint32_t)(uintptr_t)block + bikeBias);
        mem::WriteDword(0x6CE3CF, (uint32_t)(uintptr_t)block + bikeBias);
        mem::WriteDword(0x5BDA60, flyingBiasValue);
        mem::WriteDword(flyingBias, flyingBiasValue);
        mem::WriteDword(0x6F52EE, subs[1].newOffset);
        mem::WriteDword(0x6F5315, boatBias);
        mem::WriteDword(0x6F531E, subs[2].newOffset);
        mem::WriteByte(0x6F52D9, 0xFF);
        mem::WriteByte(0x6F5309, 0xFF);

        HookLookups(def);
    }
}

namespace limits
{
    void InstallHandling()
    {
        auto setting = core::GetSetting("HandlingEntries", "unlimited");
        if (!setting.apply)
            return;
        maxIds = std::clamp<uint32_t>(core::MaxCapacity(setting, MaxIds), 1, MaxIds);

        if (core::IsIII())
            InstallIII();
        else if (core::IsVC())
            InstallVC();
        else if (core::IsSA())
            InstallSA();
    }
}

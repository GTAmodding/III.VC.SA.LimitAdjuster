#include "Core.h"
#include "Adjusters.h"

// SA explosions: CExplosion::aExplosions, 16 slots of 0x7C bytes. Only CExplosion's own functions touch the array, by
// absolute address; their loops end at an address past the last slot. A full array drops the explosion: its effect,
// sound and damage are lost. The array moves to reserved space that grows when CExplosion::AddExplosion finds no free
// slot. Other mods find the array through the operand of `add esi, aExplosions` in AddExplosion (0x736AD1).
namespace
{
    constexpr uintptr_t NativeBase{ 0xC88950 };
    constexpr uint32_t SlotSize{ 0x7C }, NativeSlots{ 16 }, MaxSlots{ 255 }; // CExplosion takes slot ids as a byte
    constexpr uintptr_t NativeEnd{ NativeBase + SlotSize * NativeSlots };
    constexpr uint32_t ActiveOffset{ 0x28 };
    constexpr uintptr_t AddExplosionAddress{ 0x736A50 }, FullCompare{ 0x736AB7 };

    // Operands pointing into the array.
    const mem::Expect Refs[] =
    {
        { 0x73686C, NativeBase + 0x04 }, { 0x7368A6, NativeBase + 0x70 }, { 0x7368AB, NativeBase + 0x44 },
        { 0x7368CB, NativeBase + 0x1DC }, { 0x73690A, NativeBase + 0x29 }, { 0x73691A, NativeBase + 0x29 },
        { 0x73692A, NativeBase + 0x2A }, { 0x73693A, NativeBase + 0x00 }, { 0x736949, NativeBase + 0x04 },
        { 0x736955, NativeBase + 0x04 }, { 0x7369E1, NativeBase + 0x08 }, { 0x736A96, NativeBase + 0x28 },
        { 0x736AA1, NativeBase + 0x28 }, { 0x736AD1, NativeBase + 0x00 }, { 0x73762B, NativeBase + 0x30 },
    };
    // Loop ends: a field of the slot past the last one.
    const mem::Expect Ends[] =
    {
        { 0x7368D5, NativeEnd + 0x04 }, { 0x7369C8, NativeEnd + 0x04 }, { 0x736A2F, NativeEnd + 0x08 },
        { 0x736AA6, NativeEnd + 0x28 }, { 0x737B02, NativeEnd + 0x30 },
    };

    mem::Reserve slots;
    int32_t capacity{ NativeSlots };
    uint32_t maximum{ NativeSlots };
    SafetyHookInline addExplosionHook;

    uint8_t* Slot(uint32_t i) { return slots.data() + (size_t)i * SlotSize; }

    bool Grow(uint32_t count)
    {
        // One slot more: AddExplosion's free slot search reads the active flag of the slot past the last one.
        if (!slots.Commit((size_t)(count + 1) * SlotSize))
            return false;
        for (auto i = (uint32_t)capacity; i < count; i++)
        {
            // As CExplosion::Initialise leaves a slot
            *(float*)(Slot(i) + 0x10) = 1.0f;
            Slot(i)[0x2A] = 1;
        }
        capacity = (int32_t)count;
        for (auto& end : Ends)
            mem::WriteDword(end.operand, (uint32_t)(uintptr_t)Slot(count) + (end.value - NativeEnd));
        return true;
    }

    bool __cdecl AddExplosion(void* victim, void* creator, int type, float x, float y, float z, uint32_t lifetime,
                              uint32_t makeSound, float camShake, uint32_t visible)
    {
        bool free{};
        for (int32_t i{}; i < capacity && !free; i++)
            free = !Slot(i)[ActiveOffset];
        if (!free && (uint32_t)capacity < maximum)
        {
            if (Grow(std::min(maximum, (uint32_t)capacity + std::max((uint32_t)capacity / 2, 8u))))
                core::Log("Explosions: grown to {}", capacity);
        }
        return addExplosionHook.ccall<bool>(victim, creator, type, x, y, z, lifetime, makeSound, camShake, visible);
    }

    int UsedSlots()
    {
        int used{};
        for (int32_t i{}; i < capacity; i++)
            used += Slot(i)[ActiveOffset] != 0;
        return used;
    }
}

void limits::InstallExplosions()
{
    if (!core::IsSA())
        return;
    auto setting = core::GetSetting("Explosions", "unlimited");
    if (!setting.apply)
        return;
    if (!mem::CheckValues(Refs, "Explosions") || !mem::CheckValues(Ends, "Explosions") ||
        !mem::CheckCompares({ &FullCompare, 1 }, NativeSlots, "Explosions"))
        return;
    if (*(uint8_t*)AddExplosionAddress != 0x83 || mem::ReadDword(AddExplosionAddress + 1) != 0x55531CEC)
    {
        core::Log("Explosions: CExplosion::AddExplosion is not the expected one, left as it is");
        return;
    }
    maximum = setting.unlimited ? MaxSlots : std::clamp(setting.value, 1u, MaxSlots);
    auto initial = setting.unlimited ? NativeSlots : maximum;
    if (!slots.Create((size_t)(maximum + 1) * SlotSize, (size_t)(initial + 1) * SlotSize))
        return;
    memcpy(slots.data(), (void*)NativeBase, NativeSlots * SlotSize);
    for (auto& ref : Refs)
        mem::WriteDword(ref.operand, (uint32_t)(uintptr_t)slots.data() + (ref.value - NativeBase));
    capacity = NativeSlots;
    Grow(initial);
    if (!mem::PatchCompare(FullCompare, &capacity))
        core::Log("Explosions: the free slot check could not be patched, {} slots used", NativeSlots);
    addExplosionHook = safetyhook::create_inline(AddExplosionAddress, AddExplosion);
    core::RegisterLimit({ "Explosions", [] { return UsedSlots(); }, [] { return (int)capacity; }, [] { return (int)maximum; } });
}

#include "Core.h"
#include "Adjusters.h"
#include "ObjectData.h"

// The buffer a collision file entry is read into. The game doesn't check its size, so one large entry overwrites the
// variables after it. Only the collision loaders get the new buffer; other loaders keep sharing the game's one.

namespace
{
    const uintptr_t refsIII[] = { 0x478B60, 0x478B6A, 0x478B79 };
    const uintptr_t refsVC[] = { 0x48A84D, 0x48A8CB, 0x48A987, 0x48AA1D, 0x48AA55, 0x48AAFC, 0x48AB31, 0x48AB61 };
    const uintptr_t refsSA[] = { 0x5B4EEF, 0x5B4F7F, 0x5B4F9C, 0x5B4FB1 };

    void MoveBuffer(uintptr_t base, uint32_t size, std::span<const uintptr_t> refs)
    {
        auto setting = core::GetSetting("CollisionSize", "unlimited");
        if (!setting.apply)
            return;

        // In KB. Committed up front: the file is read straight into it.
        auto bytes = std::max<uint32_t>(core::MaxCapacity(setting, 4096) * 1024, size);
        auto moved = limits::MoveArray("CollisionSize", base, 1, size, bytes, refs, false);
        if (moved)
        {
            moved->Commit(bytes);
            core::RegisterLimit({ "CollisionSize", [] { return -1; }, [bytes] { return (int)(bytes / 1024); }, [bytes] { return (int)(bytes / 1024); } });
        }
    }
}

namespace limits
{
    void InstallCollision()
    {
        objectdata::Install();
        if (core::IsIII())
            MoveBuffer(0x715C28, 55000, refsIII);
        else if (core::IsVC())
            MoveBuffer(0x7FFEE0, 55000, refsVC);
        else if (core::IsSA())
            MoveBuffer(0xBC40D8, 0x8000, refsSA);
    }
}

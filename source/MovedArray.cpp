#include "Core.h"
#include "Adjusters.h"

namespace limits
{
    MovedArray* MoveArray(std::string_view name, uintptr_t base, size_t elemSize, uint32_t count, uint32_t capacity,
                          std::span<const uintptr_t> refs, bool copy)
    {
        static std::vector<std::unique_ptr<MovedArray>> arrays;

        auto oldSize = elemSize * count;
        if (!mem::CheckRefs(refs, base, oldSize, name))
            return nullptr;

        auto moved = std::make_unique<MovedArray>();
        if (!moved->reserve.Create(elemSize * capacity, std::min<size_t>(elemSize * capacity, oldSize * 2)))
            return nullptr;

        mem::InstallCommitHandler();
        moved->data = moved->reserve.data();
        moved->capacity = capacity;
        if (copy)
            memcpy(moved->data, (void*)base, oldSize);
        mem::RelocateRefs(refs, base, oldSize, (uintptr_t)moved->data);

        arrays.push_back(std::move(moved));
        return arrays.back().get();
    }
}

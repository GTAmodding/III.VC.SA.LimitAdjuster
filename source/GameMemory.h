#pragma once
#include "stdafx.h"

namespace mem
{
    // Address space reserved once for a limit. The base never moves, so the game and other plugins can keep pointers
    // into it. Pages are committed when the limit grows, or when the game touches them (the exception handler below).
    class Reserve
    {
    public:
        // Reserves `maxBytes` and commits the first `commitBytes`. Returns false when the address space is exhausted.
        bool Create(size_t maxBytes, size_t commitBytes);

        // Makes sure the first `bytes` are usable.
        bool Commit(size_t bytes);

        uint8_t* data() const { return base; }
        size_t reserved() const { return reservedSize; }
        size_t committed() const { return committedSize; }
        explicit operator bool() const { return base != nullptr; }

    private:
        uint8_t* base = nullptr;
        size_t reservedSize = 0;
        size_t committedSize = 0;
    };

    // Commits a page of a Reserve on first access. Installed once.
    void InstallCommitHandler();

    // Reads the dword at `address` (an operand in the game's code).
    inline uint32_t ReadDword(uintptr_t address)
    {
        return *(uint32_t*)address;
    }

    void WriteDword(uintptr_t address, uint32_t value);
    void WriteByte(uintptr_t address, uint8_t value);

    // Checks that every operand points into [oldBase, oldBase + oldSize). Logs the first that doesn't.
    bool CheckRefs(std::span<const uintptr_t> operands, uintptr_t oldBase, size_t oldSize, std::string_view name);

    // Moves every operand from [oldBase, oldBase + oldSize) to the same offset from newBase. Call CheckRefs first.
    void RelocateRefs(std::span<const uintptr_t> operands, uintptr_t oldBase, size_t oldSize, uintptr_t newBase);

    // Checks a list of (operand, expected value) pairs.
    struct Expect
    {
        uintptr_t operand;
        uint32_t value;
    };
    bool CheckValues(std::span<const Expect> values, std::string_view name);

    // True when `address` holds `cmp <operand>, imm` with the given immediate.
    bool IsCompare(uintptr_t address, uint32_t imm);

    // Checks that every address is a compare against `imm`. Logs the first that isn't.
    bool CheckCompares(std::span<const uintptr_t> addresses, uint32_t imm, std::string_view name);

    // Replaces the compare instruction at `address` with the same compare against the value at `limit`, so a bound in
    // the game's code can change at runtime. The flags it sets are the same, the conditional jump after it is kept.
    // 16-bit compares read the low word of `limit`. Nothing may jump into the first five bytes after `address` except
    // to `address` itself.
    bool PatchCompare(uintptr_t address, const int32_t* limit);
}

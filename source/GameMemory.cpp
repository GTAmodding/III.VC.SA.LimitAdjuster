#include "GameMemory.h"
#include "Core.h"
#include <Zydis.h>

namespace mem
{
    namespace
    {
        constexpr size_t PageSize = 0x1000;

        struct Range
        {
            uintptr_t begin;
            uintptr_t end;
        };

        // Written only while patching at startup, read by the exception handler from any thread.
        std::array<Range, 512> ranges{};
        std::atomic<size_t> rangeCount{ 0 };

        size_t RoundUp(size_t value, size_t to)
        {
            return (value + to - 1) / to * to;
        }

        LONG CALLBACK CommitOnAccess(PEXCEPTION_POINTERS info)
        {
            auto record = info->ExceptionRecord;
            if (record->ExceptionCode != EXCEPTION_ACCESS_VIOLATION || record->NumberParameters < 2)
                return EXCEPTION_CONTINUE_SEARCH;

            auto address = (uintptr_t)record->ExceptionInformation[1];
            auto count = rangeCount.load(std::memory_order_acquire);
            for (size_t i = 0; i < count; i++)
            {
                if (address >= ranges[i].begin && address < ranges[i].end)
                {
                    auto page = address & ~(uintptr_t)(PageSize - 1);
                    if (VirtualAlloc((void*)page, PageSize, MEM_COMMIT, PAGE_READWRITE))
                        return EXCEPTION_CONTINUE_EXECUTION;
                    return EXCEPTION_CONTINUE_SEARCH;
                }
            }
            return EXCEPTION_CONTINUE_SEARCH;
        }

        // Executable memory for the compare stubs.
        uint8_t* stubCursor = nullptr;
        uint8_t* stubEnd = nullptr;

        uint8_t* AllocStub(size_t size)
        {
            if (!stubCursor || stubCursor + size > stubEnd)
            {
                constexpr size_t chunk = 0x10000;
                stubCursor = (uint8_t*)VirtualAlloc(nullptr, chunk, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
                if (!stubCursor)
                    return nullptr;
                stubEnd = stubCursor + chunk;
            }
            auto result = stubCursor;
            stubCursor += RoundUp(size, 16);
            return result;
        }

        ZydisDecoder& Decoder()
        {
            static ZydisDecoder decoder = []
            {
                ZydisDecoder d;
                ZydisDecoderInit(&d, ZYDIS_MACHINE_MODE_LEGACY_32, ZYDIS_STACK_WIDTH_32);
                return d;
            }();
            return decoder;
        }

        class Emitter
        {
        public:
            explicit Emitter(uint8_t* out) : start(out), cursor(out)
            {
            }

            bool Emit(ZydisEncoderRequest& request)
            {
                ZyanUSize length = 16;
                if (ZYAN_FAILED(ZydisEncoderEncodeInstructionAbsolute(&request, cursor, &length, (ZyanU64)(uintptr_t)cursor)))
                    return false;
                cursor += length;
                return true;
            }

            void Copy(const uint8_t* bytes, size_t length)
            {
                memcpy(cursor, bytes, length);
                cursor += length;
            }

            size_t size() const { return cursor - start; }

        private:
            uint8_t* start;
            uint8_t* cursor;
        };

        ZydisEncoderRequest NewRequest(ZydisMnemonic mnemonic)
        {
            ZydisEncoderRequest request;
            memset(&request, 0, sizeof(request));
            request.machine_mode = ZYDIS_MACHINE_MODE_LEGACY_32;
            request.mnemonic = mnemonic;
            return request;
        }

        void SetRegister(ZydisEncoderOperand& operand, ZydisRegister reg)
        {
            operand.type = ZYDIS_OPERAND_TYPE_REGISTER;
            operand.reg.value = reg;
        }

        void SetAbsoluteMemory(ZydisEncoderOperand& operand, const void* address, uint16_t size)
        {
            operand.type = ZYDIS_OPERAND_TYPE_MEMORY;
            operand.mem.base = ZYDIS_REGISTER_NONE;
            operand.mem.index = ZYDIS_REGISTER_NONE;
            operand.mem.displacement = (ZyanI64)(uintptr_t)address;
            operand.mem.size = size;
        }

        ZydisRegister Narrow(ZydisRegister reg32, uint16_t bits)
        {
            if (bits == 32)
                return reg32;
            switch (reg32)
            {
                case ZYDIS_REGISTER_EAX: return ZYDIS_REGISTER_AX;
                case ZYDIS_REGISTER_ECX: return ZYDIS_REGISTER_CX;
                case ZYDIS_REGISTER_EDX: return ZYDIS_REGISTER_DX;
                default: return ZYDIS_REGISTER_BX;
            }
        }

        bool Uses(const ZydisDecodedOperand& memory, ZydisRegister reg32)
        {
            return ZydisRegisterGetLargestEnclosing(ZYDIS_MACHINE_MODE_LEGACY_32, memory.mem.base) == reg32 ||
                   ZydisRegisterGetLargestEnclosing(ZYDIS_MACHINE_MODE_LEGACY_32, memory.mem.index) == reg32;
        }
    }

    bool Reserve::Create(size_t maxBytes, size_t commitBytes)
    {
        reservedSize = RoundUp(std::max<size_t>(maxBytes, PageSize), 0x10000);
        base = (uint8_t*)VirtualAlloc(nullptr, reservedSize, MEM_RESERVE, PAGE_READWRITE);
        if (!base)
        {
            core::Log("Could not reserve {} KB of address space", reservedSize / 1024);
            reservedSize = 0;
            return false;
        }

        auto index = rangeCount.load();
        if (index < ranges.size())
        {
            ranges[index] = { (uintptr_t)base, (uintptr_t)base + reservedSize };
            rangeCount.store(index + 1, std::memory_order_release);
        }
        return Commit(commitBytes);
    }

    bool Reserve::Commit(size_t bytes)
    {
        bytes = RoundUp(bytes, PageSize);
        if (bytes <= committedSize)
            return true;
        if (bytes > reservedSize)
            return false;
        if (!VirtualAlloc(base + committedSize, bytes - committedSize, MEM_COMMIT, PAGE_READWRITE))
            return false;
        committedSize = bytes;
        return true;
    }

    void InstallCommitHandler()
    {
        static bool installed = false;
        if (!installed)
        {
            AddVectoredExceptionHandler(1, CommitOnAccess);
            installed = true;
        }
    }

    void WriteDword(uintptr_t address, uint32_t value)
    {
        ProtectedGameRef<uint32_t> ref((uint32_t*)address);
        ref = value;
    }

    void WriteByte(uintptr_t address, uint8_t value)
    {
        ProtectedGameRef<uint8_t> ref((uint8_t*)address);
        ref = value;
    }

    bool CheckRefs(std::span<const uintptr_t> operands, uintptr_t oldBase, size_t oldSize, std::string_view name)
    {
        for (auto operand : operands)
        {
            auto value = ReadDword(operand);
            if (value < oldBase || value >= oldBase + oldSize)
            {
                core::Log("{}: the game code at {:X} is not the expected one (another mod changed it?), left as it is", name, operand);
                return false;
            }
        }
        return true;
    }

    void RelocateRefs(std::span<const uintptr_t> operands, uintptr_t oldBase, size_t oldSize, uintptr_t newBase)
    {
        for (auto operand : operands)
        {
            auto value = ReadDword(operand);
            if (value >= oldBase && value < oldBase + oldSize)
                WriteDword(operand, (uint32_t)(newBase + (value - oldBase)));
        }
    }

    bool CheckValues(std::span<const Expect> values, std::string_view name)
    {
        for (auto& [operand, value] : values)
        {
            if (ReadDword(operand) != value)
            {
                core::Log("{}: the game code at {:X} is not the expected one (another mod changed it?), left as it is", name, operand);
                return false;
            }
        }
        return true;
    }

    bool IsCompare(uintptr_t address, uint32_t imm)
    {
        ZydisDecodedInstruction compare;
        ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
        if (ZYAN_FAILED(ZydisDecoderDecodeFull(&Decoder(), (void*)address, 16, &compare, operands)))
            return false;
        if (compare.mnemonic != ZYDIS_MNEMONIC_CMP || operands[1].type != ZYDIS_OPERAND_TYPE_IMMEDIATE)
            return false;
        if (operands[0].size != 16 && operands[0].size != 32)
            return false;
        auto value = (uint32_t)operands[1].imm.value.u;
        if (operands[0].size == 16)
            return (value & 0xFFFF) == (imm & 0xFFFF);
        return value == imm;
    }

    bool CheckCompares(std::span<const uintptr_t> addresses, uint32_t imm, std::string_view name)
    {
        for (auto address : addresses)
        {
            if (!IsCompare(address, imm))
            {
                core::Log("{}: the game code at {:X} is not the expected one (another mod changed it?), left as it is", name, address);
                return false;
            }
        }
        return true;
    }

    bool PatchCompare(uintptr_t address, const int32_t* limit)
    {
        ZydisDecodedInstruction compare;
        ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
        if (ZYAN_FAILED(ZydisDecoderDecodeFull(&Decoder(), (void*)address, 16, &compare, operands)) ||
            compare.mnemonic != ZYDIS_MNEMONIC_CMP || operands[1].type != ZYDIS_OPERAND_TYPE_IMMEDIATE ||
            (operands[0].size != 16 && operands[0].size != 32))
        {
            core::Log("Unexpected instruction at {:X}, not patched", address);
            return false;
        }

        auto stub = AllocStub(64);
        if (!stub)
            return false;

        Emitter out(stub);
        auto bits = operands[0].size;
        if (operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER)
        {
            auto request = NewRequest(ZYDIS_MNEMONIC_CMP);
            request.operand_count = 2;
            SetRegister(request.operands[0], operands[0].reg.value);
            SetAbsoluteMemory(request.operands[1], limit, bits / 8);
            if (!out.Emit(request))
                return false;
        }
        else if (operands[0].type == ZYDIS_OPERAND_TYPE_MEMORY)
        {
            // cmp mem, imm -> push reg; mov reg, [limit]; cmp mem, reg; pop reg (pop keeps the flags)
            ZydisRegister scratch = ZYDIS_REGISTER_EAX;
            for (auto candidate : { ZYDIS_REGISTER_EAX, ZYDIS_REGISTER_ECX, ZYDIS_REGISTER_EDX, ZYDIS_REGISTER_EBX })
            {
                if (!Uses(operands[0], candidate))
                {
                    scratch = candidate;
                    break;
                }
            }

            auto push = NewRequest(ZYDIS_MNEMONIC_PUSH);
            push.operand_count = 1;
            SetRegister(push.operands[0], scratch);

            auto load = NewRequest(ZYDIS_MNEMONIC_MOV);
            load.operand_count = 2;
            SetRegister(load.operands[0], Narrow(scratch, bits));
            SetAbsoluteMemory(load.operands[1], limit, bits / 8);

            auto cmp = NewRequest(ZYDIS_MNEMONIC_CMP);
            cmp.operand_count = 2;
            cmp.operands[0].type = ZYDIS_OPERAND_TYPE_MEMORY;
            cmp.operands[0].mem.base = operands[0].mem.base;
            cmp.operands[0].mem.index = operands[0].mem.index;
            cmp.operands[0].mem.scale = operands[0].mem.index != ZYDIS_REGISTER_NONE ? operands[0].mem.scale : 0;
            cmp.operands[0].mem.displacement = operands[0].mem.disp.value;
            if (operands[0].mem.base == ZYDIS_REGISTER_ESP)
                cmp.operands[0].mem.displacement += 4;
            cmp.operands[0].mem.size = bits / 8;
            SetRegister(cmp.operands[1], Narrow(scratch, bits));

            auto pop = NewRequest(ZYDIS_MNEMONIC_POP);
            pop.operand_count = 1;
            SetRegister(pop.operands[0], scratch);

            if (!out.Emit(push) || !out.Emit(load) || !out.Emit(cmp) || !out.Emit(pop))
                return false;
        }
        else
        {
            return false;
        }

        // Instructions after the compare that the 5-byte jump overwrites move into the stub.
        size_t moved = compare.length;
        while (moved < 5)
        {
            ZydisDecodedInstruction next;
            ZydisDecodedOperand nextOperands[ZYDIS_MAX_OPERAND_COUNT];
            if (ZYAN_FAILED(ZydisDecoderDecodeFull(&Decoder(), (void*)(address + moved), 16, &next, nextOperands)))
                return false;

            if (next.attributes & ZYDIS_ATTRIB_IS_RELATIVE)
            {
                ZyanU64 target = 0;
                if (ZYAN_FAILED(ZydisCalcAbsoluteAddress(&next, &nextOperands[0], address + moved, &target)))
                    return false;

                ZydisEncoderRequest request;
                if (ZYAN_FAILED(ZydisEncoderDecodedInstructionToEncoderRequest(&next, nextOperands, next.operand_count_visible, &request)))
                    return false;
                request.branch_type = ZYDIS_BRANCH_TYPE_NONE;
                request.branch_width = ZYDIS_BRANCH_WIDTH_NONE;
                request.operands[0].imm.u = target;
                if (!out.Emit(request))
                    return false;
            }
            else
            {
                out.Copy((const uint8_t*)(address + moved), next.length);
            }
            moved += next.length;
        }

        auto back = NewRequest(ZYDIS_MNEMONIC_JMP);
        back.operand_count = 1;
        back.operands[0].type = ZYDIS_OPERAND_TYPE_IMMEDIATE;
        back.operands[0].imm.u = address + moved;
        if (!out.Emit(back))
            return false;

        injector::MakeNOP(address, moved, true);
        injector::MakeJMP(address, stub, true);
        return true;
    }
}

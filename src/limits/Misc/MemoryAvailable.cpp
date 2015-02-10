/*
* Streaming Memory Available
* Copyright (c) 2014 LINK/2012 <dma_2012@hotmail.com>
* Licensed under the MIT License (http://opensource.org/licenses/MIT)
*/
#include "LimitAdjuster.h"
using namespace injector; 

struct MemoryLimit : public SimpleAdjuster
{
    const char* GetLimitName()
    { return IsIII() || IsVC() || IsSA()? "MemoryAvailable" : nullptr; }

    void ChangeLimit(int, const std::string& value) 
    { 
        static const uint32_t max_memory = MbToBytes(2048);  // 2GB
        static const auto avalmem_ptr    = mem_ptr(IsSA()? 0x5B8E64 + 6 : IsVC()? 0x410799 + 6 : IsIII()? 0x406AD4 + 6 : 0);

        uint32_t memory = ReadMemory<uint32_t>(avalmem_ptr, true);  // use same value as before if memory couldn't be set

        if(value.back() == '%')
        {
            MEMORYSTATUSEX sysmem = { sizeof(sysmem) };
            if(GlobalMemoryStatusEx(&sysmem))
            {
                memory = uint32_t(sysmem.ullTotalPhys * std::stoul(value.substr(0, value.size() - 1)) / 100);
            }
        }
        else
            memory = MbToBytes(std::stoul(value));

        memory = memory > max_memory? max_memory : memory;  // avoid going higher than 2GB, may lead to crashes due to out of memory.
        WriteMemory(avalmem_ptr, memory, true);
        if(IsIII()) MakeNOP(0x406AD2, 2);   // force the code patched above to run
    }

    bool GetUsage(int, std::string& output)
    {
        static const auto avalmem_ptr = mem_ptr(IsSA()? 0x8A5A80 : IsVC()? 0x94DD54 : IsIII()? 0x880F8C : 0);
        static const auto usedmem_ptr = mem_ptr(IsSA()? 0x8E4CB4 : IsVC()? 0xA0CFDC : IsIII()? 0x940568 : 0);
        return Adjuster::GetUsage(output, BytesToMb(ReadMemory<uint32_t>(usedmem_ptr)), BytesToMb(ReadMemory<uint32_t>(avalmem_ptr)));
    }

    static uint32_t BytesToMb(uint32_t bytes) { return bytes / 1048576; }
    static uint32_t MbToBytes(uint32_t mb)    { return mb * 1048576; }

} MemoryLimit;

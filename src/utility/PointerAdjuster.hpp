/*
* Utility for adjusting pointers many times (helpful for dynamic pointers)
* Copyright (c) 2014 LINK/2012 <dma_2012@hotmail.com>
* Licensed under the MIT License (http://opensource.org/licenses/MIT)
*/
#pragma once
#include "LimitAdjuster.h"

/*
    PointerAdjuster
        Basic object for helping in adjusting pointers
*/
class PointerAdjuster   // not derived from SimpleAdjuster!!
{
    private:
         // Pointers to be replaced in rellocations
        std::vector<std::pair<injector::memory_pointer_raw, int>> ptrs;

    public:

        // Adds a pointer for rellocation when necessary
        // Displacement is how far (in bytes) this pointer is far from the beggining of the array 
        void AddPointer(uintptr_t addr, int displacement = 0)
        {
            using namespace injector;
            ptrs.push_back(std::make_pair(raw_ptr(memory_pointer(addr)), displacement));
        }

        // Performs the rellocation itself
        void ApplyPatches(void* ptr)
        {
            for(auto it = ptrs.begin(); it != ptrs.end(); ++it)
                injector::WriteMemory(it->first, (uintptr_t)(ptr) + it->second, true);
        }

        // Reflects the content from @src into @dest assuming each element has the sizeof @elem_size
        // @dest_count and @src_count are the count of elements in each of the dest and src pointers, the lesser will be used
        void ApplyReflection(void* dest, const void* src, size_t elem_size, size_t dest_count, size_t src_count)
        {
            auto size = elem_size * (dest_count < src_count? dest_count : src_count);
            return ApplyReflection(dest, src, size);
        }

        // Reflects the content from @src into @dest
        void ApplyReflection(void* dest, const void* src, size_t num_bytes)
        {
            std::memcpy(dest, src, num_bytes);
        }
};

/*
 *  San Andreas procedural object lists and limit patch helpers.
 *  Licensed under the MIT License (http://opensource.org/licenses/MIT)
 */
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>

namespace Procedural
{
    struct Vector { float x, y, z; };

    struct PlantTriangle
    {
        Vector vertices[3], center;
        float sphereRadius, seeds[3];
        uint16_t numPlants[3];
        uint8_t surfaceId, lighting, flags;
        PlantTriangle *next, *prev;
    };

    struct CollisionEntry
    {
        void* entity;
        PlantTriangle** triangles;
        uint16_t numTriangles;
        CollisionEntry *next, *prev;
    };

    struct ObjectEntry
    {
        ObjectEntry *prev, *next;
        void* object;
        PlantTriangle* triangle;
        bool allocatedMatrix;
    };

    template<class T>
    void Prepend(T*& head, T& item)
    {
        item.prev = nullptr;
        item.next = head;
        if(head) head->prev = &item;
        head = &item;
    }

    // SA CList_c layout: head, tail, count. Not CLinkList and not the swapped
    // field names in some older headers.
    struct ObjectList
    {
        ObjectEntry *head, *tail;
        uint32_t count;

        // Mirrors CList_c::AddItem (0x4A8DF0). The game hook calls that function.
        void Add(ObjectEntry& item)
        {
            if(!head) tail = &item;
            Prepend(head, item);
            ++count;
        }
    };

    struct SurfaceInfo
    {
        uint8_t properties[0x3C];
        ObjectList objects;
    };

    struct ObjectManager
    {
        int32_t allocatedMatrices, numSurfaceInfos;
        SurfaceInfo surfaceInfos[128];
        ObjectEntry objects[512];
        ObjectList unused;
    };

    // Count occupied tracking slots independently of the game's list counters.
    inline uint32_t CountObjects(const ObjectEntry* entries, size_t count)
    {
        uint32_t used = 0;
        for(size_t i = 0; i < count; ++i)
            if(entries[i].object) ++used;
        return used;
    }

    template<class T>
    uint32_t Count(T* entry, uint32_t max)
    {
        uint32_t count = 0;
        for(; entry && count <= max; entry = entry->next) ++count;
        return count;
    }

    template<class T>
    void AttachUnused(T*& head, T* entries, size_t count)
    {
        // PlantMgr's vanilla arrays are linked in ascending address order.
        while(count)
        {
            const size_t i = --count;
            entries[i] = {};
            Prepend(head, entries[i]);
        }
    }

    inline void AttachUnused(ObjectList& list, ObjectEntry* entries, size_t count)
    {
        for(size_t i = 0; i < count; ++i)
        {
            entries[i] = {};
            list.Add(entries[i]);
        }
    }

    inline int ParseLimit(const std::string& value, int minimum)
    {
        const int limit = std::stoi(value);
        if(limit < minimum || limit > 65535)
            throw std::out_of_range("Expected a total capacity between " + std::to_string(minimum) + " and 65535");
        return limit;
    }

    // Locate a complete cmp [absolute counter], immediate instruction, including
    // its operand width and counter address. Never search for just 150 or 200:
    // those bytes can occur in unrelated instructions and addresses.
    inline size_t FindCounterCompare(const uint8_t* code, size_t size,
                                    uint32_t counter, uint32_t limit, bool word)
    {
        uint8_t instruction[10] = {};
        size_t prefix = word ? 1 : 0;
        if(word) instruction[0] = 0x66;
        instruction[prefix] = 0x81;
        instruction[prefix + 1] = 0x3D;
        std::memcpy(instruction + prefix + 2, &counter, sizeof(counter));
        const size_t immediate = prefix + 6;
        const size_t length = immediate + (word ? 2 : 4);
        std::memcpy(instruction + immediate, &limit, word ? 2 : 4);

        size_t match = size;
        for(size_t i = 0; i + length <= size; ++i)
        {
            if(std::memcmp(code + i, instruction, length) == 0)
            {
                if(match != size)
                    throw std::runtime_error("Ambiguous procedural counter comparison");
                match = i + immediate;
            }
        }
        if(match == size)
            throw std::runtime_error("Procedural counter comparison does not match SA 1.0 US");
        return match;
    }

    // The conditional jump follows the immediate. Word and dword compares differ only in operand width.
    inline size_t CompareJumpOffset(size_t immediate, bool word)
    {
        return immediate + (word ? 2 : 4);
    }
}

/*
*  SA procedural object lists and limit patch helpers.
* Copyright (c) 2026 isonisx <dblazheski96@gmail.com>
* Licensed under the MIT License (http://opensource.org/licenses/MIT)
*/
#pragma once
#include <cstddef>
#include <cstdint>
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

    struct ObjectList // CList_c: head, tail, count.
    {
        ObjectEntry *head, *tail;
        uint32_t count;
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

    inline uint32_t CountObjects(const ObjectEntry* entries, size_t count)
    {
        uint32_t used = 0;
        for(size_t i = 0; i < count; ++i)
            if(entries[i].object) ++used;
        return used;
    }

    template<class T>
    uint32_t Count(T* entry)
    {
        uint32_t count = 0;
        for(; entry && count < 65536; entry = entry->next) ++count;
        return count;
    }

    template<class T>
    void AttachUnused(T*& head, T* entries, size_t count)
    {
        while(count)
        {
            const size_t i = --count;
            entries[i] = {};
            Prepend(head, entries[i]);
        }
    }

    inline int ParseLimit(const std::string& value, int minimum)
    {
        const int limit = std::stoi(value);
        if(limit < minimum || limit > 65535)
            throw std::out_of_range("Expected a total capacity between " + std::to_string(minimum) + " and 65535");
        return limit;
    }
}

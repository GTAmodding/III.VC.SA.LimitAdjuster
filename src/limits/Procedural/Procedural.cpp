/*
* Procedural Adjuster
* Copyright (c) 2026 isonisx <dblazheski96@gmail.com>
* Licensed under the MIT License (http://opensource.org/licenses/MIT)
*/
#include "LimitAdjuster.h"
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace
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

static_assert(sizeof(PlantTriangle) == 0x54, "CPlantLocTri layout");
static_assert(offsetof(PlantTriangle, next) == 0x4C, "CPlantLocTri links");
static_assert(sizeof(CollisionEntry) == 0x14, "CPlantColEntEntry layout");
static_assert(offsetof(CollisionEntry, next) == 0xC, "CPlantColEntEntry links");
static_assert(sizeof(ObjectEntry) == 0x14, "ProcObjectListItem layout");
static_assert(offsetof(ObjectEntry, object) == 0x8, "ProcObjectListItem entity");
static_assert(sizeof(SurfaceInfo) == 0x48, "ProcSurfaceInfo layout");
static_assert(offsetof(ObjectManager, unused) == 0x4C08, "ProcObjectMan free list");
static_assert(sizeof(ObjectManager) == 0x4C14, "ProcObjectMan layout");

struct Procedural;
extern Procedural adjuster_Procedural;

struct Procedural : public Adjuster
{
    enum
    {
        ProcObjects, PlantTriangles, PlantCollisionEntries
    };

    struct Setting { int vanilla, value; };
    Setting settings[3] =
    {
        { 512, 512 }, { 256, 256 }, { 40, 40 }
    };
    std::vector<ObjectEntry> extraTrackingEntries;
    std::vector<PlantTriangle> extraTriangles;
    std::vector<CollisionEntry> extraCollisionEntries;
    bool (__fastcall *originalInit)(ObjectManager*, int) = nullptr;
    bool (__cdecl *originalReload)() = nullptr;

    const Limit* GetLimits()
    {
        if(!IsSA() || !GetGVM().IsUS() || GetGVM().GetMajorVersion() != 1 || GetGVM().GetMinorVersion() != 0)
            return nullptr;
        static const Limit limits[] =
        {
            DEFINE_LIMIT(ProcObjects),
            DEFINE_LIMIT(PlantTriangles),
            DEFINE_LIMIT(PlantCollisionEntries),
            FINISH_LIMITS()
        };
        return limits;
    }

    void ChangeLimit(int id, const std::string& value)
    {
        Setting& setting = settings[id];
        const int limit = ParseLimit(value, setting.vanilla);
        setting.value = limit;
        if(limit == setting.vanilla) return;

        switch(id)
        {
            case ProcObjects: extraTrackingEntries.resize(limit - setting.vanilla); break;
            case PlantTriangles: extraTriangles.resize(limit - setting.vanilla); break;
            case PlantCollisionEntries: extraCollisionEntries.resize(limit - setting.vanilla); break;
        }
    }

    void Process()
    {
        if(!extraTrackingEntries.empty())
        {
            originalInit = injector::MakeCALL(0x53BE53, &InitObjectTracking).get();
            injector::MakeCALL(0x5BA468, &InitObjectTracking);
        }
        if(!extraTriangles.empty() || !extraCollisionEntries.empty())
            originalReload = injector::MakeCALL(0x5DD913, &ReloadPlantCache).get();
    }

    bool GetUsage(int id, std::string& output)
    {
        switch(id)
        {
            case ProcObjects: return Report(output, id, TrackingUsed());
            case PlantTriangles: return Report(output, id, TriangleUsed());
            case PlantCollisionEntries: return Report(output, id, Count(ActiveCollisionEntries()));
        }
        return false;
    }

    uint32_t TrackingUsed()
    {
        auto& manager = Manager();
        return CountObjects(manager.objects, 512) +
               CountObjects(extraTrackingEntries.data(), extraTrackingEntries.size());
    }

    static uint32_t TriangleUsed()
    {
        uint32_t used = 0;
        auto* bands = TriangleBands();
        for(int i = 0; i < 4; ++i) used += Count(bands[i]);
        return used;
    }

    bool Report(std::string& output, int id, uint32_t used)
    {
        return Adjuster::GetUsage(output, used, static_cast<uint32_t>(settings[id].value));
    }

    static ObjectManager& Manager()
    {
        return *injector::lazy_pointer<0xBB7CB0>().get<ObjectManager>();
    }

    static PlantTriangle** TriangleBands()
    {
        return injector::lazy_pointer<0xC03988>().get<PlantTriangle*>();
    }

    static PlantTriangle*& UnusedTriangles()
    {
        return *injector::lazy_pointer<0xC03984>().get<PlantTriangle*>();
    }

    static CollisionEntry*& ActiveCollisionEntries()
    {
        return *injector::lazy_pointer<0xC0399C>().get<CollisionEntry*>();
    }

    static CollisionEntry*& UnusedCollisions()
    {
        return *injector::lazy_pointer<0xC03998>().get<CollisionEntry*>();
    }

    static bool __fastcall InitObjectTracking(ObjectManager* manager, int)
    {
        auto& self = adjuster_Procedural;
        const bool ok = self.originalInit(manager, 0);
        auto addItem = (void (__thiscall*)(ObjectList*, ObjectEntry*)) injector::lazy_pointer<0x4A8DF0>().get<void>();
        for(auto& entry : self.extraTrackingEntries)
        {
            entry = {};
            addItem(&manager->unused, &entry);
        }
        return ok;
    }

    static bool __cdecl ReloadPlantCache()
    {
        auto& self = adjuster_Procedural;
        // Release only when the reload will rebuild, or extras are abandoned.
        bool (*initialise)() = injector::lazy_pointer<0x5DD6C0>().get();
        if(initialise())
        {
            void (__fastcall *release)(CollisionEntry*, int) = injector::lazy_pointer<0x5DB8A0>().get();
            while(auto* entry = ActiveCollisionEntries()) release(entry, 0);
        }

        const bool loaded = self.originalReload();
        if(!loaded) return false;

        if(!self.extraTriangles.empty())
            AttachUnused(UnusedTriangles(), self.extraTriangles.data(), self.extraTriangles.size());
        if(!self.extraCollisionEntries.empty())
            AttachUnused(UnusedCollisions(), self.extraCollisionEntries.data(), self.extraCollisionEntries.size());
        return true;
    }

} adjuster_Procedural;

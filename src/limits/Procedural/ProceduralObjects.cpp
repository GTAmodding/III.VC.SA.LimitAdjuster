/*
* Procedural Objects Adjuster
* Copyright (c) 2026 isonisx <dblazheski96@gmail.com>
* Licensed under the MIT License (http://opensource.org/licenses/MIT)
*/
#include "LimitAdjuster.h"
#include "ProceduralObjects.hpp"

using namespace Procedural;

static_assert(sizeof(PlantTriangle) == 0x54, "CPlantLocTri layout");
static_assert(offsetof(PlantTriangle, next) == 0x4C, "CPlantLocTri links");
static_assert(sizeof(CollisionEntry) == 0x14, "CPlantColEntEntry layout");
static_assert(offsetof(CollisionEntry, next) == 0xC, "CPlantColEntEntry links");
static_assert(sizeof(ObjectEntry) == 0x14, "ProcObjectListItem layout");
static_assert(offsetof(ObjectEntry, object) == 0x8, "ProcObjectListItem entity");
static_assert(sizeof(SurfaceInfo) == 0x48, "ProcSurfaceInfo layout");
static_assert(offsetof(ObjectManager, unused) == 0x4C08, "ProcObjectMan free list");
static_assert(sizeof(ObjectManager) == 0x4C14, "ProcObjectMan layout");

struct ProceduralObjects;
extern ProceduralObjects adjuster_ProceduralObjects;

struct ProceduralObjects : public Adjuster
{
    enum
    {
        ProcObjectEntries, ProcPlantTriangles, ProcCollisionEntries
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
            DEFINE_LIMIT(ProcObjectEntries),
            DEFINE_LIMIT(ProcPlantTriangles),
            DEFINE_LIMIT(ProcCollisionEntries),
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
            case ProcObjectEntries: extraTrackingEntries.resize(limit - setting.vanilla); break;
            case ProcPlantTriangles: extraTriangles.resize(limit - setting.vanilla); break;
            case ProcCollisionEntries: extraCollisionEntries.resize(limit - setting.vanilla); break;
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
            case ProcObjectEntries: return Report(output, id, TrackingUsed());
            case ProcPlantTriangles: return Report(output, id, TriangleUsed());
            case ProcCollisionEntries: return Report(output, id, Count(ActiveCollisionEntries()));
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
        auto& self = adjuster_ProceduralObjects;
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
        auto& self = adjuster_ProceduralObjects;
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

} adjuster_ProceduralObjects;

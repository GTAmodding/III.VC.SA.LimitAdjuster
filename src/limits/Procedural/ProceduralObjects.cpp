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
        ProcObjectEntries, ProcPlantTriangles, ProcCollisionEntries,
        ProcTempObjectThreshold, ProcMatrixAllowance
    };

    struct Setting { int vanilla, value; bool threshold; };
    Setting settings[5] =
    {
        { 512, 512, false }, { 256, 256, false }, { 40, 40, false },
        { 150, 150, true }, { 200, 200, true }
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
            DEFINE_LIMIT(ProcTempObjectThreshold),
            DEFINE_LIMIT(ProcMatrixAllowance),
            FINISH_LIMITS()
        };
        return limits;
    }

    void ChangeLimit(int id, const std::string& value)
    {
        Setting& setting = settings[id];
        if(setting.threshold && IsUnlimited(value))
        {
            setting.value = -1;
            return;
        }

        const int limit = ParseLimit(value, setting.vanilla);
        setting.value = limit;
        if(limit == setting.vanilla || setting.threshold) return;

        switch(id)
        {
            case ProcObjectEntries: extraTrackingEntries.resize(limit - setting.vanilla); break;
            case ProcPlantTriangles: extraTriangles.resize(limit - setting.vanilla); break;
            case ProcCollisionEntries: extraCollisionEntries.resize(limit - setting.vanilla); break;
        }
    }

    void Process()
    {
        ApplyThreshold<uint16_t>(ProcTempObjectThreshold, 0x5A3353, 0x5A3355);
        ApplyThreshold<int32_t>(ProcMatrixAllowance, 0x5A3326, 0x5A332A);

        if(!extraTrackingEntries.empty())
        {
            originalInit = injector::MakeCALL(0x53BE53, &InitObjectTracking).get();
            injector::MakeCALL(0x5BA468, &InitObjectTracking);
        }
        if(!extraTriangles.empty() || !extraCollisionEntries.empty())
            originalReload = injector::MakeCALL(0x5DD913, &ReloadPlantCache).get();
    }

    template<class T>
    void ApplyThreshold(int id, uintptr_t immediate, uintptr_t jump)
    {
        const Setting& setting = settings[id];
        if(setting.value < 0)
            injector::MakeNOP(jump, 2, true);
        else if(setting.value != setting.vanilla)
            injector::WriteMemory(immediate, static_cast<T>(setting.value), true);
    }

    bool GetUsage(int id, std::string& output)
    {
        switch(id)
        {
            case ProcObjectEntries: return GetTrackingUsage(output);
            case ProcPlantTriangles:
            {
                uint32_t used = 0;
                auto* bands = TriangleBands();
                for(int i = 0; i < 4; ++i) used += Count(bands[i]);
                return ReportLinked(output, id, used, Count(UnusedTriangles()));
            }
            case ProcCollisionEntries:
                return ReportLinked(output, id, Count(ActiveCollisionEntries()), Count(UnusedCollisions()));
            case ProcTempObjectThreshold:
                return Report(output, id, *injector::lazy_pointer<0xBB4A70>().get<uint16_t>());
            case ProcMatrixAllowance:
                return Report(output, id, static_cast<uint32_t>(Manager().allocatedMatrices));
        }
        return false;
    }

    bool GetTrackingUsage(std::string& output)
    {
        auto& manager = Manager();
        const uint32_t used = CountObjects(manager.objects, 512) +
                              CountObjects(extraTrackingEntries.data(), extraTrackingEntries.size());
        return ReportLinked(output, ProcObjectEntries, used, Count(manager.unused.head));
    }

    bool ReportLinked(std::string& output, int id, uint32_t used, uint32_t free)
    {
        const uint32_t linked = used + free;
        const uint32_t total = linked ? linked : static_cast<uint32_t>(settings[id].value);
        return Adjuster::GetUsage(output, used, total);
    }

    bool Report(std::string& output, int id, uint32_t used)
    {
        if(settings[id].value < 0)
        {
            output = std::to_string(used) + " / unlimited";
            return true;
        }
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

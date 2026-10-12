/*
 *  Procedural Objects Limit Adjuster
 *  Licensed under the MIT License (http://opensource.org/licenses/MIT)
 */
#include "LimitAdjuster.h"
#include "ProceduralObjects.hpp"
#include <injector/hooking.hpp>

using namespace Procedural;

// SA 1.0 US layouts. See gta-reversed's ProcObjectMan, ProcSurfaceInfo,
// PlantLocTri, PlantColEntEntry and PlantMgr for the original list lifecycle.
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

/*
 *  Keys: ProcObjectEntries, ProcPlantTriangles, ProcCollisionEntries,
 *        ProcTempObjectThreshold, ProcMatrixAllowance
 *  Value: Storage total, or a creation threshold. The thresholds also accept unlimited.
 *  Game: SA 1.0 US
 */
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
    injector::scoped_jmp initObjectTracking, reloadPlantCache;

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
            DisableCheck(id);
            setting.value = -1;
            return;
        }

        const int limit = ParseLimit(value, setting.vanilla);
        if(limit == setting.vanilla) return;

        // Allocate before hooking. Linked storage keeps its address permanently.
        switch(id)
        {
            case ProcObjectEntries:
                extraTrackingEntries.resize(limit - setting.vanilla);
                initObjectTracking.make_jmp(0x5A3EA0, InitObjectTracking);
                break;
            case ProcPlantTriangles:
                extraTriangles.resize(limit - setting.vanilla);
                reloadPlantCache.make_jmp(0x5DD780, ReloadPlantCache);
                break;
            case ProcCollisionEntries:
                extraCollisionEntries.resize(limit - setting.vanilla);
                reloadPlantCache.make_jmp(0x5DD780, ReloadPlantCache);
                break;
            case ProcTempObjectThreshold:
                injector::WriteMemory<uint16_t>(FindCompare(0xBB4A70, 150, true), limit, true);
                break;
            case ProcMatrixAllowance:
                injector::WriteMemory<int32_t>(FindCompare(0xBB7CB0, 200, false), limit, true);
                break;
        }
        setting.value = limit;
    }

    bool GetUsage(int id, std::string& output)
    {
        uint32_t used = 0;
        switch(id)
        {
            case ProcObjectEntries:
                return GetTrackingUsage(output);
            case ProcPlantTriangles:
                for(int i = 0; i < 4; ++i)
                    used += Count(injector::lazy_pointer<0xC03988>().get<PlantTriangle*>()[i], settings[id].value);
                break;
            case ProcCollisionEntries:
                used = Count(ActiveCollisionEntries(), settings[id].value);
                break;
            case ProcTempObjectThreshold:
                used = *injector::lazy_pointer<0xBB4A70>().get<uint16_t>();
                break;
            case ProcMatrixAllowance: used = Manager().allocatedMatrices; break;
        }
        if(settings[id].value < 0)
        {
            output = std::to_string(used) + " / unlimited";
            return true;
        }
        return Adjuster::GetUsage(output, used, static_cast<uint32_t>(settings[id].value));
    }

    bool GetTrackingUsage(std::string& output)
    {
        // Count occupied tracking slots. Per-surface list counters are not reliable.
        auto& manager = Manager();
        const uint32_t total = 512 + static_cast<uint32_t>(extraTrackingEntries.size());
        const uint32_t used = CountObjects(manager.objects, 512) +
                              CountObjects(extraTrackingEntries.data(), extraTrackingEntries.size());
        return Adjuster::GetUsage(output, used, total);
    }

    static ObjectManager& Manager()
    {
        return *injector::lazy_pointer<0xBB7CB0>().get<ObjectManager>();
    }

    static CollisionEntry*& ActiveCollisionEntries()
    {
        return *injector::lazy_pointer<0xC0399C>().get<CollisionEntry*>();
    }

    static injector::memory_pointer_raw FindCompare(uint32_t counter, uint32_t limit, bool word)
    {
        auto* code = injector::lazy_pointer<0x5A32D0>().get<uint8_t>();
        return injector::raw_ptr(code + FindCounterCompare(code, 0x5A3850 - 0x5A32D0, counter, limit, word));
    }

    void DisableCheck(int id)
    {
        const bool word = id == ProcTempObjectThreshold;
        auto compare = FindCompare(word ? 0xBB4A70 : 0xBB7CB0, word ? 150 : 200, word);
        auto jump = compare + CompareJumpOffset(0, word);
        const uint8_t operation = injector::ReadMemory<uint8_t>(jump, true);
        if(operation < 0x70 || operation > 0x7F)
            throw std::runtime_error("Procedural check is not a short conditional jump");
        injector::MakeNOP(jump, 2, true);
    }

    static void __fastcall InitObjectTracking(ObjectManager* manager, int)
    {
        // Restore the entry point while calling SA, then reinstall it.
        auto& self = adjuster_ProceduralObjects;
        self.initObjectTracking.restore();
        void (__fastcall *init)(ObjectManager*, int) = injector::lazy_pointer<0x5A3EA0>().get();
        init(manager, 0);
        self.initObjectTracking.make_jmp(0x5A3EA0, InitObjectTracking);
        // CList_c::AddItem. Same free list the game uses for its own 512 entries.
        auto addItem = (void (__thiscall*)(ObjectList*, ObjectEntry*)) injector::lazy_pointer<0x4A8DF0>().get<void>();
        for(auto& entry : self.extraTrackingEntries)
        {
            entry = {};
            addItem(&manager->unused, &entry);
        }
    }

    static bool __cdecl ReloadPlantCache()
    {
        auto& self = adjuster_ProceduralObjects;
        self.reloadPlantCache.restore();

        // ReloadConfig fails before touching lists when PLANTS.DAT cannot load.
        // Do not release the live cache unless rebuilding will proceed.
        bool (*initialise)() = injector::lazy_pointer<0x5DD6C0>().get();
        const bool ready = initialise();
        if(ready)
        {
            void (__fastcall *release)(CollisionEntry*, int) = injector::lazy_pointer<0x5DB8A0>().get();
            while(auto* entry = ActiveCollisionEntries()) release(entry, 0);
        }

        bool (*reload)() = injector::lazy_pointer<0x5DD780>().get();
        const bool loaded = ready && reload();
        self.reloadPlantCache.make_jmp(0x5DD780, ReloadPlantCache);
        if(!loaded) return false;

        auto& triangles = *injector::lazy_pointer<0xC03984>().get<PlantTriangle*>();
        AttachUnused(triangles, self.extraTriangles.data(), self.extraTriangles.size());
        auto& collisions = *injector::lazy_pointer<0xC03998>().get<CollisionEntry*>();
        AttachUnused(collisions, self.extraCollisionEntries.data(), self.extraCollisionEntries.size());
        return true;
    }

// Instantiate the adjuster on the global scope.
} adjuster_ProceduralObjects;

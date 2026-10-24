#include "Core.h"
#include "Adjusters.h"

// SA procedural objects and grass. Three free lists of fixed size feed them: the tracking entries of procedural objects
// (512), the surface triangles they and the grass grow on (256) and the collision cache entries the triangles come from
// (40). Extra entries are added to each free list after the game builds it. Also: the object generator returns the low
// byte of the number of objects it placed, so a batch of 256 or 512 objects reads as a failure.

namespace
{
    struct Vector
    {
        float x, y, z;
    };

    struct PlantTriangle
    {
        Vector vertices[3];
        Vector center;
        float sphereRadius;
        float seeds[3];
        uint16_t numPlants[3];
        uint8_t surfaceId;
        uint8_t lighting;
        uint8_t flags;
        PlantTriangle* next;
        PlantTriangle* prev;
    };
    static_assert(sizeof(PlantTriangle) == 0x54 && offsetof(PlantTriangle, next) == 0x4C);

    struct CollisionEntry
    {
        void* entity;
        PlantTriangle** triangles;
        uint16_t numTriangles;
        CollisionEntry* next;
        CollisionEntry* prev;
    };
    static_assert(sizeof(CollisionEntry) == 0x14 && offsetof(CollisionEntry, next) == 0xC);

    struct ObjectEntry
    {
        ObjectEntry* prev;
        ObjectEntry* next;
        void* object;
        PlantTriangle* triangle;
        bool allocatedMatrix;
    };
    static_assert(sizeof(ObjectEntry) == 0x14 && offsetof(ObjectEntry, object) == 0x8);

    struct ObjectList
    {
        ObjectEntry* head;
        ObjectEntry* tail;
        uint32_t count;
    };

    struct SurfaceInfo
    {
        uint8_t properties[0x3C]; // properties[0] is the surface type
        ObjectList objects;
    };
    static_assert(sizeof(SurfaceInfo) == 0x48);

    struct ObjectManager
    {
        int32_t allocatedMatrices;
        int32_t numSurfaceInfos;
        SurfaceInfo surfaceInfos[128];
        ObjectEntry objects[512];
        ObjectList unused;
    };
    static_assert(offsetof(ObjectManager, unused) == 0x4C08);

    constexpr uintptr_t ObjectManagerAddress = 0xBB7CB0;
    constexpr uintptr_t UnusedTriangles = 0xC03984;
    constexpr uintptr_t TriangleBands = 0xC03988; // four lists of triangles in use
    constexpr uintptr_t UnusedCollisions = 0xC03998;
    constexpr uintptr_t ActiveCollisions = 0xC0399C;

    std::vector<ObjectEntry> extraObjects;
    std::vector<PlantTriangle> extraTriangles;
    std::vector<CollisionEntry> extraCollisions;

    using InitObjectsFn = bool(__thiscall*)(ObjectManager*);
    using AddObjectsFn = int(__thiscall*)(SurfaceInfo*, PlantTriangle*);
    using AddToListFn = void(__thiscall*)(ObjectList*, ObjectEntry*);
    using ReloadFn = bool(__cdecl*)();
    InitObjectsFn initObjects = (InitObjectsFn)0x5A3EA0;
    AddObjectsFn addObjects = (AddObjectsFn)0x5A3850;
    AddToListFn addToList = (AddToListFn)0x4A8DF0;
    ReloadFn reloadPlants = (ReloadFn)0x5DD780;
    auto plantsInitialise = (bool(__cdecl*)())0x5DD6C0;
    auto releaseCollision = (void(__thiscall*)(CollisionEntry*))0x5DB8A0;

    template<class T>
    void PrependAll(T*& head, std::vector<T>& entries)
    {
        for (size_t i = entries.size(); i-- > 0;)
        {
            entries[i] = {};
            entries[i].prev = nullptr;
            entries[i].next = head;
            if (head)
                head->prev = &entries[i];
            head = &entries[i];
        }
    }

    template<class T>
    int CountList(T* entry)
    {
        int count = 0;
        for (; entry && count < 0x10000; entry = entry->next)
            count++;
        return count;
    }

    bool __fastcall InitObjects(ObjectManager* manager, void*)
    {
        bool ok = initObjects(manager);
        for (auto& entry : extraObjects)
        {
            entry = {};
            addToList(&manager->unused, &entry);
        }
        return ok;
    }

    // Every surface rule that matches the triangle places its objects; the result is whether any were placed.
    bool __fastcall GenerateObjects(ObjectManager* manager, void*, PlantTriangle* triangle)
    {
        bool placed = false;
        for (int i = 0; i < manager->numSurfaceInfos; i++)
        {
            auto& surface = manager->surfaceInfos[i];
            if (surface.properties[0] == triangle->surfaceId && addObjects(&surface, triangle) > 0)
                placed = true;
        }
        return placed;
    }

    bool __cdecl ReloadPlants()
    {
        // The reload rebuilds the free lists from the game's own arrays, so entries in use are released first
        if (plantsInitialise())
        {
            while (auto entry = *(CollisionEntry**)ActiveCollisions)
                releaseCollision(entry);
        }
        if (!reloadPlants())
            return false;
        PrependAll(*(PlantTriangle**)UnusedTriangles, extraTriangles);
        PrependAll(*(CollisionEntry**)UnusedCollisions, extraCollisions);
        return true;
    }

    void RedirectCall(uintptr_t site, void* target)
    {
        mem::WriteDword(site + 1, (uint32_t)((uintptr_t)target - (site + 5)));
    }

    bool CallsTo(uintptr_t site, uintptr_t target)
    {
        return *(uint8_t*)site == 0xE8 && site + 5 + *(int32_t*)(site + 1) == target;
    }

    uint32_t Capacity(std::string_view key, uint32_t vanilla, uint32_t unlimited)
    {
        auto setting = core::GetSetting(key, "unlimited");
        if (!setting.apply)
            return vanilla;
        if (setting.unlimited || setting.automatic)
            return unlimited;
        return std::clamp<uint32_t>(setting.value, vanilla, 0xFFFF);
    }
}

namespace limits
{
    void InstallProcedural()
    {
        if (!core::IsSA())
            return;
        if (!CallsTo(0x53BE53, 0x5A3EA0) || !CallsTo(0x5BA468, 0x5A3EA0) || !CallsTo(0x5DCD31, 0x5A3F20) ||
            !CallsTo(0x5DBF6B, 0x5A3F20) || !CallsTo(0x5DD913, 0x5DD780))
        {
            core::Log("Procedural: the game code is not the expected one (another mod changed it?), left as it is");
            return;
        }

        // unlimited: far more than the game uses even with long procedural draw distances
        auto objects = Capacity("ProcObjects", 512, 16384);
        auto triangles = Capacity("PlantTriangles", 256, 8192);
        auto collisions = Capacity("PlantCollisionEntries", 40, 2048);
        extraObjects.resize(objects - 512);
        extraTriangles.resize(triangles - 256);
        extraCollisions.resize(collisions - 40);

        if (!extraObjects.empty())
        {
            RedirectCall(0x53BE53, (void*)InitObjects);
            RedirectCall(0x5BA468, (void*)InitObjects);
        }
        RedirectCall(0x5DCD31, (void*)GenerateObjects);
        RedirectCall(0x5DBF6B, (void*)GenerateObjects);
        if (!extraTriangles.empty() || !extraCollisions.empty())
            RedirectCall(0x5DD913, (void*)ReloadPlants);

        core::RegisterLimit({ "ProcObjects", []
        {
            auto& manager = *(ObjectManager*)ObjectManagerAddress;
            int used = 0;
            for (auto& entry : manager.objects)
                used += entry.object != nullptr;
            for (auto& entry : extraObjects)
                used += entry.object != nullptr;
            return used;
        }, [objects] { return (int)objects; }, [objects] { return (int)objects; } });
        core::RegisterLimit({ "PlantTriangles", []
        {
            int used = 0;
            for (int i = 0; i < 4; i++)
                used += CountList(((PlantTriangle**)TriangleBands)[i]);
            return used;
        }, [triangles] { return (int)triangles; }, [triangles] { return (int)triangles; } });
        core::RegisterLimit({ "PlantCollisionEntries", [] { return CountList(*(CollisionEntry**)ActiveCollisions); },
            [collisions] { return (int)collisions; }, [collisions] { return (int)collisions; } });
    }
}

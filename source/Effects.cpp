#include "Core.h"
#include "Adjusters.h"
#include <cmath>
#include <new>
#include "WaterReferences.h"

namespace
{
    std::vector<SafetyHookMid> hooks;

    // III/VC particles come from a free list. When it runs empty another block is linked in. CParticle::ReloadConfig
    // (new game, load) rebuilds the free list from the game's own array, so the extra blocks are linked in again after it.
    struct Particles
    {
        uintptr_t head;
        uintptr_t array;
        uint32_t elemSize;
        uint32_t nextOffset;
        uint32_t total;
        uint32_t maximum;
        std::vector<std::pair<uint8_t*, uint32_t>> blocks;
        std::vector<uint8_t> fresh; // a particle as ReloadConfig leaves it
    };
    Particles particles{};
    SafetyHookInline reloadConfigHook;

    void LinkBlock(uint8_t* block, uint32_t count)
    {
        auto& p = particles;
        for (uint32_t i = 0; i < count; i++)
        {
            auto particle = block + (size_t)i * p.elemSize;
            if (!p.fresh.empty())
                memcpy(particle, p.fresh.data(), p.elemSize);
            *(uintptr_t*)(particle + p.nextOffset) = i + 1 < count ? (uintptr_t)(particle + p.elemSize) : *(uintptr_t*)p.head;
        }
        *(uintptr_t*)p.head = (uintptr_t)block;
    }

    void GrowParticles(SafetyHookContext&)
    {
        auto& p = particles;
        if (*(uintptr_t*)p.head != 0 || p.total >= p.maximum)
            return;

        auto count = std::min<uint32_t>(std::max<uint32_t>(p.total / 2, 256), p.maximum - p.total);
        auto block = (uint8_t*)calloc(count, p.elemSize);
        if (!block)
            return;
        p.blocks.push_back({ block, count });
        p.total += count;
        LinkBlock(block, count);
    }

    void __cdecl ReloadConfigHook()
    {
        reloadConfigHook.ccall();
        auto& p = particles;
        p.fresh.assign((uint8_t*)p.array, (uint8_t*)p.array + p.elemSize);
        for (auto& [block, count] : p.blocks)
            LinkBlock(block, count);
    }

    void HookParticles(uintptr_t head, uintptr_t array, uint32_t elemSize, uint32_t nextOffset, uint32_t vanilla, uintptr_t emptyCheck, uintptr_t reloadConfig)
    {
        auto setting = core::GetSetting("Particles", "unlimited");
        if (!setting.apply)
            return;

        particles.head = head;
        particles.array = array;
        particles.elemSize = elemSize;
        particles.nextOffset = nextOffset;
        particles.total = vanilla;
        particles.maximum = std::max(core::MaxCapacity(setting, 100000), vanilla);
        hooks.push_back(safetyhook::create_mid(emptyCheck, GrowParticles));
        reloadConfigHook = safetyhook::create_inline(reloadConfig, ReloadConfigHook);
        core::RegisterLimit({ "Particles", [] { return -1; }, [] { return (int)particles.total; }, [] { return (int)particles.maximum; } });
    }

    // SA effects: the emitter particles live in the FX memory pool, so both grow together.
    void SetFxParticlesSA()
    {
        auto setting = core::GetSetting("FxParticles", "4000");
        if (!setting.apply)
            return;

        const mem::Expect expected[] =
        {
            { 0x4A992E, 1000 * 0x3C + 4 }, { 0x4A9951, 1000 }, { 0x4A995D, 1000 }, { 0x4A998A, 1000 * 0x3C },
            { 0x4A9B30, 1000 * 0x3C + 4 }, { 0x4A9B57, 1000 }, { 0x4A9B63, 1000 }, { 0x4A9B92, 1000 * 0x3C },
            { 0x4A9C37, 0x100000 }, { 0x4A9C3E, 0x100000 },
        };
        if (!mem::CheckValues(expected, "FxParticles"))
            return;

        auto count = std::clamp<uint32_t>(setting.unlimited ? 4000 : setting.value, 1000, 100000);
        for (auto operand : { 0x4A992E, 0x4A9B30 })
            mem::WriteDword(operand, count * 0x3C + 4);
        for (auto operand : { 0x4A9951, 0x4A995D, 0x4A9B57, 0x4A9B63 })
            mem::WriteDword(operand, count);
        for (auto operand : { 0x4A998A, 0x4A9B92 })
            mem::WriteDword(operand, count * 0x3C);
        for (auto operand : { 0x4A9C37, 0x4A9C3E })
            mem::WriteDword(operand, 0x100000 + (count - 1000) * 0x3C);

        core::RegisterLimit({ "FxParticles", [] { return -1; }, [count] { return (int)count; }, [count] { return (int)count; } });
    }

    struct WaterStorage
    {
        const char* key;
        uintptr_t count;
        uint32_t stride;
        uint32_t capacity;
        uint32_t maximum;
        mem::Reserve memory;
    };
    WaterStorage waterVertices{ "WaterVertices", 0xC2288C, 20, 1021, 1021 };
    WaterStorage waterQuads{ "WaterQuads", 0xC22888, 10, 301, 301 };
    WaterStorage waterTris{ "WaterTris", 0xC22884, 8, 6, 6 };
    WaterStorage waterZones{ "WaterZonePolys", 0xC215F0, 2, 700, 700 };

    bool PrepareWater(WaterStorage& storage, uintptr_t base, uint32_t maximum, std::span<const mem::Expect> refs)
    {
        storage.maximum = maximum;
        if (!storage.memory.Create((size_t)maximum * storage.stride, (size_t)storage.capacity * storage.stride))
            return false;
        return true;
    }

    void MoveWater(WaterStorage& storage, uintptr_t base, std::span<const mem::Expect> refs)
    {
        for (auto& ref : refs)
            mem::WriteDword(ref.operand, (uintptr_t)storage.memory.data() + ref.value - base);
        core::RegisterLimit({ storage.key, [&storage] { return *(int*)storage.count; },
            [&storage] { return (int)storage.capacity; }, [&storage] { return (int)storage.maximum; } });
    }

    bool EnsureWater(WaterStorage& storage, uint32_t needed)
    {
        if (needed > storage.maximum)
            return false;
        if (needed <= storage.capacity)
            return true;
        auto capacity = std::min(storage.maximum, std::max(needed, storage.capacity + storage.capacity / 2 + 16));
        if (!storage.memory || !storage.memory.Commit((size_t)capacity * storage.stride))
            return false;
        storage.capacity = capacity;
        return true;
    }

    void GuardWaterPolygon(SafetyHookContext& ctx, bool quad)
    {
        auto& polygons = quad ? waterQuads : waterTris;
        auto count = *(uint32_t*)polygons.count;
        auto vertices = *(uint32_t*)waterVertices.count;
        // Some consumers sign-extend the vertex words. Reserve every possible new vertex before the first write.
        if (!EnsureWater(polygons, count + 1) || !EnsureWater(waterVertices, vertices + (quad ? 4 : 3)))
        {
            ctx.eax = 0;
            ctx.eip = quad ? 0x6E81DE : 0x6E7EEA;
            core::Log("{}: water plane omitted at the configured/index bound", polygons.key);
        }
    }

    void GuardWaterZone(SafetyHookContext& ctx)
    {
        auto column = *(uint32_t*)(ctx.esp + 4);
        auto row = *(uint32_t*)(ctx.esp + 8);
        if (column >= 12 || row >= 12)
        {
            ctx.eip = 0x6E5787;
            return;
        }
        auto type = *(uint16_t*)(0xC21B70 + (column * 12 + row) * 2) >> 14;
        auto count = *(uint32_t*)waterZones.count;
        auto extra = type == 0 ? 0u : type == 3 ? 1u : 3u;
        if (!EnsureWater(waterZones, count + extra))
        {
            ctx.eax = 0;
            ctx.eip = 0x6E5787;
            core::Log("WaterZonePolys: zone entry omitted at the 14-bit index bound");
        }
    }

    void SetWaterPlanesSA()
    {
        auto vertices = core::GetSetting("WaterVertices", "unlimited");
        auto quads = core::GetSetting("WaterQuads", "unlimited");
        auto tris = core::GetSetting("WaterTris", "unlimited");
        if (!vertices.apply && !quads.apply && !tris.apply)
            return;
        if (!mem::CheckValues(waterVertexRefs, "WaterVertices") || !mem::CheckValues(waterQuadRefs, "WaterQuads") ||
            !mem::CheckValues(waterTriRefs, "WaterTris") || !mem::CheckValues(waterZoneRefs, "WaterZonePolys"))
            return;
        const uint8_t quadBytes[] = { 0x8B, 0x4C, 0x24, 0x04, 0x8B, 0x44, 0x24, 0x1C };
        const uint8_t triBytes[] = { 0x8B, 0x4C, 0x24, 0x04, 0x53, 0x55 };
        const uint8_t zoneBytes[] = { 0x8B, 0x44, 0x24, 0x04, 0x8B, 0x4C, 0x24, 0x08 };
        if (memcmp((void*)0x6E7EF0, quadBytes, sizeof(quadBytes)) ||
            memcmp((void*)0x6E7D40, triBytes, sizeof(triBytes)) ||
            memcmp((void*)0x6E5750, zoneBytes, sizeof(zoneBytes)) ||
            *(uint8_t*)0x6E81DE != 0xC3 || *(uint8_t*)0x6E7EEA != 0xC3 || *(uint8_t*)0x6E5787 != 0xC3)
        {
            core::Log("Water planes: unexpected insertion code, left as it is");
            return;
        }
        auto q = quads.apply ? std::max(301u, core::MaxCapacity(quads, 16383)) : 301u;
        auto t = tris.apply ? std::max(6u, core::MaxCapacity(tris, 16383)) : 6u;
        auto derived = std::min(32768u, q * 4 + t * 3);
        auto v = vertices.apply ? std::max(1021u, vertices.unlimited ? derived : core::MaxCapacity(vertices, 32768)) : 1021u;
        // Prepare every allocation before changing any reference.
        if ((vertices.apply && !PrepareWater(waterVertices, 0xC22910, v, waterVertexRefs)) ||
            (quads.apply && !PrepareWater(waterQuads, 0xC21C90, q, waterQuadRefs)) ||
            (tris.apply && !PrepareWater(waterTris, 0xC22854, t, waterTriRefs)) ||
            !PrepareWater(waterZones, 0xC215F8, 16384, waterZoneRefs))
            return;
        auto quadHook = safetyhook::create_mid(0x6E7EF0, [](SafetyHookContext& ctx) { GuardWaterPolygon(ctx, true); });
        auto triHook = safetyhook::create_mid(0x6E7D40, [](SafetyHookContext& ctx) { GuardWaterPolygon(ctx, false); });
        auto zoneHook = safetyhook::create_mid(0x6E5750, GuardWaterZone);
        if (!quadHook || !triHook || !zoneHook)
            return;
        if (vertices.apply)
            MoveWater(waterVertices, 0xC22910, waterVertexRefs);
        if (quads.apply)
            MoveWater(waterQuads, 0xC21C90, waterQuadRefs);
        if (tris.apply)
            MoveWater(waterTris, 0xC22854, waterTriRefs);
        MoveWater(waterZones, 0xC215F8, waterZoneRefs);
        hooks.push_back(std::move(quadHook));
        hooks.push_back(std::move(triHook));
        hooks.push_back(std::move(zoneHook));
    }

    const char* waterName = "OutsideWorldWaterBlocks";
    const uintptr_t waterXRefsSA[] = { 0x6E6CF2, 0x6EF6E4, 0x6EFE86 };
    const uintptr_t waterYRefsSA[] = { 0x6E6CFA, 0x6EF6EC, 0x6EFE99, 0x6EFEB3 };
    std::unique_ptr<int16_t[]> waterX, waterY;
    int32_t waterCapacity{ 70 };
    bool waterUnlimited{};

    bool CheckWaterBytes(uintptr_t address, std::initializer_list<uint8_t> bytes)
    {
        if (memcmp((void*)address, bytes.begin(), bytes.size()) == 0)
            return true;
        core::Log("{}: unexpected code at {:X}, left as it is", waterName, address);
        return false;
    }

    bool ResizeWaterBlocks(uint32_t count)
    {
        if (count <= (uint32_t)waterCapacity && waterX)
            return true;
        auto x = std::unique_ptr<int16_t[]>(new (std::nothrow) int16_t[count]);
        auto y = std::unique_ptr<int16_t[]>(new (std::nothrow) int16_t[count]);
        if (!x || !y)
        {
            core::Log("{}: cannot allocate {} blocks", waterName, count);
            return false;
        }
        // Called before the scan resets the list; no live block pointers escape the renderer.
        for (auto operand : waterXRefsSA)
            mem::WriteDword(operand, (uintptr_t)x.get());
        for (auto operand : waterYRefsSA)
            mem::WriteDword(operand, (uintptr_t)y.get());
        waterX = std::move(x);
        waterY = std::move(y);
        waterCapacity = (int32_t)count;
        return true;
    }

    // The far plane is a rectangle, so its corners reach further than the centre of the view.
    double WaterViewRadius(uintptr_t camera, float timeCycleFarClip)
    {
        auto farClip = std::max(timeCycleFarClip, *(float*)(camera + 0x84));
        auto x = *(float*)(camera + 0x68);
        auto y = *(float*)(camera + 0x6C);
        return farClip * std::sqrt(1.0 + (double)x * x + (double)y * y);
    }

    void UpdateWaterBlocksSA(SafetyHookContext&)
    {
        auto camera = *(uintptr_t*)0xB6F97C;
        if (!camera)
            return;
        auto radius = WaterViewRadius(camera, *(float*)0xB7C4F0);
        if (!std::isfinite(radius) || radius < 0.0)
            return;
        // Every orientation and sub-block camera position fits inside this square.
        auto side = std::ceil(2.0 * radius / 500.0) + 3.0;
        auto count = side * side;
        if (count <= waterCapacity || count > INT32_MAX)
            return;
        auto grown = std::min((uint32_t)INT32_MAX, (uint32_t)waterCapacity + (uint32_t)waterCapacity / 2);
        auto capacity = std::max((uint32_t)count, grown);
        static uint32_t failedCapacity{};
        if (capacity != failedCapacity && !ResizeWaterBlocks(capacity))
            failedCapacity = capacity;
    }

    void FlushWaterBlocksSA(uint32_t vertices, uint32_t indices)
    {
        if (*(uint32_t*)0xC4B950 + vertices > 2048 || *(uint32_t*)0xC4B954 + indices > 4096)
            reinterpret_cast<void(__cdecl*)()>(0x6E7680)();
    }

    void SetWaterBlocksSA()
    {
        auto setting = core::GetSetting(waterName, "unlimited");
        const uintptr_t full[] = { 0x6E6CE9 };
        if (!setting.apply || !mem::CheckCompares(full, 70, waterName) ||
            !mem::CheckRefs(waterXRefsSA, 0xC21560, 140, waterName) ||
            !mem::CheckRefs(waterYRefsSA, 0xC214D0, 140, waterName) ||
            !CheckWaterBytes(0x6E6D10, { 0x83, 0xEC, 0x64, 0xA1, 0x7C, 0xF9, 0xB6, 0 }) ||
            !CheckWaterBytes(0x6E6870, { 0x8B, 0x0D, 0x50, 0xB9, 0xC4, 0 }) ||
            !CheckWaterBytes(0x6E6A10, { 0x83, 0xEC, 0x20, 0xD9, 0x44, 0x24, 0x30 }) ||
            !CheckWaterBytes(0x6E7680, { 0xA1, 0x50, 0xB9, 0xC4, 0, 0x56, 0x33, 0xF6 }) ||
            !CheckWaterBytes(0x6E9963, { 0xE8, 0x18, 0xDD, 0xFF, 0xFF }))
            return;

        auto count = setting.unlimited ? 70u : std::max(70u, std::min(setting.value, (uint32_t)INT32_MAX));
        // Both seabed writers lack the capacity checks already present in the surface renderer.
        auto flat = safetyhook::create_mid(0x6E6870, [](SafetyHookContext&) { FlushWaterBlocksSA(4, 6); });
        auto detailed = safetyhook::create_mid(0x6E6A10, [](SafetyHookContext&) { FlushWaterBlocksSA(64, 96); });
        SafetyHookMid scan;
        if (setting.unlimited)
            scan = safetyhook::create_mid(0x6E6D10, UpdateWaterBlocksSA);
        if (!flat || !detailed || (setting.unlimited && !scan))
            return;
        if (!mem::PatchCompare(0x6E6CE9, &waterCapacity) || !ResizeWaterBlocks(count))
            return;
        hooks.push_back(std::move(flat));
        hooks.push_back(std::move(detailed));
        waterUnlimited = setting.unlimited;
        if (waterUnlimited)
            hooks.push_back(std::move(scan));
        core::RegisterLimit({ waterName, [] { return *(int*)0xC215EC; }, [] { return waterCapacity; },
                              [] { return waterUnlimited ? INT32_MAX : waterCapacity; } });
    }

    struct WaterLines
    {
        int32_t lines{ 5 };
        int32_t across{ 26 };
        int32_t end{ 21 };
        uintptr_t start{};
        uintptr_t offset{};
        uintptr_t camera{};
        uintptr_t farClip{};
        float xOffset{};
    };
    WaterLines waterLines;

    void RenderNorthWaterIII(SafetyHookContext& ctx)
    {
        struct Point
        {
            float x, y, z;
        };
        auto camera = (void*)0x6FACF8;
        auto matrix = reinterpret_cast<void*(__thiscall*)(void*)>(0x42C8B0)(camera);
        auto color = (const void*)(ctx.esp + 0xCC);
        // III only draws the southern strip and the two sides. Complete the northern corners as well.
        for (int32_t y = 0; y < waterLines.lines; y++)
        {
            for (int32_t x = 0; x < waterLines.across; x++)
            {
                Point centre{ x * 256.0f - 2048.0f - waterLines.lines * 256.0f + 128.0f,
                              2048.0f + y * 256.0f + 128.0f, 0.0f };
                if (reinterpret_cast<bool(__thiscall*)(void*, const Point*, float, void*)>(0x42C760)(camera, &centre, 363.52f, matrix))
                    reinterpret_cast<void(__cdecl*)(float, float, float, const void*)>(0x557320)(centre.x - 128.0f, centre.y - 128.0f, 0.0f, color);
            }
        }
    }

    void SetWaterLineCount(int32_t lines)
    {
        auto& w = waterLines;
        if (w.lines == lines)
            return;
        w.lines = lines;
        w.across = lines * 2 + 16;
        w.end = lines + 16;
        mem::WriteDword(w.start, lines);
        ProtectedGameRef<float>((float*)w.offset) = lines * 256.0f;
    }

    void UpdateWaterLines(SafetyHookContext&)
    {
        auto& w = waterLines;
        auto camera = *(uintptr_t*)w.camera;
        if (!camera)
            return;
        auto radius = WaterViewRadius(camera, *(float*)w.farClip);
        auto frame = *(uintptr_t*)(camera + 4);
        if (!frame || !std::isfinite(radius) || radius < 0.0)
            return;
        auto x = std::abs(*(float*)(frame + 0x40) + w.xOffset);
        auto y = std::abs(*(float*)(frame + 0x44));
        auto outside = std::max(0.0f, std::max(x, y) - 2048.0f);
        auto lines = std::ceil((radius + outside) / 256.0) + 1.0;
        if (lines <= (INT32_MAX - 16) / 2)
            SetWaterLineCount(std::max(5, (int32_t)lines));
    }

    // III/VC draw strips of 256-unit tiles. The compares use full-width live bounds.
    void SetWaterLines(uintptr_t inner, uintptr_t outer, uintptr_t start, uintptr_t inner2, uintptr_t outer2,
                       uintptr_t offset, uintptr_t render, uintptr_t camera, uintptr_t farClip)
    {
        auto setting = core::GetSetting(waterName, "unlimited");
        if (!setting.apply)
            return;
        const uintptr_t inside[] = { inner, inner2 };
        const uintptr_t across[] = { outer };
        const uintptr_t end[] = { outer2 };
        const mem::Expect expected[] = { { start, 5 }, { offset, 0x44A00000 } };
        if (!mem::CheckCompares(inside, 5, waterName) || !mem::CheckCompares(across, 26, waterName) ||
            !mem::CheckCompares(end, 21, waterName) || !mem::CheckValues(expected, waterName) ||
            !CheckWaterBytes(render, { 0x53, 0x56, 0x57, 0x55, 0x81, 0xEC }))
            return;

        SafetyHookMid north;
        if (core::IsIII())
        {
            const uintptr_t indices[] = { 0x557320 };
            const uintptr_t vertices[] = { 0x557331 };
            if (!CheckWaterBytes(0x556780, { 0xE8, 0x1B, 0x17, 0, 0 }) ||
                !CheckWaterBytes(0x42C8B0, { 0x8D, 0x81, 0xA8, 0xE8, 0, 0, 0xC3 }) ||
                !CheckWaterBytes(0x42C760, { 0x53, 0x89, 0xCB, 0x83, 0xEC, 0x10 }) ||
                !mem::CheckCompares(indices, 1018, waterName) || !mem::CheckCompares(vertices, 252, waterName) ||
                !CheckWaterBytes(0x55733D, { 0xE8, 0x5E, 0x0B, 0, 0 }))
                return;
            north = safetyhook::create_mid(0x556780, RenderNorthWaterIII);
            if (!north)
                return;
        }

        waterLines.start = start;
        waterLines.offset = offset;
        waterLines.camera = camera;
        waterLines.farClip = farClip;
        waterLines.xOffset = core::IsVC() ? 400.0f : 0.0f;
        for (auto address : inside)
            if (!mem::PatchCompare(address, &waterLines.lines))
                return;
        if (!mem::PatchCompare(outer, &waterLines.across) || !mem::PatchCompare(outer2, &waterLines.end))
            return;
        if (setting.unlimited)
            hooks.push_back(safetyhook::create_mid(render, UpdateWaterLines));
        else
            SetWaterLineCount((int32_t)std::clamp<uint32_t>(setting.value, 5, (INT32_MAX - 16) / 2));
        if (north)
            hooks.push_back(std::move(north));
        core::RegisterLimit({ waterName, [] { return -1; }, [] { return waterLines.lines; },
                              [unlimited = setting.unlimited] { return unlimited ? (INT32_MAX - 16) / 2 : waterLines.lines; } });
    }
}

namespace limits
{
    void InstallEffects()
    {
        if (core::IsIII())
        {
            HookParticles(0x8F3958, 0x62F88C, 0x68, 0x64, 1000, 0x50D214, 0x50C430);
            SetWaterLines(0x5564BD, 0x5564C7, 0x5564D1, 0x55676D, 0x556777, 0x602B48, 0x5554E0, 0x72676C, 0x8F5FD8);
        }
        else if (core::IsVC())
        {
            HookParticles(0x98002C, 0x76E4EC, 0x58, 0x54, 750, 0x563D74, 0x565940);
            SetWaterLines(0x5C24CF, 0x5C24D9, 0x5C24E3, 0x5C28DF, 0x5C28E9, 0x69CDCC, 0x5C1710, 0x8100BC, 0x9B6A6C);
        }
        else if (core::IsSA())
        {
            SetFxParticlesSA();
            SetWaterBlocksSA();
            SetWaterPlanesSA();
        }
    }
}

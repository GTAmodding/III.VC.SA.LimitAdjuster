// Host-side regression tests for intrusive-list lifetime and patch operand width.
// The ASI cross-build separately checks all 32-bit game structure layouts.
#include "../../src/limits/Procedural/ProceduralObjects.hpp"
#include <cassert>
#include <iostream>
#include <vector>

using namespace Procedural;

template<class F>
void Throws(F action)
{
    bool threw = false;
    try { action(); } catch(const std::exception&) { threw = true; }
    assert(threw);
}

ObjectEntry* Pop(ObjectList& list)
{
    auto* entry = list.head;
    if(entry)
    {
        list.head = entry->next;
        if(list.head) list.head->prev = nullptr;
        else list.tail = nullptr;
        --list.count;
        entry->prev = entry->next = nullptr;
    }
    return entry;
}

void CheckList(const ObjectList& list)
{
    uint32_t count = 0;
    ObjectEntry* previous = nullptr;
    for(auto* entry = list.head; entry; entry = entry->next)
    {
        assert(++count <= list.count); // Also detects cycles / double insertion.
        assert(entry->prev == previous);
        previous = entry;
    }
    assert(count == list.count && previous == list.tail);
}

void ObjectLifecycle()
{
    ObjectEntry vanilla[512] = {};
    std::vector<ObjectEntry> extra(512);
    SurfaceInfo surface = {};
    ObjectList unused = {};
    for(int game = 0; game < 3; ++game)
    {
        // SA's original Init links vanilla entries; the hook only adds extras.
        for(auto& entry : vanilla) unused.Add(entry);
        AttachUnused(unused, extra.data(), extra.size());
        assert(unused.count == 1024);
        CheckList(unused);

        // Allocate all tracking entries across vanilla and extended storage.
        while(unused.head)
        {
            auto* entry = Pop(unused);
            entry->object = &surface;
            surface.objects.Add(*entry);
        }
        assert(surface.objects.count == 1024);
        CheckList(surface.objects);
        assert(!unused.head && !unused.tail && !unused.count);

        // Exit deletes entities and returns tracking entries before clearing the
        // free list. Reuse the same backing arrays on the next game.
        while(auto* entry = Pop(surface.objects))
        {
            entry->object = nullptr;
            unused.Add(*entry);
        }
        CheckList(unused);
        unused = {};
    }

}

template<class T>
void PlantLifecycle(size_t vanillaCount, size_t extraCount)
{
    std::vector<T> vanilla(vanillaCount), extra(extraCount);
    for(int reload = 0; reload < 3; ++reload)
    {
        T* head = nullptr;
        AttachUnused(head, vanilla.data(), vanilla.size());
        assert(head == vanilla.data());
        AttachUnused(head, extra.data(), extra.size());
        T* previous = nullptr;
        size_t count = 0;
        for(auto* entry = head; entry; entry = entry->next)
        {
            assert(++count <= vanillaCount + extraCount);
            assert(entry->prev == previous);
            previous = entry;
        }
        assert(count == vanillaCount + extraCount);
    }
}

void CounterPatches()
{
    uint8_t code[] = {
        0xB8, 0x96, 0, 0, 0,                         // unrelated mov eax,150
        0x66, 0x81, 0x3D, 0x70, 0x4A, 0xBB, 0, 0x96, 0, // cmp word [BB4A70],150
        0x73, 0x10,                                  // branch must remain intact
        0x81, 0x3D, 0xB0, 0x7C, 0xBB, 0, 0xC8, 0, 0, 0, // cmp dword [BB7CB0],200
        0x7D, 0x10
    };
    const auto temp = FindCounterCompare(code, sizeof(code), 0xBB4A70, 150, true);
    const auto matrix = FindCounterCompare(code, sizeof(code), 0xBB7CB0, 200, false);
    assert(temp == 12 && matrix == 22);
    assert(CompareJumpOffset(temp, true) == 14);
    assert(CompareJumpOffset(matrix, false) == 26);
    assert(code[CompareJumpOffset(temp, true)] == 0x73);
    assert(code[CompareJumpOffset(matrix, false)] == 0x7D);
    Throws([&] { FindCounterCompare(code, sizeof(code), 0xBB4A72, 150, true); });
    Throws([&] { FindCounterCompare(code, 13, 0xBB4A70, 150, true); });
    std::vector<uint8_t> ambiguous(code, code + sizeof(code));
    ambiguous.insert(ambiguous.end(), code, code + sizeof(code));
    Throws([&] { FindCounterCompare(ambiguous.data(), ambiguous.size(), 0xBB4A70, 150, true); });

    uint16_t temporaryLimit = 65535;
    uint32_t matrixLimit = 400;
    std::memcpy(code + temp, &temporaryLimit, sizeof(temporaryLimit));
    std::memcpy(code + matrix, &matrixLimit, sizeof(matrixLimit));
    assert(code[14] == 0x73 && code[15] == 0x10);
    assert(code[26] == 0x7D && code[27] == 0x10);
    assert(FindCounterCompare(code, sizeof(code), 0xBB4A70, 65535, true) == temp);
    assert(FindCounterCompare(code, sizeof(code), 0xBB7CB0, 400, false) == matrix);
}

void ObjectUsage()
{
    // The reported occupancy must not depend on stale/inflated list counters.
    std::vector<ObjectEntry> vanilla(512), extra(65535 - 512);
    SurfaceInfo surface = {};
    surface.objects.count = 70000;
    assert(CountObjects(vanilla.data(), vanilla.size()) == 0);
    assert(CountObjects(nullptr, 0) == 0);
    for(auto& entry : vanilla) entry.object = &surface;
    for(auto& entry : extra) entry.object = &surface;
    assert(CountObjects(vanilla.data(), vanilla.size()) +
           CountObjects(extra.data(), extra.size()) == 65535);
    extra.back().object = nullptr;
    assert(CountObjects(vanilla.data(), vanilla.size()) +
           CountObjects(extra.data(), extra.size()) == 65534);
}

int main()
{
    ObjectLifecycle();
    PlantLifecycle<PlantTriangle>(256, 256);
    PlantLifecycle<CollisionEntry>(40, 40);
    PlantLifecycle<PlantTriangle>(256, 0);
    CounterPatches();
    ObjectUsage();
    for(int minimum : {512, 256, 40, 150, 200})
    {
        assert(ParseLimit(std::to_string(minimum), minimum) == minimum);
        assert(ParseLimit("65535", minimum) == 65535);
        Throws([&] { ParseLimit(std::to_string(minimum - 1), minimum); });
        Throws([&] { ParseLimit("65536", minimum); });
        Throws([&] { ParseLimit("unlimited", minimum); });
    }
    std::cout << "Procedural object regression tests passed\n";
}

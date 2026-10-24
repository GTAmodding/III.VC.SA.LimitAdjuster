#pragma once
#include <cstdio>

namespace objectdata
{
    constexpr uint32_t FileCapacity = 4 * 1024 * 1024;
    constexpr uint32_t Ceiling = 32767;
    SafetyHookInline initialiseHook;
    SafetyHookMid publishHook;
    std::vector<uint8_t> input;
    uint32_t capacity = 0;
    uint32_t used = 0;
    int inputSize = 0;
    uintptr_t loadFile = 0;
    uintptr_t setDir = 0;
    uint32_t defaults = 0;
    uint32_t nativeCapacity = 0;
    uintptr_t readCallSite = 0;
    bool nativeFallback = false;
    bool fullLogged = false;
    std::vector<mem::Expect> originalValues;

    void UseNative(const char* reason)
    {
        core::Log("ObjectData: {}; using native object.dat handling", reason);
        nativeFallback = true;
        (void)publishHook.disable();
        for (auto& value : originalValues)
            mem::WriteDword(value.operand, value.value);
        if (readCallSite)
            injector::MakeCALL(readCallSite, loadFile, true);
        capacity = nativeCapacity;
        used = 0;
    }

    [[noreturn]] void Reject(const char* reason)
    {
        throw reason;
    }

    void Validate()
    {
        used = defaults;
        auto end = (size_t)inputSize;
        bool terminator = false;
        for (size_t begin = 0; begin < end;)
        {
            auto next = begin;
            while (next < end && input[next] != '\n')
                next++;
            auto line = std::string((char*)input.data() + begin, next - begin);
            auto first = line.find_first_not_of(" \t\r");
            if (first != std::string::npos && line[first] == '*')
            {
                terminator = true;
                break;
            }
            if (first != std::string::npos && line[first] != ';' && (!core::IsSA() || line[first] != '#'))
            {
                if (line.find('*') != std::string::npos)
                    Reject("asterisk inside a record");
                if (line.size() >= (core::IsSA() ? 510u : 1023u))
                    Reject("line is too long for the native parser");
                std::replace(line.begin(), line.end(), ',', ' ');
                // Both string fields in the SA parser and the name in III/VC have 256-byte buffers.
                size_t token = 0;
                for (auto ch : line)
                {
                    token = ch == ' ' || ch == '\t' || ch == '\r' ? 0 : token + 1;
                    if (token > 255)
                        Reject("token is too long for the native parser");
                }
                char name[256];
                float mass, turnMass, air, elasticity, submerged, uproot, damage;
                int effect, response, camera;
                if (sscanf(line.c_str(), "%255s %f %f %f %f %f %f %f %d %d %d", name,
                    &mass, &turnMass, &air, &elasticity, &submerged, &uproot, &damage, &effect, &response, &camera) != 11)
                    Reject("invalid physics record");
            }
            begin = next + 1;
        }
        if (!terminator)
            Reject("missing terminating asterisk");
    }

    void Publish(SafetyHookContext& context)
    {
        auto index = core::IsIII() ? context.edi : *(uint32_t*)(context.esp + (core::IsVC() ? 0xC : 0x18));
        if (index >= capacity)
        {
            if (!fullLogged)
                core::Log("ObjectData: record capacity exhausted; additional models retain default physics");
            fullLogged = true;
            auto model = core::IsSA() ? *(uint8_t**)(context.esp + 0x14) : (uint8_t*)context.esi;
            if (core::IsIII())
            {
                // III enters this block before moving ESI to the next record.
                model = (uint8_t*)context.eax;
                *(int16_t*)(model + 0x24) = -1;
                context.eip = 0x4BC254;
            }
            else
            {
                *(int16_t*)(model + (core::IsVC() ? 0x22 : 0x10)) = 0;
                context.eip = core::IsVC() ? 0x4E4964 : 0x5B5793;
            }
            return;
        }
        used = std::max(used, index + 1);
    }

    int __cdecl ReadValidated(const char*, uint8_t*, int, const char*)
    {
        return inputSize;
    }

    int __cdecl Initialise(const char* filename)
    {
        if (nativeFallback)
            return initialiseHook.ccall<int>(filename);
        ((void (__cdecl*)(const char*))setDir)("");
        inputSize = ((int (__cdecl*)(const char*, uint8_t*, int, const char*))loadFile)(filename, input.data(), FileCapacity, "rb");
        try
        {
            if (inputSize <= 0 || inputSize >= (int)FileCapacity)
                Reject("cannot read file or file exceeds 4 MiB");
            input[inputSize] = 0;
            Validate();
        }
        catch (const char* reason)
        {
            UseNative(reason);
        }
        return initialiseHook.ccall<int>(filename);
    }

    void Install()
    {
        auto setting = core::GetSetting("ObjectData", "unlimited");
        if (!setting.apply)
            return;
        uintptr_t base = 0, initialise = 0;
        uint32_t vanilla = 0, elementSize = 0;
        std::span<const uintptr_t> refs;
        std::span<const uintptr_t> bufferRefs;
        uintptr_t readCall = 0, publish = 0;
        uint32_t publishBytes = 0;
        if (core::IsIII())
        {
            static const uintptr_t references[] = { 0x4BC11E, 0x4BC1E8, 0x4BC1FC, 0x4BC202, 0x4BC20F, 0x4BC21C, 0x4BC295 };
            static const uintptr_t buffers[] = { 0x4BC107, 0x4BC119 };
            bufferRefs = buffers;
            publish = 0x4BC233;
            publishBytes = 0x6620C683;
            base = 0x6F4708;
            vanilla = 168;
            elementSize = 32;
            defaults = 0;
            initialise = 0x4BC0E0;
            loadFile = 0x479090;
            setDir = 0x479020;
            readCall = 0x4BC10C;
            refs = references;
            if (mem::ReadDword(initialise) != 0x55575653)
                return;
        }
        else if (core::IsVC())
        {
            static const uintptr_t references[] = { 0x4E44B5, 0x4E4568, 0x4E4572, 0x4E457C, 0x4E4586, 0x4E4590, 0x4E45AB, 0x4E45B1, 0x4E45BB, 0x4E45C5, 0x4E45CC, 0x4E45D3, 0x4E45DA, 0x4E45E0, 0x4E45E6, 0x4E45EC, 0x4E45F2, 0x4E45F8, 0x4E45FE, 0x4E4604, 0x4E460A, 0x4E4610, 0x4E4616, 0x4E461C, 0x4E4622, 0x4E4628, 0x4E462D, 0x4E4632, 0x4E4638, 0x4E463E, 0x4E4643, 0x4E4648, 0x4E464E, 0x4E4655, 0x4E465B, 0x4E4661, 0x4E4667, 0x4E466D, 0x4E4673, 0x4E4679, 0x4E467F, 0x4E4685, 0x4E468B, 0x4E4691, 0x4E4697, 0x4E469D, 0x4E46A3, 0x4E46A8, 0x4E46AD, 0x4E46B3, 0x4E46B9, 0x4E46BE, 0x4E46C3, 0x4E46C9, 0x4E46D0, 0x4E46D6, 0x4E46DC, 0x4E46E2, 0x4E46E8, 0x4E46EE, 0x4E46F4, 0x4E46FA, 0x4E4700, 0x4E4706, 0x4E470C, 0x4E4712, 0x4E4718, 0x4E471E, 0x4E4723, 0x4E4728, 0x4E472E, 0x4E4734, 0x4E4739, 0x4E473E, 0x4E4744, 0x4E474B, 0x4E4788, 0x4E484B, 0x4E485F, 0x4E4865, 0x4E4872, 0x4E487F, 0x4E489E, 0x4E48A4, 0x4E48B8, 0x4E48BE, 0x4E48CE, 0x4E48D4, 0x4E48DE, 0x4E48E4, 0x4E48EE, 0x4E48FA, 0x4E4900, 0x4E491A, 0x4E4920 };
            static const uintptr_t buffers[] = { 0x4E4768, 0x4E4783 };
            bufferRefs = buffers;
            publish = 0x4E4938;
            publishBytes = 0x24448B66;
            base = 0x7DE200;
            vanilla = 210;
            elementSize = 32;
            defaults = 4;
            initialise = 0x4E4560;
            loadFile = 0x48DFB0;
            setDir = 0x48E030;
            readCall = 0x4E476D;
            refs = references;
            if (mem::ReadDword(initialise) != 0x17EC05D9)
                return;
        }
        else if (core::IsSA())
        {
            static const uintptr_t references[] = { 0x59F85D, 0x5A2D2A, 0x5A2E4A, 0x5B536B, 0x5B5375, 0x5B537F, 0x5B5389, 0x5B5393, 0x5B539D, 0x5B53A7, 0x5B53B1, 0x5B53B8, 0x5B53BF, 0x5B53CA, 0x5B53CF, 0x5B53D7, 0x5B53E2, 0x5B53E7, 0x5B53F0, 0x5B53FA, 0x5B53FF, 0x5B5406, 0x5B540C, 0x5B5417, 0x5B541C, 0x5B542E, 0x5B5467, 0x5B5589, 0x5B5598, 0x5B55A8, 0x5B55B3, 0x5B55BD, 0x5B5609, 0x738612 };
            publish = 0x5B5774;
            publishBytes = 0x1824448B;
            base = 0xBB4A90;
            vanilla = 160;
            elementSize = 80;
            defaults = 5;
            initialise = 0x5B5360;
            loadFile = 0x538890;
            setDir = 0x5387D0;
            readCall = 0x0;
            refs = references;
            if (mem::ReadDword(initialise) != 0x022CEC81)
                return;
        }
        if (!base || mem::ReadDword(publish) != publishBytes || !mem::CheckRefs(refs, base, (size_t)vanilla * elementSize, "ObjectData"))
            return;
        auto oldBuffer = core::IsIII() ? 0x715C28u : 0x7FFEE0u;
        if (readCall && (!mem::CheckRefs(bufferRefs, oldBuffer, 55000, "ObjectData input") ||
            *(uint8_t*)readCall != 0xE8 || readCall + 5 + *(int32_t*)(readCall + 1) != loadFile))
            return;
        nativeCapacity = vanilla;
        readCallSite = readCall;
        for (auto address : refs)
            originalValues.push_back({ address, mem::ReadDword(address) });
        for (auto address : bufferRefs)
            originalValues.push_back({ address, mem::ReadDword(address) });
        capacity = std::max(vanilla, core::MaxCapacity(setting, Ceiling));
        input.resize(FileCapacity + 1);
        auto moved = limits::MoveArray("ObjectData", base, elementSize, vanilla, capacity + 1, refs, false);
        if (!moved)
            return;
        // Use the validated file image directly in the older whole-file parsers.
        if (readCall)
        {
            mem::RelocateRefs(bufferRefs, oldBuffer, 55000, (uintptr_t)input.data());
            injector::MakeCALL(readCall, ReadValidated, true);
        }
        publishHook = safetyhook::create_mid(publish, Publish);
        if (!publishHook)
        {
            UseNative("could not install record bound check");
            return;
        }
        initialiseHook = safetyhook::create_inline(initialise, Initialise);
        if (!initialiseHook)
        {
            UseNative("could not install parser preflight");
            return;
        }
        core::RegisterLimit({ "ObjectData", [] { return (int)used; }, [] { return (int)capacity; }, [] { return (int)capacity; } });
    }
}

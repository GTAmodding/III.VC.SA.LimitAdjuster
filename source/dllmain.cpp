#include "Core.h"
#include "Adjusters.h"

namespace limits
{
    void InstallCollision();
}

namespace
{
    bool installed = false;

    void Install()
    {
        if (installed)
            return;
        installed = true;

        if (GetModuleHandleW(L"$fastman92limitAdjuster.asi") || GetModuleHandleW(L"$fastman92limitAdjuster"))
            core::Log("fastman92 limit adjuster found: limits it already changed are left to it");

        limits::InstallPools();
        limits::InstallRenderLists();
        limits::InstallModelStores();
        limits::InstallStreaming();
        limits::InstallIpl();
        limits::InstallCollision();
        limits::InstallLights();
        limits::InstallEffects();
        limits::InstallHandling();
        limits::InstallVehicleAudio();
        limits::InstallProcedural();
        limits::InstallScm();
        limits::InstallOverlay();
        core::Log("Ready");
    }

    // A piece of code the unpacked exe has, so the patches go in right away; a packed exe gets them once its startup
    // code runs, which is still before the game itself starts.
    hook::pattern UnpackedCode()
    {
        if (core::IsIII())
            return hook::pattern("6A 10 E8 ? ? ? ? 85 C0 59 74 0C 89 C1 68");
        if (core::IsVC())
            return hook::pattern("6A 10 E8 ? ? ? ? 85 C0 59 74 11 89 C1 68 ? ? ? ? 68");
        return hook::pattern("6A FF 68 0B CC 83 00 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 6A 14");
    }
}

// For other plugins (Project2DFX): find this module by the export names.
extern "C" __declspec(dllexport) int OpenLA_GetVersion()
{
    return installed ? 2 : 0;
}

// `used` is -1 when the game doesn't count it. Returns false for an unknown name.
extern "C" __declspec(dllexport) bool OpenLA_GetLimit(const char* name, int* used, int* capacity, int* maximum)
{
    for (auto& limit : core::GetLimits())
    {
        if (_stricmp(limit.name.c_str(), name) == 0)
        {
            if (used)
                *used = limit.used();
            if (capacity)
                *capacity = limit.capacity();
            if (maximum)
                *maximum = limit.maximum();
            return true;
        }
    }
    return false;
}

// Streaming memory in bytes (0 when the game's own value is used).
extern "C" __declspec(dllexport) uint32_t OpenLA_GetStreamingBudget()
{
    return limits::StreamingBudget();
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!core::DetectGame())
            return TRUE;

        core::LoadConfig(module);
        core::Log("Open Limit Adjuster, {}{}", core::IsIII() ? "GTA III 1.0" : core::IsVC() ? "GTA Vice City 1.0" : "GTA San Andreas 1.0 US",
                  core::IsSA() ? (core::hoodlum ? " (HOODLUM exe)" : " (compact exe)") : "");
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Install, UnpackedCode());
    }
    return TRUE;
}

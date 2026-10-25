#pragma once
#include "stdafx.h"
#include <filesystem>
#include <fstream>

enum class Game
{
    Unknown,
    III,
    VC,
    SA,
};

namespace core
{
    inline Game game = Game::Unknown;
    inline bool hoodlum = false; // SA 1.0 US exe with the SecuROM sections (the common retail/cracked one)

    inline bool IsIII() { return game == Game::III; }
    inline bool IsVC() { return game == Game::VC; }
    inline bool IsSA() { return game == Game::SA; }

    // Picks the address of the running SA build: the clean (compact) layout or the HOODLUM one.
    inline uintptr_t SA(uintptr_t compact, uintptr_t hoodlumAddress)
    {
        return hoodlum ? hoodlumAddress : compact;
    }

    bool DetectGame();

    // Where another plugin (e.g. L"modloader.asi") is installed: loaded, or where the ASI loader or Mod Loader picks it
    // up later (Mod Loader loads plugins from any folder inside "modloader"). Empty when it isn't.
    inline std::filesystem::path PluginPath(const wchar_t* file)
    {
        if (auto module = GetModuleHandleW(file))
        {
            wchar_t path[MAX_PATH]{};
            GetModuleFileNameW(module, path, MAX_PATH);
            return path;
        }
        for (auto dir : { L"", L"scripts\\", L"plugins\\" })
        {
            std::filesystem::path path = std::wstring(dir) + file;
            if (GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES)
                return path;
        }
        std::error_code error;
        for (std::filesystem::recursive_directory_iterator it(L"modloader", std::filesystem::directory_options::skip_permission_denied, error), end;
             !error && it != end; it.increment(error))
        {
            if (it->is_regular_file(error) && _wcsicmp(it->path().filename().c_str(), file) == 0)
                return it->path();
        }
        return {};
    }

    inline bool PluginInstalled(const wchar_t* file)
    {
        return !PluginPath(file).empty();
    }

    // skygfx reads a few game arrays at their fixed addresses (III/VC the visible entity list, SA the point lights);
    // builds that export SkyGfx_FollowsMovedArrays read them through the game's code. skygfx loads after this plugin,
    // so its file is checked.
    inline bool OlderSkyGfxInstalled()
    {
        static const bool older = []
        {
            auto path = PluginPath(L"skygfx.asi");
            if (path.empty())
                return false;
            std::ifstream file(path, std::ios::binary);
            std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            return data.find("SkyGfx_FollowsMovedArrays") == std::string::npos;
        }();
        return older;
    }

    void Log(std::string_view text);

    template <typename... Args>
    void Log(std::format_string<Args...> fmt, Args&&... args)
    {
        Log(std::string_view(std::format(fmt, std::forward<Args>(args)...)));
    }

    // One limit as configured in the ini.
    struct Setting
    {
        bool apply = true;       // false: "0" in the ini, the game's own value stays
        bool unlimited = true;   // grows on demand up to the internal ceiling
        uint32_t value = 0;      // the cap (or exact size of a fixed limit) when not unlimited
        bool percent = false;    // value is a percentage (MemoryAvailable)
        bool automatic = false;  // "auto"
    };

    void LoadConfig(HMODULE self);

    // Reads a limit of the running game. `defaultText` is used when the ini doesn't set the key.
    Setting GetSetting(std::string_view key, std::string_view defaultText);

    // The capacity a limit may grow to: the ini number, or the ceiling when unlimited.
    uint32_t MaxCapacity(const Setting& setting, uint32_t ceiling);

    std::string_view GetOption(std::string_view key);

    // Usage reporting (F5 overlay, exported API).
    struct LimitInfo
    {
        std::string name;
        std::function<int()> used;     // -1 when the game doesn't count it
        std::function<int()> capacity; // the current capacity
        std::function<int()> maximum;  // what it can grow to
    };

    void RegisterLimit(LimitInfo info);
    const std::vector<LimitInfo>& GetLimits();
}

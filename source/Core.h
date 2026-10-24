#pragma once
#include "stdafx.h"

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

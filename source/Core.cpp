#include "Core.h"
#include <fstream>
#include <filesystem>
#include <map>

namespace core
{
    namespace
    {
        std::filesystem::path baseDir;
        std::mutex logMutex;
        std::ofstream logFile;
        std::map<std::string, std::string, std::less<>> sections[4]; // GTA3LIMITS, VCLIMITS, SALIMITS, OPTIONS
        std::vector<LimitInfo> limits;

        std::string Lower(std::string_view s)
        {
            std::string out(s);
            std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return (char)std::tolower(c); });
            return out;
        }

        std::string_view Trim(std::string_view s)
        {
            while (!s.empty() && (s.front() == ' ' || s.front() == '\t'))
                s.remove_prefix(1);
            while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n'))
                s.remove_suffix(1);
            return s;
        }

        int SectionIndex(std::string_view name)
        {
            auto n = Lower(name);
            if (n == "gta3limits")
                return 0;
            if (n == "vclimits")
                return 1;
            if (n == "salimits")
                return 2;
            if (n == "options")
                return 3;
            return -1;
        }

        int GameSection()
        {
            switch (game)
            {
                case Game::III: return 0;
                case Game::VC: return 1;
                case Game::SA: return 2;
                default: return -1;
            }
        }
    }

    bool DetectGame()
    {
        auto base = (uintptr_t)GetModuleHandleW(nullptr);
        auto dos = (IMAGE_DOS_HEADER*)base;
        auto nt = (IMAGE_NT_HEADERS*)(base + dos->e_lfanew);
        auto entry = 0x400000 + nt->OptionalHeader.AddressOfEntryPoint;

        switch (entry)
        {
            case 0x5C1E70:
                game = Game::III;
                return true;
            case 0x667BF0:
                game = Game::VC;
                return true;
            case 0x82457C:
            case 0x824570:
                game = Game::SA;
                hoodlum = *(uint8_t*)0x406A20 == 0xE9;
                return true;
            default:
                return false;
        }
    }

    void Log(std::string_view text)
    {
        // Off unless [OPTIONS] Log = 1
        if (auto it = sections[3].find("log"); it == sections[3].end() || it->second != "1")
            return;
        std::lock_guard lock(logMutex);
        if (!logFile.is_open())
            logFile.open(baseDir / "III.VC.SA.LimitAdjuster.log", std::ios::out | std::ios::trunc);
        if (logFile.is_open())
        {
            logFile << text << '\n';
            logFile.flush();
        }
    }

    void LoadConfig(HMODULE self)
    {
        wchar_t buffer[MAX_PATH]{};
        GetModuleFileNameW(self, buffer, MAX_PATH);
        baseDir = std::filesystem::path(buffer).parent_path();

        std::ifstream file(baseDir / "III.VC.SA.LimitAdjuster.ini");
        int section = -1;
        std::string line;
        while (std::getline(file, line))
        {
            auto view = std::string_view(line);
            if (auto comment = view.find_first_of(";#"); comment != view.npos)
                view = view.substr(0, comment);
            view = Trim(view);
            if (view.empty())
                continue;

            if (view.front() == '[')
            {
                auto end = view.find(']');
                section = SectionIndex(view.substr(1, end == view.npos ? view.npos : end - 1));
                continue;
            }

            auto eq = view.find('=');
            if (section < 0 || eq == view.npos)
                continue;
            sections[section][Lower(Trim(view.substr(0, eq)))] = Lower(Trim(view.substr(eq + 1)));
        }
    }

    Setting GetSetting(std::string_view key, std::string_view defaultText)
    {
        std::string text(defaultText);
        if (auto index = GameSection(); index >= 0)
        {
            if (auto it = sections[index].find(Lower(key)); it != sections[index].end())
                text = it->second;
        }

        Setting setting;
        if (text == "unlimited" || text.empty())
            return setting;

        if (text == "auto")
        {
            setting.automatic = true;
            return setting;
        }

        try
        {
            size_t used = 0;
            auto number = std::stoul(text, &used, 0);
            setting.unlimited = false;
            setting.value = (uint32_t)number;
            setting.percent = used < text.size() && text[used] == '%';
            setting.apply = number != 0;
        }
        catch (...)
        {
            Log("{}: unknown value '{}', using the default '{}'", key, text, defaultText);
            if (text != defaultText)
                return GetSetting(key, defaultText);
        }
        return setting;
    }

    uint32_t MaxCapacity(const Setting& setting, uint32_t ceiling)
    {
        if (setting.unlimited || setting.automatic)
            return ceiling;
        return std::min(setting.value, ceiling);
    }

    std::string_view GetOption(std::string_view key)
    {
        if (auto it = sections[3].find(Lower(key)); it != sections[3].end())
            return it->second;
        return {};
    }

    void RegisterLimit(LimitInfo info)
    {
        limits.push_back(std::move(info));
    }

    const std::vector<LimitInfo>& GetLimits()
    {
        return limits;
    }
}

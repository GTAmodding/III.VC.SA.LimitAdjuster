#include "Core.h"
#include "Adjusters.h"

// Debug overlay: a key set in [OPTIONS] DebugTextKey pages through the usage of every limit on screen.

namespace
{
    struct FontApi
    {
        uintptr_t rsGlobal;
        uintptr_t setScale, setColor, setFontStyle, setDropColor, setDropShadow, setProportional, setBackground, setJustify;
        uintptr_t setRightJustify, setWrapx, setCentre, printString, renderFontBuffer;
    };

    struct CRGBA
    {
        uint8_t r, g, b, a;
    };

    FontApi font{};
    int key = 0;
    size_t page = 0;
    bool visible = false;
    bool keyDown = false;
    constexpr size_t PerPage = 18;
    uintptr_t drawHud = 0;

    void DrawLine(const std::string& text, float y)
    {
        auto rs = (int*)font.rsGlobal;
        float sx = rs[1] / 640.0f;
        float sy = rs[2] / 448.0f;

        CRGBA color = core::IsSA() ? CRGBA{ 0xFF, 0xFF, 0xFF, 0xFF } : CRGBA{ 0x1B, 0x59, 0x82, 0xFF };
        if (core::IsSA())
        {
            injector::cstd<void(short)>::call(font.setFontStyle, 1);
            injector::cstd<void(bool)>::call(font.setJustify, false);
            injector::cstd<void(bool, bool)>::call(font.setBackground, false, false);
            injector::cstd<void(bool)>::call(font.setProportional, true);
            injector::cstd<void(int)>::call(font.setCentre, 1);
            injector::cstd<void(float)>::call(font.setRightJustify, 0.0f);
            injector::cstd<void(float)>::call(font.setWrapx, 640.0f * sx);
            injector::cstd<void(short)>::call(font.setDropShadow, 1);
            injector::cstd<void(CRGBA)>::call(font.setDropColor, CRGBA{ 0, 0, 0, 0xFF });
            injector::cstd<void(CRGBA)>::call(font.setColor, color);
        }
        else
        {
            // III/VC: Set*Off functions without arguments
            injector::cstd<void()>::call(font.setJustify);
            injector::cstd<void()>::call(font.setCentre);
            injector::cstd<void()>::call(font.setRightJustify);
            injector::cstd<void()>::call(font.setBackground);
            injector::cstd<void()>::call(font.setProportional);
            injector::cstd<void(float)>::call(font.setWrapx, 640.0f * sx);
            injector::cstd<void(short)>::call(font.setFontStyle, 0);
            injector::cstd<void(short)>::call(font.setDropShadow, 1);
            // these take the colour by pointer
            CRGBA black{ 0, 0, 0, 0xFF };
            injector::cstd<void(CRGBA*)>::call(font.setDropColor, &black);
            injector::cstd<void(CRGBA*)>::call(font.setColor, &color);
        }
        injector::cstd<void(float, float)>::call(font.setScale, sx * 0.39f, sy * 0.58f);
        if (core::IsSA())
        {
            injector::cstd<void(float, float, const char*)>::call(font.printString, sx * 10.0f, sy * y, text.c_str());
        }
        else
        {
            // III and VC print UTF-16 text
            std::wstring wide(text.begin(), text.end());
            injector::cstd<void(float, float, const wchar_t*)>::call(font.printString, sx * 10.0f, sy * y, wide.c_str());
        }
    }

    void DrawLimits()
    {
        auto down = (GetKeyState(key) & 0x8000) != 0;
        if (down && !keyDown)
        {
            if (!visible)
            {
                visible = true;
                page = 0;
            }
            else
            {
                page++;
                visible = page * PerPage < core::GetLimits().size();
            }
        }
        keyDown = down;
        if (!visible)
            return;

        auto& limits = core::GetLimits();
        float y = 105.0f;
        for (size_t i = page * PerPage; i < limits.size() && i < (page + 1) * PerPage; i++)
        {
            auto& limit = limits[i];
            auto used = limit.used();
            auto text = used >= 0 ? std::format("{}: {} / {}", limit.name, used, limit.capacity()) : std::format("{}: {}", limit.name, limit.capacity());
            DrawLine(text, y);
            y += 11.6f;
        }
        if (font.renderFontBuffer)
            injector::cstd<void()>::call(font.renderFontBuffer);
    }

    void __cdecl DrawHudHook()
    {
        injector::cstd<void()>::call(drawHud);
        DrawLimits();
    }
}

namespace limits
{
    void InstallOverlay()
    {
        auto option = core::GetOption("DebugTextKey");
        if (option.empty())
            return;
        key = (int)std::strtol(std::string(option).c_str(), nullptr, 0);
        if (key <= 0)
            return;

        uintptr_t call = 0;
        if (core::IsIII())
        {
            font = { 0x8F4360, 0x501B80, 0x501BD0, 0x501DB0, 0x501DE0, 0x501E70, 0x501DA0, 0x501CF0, 0x501C80, 0x501D70, 0x501CC0, 0x501CB0, 0x500F50, 0 };
            call = 0x48E420;
        }
        else if (core::IsVC())
        {
            font = { 0x9B48D8, 0x550230, 0x550170, 0x54FFE0, 0x54FF30, 0x54FF20, 0x550020, 0x5500D0, 0x550140, 0x550040, 0x550100, 0x550110, 0x551040, 0x551A30 };
            call = 0x4A64D0;
        }
        else if (core::IsSA())
        {
            font = { 0xC17040, 0x719380, 0x719430, 0x719490, 0x719510, 0x719590, 0x7195B0, 0x7195C0, 0x719600, 0x7194F0, 0x7194D0, 0x719610, 0x71A700, 0x719840 };
            call = 0x53E4FF;
        }
        if (!call || *(uint8_t*)call != 0xE8)
            return;

        drawHud = injector::GetBranchDestination(call).as_int();
        injector::MakeCALL(call, DrawHudHook, true);
    }
}

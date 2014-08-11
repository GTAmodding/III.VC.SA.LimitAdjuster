/*
 *  Grand Theft Auto Limit Adjuster - Base Code
 *      Copyright (C) 2013-2014 ThirteenAG <thirteenag@gmail.com>
 *      Copyright (C) 2014 LINK/2012 <dma_2012@hotmail.com>
 *      Licensed under the MIT License (http://opensource.org/licenses/MIT)
 */
#include <windows.h>
#include <LimitAdjuster.h>
#include <ini_parser/ini_parser.hpp>
#include <injector/utility.hpp>

// Stores an handler for a specific limit
struct LimitHandler
{ 
    int         id;         // Id for the key
    Adjuster*   adjuster;   // Adjuster to handle the key
};

static int vkDebugText = VK_F5; // 0x74
int AdjustLimits();
void AdjustLimits(const std::map<std::string, std::string>&);
void PatchDrawer();

// Map of name of limit and adjuster to handle that limit
static std::map<std::string, LimitHandler> limits;


// Entry point
extern "C"
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if(reason == DLL_PROCESS_ATTACH)
    {
        auto& gvm = injector::address_manager::singleton();
        
        if(gvm.IsUnknown()) // Game could not be detected?
            gvm.RaiseCouldNotDetect();  // Ops...
        else
        {
            AdjustLimits();             // Run the limit adjuster
            PatchDrawer();
        }
    }
    return TRUE;
}




/*
 * -------- Basic Limit Adjuster Running --------
 */

/*
 *  Reads all key-value pairs from the 'ini' section 'secname' and add them to the 'output' pair
 *  If pair already exists on the output pair it's overriden
 */
void ReadSection(const linb::ini& ini, const char* secname, std::map<std::string, std::string>& output)
{
    if(secname)
    {
        for(auto sec = ini.begin(); sec != ini.end(); ++sec)
        {
            // Find section named secname
            if(!strcmp(sec->first.c_str(), secname))
            {
                // Push all kv pairs to the output map
                auto& section = sec->second;
                for(auto it = section.begin(); it != section.end(); ++it)
                    output[it->first] = it->second;
                break;
            }
        }
    }
}

/*
 *  Adjust the limits asked by the ini file
 */
int AdjustLimits()
{
    std::map<std::string, std::string> keys;
    linb::ini ini("limit_adjuster_gta3vcsa.ini");           // Open the ini
    const char* secname = 0;

    // Find which section we should read on the ini
    if(Adjuster::IsSA()) secname = "SALIMITS";
    else if(Adjuster::IsVC()) secname = "VCLIMITS";
    else if(Adjuster::IsIII()) secname = "GTA3LIMITS";

    // Configurable options such as debug text key
    auto& options = ini["OPTIONS"];
    for(auto it = options.begin(); it != options.end(); ++it)
    {
        auto& pair = *it;
        try {
            
            if(!pair.first.compare("DebugTextKey"))
                vkDebugText = std::stoi(pair.second, nullptr, 0);

        } catch(const std::logic_error& ex) {
            char buf[512];
            sprintf(buf, "Invalid value on option %s\n%s\n", pair.first.c_str(), ex.what());
            MessageBoxA(0, buf, "Limit Adjuster", MB_ICONERROR);
        }
    }

    // Parse section in search of limits
    if(secname)
    {
        ReadSection(ini, "LIMITS", keys);
        ReadSection(ini, secname, keys);
        AdjustLimits(keys);
    }

	return 0;
}


/*
 *  Finds all the limit handlers
 */
void FindHandlers(std::map<std::string, LimitHandler>& handlers)
{
    LimitHandler k;
    auto& adjusters = Adjuster::GetAdjusters();
    
    // Find all adjusters present on this build and build the map of handlers
    for(auto it = adjusters.begin(); it != adjusters.end(); ++it)
    {
        Adjuster* adjuster = *it;
        if(auto* limits = adjuster->GetLimits())        // Get limits this adjuster can handle
        {
            // For each limit this adjuster can handle...
            for(auto* limit = limits; limit->name; ++limit)
            {
                // ...add it to the handlers map
                k.adjuster  = adjuster;
                k.id        = limit->id;
                handlers[limit->name] = k;
            }
        }
    }
}

/*
 *  Reads an section of limits to be changed
 */
void AdjustLimits(const std::map<std::string, std::string>& section)
{
    // Find the handler for each limit
    FindHandlers(::limits);
    
    // Parse the section and call the handler for each limit
    for(auto it = section.begin(); it != section.end(); ++it)
    {
        auto pair = limits.find(it->first);   // Find this limit handler
        if(pair != limits.end())
        {
            try
            {
                // Change the limit
                LimitHandler& handler = pair->second;
                handler.adjuster->ChangeLimit(handler.id, it->second);
            }
            catch(const std::exception& ex)
            {
                // ...ops... something wrong happened at ChangeLimit, failed to convert from string to int?
                // Dunno, let's report it to the user
                char buf[512];
                sprintf(buf, "Failed to change limit for '%s' with value '%s'.\nReason: %s\n",
                                it->first.c_str(), it->second.c_str(), ex.what());
                MessageBoxA(0, buf, "Limit Adjuster", MB_ICONERROR);
            }
        }
    }
}



/*
 * -------- Limits Drawing on HUD --------
 */

static injector::hook_back<void(*)()> DrawHUD;
static uint32_t current_limit         = 0;  // Index of the first limit we should draw
static const uint32_t limits_per_page = 20; // Max number of limits in one page
static float currposx;                      // Current drawing position
static float currposy;                      // Current drawing position

// Setups variables to start drawing the limits values
bool BeginDraw()
{
    currposx = 10.0;
    currposy = 105.0;
    return true;
}

// Finishes drawing the limits values
void EndDraw()
{
    static void (*RenderFontBuffer)() = injector::lazy_pointer<0x719840>::get();   // CFont method
    RenderFontBuffer();
}

// Draw a specific text at xy (local screen space) with the specified scale (too)
void DrawText(const char* text, float x, float y, float scalex, float scaley)
{
    // CRGBA structure
    struct CRGBA
    {
        unsigned char red, green, blue, alpha;
        CRGBA() {}
	    CRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) :
            red(r), green(g), blue(b), alpha(a)
        {}
    };

    // Gets the interface colour an specific interface colour
    static auto GetInterfaceColour = [](unsigned char index)
    {
        CRGBA temp;
	    return *((CRGBA* (__thiscall *)(void*, CRGBA*, unsigned char))0x58FEA0)((void*)(0xBAB22C), &temp, index);
    };

    // Game method pointers
    static int*  pRsGlobal                                                  = injector::lazy_pointer<0xC17040>::get();   // To find the resolution
    static void (*SetScale)(float w, float h)                               = injector::lazy_pointer<0x719380>::get();   // CFont method
    static void (*SetColor)(CRGBA color)                                    = injector::lazy_pointer<0x719430>::get();   // CFont method
    static void (*SetFontStyle)(short style)                                = injector::lazy_pointer<0x719490>::get();   // CFont method
    static void (*SetDropColor)(CRGBA color)                                = injector::lazy_pointer<0x719510>::get();   // CFont method
    static void (*SetEdge)(short value)                                     = injector::lazy_pointer<0x719590>::get();   // CFont method
    static void (*SetProportional)(bool on)                                 = injector::lazy_pointer<0x7195B0>::get();   // CFont method
    static void (*SetBackground)(bool background, bool backgroundOnlyText)  = injector::lazy_pointer<0x7195C0>::get();   // CFont method
    static void (*SetJustify)(bool on)                                      = injector::lazy_pointer<0x719600>::get();   // CFont method
    static void (*SetRightJustifyWrap)(float value)                         = injector::lazy_pointer<0x7194F0>::get();   // CFont method
    static void (*SetOrientation)(int alignment)                            = injector::lazy_pointer<0x719610>::get();   // CFont method
    static void (*PrintString)(float x, float y, const char *text)          = injector::lazy_pointer<0x71A700>::get();   // CFont method

    // Transformer from global screen space to local screen space
    float screenx((float)(signed int)*(pRsGlobal + 2) * 0.00223f);
    float screeny((float)(signed int)*(pRsGlobal + 1) * 0.00156f);

    // Setup the font style
    SetFontStyle(1); 
    SetJustify(0);
    SetBackground(0, 0);
    SetProportional(true);
    SetOrientation(1);
    SetRightJustifyWrap(0);
    SetEdge(1);
    SetDropColor(CRGBA(0, 0, 0, 0xFF));
    SetColor(GetInterfaceColour(4));
    SetScale(screenx * scalex, screeny * scaley);

    // Print the text
    PrintString(screenx * x, screeny * y, text);
}

// Draw the specified text and advance the posy to draw the next text
void DrawText(const char* text)
{
    const float x = currposx, y = currposy;             // Pos
    const float sx = 0.60f * 0.65f, sy = 0.89f * 0.65f; // Scale
    currposy += sy * 20;                                // Advance current position
    return DrawText(text, x, y, sx, sy);                // Drawzz
}

// Draw the limit named 'name' with usage counter 'usage'
void DrawLimit(const char* name, const char* usage)
{
    char buffer[1024];
    sprintf(buffer, "%s: %s", name, usage);
    return DrawText(buffer);
}

// Test hotkeys and find out if we should draw the limits on screen
bool TestShouldDraw()
{
    static bool should_draw = false;    // Should draw flag
    static bool prevF5 = false;         // Prev state of the debug text key
    static bool currF5 = false;         // Curr state of the debug text key
    
    // Update key states and find if hot key pressed
    currF5 = (GetKeyState(vkDebugText) & 0x8000) != 0;
    if(currF5 && !prevF5)   // Hotkey pressed?!?
    {
        if(!should_draw)
        {
            // So it should draw since this key press
            current_limit = 0;
            should_draw = true;
        }
        else
        {
            // Go to the next page, or stop drawing the limits
            current_limit += limits_per_page;
            should_draw = current_limit < limits.size();
        }
    }
    prevF5 = currF5;

    return should_draw;
}

// Draw the limits on screen
void DrawLimits()
{
    if(TestShouldDraw() && BeginDraw())
    {
        std::string usage;
        unsigned int i = 0, drawn = 0;  // Limit index and num drawn limits

        for(auto& pair : limits)
        {
            // Check if are already in the limit we should start drawing from
            if(i >= current_limit)
            {
                if(pair.second.adjuster->GetUsage(pair.second.id, usage))
                {
                    // Check if we are still in the limits per page range
                    // OR we didn't draw enought stuff to complete limits per page
                    if(i < current_limit + limits_per_page || drawn < limits_per_page)
                    {
                        ++drawn;
                        DrawLimit(pair.first.c_str(), usage.c_str());
                    }
                }
            }

            ++i;    // Increase limit index
        }

        EndDraw();
    }

    // Go to the replaced function, which is CHud::Draw
    return DrawHUD.fun? DrawHUD.fun() : void();
}

// Patches CHud::Draw to draw additional stuff (limits details)
void PatchDrawer()
{
    if(Adjuster::IsSA())
    {
        DrawHUD.fun = injector::MakeCALL(0x53E4FF, DrawLimits).get();
    }
}

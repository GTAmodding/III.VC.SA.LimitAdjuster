/*
 *  Grand Theft Auto Limit Adjuster - Base Code
 *      Copyright (C) 2013-2014 ThirteenAG <>
 *      Copyright (C) 2014 LINK/2012 <dma_2012@hotmail.com>
 *      Licensed under the MIT License (http://opensource.org/licenses/MIT)
 */
#include <windows.h>
#include "ini_parser/ini_parser.hpp"
#include "LimitAdjuster.h"

int AdjustLimits();
void AdjustLimits(const std::map<std::string, std::string>&);

// Entry point
extern "C"
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if(reason == DLL_PROCESS_ATTACH) AdjustLimits();
    return TRUE;
}




// Stores an handler for a specific limit
struct LimitHandler
{ 
    int         id;         // Id for the key
    Adjuster*   adjuster;   // Adjuster to handle the key
};


/*
 *  Adjust the limits asked by the ini file
 */
int AdjustLimits()
{
    linb::ini ini("limit_adjuster_gta3vcsa.ini");           // Open the ini
    auto& gvm = injector::address_manager::singleton();
    const char* secname = 0;

    // Find which section we should read on the ini
    if(gvm.IsSA()) secname = "SALIMITS";
    else if(gvm.IsVC()) secname = "VCLIMITS";
    else if(gvm.IsIII()) secname = "GTA3LIMITS";

    if(secname)
    {
        // Find the section we should read
        for(auto sec = ini.begin(); sec != ini.end(); ++sec)
        {
            if(!strcmp(sec->first.c_str(), secname))
            {
                // Found it, let's read that section :)
                AdjustLimits(sec->second);
                break;
            }
        }
    }
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
    // Map of name of limit and adjuster to handle that limit
    std::map<std::string, LimitHandler> handlers;

    // Find the handler for each limit
    FindHandlers(handlers);
    
    // Parse the section and call the handler for each limit
    for(auto it = section.begin(); it != section.end(); ++it)
    {
        auto pair = handlers.find(it->first);   // Find this limit handler
        if(pair != handlers.end())
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



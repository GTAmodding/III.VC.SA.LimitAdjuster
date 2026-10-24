--[[
    Open Limit Adjuster build script
    Use 'premake5 --help' for help
--]]

newoption {
    trigger     = "with-version",
    value       = "STRING",
    description = "Current version"
}

-- The game folders a build is copied to, and the game started when debugging, are the paths of one machine. They are
-- read from a `.env` file next to this script (not tracked by git), one `<KEY>=<folder>` line per game.
local envkeys = nil
function envdir(key)
    if not envkeys then
        envkeys = {}
        local text = io.readfile(path.join(_SCRIPT_DIR, ".env")) or ""
        for line in text:gmatch("[^\r\n]+") do
            local k, v = line:match("^%s*([%w_]+)%s*=%s*(.-)%s*$")
            if k and v ~= "" then
                v = v:gsub('^"', ""):gsub('"$', ""):gsub("^'", ""):gsub("'$", "")
                envkeys[k] = v
            end
        end
    end

    local value = envkeys[key]
    if not value then return nil end

    value = value:gsub("[%s\\/]+$", "")
    if value == "" then return nil end

    return path.translate(value)
end

-- Copies the built .asi into the scripts folder of each game named in the .env file, only where it's already installed.
function setpaths(keys)
    for _, key in ipairs(keys) do
        local gamepath = envdir(key)
        if gamepath then
            local target = gamepath .. "\\scripts\\"
            postbuildcommands {
                "if exist \"" .. target .. "$(TargetFileName)\" copy /y \"$(TargetPath)\" \"" .. target .. "\"",
            }
        end
    end
end

workspace "OpenLA"
    configurations { "Release", "Debug" }
    platforms { "Win32" }
    architecture "x86"
    location "build"
    objdir ("build/obj")
    buildlog ("build/log/%{prj.name}.log")
    cppdialect "C++latest"
    toolset "v145"
    buildoptions { "/Zc:__cplusplus /utf-8" }
    multiprocessorcompile ("On")
    characterset ("Unicode")
    staticruntime "On"

    defines { "rsc_CompanyName=\"ThirteenAG\"" }
    defines { "rsc_LegalCopyright=\"MIT License\"" }
    defines { "rsc_InternalName=\"%{prj.name}\"", "rsc_ProductName=\"%{prj.name}\"", "rsc_OriginalFilename=\"%{cfg.buildtarget.name}\"" }
    defines { "rsc_FileDescription=\"Open Limit Adjuster for GTA III, Vice City and San Andreas\"" }
    defines { "rsc_UpdateUrl=\"https://github.com/GTAmodding/III.VC.SA.LimitAdjuster\"" }

    local major = os.date("%d")
    local minor = os.date("%m")
    local build = os.date("%Y")
    local revision = os.date("%H") .. os.date("%M")

    if _OPTIONS["with-version"] then
        local t = {}
        for i in _OPTIONS["with-version"]:gmatch("([^.]+)") do
            t[#t + 1], _ = i:gsub("%D+", "")
        end
        while #t < 4 do t[#t + 1] = 0 end
        major    = math.min(tonumber(t[1]), 255)
        minor    = math.min(tonumber(t[2]), 255)
        build    = math.min(tonumber(t[3]), 65535)
        revision = math.min(tonumber(t[4]), 65535)
    end

    local githash = ""
    local f = io.popen("git rev-parse --short HEAD")
    if f then
        githash = f:read("*a"):gsub("%s+", "")
        f:close()
    end

    local productVersion = major .. "." .. minor .. "." .. build .. "." .. revision
    if githash ~= "" then
        productVersion = productVersion .. "-" .. githash
    end

    defines { "rsc_FileVersion_MAJOR=" .. major }
    defines { "rsc_FileVersion_MINOR=" .. minor }
    defines { "rsc_FileVersion_BUILD=" .. build }
    defines { "rsc_FileVersion_REVISION=" .. revision }
    defines { "rsc_FileVersion=\"" .. major .. "." .. minor .. "." .. build .. "\"" }
    defines { "rsc_ProductVersion=\"" .. productVersion .. "\"" }
    defines { "rsc_GitSHA1=\"" .. githash .. "\"" }
    defines { "rsc_GitSHA1W=L\"" .. githash .. "\"" }

    defines { "_CRT_SECURE_NO_WARNINGS" }

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"
        symbols "On"

    filter {}

project "III.VC.SA.LimitAdjuster"
    kind "SharedLib"
    language "C++"
    targetdir "bin"
    targetname "III.VC.SA.LimitAdjuster"
    targetextension ".asi"

    files { "source/**.h", "source/**.cpp" }
    files { "resources/*.rc", "resources/*.h" }
    files { "doc/III.VC.SA.LimitAdjuster.ini" }
    files { "external/hooking/Hooking.Patterns.h", "external/hooking/Hooking.Patterns.cpp" }
    files { "external/injector/safetyhook/include/**.hpp", "external/injector/safetyhook/src/**.cpp" }
    files { "external/injector/zydis/**.h", "external/injector/zydis/**.c" }

    includedirs { "source" }
    includedirs { "external/FusionFixUtils/include" }
    includedirs { "external/hooking" }
    includedirs { "external/injector/include" }
    includedirs { "external/injector/safetyhook/include" }
    includedirs { "external/injector/zydis" }

    vpaths {
        ["source/*"] = { "source/**.*" },
        ["resources/*"] = { "resources/*" },
        ["doc/*"] = { "doc/*" },
        ["external/*"] = { "external/**" },
    }

    setpaths({ "GTAIII_DIR", "GRAND_THEFT_AUTO_VICE_CITY_DIR", "GTA_SAN_ANDREAS_DIR" })

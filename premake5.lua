--[[
    OpenLA Build Script
    Use 'premake5 --help' for help
--]]


--[[
    Options and Actions
--]]

newoption {
    trigger     = "outdir",
    value       = "path",
    description = "Output directory for the build files"
}
if not _OPTIONS["outdir"] then
    _OPTIONS["outdir"] = "build"
end

newoption {
    trigger     = "with-version",
    value       = "STRING",
    description = "Current version"
}


--[[
    The Solution
--]]
solution "OpenLA"

    configurations { "Release", "Debug" }

    location( _OPTIONS["outdir"] )
    targetprefix "" -- no 'lib' prefix on gcc
    targetdir "bin"
    implibdir "bin"
	
    defines { "rsc_CompanyName=\"LimitAdjuster\"" }
    defines { "rsc_LegalCopyright=\"MIT License\""}
    defines { "rsc_InternalName=\"%{prj.name}\"", "rsc_ProductName=\"%{prj.name}\"", "rsc_OriginalFilename=\"%{cfg.buildtarget.name}\"" }
    defines { "rsc_FileDescription=\"This is a open source limit adjuster for Grand Theft Auto III, Vice City and San Andreas\"" }
    defines { "rsc_UpdateUrl=\"https://github.com/ThirteenAG/III.VC.SA.LimitAdjuster\"" }

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

    flags {
        "StaticRuntime",
        "NoImportLib",
        rtti ("Off"),
        "NoBufferSecurityCheck"
    }

    defines {
        "INJECTOR_GVM_HAS_TRANSLATOR",
        'INJECTOR_GVM_PLUGIN_NAME=\"Open Limit Adjuster\"'    -- (additional quotes needed for gmake)
    }

    defines {
        "_CRT_SECURE_NO_WARNINGS",
        "_SCL_SECURE_NO_WARNINGS"
    }

    includedirs {
        "src",
        "src/shared",
        "src/shared/cpatch",
        "src/shared/structs",
    }

    configuration "Debug*"
        flags { "Symbols" }
        
    configuration "Release*"
        defines { "NDEBUG" }
        optimize "Speed"

    configuration "vs*"
        buildoptions { "/arch:IA32" }           -- disable the use of SSE/SSE2 instructions

    project "III.VC.SA.LimitAdjuster"
        language "C++"
        kind "SharedLib"
        targetname "III.VC.SA.LimitAdjuster"
        targetextension ".asi"
        
        flags { "NoPCH" }
        
        files {
            "src/**.cpp",
            "src/**.hpp",
            "src/**.h",
            "doc/**"
        }
        files { "src/resources/Versioninfo.rc" }
        excludes { "sample.cpp" }
        
        


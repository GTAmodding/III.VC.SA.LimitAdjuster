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


--[[
    The Solution
--]]
solution "OpenLA"

    configurations { "Release", "Debug" }

    location( _OPTIONS["outdir"] )
    targetprefix "" -- no 'lib' prefix on gcc
    targetdir "bin"
    implibdir "bin"

    flags {
        "StaticRuntime",
        "NoImportLib",
        rtti ("Off"),
        "NoBufferSecurityCheck"
    }

    defines {
        "INJECTOR_GVM_HAS_TRANSLATOR",
        'INJECTOR_GVM_PLUGIN_NAME="\\"Open Limit Adjuster\\""'    -- (additional quotes needed for gmake)
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
        excludes { "sample.cpp" }
        
        


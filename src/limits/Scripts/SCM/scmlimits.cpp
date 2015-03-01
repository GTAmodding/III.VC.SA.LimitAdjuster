/*
* SCM Limits Adjuster
* Copyright (c) 2015 fastman92
* Licensed under the MIT License (http://opensource.org/licenses/MIT)
*/
#include "LimitAdjuster.h"
#include "SCMlimits.h"
#include "..\src\shared\cpatch\SCM\CPatch.h"

unsigned int Game_GTASA::SCMlimits::iMAINsegmentSize = 200000;
unsigned int Game_GTASA::SCMlimits::iMissionSize = 69000;
bool bNeedsPatch;

class SCMlimitsSA : public Adjuster
{
public:
    enum
    {
        MAINsegmentSize,
        MissionSize,
    };

    const Limit* GetLimits()
    {
        static Limit limits[] =
        {
            { "MAINsegmentSize", MAINsegmentSize },
            { "MissionSize", MissionSize },
            { nullptr, 0 }
        };

        if (GetGVM().IsSA()) return limits;
        return nullptr;
    }

    void ChangeLimit(int id, const std::string& value)
    {
        if (id == MAINsegmentSize)
        {
            Game_GTASA::SCMlimits::iMAINsegmentSize = std::stoi(value);
            bNeedsPatch = true;
        }

        if (id == MissionSize)
        {
            Game_GTASA::SCMlimits::iMissionSize = std::stoi(value);
            bNeedsPatch = true;
        }
    }
    void Process()
    {
        if (bNeedsPatch)
        Game_GTASA::SCMlimits::PatchScriptLimits();
    }
} SCMlimitsSA;

namespace Game_GTASA
{
    unsigned __int8* SCMlimits::CTheScripts__ScriptSpace = (unsigned __int8*)0xA49960;

    // Returns script space size
    unsigned int SCMlimits::GetScriptSpaceSize()
    {
        return SCMlimits::iMAINsegmentSize + SCMlimits::iMissionSize;
    }

    // Sets max size of main.scm
    void SCMlimits::SetMaxMAINsegmentSize(int maxMAINsegmentSize)
    {
        SCMlimits::iMAINsegmentSize = maxMAINsegmentSize;
    }

    // Sets max size of mission
    void SCMlimits::SetMaxMissionSize(int iMaxMissionSize)
    {
        SCMlimits::iMissionSize = iMaxMissionSize;
    }

    // Patches script space limits, should be executed when limits are set.
    void SCMlimits::PatchScriptLimits()
    {
            if (SCMlimits::iMAINsegmentSize == 200000
                && SCMlimits::iMissionSize == 69000)
                return;


        // See if sizes are multiplies of 4
        if (SCMlimits::iMAINsegmentSize % 4 != 0 || SCMlimits::iMissionSize % 4 != 0)
        {
            /*printf_MessageBox(
                "'Max size of MAIN segment' and 'Max mission size' have to be multiplies of 4\n"
                "Given values:\n"
                "SCMlimits::iMAINsegmentSize = %d\n"
                "SCMlimits::iMissionSize = %d",

                SCMlimits::iMAINsegmentSize,
                SCMlimits::iMissionSize

                );*/

            return;
        }

        unsigned int ScriptSpaceSize = SCMlimits::GetScriptSpaceSize();

        if (ScriptSpaceSize > 269000)
            SCMlimits::CTheScripts__ScriptSpace = new unsigned __int8[ScriptSpaceSize];

        // Patch references to CTheScripts::ScriptSpace
#if TRUE
        {
            CPatch::PatchPointer(0x44CA42 + 2, CTheScripts__ScriptSpace);	// mov     edx, dword ptr ds:_ZN11CTheScripts11ScriptSpaceE[eax]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x44CA92 + 2, CTheScripts__ScriptSpace);	// mov     eax, dword ptr ds:_ZN11CTheScripts11ScriptSpaceE[eax]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x44CAB1 + 2, CTheScripts__ScriptSpace);	// mov     esi, dword ptr ds:_ZN11CTheScripts11ScriptSpaceE[esi]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x44CB36 + 2, CTheScripts__ScriptSpace);	// mov     ecx, dword ptr ds:_ZN11CTheScripts11ScriptSpaceE[edi]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x44CB58 + 2, CTheScripts__ScriptSpace);	// mov     dword ptr ds:_ZN11CTheScripts11ScriptSpaceE[edi], ecx; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x44CB60 + 2, CTheScripts__ScriptSpace);	// mov     ecx, dword ptr ds:_ZN11CTheScripts11ScriptSpaceE[eax]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x44CB6B + 2, CTheScripts__ScriptSpace);	// mov     dword ptr ds:_ZN11CTheScripts11ScriptSpaceE[eax], 0; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x44CBAD + 2, CTheScripts__ScriptSpace);	// mov     dword ptr ds:_ZN11CTheScripts11ScriptSpaceE[edi], ecx; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x463DAD + 1, CTheScripts__ScriptSpace);	// add     eax, offset _ZN11CTheScripts11ScriptSpaceE; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x463E00 + 3, CTheScripts__ScriptSpace);	// lea     eax, _ZN11CTheScripts11ScriptSpaceE[ecx+edx*8]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x463E38 + 3, CTheScripts__ScriptSpace);	// lea     edx, _ZN11CTheScripts11ScriptSpaceE[eax+ecx]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x463E51 + 3, CTheScripts__ScriptSpace);	// lea     eax, _ZN11CTheScripts11ScriptSpaceE[ecx+edx]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x463FC7 + 2, CTheScripts__ScriptSpace);	// add     ecx, offset _ZN11CTheScripts11ScriptSpaceE; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x463FE9 + 1, CTheScripts__ScriptSpace);	// add     eax, offset _ZN11CTheScripts11ScriptSpaceE; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x4640DE + 2, CTheScripts__ScriptSpace);	// mov     edx, dword ptr ds:_ZN11CTheScripts11ScriptSpaceE[eax]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x464159 + 2, CTheScripts__ScriptSpace);	// mov     edx, dword ptr ds:_ZN11CTheScripts11ScriptSpaceE[edx]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x464165 + 3, CTheScripts__ScriptSpace);	// mov     edx, dword ptr ds:_ZN11CTheScripts11ScriptSpaceE[eax+edx*4]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x464191 + 3, CTheScripts__ScriptSpace);	// mov     edx, dword ptr ds:_ZN11CTheScripts11ScriptSpaceE[eax+edx*4]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x4641BA + 2, CTheScripts__ScriptSpace);	// mov     edx, dword ptr ds:_ZN11CTheScripts11ScriptSpaceE[edx]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x46429A + 2, CTheScripts__ScriptSpace);	// mov     eax, dword ptr ds:_ZN11CTheScripts11ScriptSpaceE[eax]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x464310 + 3, CTheScripts__ScriptSpace);	// mov     eax, dword ptr ds:_ZN11CTheScripts11ScriptSpaceE[eax+edx*4]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x4643C0 + 2, CTheScripts__ScriptSpace);	// mov     dword ptr ds:_ZN11CTheScripts11ScriptSpaceE[eax], edx; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x464412 + 2, CTheScripts__ScriptSpace);	// mov     edx, dword ptr ds:_ZN11CTheScripts11ScriptSpaceE[edx]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x464420 + 3, CTheScripts__ScriptSpace);	// mov     dword ptr ds:_ZN11CTheScripts11ScriptSpaceE[eax+edx*4], esi; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x46444C + 3, CTheScripts__ScriptSpace);	// mov     dword ptr ds:_ZN11CTheScripts11ScriptSpaceE[eax+edx*4], esi; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x464473 + 2, CTheScripts__ScriptSpace);	// mov     edx, dword ptr ds:_ZN11CTheScripts11ScriptSpaceE[edx]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x464555 + 2, CTheScripts__ScriptSpace);	// mov     edx, dword ptr ds:_ZN11CTheScripts11ScriptSpaceE[eax]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x4645D0 + 2, CTheScripts__ScriptSpace);	// mov     edx, dword ptr ds:_ZN11CTheScripts11ScriptSpaceE[edx]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x4645DC + 3, CTheScripts__ScriptSpace);	// mov     edx, dword ptr ds:_ZN11CTheScripts11ScriptSpaceE[eax+edx*4]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x464608 + 3, CTheScripts__ScriptSpace);	// mov     edx, dword ptr ds:_ZN11CTheScripts11ScriptSpaceE[eax+edx*4]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x464631 + 2, CTheScripts__ScriptSpace);	// mov     edx, dword ptr ds:_ZN11CTheScripts11ScriptSpaceE[edx]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x4647CA + 1, CTheScripts__ScriptSpace);	// add     eax, offset _ZN11CTheScripts11ScriptSpaceE; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x46482A + 3, CTheScripts__ScriptSpace);	// lea     eax, _ZN11CTheScripts11ScriptSpaceE[ecx+edx]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x464846 + 3, CTheScripts__ScriptSpace);	// lea     eax, _ZN11CTheScripts11ScriptSpaceE[eax+ecx*8]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x46485D + 3, CTheScripts__ScriptSpace);	// lea     eax, _ZN11CTheScripts11ScriptSpaceE[edx+eax*4]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x466834 + 2, CTheScripts__ScriptSpace);	// lea     eax, _ZN11CTheScripts11ScriptSpaceE[eax]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x468D5D + 1, CTheScripts__ScriptSpace);	// mov     edi, offset _ZN11CTheScripts11ScriptSpaceE; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x468E79 + 1, CTheScripts__ScriptSpace);	// push    offset _ZN11CTheScripts11ScriptSpaceE; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x468ED5 + 1, CTheScripts__ScriptSpace);	// push    offset _ZN11CTheScripts11ScriptSpaceE; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x469448 + 2, CTheScripts__ScriptSpace);	// lea     ecx, _ZN11CTheScripts11ScriptSpaceE[eax]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x5D4C43 + 1, CTheScripts__ScriptSpace + 3);	// mov     eax, dword ptr ds:_ZN11CTheScripts11ScriptSpaceE+3
            CPatch::PatchPointer(0x5D4C92 + 2, CTheScripts__ScriptSpace);	// lea     ecx, _ZN11CTheScripts11ScriptSpaceE[edi]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x5D4CAF + 2, CTheScripts__ScriptSpace);	// lea     edx, _ZN11CTheScripts11ScriptSpaceE[edi]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x5D4F6A + 1, CTheScripts__ScriptSpace);	// sub     eax, offset _ZN11CTheScripts11ScriptSpaceE; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x5D4F92 + 1, CTheScripts__ScriptSpace);	// sub     eax, offset _ZN11CTheScripts11ScriptSpaceE; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x5D5020 + 2, CTheScripts__ScriptSpace);	// lea     ecx, _ZN11CTheScripts11ScriptSpaceE[edi]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x5D503D + 2, CTheScripts__ScriptSpace);	// lea     edx, _ZN11CTheScripts11ScriptSpaceE[edi]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x5D534F + 2, CTheScripts__ScriptSpace);	// lea     eax, _ZN11CTheScripts11ScriptSpaceE[edx]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x5D537E + 2, CTheScripts__ScriptSpace);	// lea     edx, _ZN11CTheScripts11ScriptSpaceE[eax]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x15620C3 + 2, CTheScripts__ScriptSpace);	// lea     eax, _ZN11CTheScripts11ScriptSpaceE[eax]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x1564F1E + 2, CTheScripts__ScriptSpace);	// mov     ecx, dword ptr ds:_ZN11CTheScripts11ScriptSpaceE[eax]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x1564FB2 + 2, CTheScripts__ScriptSpace);	// mov     dword ptr ds:_ZN11CTheScripts11ScriptSpaceE[edx], 0; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x1565610 + 1, CTheScripts__ScriptSpace);	// push    offset _ZN11CTheScripts11ScriptSpaceE; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x1565E20 + 1, CTheScripts__ScriptSpace + 3);	// mov     eax, dword ptr ds:_ZN11CTheScripts11ScriptSpaceE+3
            CPatch::PatchPointer(0x1565E25 + 2, CTheScripts__ScriptSpace + 3);	// mov     eax, dword ptr ds:(_ZN11CTheScripts11ScriptSpaceE+3)[eax]
            CPatch::PatchPointer(0x1565E2B + 1, CTheScripts__ScriptSpace + 8);	// add     eax, (offset _ZN11CTheScripts11ScriptSpaceE+8)
            CPatch::PatchPointer(0x1569FF2 + 2, CTheScripts__ScriptSpace);	// mov     ecx, dword ptr ds:_ZN11CTheScripts11ScriptSpaceE[eax]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x156A082 + 2, CTheScripts__ScriptSpace);	// mov     eax, dword ptr ds:_ZN11CTheScripts11ScriptSpaceE[eax]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x156A09F + 2, CTheScripts__ScriptSpace);	// mov     ebx, dword ptr ds:_ZN11CTheScripts11ScriptSpaceE[eax]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x156AA19 + 2, CTheScripts__ScriptSpace);	// cmp     dword ptr ds:_ZN11CTheScripts11ScriptSpaceE[eax], 1; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x156E375 + 2, CTheScripts__ScriptSpace);	// mov     edx, dword ptr ds:_ZN11CTheScripts11ScriptSpaceE[edx]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x156F378 + 2, CTheScripts__ScriptSpace);	// mov     edx, dword ptr ds:_ZN11CTheScripts11ScriptSpaceE[edx]; CTheScripts::ScriptSpace
            CPatch::PatchPointer(0x156F453 + 1, CTheScripts__ScriptSpace + 3);	// mov     eax, dword ptr ds:_ZN11CTheScripts11ScriptSpaceE+3
            CPatch::PatchPointer(0x156F45C + 2, CTheScripts__ScriptSpace + 3);	// mov     ecx, dword ptr ds:(_ZN11CTheScripts11ScriptSpaceE+3)[eax]
            CPatch::PatchPointer(0x156F462 + 2, CTheScripts__ScriptSpace + 3);	// mov     esi, dword ptr ds:(_ZN11CTheScripts11ScriptSpaceE+3)[ecx]
            CPatch::PatchPointer(0x156F468 + 2, CTheScripts__ScriptSpace + 8);	// add     esi, (offset _ZN11CTheScripts11ScriptSpaceE+8)
            CPatch::PatchPointer(0x156F5A0 + 2, CTheScripts__ScriptSpace + 3);	// mov     ecx, dword ptr ds:_ZN11CTheScripts11ScriptSpaceE+3
            CPatch::PatchPointer(0x156F5A6 + 3, CTheScripts__ScriptSpace + 8);	// mov     ax, word ptr ds:(_ZN11CTheScripts11ScriptSpaceE+8)[ecx]
            CPatch::PatchPointer(0x156F5AD + 2, CTheScripts__ScriptSpace + 8);	// lea     ecx, (_ZN11CTheScripts11ScriptSpaceE+8)[ecx]
        }
#endif

        // Patch sizes of MAIN segment size
#if TRUE
        {
            CPatch::PatchDWORD(0x468E74 + 1, iMAINsegmentSize);	// push    200000          ; len
            CPatch::PatchDWORD(0x468ECE + 1, iMAINsegmentSize);	// push    200000          ; len
        }
#endif

        // Patch references to CTheScripts::ScriptSpace + iMissionSize
#if TRUE
        {
            CPatch::PatchPointer(0x4899D6 + 1, CTheScripts__ScriptSpace + iMAINsegmentSize);	// push    offset unk_A7A6A0
            CPatch::PatchPointer(0x4899F5 + 1, CTheScripts__ScriptSpace + iMAINsegmentSize);	// push    offset unk_A7A6A0
            CPatch::PatchPointer(0x489A0B + 3, CTheScripts__ScriptSpace + iMAINsegmentSize);	// mov     dword ptr [eax+10h], offset unk_A7A6A0
            CPatch::PatchPointer(0x489A5F + 1, CTheScripts__ScriptSpace + iMAINsegmentSize);	// push    offset unk_A7A6A0
            CPatch::PatchPointer(0x489A75 + 1, CTheScripts__ScriptSpace + iMAINsegmentSize);	// push    offset unk_A7A6A0
            CPatch::PatchPointer(0x489A8B + 3, CTheScripts__ScriptSpace + iMAINsegmentSize);	// mov     dword ptr [eax+10h], offset unk_A7A6A0
        }
#endif

        // Patch sizes of mission space
#if TRUE
        {
            CPatch::PatchDWORD(0x4899D1 + 1, iMissionSize);	// push    69000
            CPatch::PatchDWORD(0x489A5A + 1, iMissionSize);	// push    69000

        }
#endif

        // Patch sizes of CTheScripts::ScriptSpace
#if TRUE
        {
            CPatch::PatchDWORD(
                0x401CF8 + 1,
                ScriptSpaceSize / sizeof(DWORD)
                );	// mov     ecx, 106B2h

        }
#endif

        //CPatch::DisableDebugMode();
        /*CGenericLogStorage::SaveFormattedTextLn(
            "Modified SCM limits:\n"
            " - max size of MAIN segment from main.scm: %d bytes\n"
            " - max size of mission from main.scm: %d bytes\n"
            "Total size of CTheScripts::ScriptSpace: %d bytes"
            ,
            SCMlimits::iMAINsegmentSize,
            SCMlimits::iMissionSize,
            ScriptSpaceSize);
        CGenericLogStorage::WriteLineSeparator();*/
    }
}
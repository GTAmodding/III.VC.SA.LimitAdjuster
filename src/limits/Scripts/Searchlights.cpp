/*
* Searchlights Adjuster
* Copyright (c) 2014 ThirteenAG
* Licensed under the MIT License (http://opensource.org/licenses/MIT)
*/
#include "LimitAdjuster.h"
#include "CPatch.h"

std::vector<char> aSearchlights;

class SearchlightsSA : public SimpleAdjuster
{
public:
    const char* GetLimitName() { return GetGVM().IsSA() ? "ScriptSearchLights" : nullptr; }

    void ChangeLimit(int, const std::string& value) 
    {
        int nCount = std::stoi(value);
        aSearchlights.resize(nCount * 0x7C);

        if (GetGVM().IsHoodlum())
        {
            CPatch::AdjustPointer(0x483A31, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493183, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x49337B, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x49399A, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x5870CD, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x156223F, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x1565793, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x15692C1, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x1569305, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x156F01D, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x4773A3, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x4773B8, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493189, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x15692F6, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x47B667, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x49318F, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x156930B, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493196, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x15692FC, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x4837C3, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x483A23, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x4939D4, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x49319C, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x1569315, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x4931A2, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x156931F, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x4692C6, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x4931A8, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x1569329, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x4931AE, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x1569333, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x4931B4, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x4939F8, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x156933D, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x4931BA, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x1569347, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x4931C0, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x1569353, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x4931C6, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x156934D, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x4931CC, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x1569359, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x4931D2, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x156935F, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x4931D8, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x1569365, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x4931DF, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x156936C, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x4931E5, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x1569372, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x4931EB, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x1569378, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x4931FF, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x156938C, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x49320D, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493217, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493E3D, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x156939A, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x15693A4, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x4931F1, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x4931F9, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x156937E, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x1569386, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493225, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x49322F, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x15693C2, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x15693CA, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x49323D, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493247, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x15693D8, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x15693E2, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493255, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x49325D, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x15693F0, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x15693F8, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x4932E0, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x4932E9, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x4932A0, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x15692DB, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x49328E, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x4932F2, &aSearchlights[0], 0xA94D68, 0xA95148);

            CPatch::SetInt(0x015692D3, 992 * nCount);
            CPatch::SetInt(0x493E42, nCount);
        }
        else
        {
            CPatch::AdjustPointer(0x4839A0 + 0x8F, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493000, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493000 + 0x43, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493160 + 0x21, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493360 + 0x1A, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493420 + 0x1C, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493480 + 0x1E, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x4934F0 + 0x22, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493960 + 0x39, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x587000 + 0xCC, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x4773A1, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x4773B6, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493000 + 0x34, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493160 + 0x27, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x47A760 + 0xF05, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493000 + 0x49, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493160 + 0x2D, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493000 + 0x3A, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493160 + 0x34, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x483720 + 0xA1, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x4839A0 + 0x80, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493960 + 0x71, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493000 + 0x53, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493160 + 0x3A, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493000 + 0x5D, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493160 + 0x40, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x468D50 + 0x575, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493000 + 0x67, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493160 + 0x46, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493000 + 0x71, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493160 + 0x4C, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493000 + 0x7B, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493160 + 0x52, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x4939F0 + 0x7, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493000 + 0x85, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493160 + 0x58, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493000 + 0x91, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493160 + 0x5E, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493000 + 0x8B, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493160 + 0x64, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493000 + 0x97, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493160 + 0x6A, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493000 + 0x9D, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493160 + 0x70, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493000 + 0xA3, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493160 + 0x76, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493000 + 0xAA, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493160 + 0x7D, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493000 + 0xB0, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493160 + 0x83, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493000 + 0xB6, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493160 + 0x89, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493000 + 0xCA, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493160 + 0x9D, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x4930D8, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493000 + 0xE2, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x49320B, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493160 + 0xB5, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493E30 + 0xC, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493000 + 0xBC, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493000 + 0xC4, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493160 + 0x8F, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493160 + 0x97, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493100, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493000 + 0x108, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493223, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493160 + 0xCD, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493116, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493000 + 0x120, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x49323B, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493160 + 0xE5, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x49312E, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493000 + 0x136, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493253, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493160 + 0xFB, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493280 + 0xC, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493280 + 0x1E, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x493000 + 0x19, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x4932E0, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x4932E9, &aSearchlights[0], 0xA94D68, 0xA95148);
            CPatch::AdjustPointer(0x4932F2, &aSearchlights[0], 0xA94D68, 0xA95148);
            
            CPatch::SetInt(0x493013, 992 * nCount);
            CPatch::SetInt(0x493E42, nCount);
        }
    }

} SearchlightsSA;

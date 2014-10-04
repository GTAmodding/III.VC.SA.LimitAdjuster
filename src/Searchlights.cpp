/*
* Searchlights Adjuster
* Copyright (c) 2014 ThirteenAG
* Copyright (c) 2014 LINK/2012
* Licensed under the MIT License (http://opensource.org/licenses/MIT)
*/
#include "LimitAdjuster.h"
#include "injector/assembly.hpp"
#include "utility/dummy_object.hpp"
#include "utility/StaticArrayAdjuster.hpp"

typedef dummy_object_zero<0x7C> CSearchlights_SA;

struct ScriptSearchlightsSA : public StaticArrayAdjuster<CSearchlights_SA>
{
    const char* GetLimitName()
    {
        return IsSA()? "ScriptSearchLights" : nullptr;
    }

    ScriptSearchlightsSA() : StaticArrayAdjuster(0xA94D68, 8)
    {
        if(GetLimitName())
        {
            // TODO HOODLUM https://www.diffchecker.com/1um77dt6
            bool isHoodlum = GetGVM().IsHoodlum();
            this->AddPointer(0x483A31, 0x0);
            this->AddPointer(0x493001, 0x0);
            this->AddPointer(0x493045, 0x0);
            this->AddPointer(0x493183, 0x0);
            this->AddPointer(0x49337B, 0x0);
            this->AddPointer(0x49343D, 0x0);
            this->AddPointer(0x49349F, 0x0);
            this->AddPointer(0x493513, 0x0);
            this->AddPointer(0x49399A, 0x0);
            this->AddPointer(0x5870CD, 0x0);
            this->AddPointer(0x4773A3, 0x1);
            this->AddPointer(0x4773B8, 0x1);
            this->AddPointer(0x493036, 0x1);
            this->AddPointer(0x493189, 0x1);
            this->AddPointer(0x47B667, 0x2);
            this->AddPointer(0x49304B, 0x2);
            this->AddPointer(0x49318F, 0x2);
            this->AddPointer(0x49303C, 0x3);
            this->AddPointer(0x493196, 0x3);
            this->AddPointer(0x4837C3, 0x4);
            this->AddPointer(0x483A23, 0x4);
            this->AddPointer(0x4939D4, 0x4);
            this->AddPointer(0x493055, 0x8);
            this->AddPointer(0x49319C, 0x8);
            this->AddPointer(0x49305F, 0xC);
            this->AddPointer(0x4931A2, 0xC);
            this->AddPointer(0x4692C6, 0x10);
            this->AddPointer(0x493069, 0x10);
            this->AddPointer(0x4931A8, 0x10);
            this->AddPointer(0x493073, 0x14);
            this->AddPointer(0x4931AE, 0x14);
            this->AddPointer(0x49307D, 0x18);
            this->AddPointer(0x4931B4, 0x18);
            this->AddPointer(0x4939F8, 0x18);
            this->AddPointer(0x493087, 0x1C);
            this->AddPointer(0x4931BA, 0x1C);
            this->AddPointer(0x493093, 0x20);
            this->AddPointer(0x4931C0, 0x20);
            this->AddPointer(0x49308D, 0x24);
            this->AddPointer(0x4931C6, 0x24);
            this->AddPointer(0x493099, 0x28);
            this->AddPointer(0x4931CC, 0x28);
            this->AddPointer(0x49309F, 0x2C);
            this->AddPointer(0x4931D2, 0x2C);
            this->AddPointer(0x4930A5, 0x30);
            this->AddPointer(0x4931D8, 0x30);
            this->AddPointer(0x4930AC, 0x34);
            this->AddPointer(0x4931DF, 0x34);
            this->AddPointer(0x4930B2, 0x38);
            this->AddPointer(0x4931E5, 0x38);
            this->AddPointer(0x4930B8, 0x3C);
            this->AddPointer(0x4931EB, 0x3C);
            this->AddPointer(0x4930CC, 0x40);
            this->AddPointer(0x4931FF, 0x40);
            this->AddPointer(0x4930DA, 0x44);
            this->AddPointer(0x4930E4, 0x44);
            this->AddPointer(0x49320D, 0x44);
            this->AddPointer(0x493217, 0x44);
            this->AddPointer(0x493E3D, 0x44);
            this->AddPointer(0x4930BE, 0x48);
            this->AddPointer(0x4930C6, 0x48);
            this->AddPointer(0x4931F1, 0x48);
            this->AddPointer(0x4931F9, 0x48);
            this->AddPointer(0x493102, 0x4C);
            this->AddPointer(0x49310A, 0x4C);
            this->AddPointer(0x493225, 0x4C);
            this->AddPointer(0x49322F, 0x4C);
            this->AddPointer(0x493118, 0x50);
            this->AddPointer(0x493122, 0x50);
            this->AddPointer(0x49323D, 0x50);
            this->AddPointer(0x493247, 0x50);
            this->AddPointer(0x493130, 0x54);
            this->AddPointer(0x493138, 0x54);
            this->AddPointer(0x493255, 0x54);
            this->AddPointer(0x49325D, 0x54);
            this->AddPointer(0x4932E0, 0x58);
            this->AddPointer(0x4932E9, 0x5C);
            this->AddPointer(0x4932F2, 0x60);
            this->AddPointer(0x49328E, 0x64);
            this->AddPointer(0x4932A0, 0x70);
            this->AddPointer(0x49301B, 0x7C);
        }
    }

    void ChangeLimit(int, const std::string& value)
    {
        // Note: Limited to int16 bounds.
        // To increase to uint16 patch the 'jl' @468DA1 to 'jb'. Must check if there's no other signed compare somewhere (rendering iteration?)
        static const uint32_t max = (std::numeric_limits<int16_t>::max)();

        if(Adjuster::IsUnlimited(value))
        {
            auto grower = [this](injector::reg_pack& regs)
            {
                // Grow the array if the 'i' (free index) is out of the bounds of the array
                if(regs.ebp == this->GetArraySize())
                {
                    this->GrowArray();
                    if(GetArraySize() > max) throw std::runtime_error("Out of searchlight bounds");
                }

                // Perform the code we've replaced with this hook
                regs.ecx = *(uint32_t*)(regs.esp + 0xC + 0x8);
                regs.edx = *(uint32_t*)(regs.esp + 0xC + 0xC);
            };

            if(GetGVM().IsHoodlum())
                injector::MakeInline<0x493027, 0x49302F>(grower); // TODO FIX FOR HOODLUM
            else
                injector::MakeInline<0x493027, 0x49302F>(grower);
        }
        else
        {
            auto value_ = std::stoul(value);
            if(value_ > max) throw std::runtime_error("Invalid integer boundaries, integer boundary is int16");
            this->RellocArray(value_);
        }
    }

    protected:
    virtual void ApplyPatches()
    {
        StaticArrayAdjuster::ApplyPatches();
        injector::WriteMemory<size_t>(0x493013, sizeof(CSearchlights_SA) * GetArraySize(), true); // TODO HOODLUM
        injector::WriteMemory<size_t>(GetGVM().IsHoodlum()? 0x015692D3 : 0x493E42, GetArraySize(), true);
    }

} ScriptSearchlightsSA;
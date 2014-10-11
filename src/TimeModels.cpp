/*
* TimeModelInfo Adjuster for GTA 3/VC/SA
* Copyright (c) 2014 ThirteenAG <thirteenag@gmail.com>
* Licensed under the MIT License (http://opensource.org/licenses/MIT)
*/
#include "utility/StoreAdjuster.hpp"
#include "utility/dummy_object.hpp"

typedef dummy_object_vmt<0x58, 0x50C0A0> CTimeModelInfo_III;
typedef dummy_object_vmt<0x50, 0x560280> CTimeModelInfo_VC;
typedef dummy_object_vmt<0x24, 0x4C5640> CTimeModelInfo_SA;

struct TimeModelInfoIII : public StoreAdjuster<CTimeModelInfo_III, 0x94076C, 30>    // T, pDefaultStore, dwDefaultCapacity
{
    const char* GetLimitName()
    {
        return IsIII() ? "TimeModels" : nullptr;
    }

    TimeModelInfoIII()
    {
        this->SetGrower(0x477618);
        this->AddPointer(0x50B3F8, 0x0);
        this->AddPointer(0x50B63D, 0x0);
        this->AddPointer(0x50B645, 0x0);
        this->AddPointer(0x50B663, 0x0);
        this->AddPointer(0x50B7F0, 0x0);
        this->AddPointer(0x50B9C2, 0x0);
        this->AddPointer(0x50B9DF, 0x0);
        this->AddPointer(0x50B9E5, 0x0);
        this->AddPointer(0x50B9EE, 0x0);
        //this->AddPointer(0x50BC98, 0x0);
        //this->AddPointer(0x50BCAB, 0x0);
        //this->AddPointer(0x50BC8A, 0x4);
        if (GetLimitName()) injector::MakeNOP(0x50B9D6, 5, true);
    }

} TimeModelInfoIII;

struct TimeModelInfoVC : public StoreAdjuster<CTimeModelInfo_VC, 0x74A6B0, 385>    // T, pDefaultStore, dwDefaultCapacity
{
    const char* GetLimitName()
    {
        return IsVC() ? "TimeModels" : nullptr;
    }

    TimeModelInfoVC()
    {
        if (*(DWORD *)0x55FF11 != 0x0074A6B4) //exe modified by another adjuster
            return;

        this->SetGrower(0x48C498);
        this->AddPointer(0x55F6E0 + 0x2, 0x0);
        this->AddPointer(0x55F703 + 0x2, 0x0);
        this->AddPointer(0x55F6E0 + 0x29 + 0x2, 0x0);
        this->AddPointer(0x55F6E0 + 0x32 + 0x2, 0x0);
        this->AddPointer(0x55F820 + 0x4C + 0x2, 0x0);
        this->AddPointer(0x55F820 + 0x55 + 0x1, 0x0);
        this->AddPointer(0x55F820 + 0x71 + 0x2, 0x0);
        this->AddPointer(0x55F820 + 0x1AC + 0x2, 0x0);
        this->AddPointer(0x55FA40 + 0x154 + 0x2, 0x0);
        //this->AddPointer(0x55FEC0 + 0x5D + 0x2, 0x0);
        //this->AddPointer(0x55FEC0 + 0x71 + 0x1, 0x0);
        if (GetLimitName()) injector::MakeNOP(0x55F6FC, 5, true);
    }

} TimeModelInfoVC;

struct TimeModelInfoSA : public StoreAdjuster<CTimeModelInfo_SA, 0xB1C960, 169>    // T, pDefaultStore, dwDefaultCapacity
{
    const char* GetLimitName()
    {
        return IsSA()? "TimeModels" : nullptr;
    }

    TimeModelInfoSA()
    {
        this->SetGrower (0x5B3F32);
        this->AddPointer(0x4C6459, 0x0);
        this->AddPointer(0x4C6470, 0x0);
        this->AddPointer(0x4C65EC, 0x0);
        this->AddPointer(0x4C66B1, 0x0);
        this->AddPointer(0x4C66C2, 0x0);
        this->AddPointer(0x4C683B, 0x0);
        //this->AddPointer(0x84BC51, 0x0);
        //this->AddPointer(0x856261, 0x0);
        this->AddPointer(0x4C6464, 0x4);
        this->AddPointer(0x4C66BD, 0x4);
    }

} TimeModelInfoSA;
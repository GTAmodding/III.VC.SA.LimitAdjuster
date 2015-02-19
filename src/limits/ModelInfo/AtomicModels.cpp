/*
* Atomic Models Adjuster
* Copyright (c) 2014 LINK/2012 <dma_2012@hotmail.com>
* Licensed under the MIT License (http://opensource.org/licenses/MIT)
*/
#include "StoreAdjuster.hpp"
#include "utility/dummy_object.hpp"

typedef dummy_object_vmt<0x20, 0x4C5540> CAtomicModelInfo_SA;
typedef dummy_object_vmt<0x44, 0x5602D0> CAtomicModelInfo_VC;


struct AtomicModelsVC : public StoreAdjuster<CAtomicModelInfo_VC, 0x709EAC, 3885>    // T, pDefaultStore, dwDefaultCapacity
{
    const char* GetLimitName()
    {
        return IsVC() ? "AtomicModels" : nullptr;
    }

    AtomicModelsVC()
    {
        if (*(DWORD *)0x55FEC1 != 3885) //exe modified by another adjuster
            return;

        this->SetGrower(0x48C688);
        this->AddPointer(0x55F732, 0x0);
        this->AddPointer(0x55F755, 0x0);
        this->AddPointer(0x55F75B, 0x0);
        this->AddPointer(0x55F764, 0x0);
        this->AddPointer(0x55F83A, 0x0);
        this->AddPointer(0x55F842, 0x0);
        this->AddPointer(0x55F858, 0x0);
        this->AddPointer(0x55F9BA, 0x0);
        this->AddPointer(0x55FB24, 0x0);
        this->AddPointer(0x55FB2E, 0x0);
        this->AddPointer(0x55FBB3, 0x0);
        this->AddPointer(0x55FBB9, 0x0);
        this->AddPointer(0x55FBC2, 0x0);
        this->AddPointer(0x55FBFA, 0x0);
        this->AddPointer(0x55FC17, 0x0);
        this->AddPointer(0x55FC1D, 0x0);
        this->AddPointer(0x55FC26, 0x0);
        this->AddPointer(0x55FC5E, 0x0);
        this->AddPointer(0x55FC7B, 0x0);
        this->AddPointer(0x55FC81, 0x0);
        this->AddPointer(0x55FC8A, 0x0);
        this->AddPointer(0x55FCC2, 0x0);
        this->AddPointer(0x55FCDF, 0x0);
        this->AddPointer(0x55FCE5, 0x0);
        this->AddPointer(0x55FCEE, 0x0);
        this->AddPointer(0x55FD26, 0x0);
        this->AddPointer(0x55FD43, 0x0);
        this->AddPointer(0x55FD49, 0x0);
        this->AddPointer(0x55FD52, 0x0);
        this->AddPointer(0x55FD8A, 0x0);
        this->AddPointer(0x55FDA7, 0x0);
        this->AddPointer(0x55FDAD, 0x0);
        this->AddPointer(0x55FDB6, 0x0);
        this->AddPointer(0x55FDEE, 0x0);
        this->AddPointer(0x55FE0B, 0x0);
        this->AddPointer(0x55FE11, 0x0);
        this->AddPointer(0x55FE1A, 0x0);
        this->AddPointer(0x55FE52, 0x0);
        this->AddPointer(0x55FE6F, 0x0);
        this->AddPointer(0x55FE75, 0x0);
        this->AddPointer(0x55FE7E, 0x0);
        //this->AddPointer(0x55FEE0, 0x0);
        //this->AddPointer(0x55FEF3, 0x0);
        if (GetLimitName()) injector::MakeNOP(0x55F74C, 5, true);
    }

} AtomicModelsVC;

struct AtomicModelsSA : public StoreAdjuster<CAtomicModelInfo_SA, 0xAAE950, 14000>    // T, pDefaultStore, dwDefaultCapacity
{
    const char* GetLimitName()
    {
        return IsSA()? "AtomicModels" : nullptr;
    }

    AtomicModelsSA()
    {
        this->SetGrower (0x5B3D9A);
        this->AddPointer(0x4C63E1, 0x0);
        this->AddPointer(0x4C63FE, 0x0);
        this->AddPointer(0x4C65DA, 0x0);
        this->AddPointer(0x4C6621, 0x0);
        this->AddPointer(0x4C6633, 0x0);
        this->AddPointer(0x4C6865, 0x0);
        this->AddPointer(0x4C689A, 0x0);
        this->AddPointer(0x4C68B5, 0x0);
        this->AddPointer(0x4C68E8, 0x0);
        this->AddPointer(0x4C68F9, 0x0);
        this->AddPointer(0x4C6927, 0x0);
        this->AddPointer(0x4C6938, 0x0);
        this->AddPointer(0x4C6966, 0x0);
        this->AddPointer(0x4C6977, 0x0);
        this->AddPointer(0x4C69A5, 0x0);
        this->AddPointer(0x4C69B6, 0x0);
        this->AddPointer(0x4C69E4, 0x0);
        this->AddPointer(0x4C69F5, 0x0);
        this->AddPointer(0x4C6A23, 0x0);
        this->AddPointer(0x4C6A34, 0x0);
        //this->AddPointer(0x84BBF1, 0x0);
        //this->AddPointer(0x856231, 0x0);
        this->AddPointer(0x4C63F2, 0x4);
        this->AddPointer(0x4C662D, 0x4);
        this->AddPointer(0x4C6822, 0x4);
        this->AddPointer(0x4C6829, 0x4);
        this->AddPointer(0x4C6877, 0x4);
        this->AddPointer(0x4C6881, 0x4);
        this->AddPointer(0x4C6890, 0x4);
        this->AddPointer(0x4C68A5, 0x4);
        this->AddPointer(0x4C68F3, 0x4);
        this->AddPointer(0x4C6932, 0x4);
        this->AddPointer(0x4C6971, 0x4);
        this->AddPointer(0x4C69B0, 0x4);
        this->AddPointer(0x4C69EF, 0x4);
        this->AddPointer(0x4C6A2E, 0x4);
        this->AddPointer(0x4C68AC, 0x1C);
    }

} AtomicModelInfoSA;
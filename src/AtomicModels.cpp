/*
* Atomic Models Adjuster
* Copyright (c) 2014 LINK/2012 <dma_2012@hotmail.com>
* Licensed under the MIT License (http://opensource.org/licenses/MIT)
*/
#include "utility/StoreAdjuster.hpp"
#include "utility/dummy_object.hpp"

typedef dummy_object_vmt<0x20, 0x4C5540> CAtomicModelInfo_SA;


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

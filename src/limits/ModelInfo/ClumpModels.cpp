/*
* Clump Models Adjuster
* Copyright (c) 2014 LINK/2012 <dma_2012@hotmail.com>
* Licensed under the MIT License (http://opensource.org/licenses/MIT)
*/
#include "StoreAdjuster.hpp"
#include "utility/dummy_object.hpp"

typedef dummy_object_vmt<0x24, 0x4C56F0> CClumpModelInfo_SA;


struct ClumpModelsSA : public StoreAdjuster<CClumpModelInfo_SA, 0xB1E958, 92>    // T, pDefaultStore, dwDefaultCapacity
{
	const char* GetLimitName()
    {
        return IsSA()? "ClumpModels" : nullptr;
    }

	ClumpModelsSA()
	{
        this->SetGrower (0x5B407E, 0x5B413B);
        this->AddPointer(0x4C64C9, 0x0);
        this->AddPointer(0x4C64E0, 0x0);
        this->AddPointer(0x4C65FE, 0x0);
        this->AddPointer(0x4C6741, 0x0);
        this->AddPointer(0x4C6752, 0x0);
        this->AddPointer(0x4C684D, 0x0);
        //this->AddPointer(0x84BCB1, 0x0);
        //this->AddPointer(0x856291, 0x0);
        this->AddPointer(0x4C64D4, 0x4);
        this->AddPointer(0x4C674D, 0x4);
	}

} ClumpModelsSA;

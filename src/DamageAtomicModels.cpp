/*
* Damage Atomic Models Models Adjuster
* Copyright (c) 2014 LINK/2012 <dma_2012@hotmail.com>
* Licensed under the MIT License (http://opensource.org/licenses/MIT)
*/
#include "utility/StoreAdjuster.hpp"
#include "utility/dummy_object.hpp"

typedef dummy_object_vmt<0x24, 0x4C55A0> CDamageAtomicModelInfo_SA;


struct DamageAtomicModelsSA : public StoreAdjuster<CDamageAtomicModelInfo_SA, 0xB1BF58, 70>    // T, pDefaultStore, dwDefaultCapacity
{
	const char* GetLimitName()
    {
        return IsSA()? "DamageAtomicModels" : nullptr;
    }

	DamageAtomicModelsSA()
	{
        this->SetGrower (0x5B3D8E);
        this->AddPointer(0x4C640B, 0x0);
        this->AddPointer(0x4C6428, 0x0);
        this->AddPointer(0x4C65E0, 0x0);
        this->AddPointer(0x4C6651, 0x0);
        this->AddPointer(0x4C6662, 0x0);
        this->AddPointer(0x4C682F, 0x0);
        //this->AddPointer(0x84BC11, 0x0);
        //this->AddPointer(0x856241, 0x0);
        this->AddPointer(0x4C6416, 0x4);
        this->AddPointer(0x4C665D, 0x4);
	}

} DamageAtomicModelSA;

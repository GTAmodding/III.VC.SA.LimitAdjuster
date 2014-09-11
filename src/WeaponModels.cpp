/*
* Weapon Models Adjuster
* Copyright (c) 2014 LINK/2012 <dma_2012@hotmail.com>
* Licensed under the MIT License (http://opensource.org/licenses/MIT)
*/
#include "utility/StoreAdjuster.hpp"
#include "utility/dummy_object.hpp"

typedef dummy_object_vmt<0x28, 0x4C5760> CWeaponModelInfo_SA;


struct WeaponModelsSA : public StoreAdjuster<CWeaponModelInfo_SA, 0xB1E158, 51>    // T, pDefaultStore, dwDefaultCapacity
{
	const char* GetLimitName()
    {
        return IsSA()? "WeaponModels" : nullptr;
    }

	WeaponModelsSA()
	{
        this->SetGrower (0x5B3FE6);
        this->AddPointer(0x4C64A5, 0x0);
        this->AddPointer(0x4C64BC, 0x0);
        this->AddPointer(0x4C65F8, 0x0);
        this->AddPointer(0x4C6711, 0x0);
        this->AddPointer(0x4C6722, 0x0);
        this->AddPointer(0x4C6847, 0x0);
        //this->AddPointer(0x84BC91, 0x0);
        //this->AddPointer(0x856281, 0x0);
        this->AddPointer(0x4C64B0, 0x4);
        this->AddPointer(0x4C671D, 0x4);
	}

} WeaponModelsSA;


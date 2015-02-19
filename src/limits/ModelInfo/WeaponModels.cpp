/*
* Weapon Models Adjuster
* Copyright (c) 2014 LINK/2012 <dma_2012@hotmail.com>
* Licensed under the MIT License (http://opensource.org/licenses/MIT)
*/
#include "StoreAdjuster.hpp"
#include "utility/dummy_object.hpp"

typedef dummy_object_vmt<0x28, 0x4C5760> CWeaponModelInfo_SA;
typedef dummy_object_vmt<0x48, 0x560220> CWeaponModelInfo_VC;


struct WeaponModelsVC : public StoreAdjuster<CWeaponModelInfo_VC, 0x751F10, 37>    // T, pDefaultStore, dwDefaultCapacity
{
    const char* GetLimitName()
    {
        return IsVC() ? "WeaponModels" : nullptr;
    }

    WeaponModelsVC()
    {
        this->SetGrower(0x48CA2B);
        this->AddPointer(0x55F692, 0x0);
        this->AddPointer(0x55F6AF, 0x0);
        this->AddPointer(0x55F6B5, 0x0);
        this->AddPointer(0x55F6BE, 0x0);
        this->AddPointer(0x55F8A9, 0x0);
        this->AddPointer(0x55F8B1, 0x0);
        this->AddPointer(0x55F8C8, 0x0);
        this->AddPointer(0x55F9E2, 0x0);
        this->AddPointer(0x55FB8C, 0x0);
        //this->AddPointer(0x55FF5B, 0x0);
        //this->AddPointer(0x55FF6E, 0x0);
    }

} WeaponModelsVC;

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


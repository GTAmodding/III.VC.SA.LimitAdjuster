/*
* Vehicle Models Adjuster
* Copyright (c) 2014 LINK/2012 <dma_2012@hotmail.com>
* Licensed under the MIT License (http://opensource.org/licenses/MIT)
*/
#include "StoreAdjuster.hpp"
#include "utility/dummy_object.hpp"
#include "injector/utility.hpp"

static injector::hook_back<void*(__fastcall*)(void*)> hb_ctor;

typedef dummy_object_vmt<0x308, 0x4C75E0> CVehicleModelInfo_SA;
typedef dummy_object_vmt<0x174, 0x57AA20> CVehicleModelInfo_VC;


struct VehicleModelsVC : public StoreAdjuster<CVehicleModelInfo_VC, 0x752A88, 110>    // T, pDefaultStore, dwDefaultCapacity
{
    const char* GetLimitName()
    {
        return IsVC() ? "VehicleModels" : nullptr;
    }

    VehicleModelsVC()
    {
        this->SetGrower(0x48BF16);
        this->AddPointer(0x55F5D2, 0x0);
        this->AddPointer(0x55F5ED, 0x0);
        this->AddPointer(0x55F5F3, 0x0);
        this->AddPointer(0x55F5FE, 0x0);
        this->AddPointer(0x55F919, 0x0);
        this->AddPointer(0x55F921, 0x0);
        this->AddPointer(0x55F93B, 0x0);
        this->AddPointer(0x55F9D8, 0x0);
        this->AddPointer(0x55FB62, 0x0);
        //this->AddPointer(0x55FFD6, 0x0);
        //this->AddPointer(0x55FFE9, 0x0);
    }

} VehicleModelsVC;

struct VehicleModelsSA : public StoreAdjuster<CVehicleModelInfo_SA, 0xB1F650, 212>    // T, pDefaultStore, dwDefaultCapacity
{
    const char* GetLimitName()
    {
        return IsSA()? "VehicleModels" : nullptr;
    }

    void ChangeLimit(int, const std::string& value)
    {
        // Well, R* forgot to initialize CVehicleModelInfo::m_apDirtMaterials and this directly affects the adjuster
        // since it needs proper constructors. It wasn't noticeable before because the .data seg is initialized with 0's
        // Not doing this fix, the game will crash whenever a vehicle gets rendered
        hb_ctor.fun = injector::MakeCALL(0x4C75E4, FixConstructor).get();
        return StoreAdjuster::ChangeLimit(0, value);
    }

    VehicleModelsSA()
    {
        // -----
        this->SetGrower (0x5B6FD1);
        this->AddPointer(0x4C64ED, 0x0);
        this->AddPointer(0x4C6508, 0x0);
        this->AddPointer(0x4C6604, 0x0);
        this->AddPointer(0x4C6771, 0x0);
        this->AddPointer(0x4C6786, 0x0);
        this->AddPointer(0x4C6853, 0x0);
        //this->AddPointer(0x84BCD1, 0x0);
        //this->AddPointer(0x8562A1, 0x0);
        this->AddPointer(0x4C64F8, 0x4);
        this->AddPointer(0x4C6780, 0x4);
    }

    // Initialises CVehicleModelInfo::m_apDirtMaterials at CVehicleModelInfo constructor 
    static void* __fastcall FixConstructor(CVehicleModelInfo_SA* p)
    {
        std::memset((char*)(p) + 0x230, 0, 16 * 4);
        return hb_ctor.fun(p);
    }

} VehicleModelsSA;


/*
* Ped Models Adjuster
* Copyright (c) 2014 LINK/2012 <dma_2012@hotmail.com>
* Licensed under the MIT License (http://opensource.org/licenses/MIT)
*/
#include "StoreAdjuster.hpp"
#include "utility/dummy_object.hpp"

typedef dummy_object_vmt<0x44, 0x4C57A0> CPedModelInfo_SA;
typedef dummy_object_vmt<0x48, 0x560120> CPedModelInfo_VC;

struct PedModelsVC : public StoreAdjuster<CPedModelInfo_VC, 0x75CA70, 130>    // T, pDefaultStore, dwDefaultCapacity
{
    const char* GetLimitName()
    {
        return IsVC() ? "PedModels" : nullptr;
    }

    PedModelsVC()
    {
        this->SetGrower(0x48BDE6);
        this->AddPointer(0x55F582, 0x0);
        this->AddPointer(0x55F5A3, 0x0);
        this->AddPointer(0x55F5AB, 0x0);
        this->AddPointer(0x55F5B4, 0x0);
        this->AddPointer(0x55F951, 0x0);
        this->AddPointer(0x55F959, 0x0);
        this->AddPointer(0x55F973, 0x0);
        this->AddPointer(0x55F9C4, 0x0);
        this->AddPointer(0x55FB4D, 0x0);
        //this->AddPointer(0x560015, 0x0);
        //this->AddPointer(0x560028, 0x0);
    }

} PedModelsVC;

struct PedModelsSA : public StoreAdjuster<CPedModelInfo_SA, 0xB478F8, 278>    // T, pDefaultStore, dwDefaultCapacity
{
    const char* GetLimitName()
    {
        return IsSA()? "PedModels" : nullptr;
    }

    PedModelsSA()
    {
        this->SetGrower (0x5B74A7);
        this->AddPointer(0x4C6518, 0x0);
        this->AddPointer(0x4C652F, 0x0);
        this->AddPointer(0x4C660A, 0x0);
        this->AddPointer(0x4C67A1, 0x0);
        this->AddPointer(0x4C67B3, 0x0);
        this->AddPointer(0x4C6859, 0x0);
        //this->AddPointer(0x84BCF1, 0x0);
        //this->AddPointer(0x8562B1, 0x0);
        this->AddPointer(0x4C6523, 0x4);
        this->AddPointer(0x4C67AD, 0x4);
    }

} PedModelsSA;

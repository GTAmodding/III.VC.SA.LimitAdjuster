/*
 *  Matrices Limit Adjuster
 *  Copyright (C) 2014 LINK/2012 <dma_2012@hotmail.com>
 *  Licensed under the MIT License (http://opensource.org/licenses/MIT)
 */
#include "LimitAdjuster.h"

/*
 *  Key: StaticMatrices
 *  Value: Integer
 *  Game: SA
 */
struct StaticMatrices : public SimpleAdjuster
{
    // Limit Name
    const char* GetLimitName()
    {
        return (GetGVM().IsSA()? "StaticMatrices" : nullptr);   // Name is wrong, should be Matrices, or MatrixArray, or something
                                                                // Can we change it or...?
    }

    // Sets the limit
    void ChangeLimit(int, const std::string& value)
    {
        injector::WriteMemory(0x54F3A0 + 0x1, std::stoi(value), true);
    }

    // Get matrices usage
    bool GetUsage(int, std::string& output)
    {
        static void* gMatrixList                         = injector::lazy_pointer<0xB74288>::get<void>();
        static uint32_t (__fastcall *GetNumFree)(void*)  = injector::lazy_pointer<0x54EB10>::get(); 
        static uint32_t (__fastcall *GetNumUsed1)(void*) = injector::lazy_pointer<0x54EB30>::get();
        static uint32_t (__fastcall *GetNumUsed2)(void*) = injector::lazy_pointer<0x54EB50>::get();

        auto nFree       = GetNumFree(gMatrixList);     // Amount of free matrices
        auto nUsed       = GetNumUsed1(gMatrixList);    // Amount of non-static matrices
        auto nStaticUsed = GetNumUsed2(gMatrixList);    // Amount of static matrices

        return Adjuster::GetUsage(output, nUsed + nStaticUsed, nUsed + nStaticUsed + nFree);
    }


// Instantiate the adjuster on the global scope
} adjuster_StaticMatrices;


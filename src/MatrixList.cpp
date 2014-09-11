/*
 *  Matrices Limit Adjuster
 *  Copyright (C) 2014 LINK/2012 <dma_2012@hotmail.com>
 *  Licensed under the MIT License (http://opensource.org/licenses/MIT)
 */
#include "LimitAdjuster.h"
#include "utility/dummy_object.hpp"
#include <injector/utility.hpp>
#include <memory>


/*
 *  Key: MatrixList
 *  Value: Integer
 *  Game: SA
 */
struct MatrixList : public SimpleAdjuster
{
    typedef dummy_object<0x54> CMatrixLink;     // dummy CMatrixLink

    injector::hook_back<void*(__fastcall*)(void*)> hbRemoveMatrix_1S;
    injector::hook_back<void*(__fastcall*)(void*)> hbRemoveMatrix_1T;

    // Additional matrices, if asked for unlimited storage
    int total;                                              // Total amount of allocated matrices (by the game and by ourselves)
    std::vector<std::unique_ptr<CMatrixLink[]>> matrices;   // Our additional matrices
    


    // Limit Name
    const char* GetLimitName()
    {
        return (IsSA()? "MatrixList" : nullptr);
    }

    // Sets the limit
    void ChangeLimit(int, const std::string& value)
    {
        if(Adjuster::IsUnlimited(value))
        {
            total = -1;     // We don't know how much yet
            hbRemoveMatrix_1T.fun = injector::MakeCALL(0x54F5B6, CheckMatrix1T).get();
            hbRemoveMatrix_1S.fun = injector::MakeCALL(0x54F548, CheckMatrix1S).get();
        }
        else
        {
            injector::WriteMemory(0x54F3A0 + 0x1, std::stoi(value), true);
        }
    }

    // Get matrices usage
    bool GetUsage(int, std::string& output)
    {
        auto nUsed = GetTotalUsage();   // Amount of used matrices
        auto nFree = GetNumFree();      // Amount of free matrices
        return Adjuster::GetUsage(output, nUsed, nUsed + nFree);
    }



    // Allocates more free matrices
    void* GrowFreeLinkedList()
    {
        // If this is the first time we run, we still don't know the amount of matrices allocated, get it
        if(total == -1) total = GetTotalUsage();

        // Grow the linked list by 100%
        auto grow_by = total;
        total += grow_by;

        // We'll need to insert more CMatrixLinkz to the FreeMatrices linked list, get necessary pointers to perform this operation
        auto FreeMatricesHead = (CMatrixLink*)((uintptr_t)(gMatrixList()) + 0x150);
        auto CMatrixLink__Insert = (CMatrixLink* (__thiscall*)(CMatrixLink*, CMatrixLink*)) injector::lazy_pointer<0x54E8F0>().get<void>();
        
        // Allocate more matrices
        matrices.push_back(std::unique_ptr<CMatrixLink[]>( new CMatrixLink[grow_by] ));
        CMatrixLink *last = nullptr, *m = matrices.back().get();

        // Initialise and insert the newly allocated matrices into the free matrices linked list
        for(int i = 0; i < grow_by; ++i)
        {
            last = &m[i];
            std::memset(last, 0, sizeof(CMatrixLink));      // Barely constructs CMatrixLink, we won't destruct it =////
            CMatrixLink__Insert(FreeMatricesHead, last);
        }

        return last;
    }


    // Hooks at call to CPlaceable::RemoveMatrix to check if no more matrices available
    // When there are no more matrices available the game end up sending a null CPlaceable as the this pointer
    // because it gets a null CMatrixLink::m_pPlaceable from the temp matrix list since there's no more temp matrices to be used
    // So whenever this happens we should add more matrices to the free matrix list

    static void* __fastcall CheckMatrix1T(void* placeable)  // @CPlaceable::AllocateMatrix
    {
        if(placeable == nullptr && GetTempMatrixUsage() == 0)
            return adjuster_MatrixList.GrowFreeLinkedList();
        else
            return adjuster_MatrixList.hbRemoveMatrix_1T.fun(placeable);
    }

    static void* __fastcall CheckMatrix1S(void* placeable)    // @CPlaceable::AllocateStaticMatrix
    {
        if(placeable == nullptr && GetTempMatrixUsage() == 0)
            return adjuster_MatrixList.GrowFreeLinkedList();
        else
            return adjuster_MatrixList.hbRemoveMatrix_1S.fun(placeable);
    }



    //////// Game Information ////////

    // Gets the CMatrixLinkList object
    static void* gMatrixList()
    {
        return injector::lazy_pointer<0xB74288>::get<void>();
    }

    // Gets the number of free matrices
    static uint32_t GetNumFree()
    {
        uint32_t (__fastcall *GetNumFree)(void*) = injector::lazy_pointer<0x54EB10>::get();
        return GetNumFree(gMatrixList());
    }

    // Gets the usage of temp matrices
    static uint32_t GetTempMatrixUsage()
    {
        uint32_t (__fastcall *GetNumUsed1)(void*) = injector::lazy_pointer<0x54EB30>::get();
        return GetNumUsed1(gMatrixList());
    }

    // Gets the usage of static matrices
    static uint32_t GetStaticMatrixUsage()
    {
        uint32_t (__fastcall *GetNumUsed2)(void*) = injector::lazy_pointer<0x54EB50>::get();
        return GetNumUsed2(gMatrixList());
    }

    // Gets the total usage of matrices (temp matrices + static matrices)
    static uint32_t GetTotalUsage()
    {
        return GetTempMatrixUsage() +  GetStaticMatrixUsage();
    }


// Instantiate the adjuster on the global scope
} adjuster_MatrixList;


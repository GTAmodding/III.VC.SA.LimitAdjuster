/*
* Utility for adjusting CStore<> objects
* Copyright (C) 2014 LINK/2012 <dma_2012@hotmail.com>
* Licensed under the MIT License (http://opensource.org/licenses/MIT)
*/
#pragma once
#include "LimitAdjuster.h"
#include "utility/PointerAdjuster.hpp"
#include "DynamicStore.hpp"
#include <algorithm>
#include <injector/utility.hpp>

/*
    StoreAdjuster
        Dynamic adjuster for CStore<> objects
        @T is the type of the element in the CStore (as in CStore<T>)
*/
template<class T, uintptr_t pDefaultStore, size_t dwDefaultCapacity>
class StoreAdjuster : public SimpleAdjuster
{
    public: // Those should be at the top to be initialized before CDynamicStore
        T* const     default_ptr;                                       // Default CStore<T> pointer
        const size_t default_capacity;                                  // Default capacity of the above store

    private:
        PointerAdjuster  ptrs;                                          // Pointers to be replaced in rellocations
        CDynamicStore<T> store;                                         // The new dynamic store
        
        injector::hook_back<void(*)(void*)> hb_r;                       // < for reflector
        injector::hook_back<T*(*)(int)>     hb[2];                      // Go back to the original call for CModelInfo::AddSomething
        injector::memory_pointer_raw        pgrow[2];                   // Pointer to the grower call

        static StoreAdjuster*& Instance() { static StoreAdjuster* x; return x; }

    public:

        StoreAdjuster()
            : default_capacity(dwDefaultCapacity), default_ptr(injector::memory_pointer(pDefaultStore).get()),
              store((uintptr_t)default_ptr, default_capacity)
        {
            Instance() = this;
        }

        // Adds a pointer for rellocation when necessary
        // Displacement is how far (in bytes) this pointer is far from the beggining of the array
        // The displacement is relative to the beggining of the CStore<>, so +0 is CStore<>::m_nCount and +4 is the first object
        void AddPointer(uintptr_t addr, int displacement = 0)
        {
            ptrs.AddPointer(addr, displacement);
        }

        // Sets the call that will grow the CStore<> if necessary
        void SetGrower(uintptr_t addr1, uintptr_t addr2 = 0)
        {
            pgrow[0] = raw_ptr(injector::memory_pointer(addr1).get());
            if(addr2) pgrow[1] = raw_ptr(injector::memory_pointer(addr2).get());
        }


        // Called when the limit is found on the ini
	    virtual void ChangeLimit(int, const std::string& value)
	    {
            if(Adjuster::IsUnlimited(value))
            {
                if(!pgrow[0].is_null()) hb[0].fun = injector::MakeCALL(pgrow[0], H_AddAnother<0>).get();
                if(!pgrow[1].is_null()) hb[1].fun = injector::MakeCALL(pgrow[1], H_AddAnother<1>).get();
                
                #if defined(_DEBUG) && 0            // For testing
                this->store.ForceCapacity(16);  
                #endif
            }
            else
                this->store.ForceCapacity(std::stoi(value));

            this->ApplyPatches();
            hb_r.fun = injector::MakeCALL(  // Set up reflector
                    IsIII()? 0x476468 :
                    IsVC() ? 0x48DCE5 :
                    IsSA() ? 0x5B9253 : 0x0,
                    H_DoReflection).get();
	    }

        // Gets the current usage of this CStore<>
        virtual bool GetUsage(int, std::string& output)
        {
            return Adjuster::GetUsage(output, store.GetUsage(), store.GetCapacity());
        }


    private:

        // Called when necessary to add another item to the CStore
        template<int I>
        static T* H_AddAnother(int id)
        {
            Instance()->AddAnother(id);
            return Instance()->hb[I].fun(id);
        }

        // Called after reading all IDEs to make a reflection of this CStore in the default CStore
        static void H_DoReflection(void* a)
        {
            Instance()->ApplyReflection();
            return Instance()->hb_r.fun(a);
        }

        // Checks if it's possible to add another element in the CStore (called only if 'unlimited')
        void AddAnother(int id)
        {
            // Store the old store bounds, so we can fix the model ptrs array
            auto* begin = &store.GetStore()->m_Objects[0];
            auto* end   = &store.GetStore()->m_Objects[store.GetUsage()];
            if(store.EnsureHasCapacityForAnother())
            {
                // Reapply pointer patches and fix model ptrs array
                this->ApplyPatches();
                this->FixModelPtrs(begin, end);
            }
        }


        // Applies pointer patches
        void ApplyPatches()
        {
            ptrs.ApplyPatches(store.GetStore());
        }

        // Makes a reflection of the current dynamic store in the static store
        void ApplyReflection()
        {
            if(store.IsUsingGameAllocation() == false)
            {
                auto min_caps = (std::min)(default_capacity, store.GetCapacity());
                auto min_use  = (std::min)(default_capacity, store.GetUsage());
                ptrs.ApplyReflection((char*)(default_ptr) + sizeof(int), store.GetStore()->m_Objects, sizeof(T) * min_caps);
                *(size_t*)(default_ptr) = min_use;
            }
        }

        // Fixes CModelInfo::ms_modelInfoPtrs pointers after a CStore reallocation
        void FixModelPtrs(void* begin, void* end)
        {
            using namespace injector;
            void **pModelInfoPtrs, **pModelInfoPtrs_end;

            // Be compatible with id limit adjuster by taking the array bounds from the code segment
            if(IsIII())
            {
                pModelInfoPtrs      = lazy_object<0x50B870+3, void**>::get();
                pModelInfoPtrs_end  = &pModelInfoPtrs[lazy_object<0x50B901+2, size_t>::get()];
            }
            else if(IsVC())
            {
                
                pModelInfoPtrs      = lazy_object<0x48DD04+3, void**>::get();
                pModelInfoPtrs_end  = &pModelInfoPtrs[lazy_object<0x48DD15+2, size_t>::get()];
            }
            else if(IsSA())
            {
                pModelInfoPtrs     = lazy_object<0x5B9299+1, void**>::get();
                pModelInfoPtrs_end = lazy_object<0x5B92AE+2, void**>::get();
            }

            // Find pointers in the array that are in the old bounds and then fix it to the current bound
            for(void** p = pModelInfoPtrs; p != pModelInfoPtrs_end; ++p)
            {
                if(*p >= begin && *p <= end)
                    *p = (char*)*p - ((char*)begin - (char*)&store.GetStore()->m_Objects[0]);
            }
        }

};

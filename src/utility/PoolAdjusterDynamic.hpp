/*
 *  Utility for adjusting pools
 *  Copyright (C) 2014 LINK/2012 <dma_2012@hotmail.com>
 *  Licensed under the MIT License (http://opensource.org/licenses/MIT)
 */
#pragma once
#include "PoolAdjuster.hpp"
#include "DynamicPool.hpp"

namespace PoolAdjusterDynamic_HOOKS
{
    template<class T>
    struct Wrapper
    {
        static void Delete(T* pool, typename T::value_type* obj)
        {
            return pool->Delete(obj);
        }

        static void Instantiate()
        {

        }

    };

    template<class T>
    inline void __declspec(naked) edx_release_eax()
    {
        _asm pushad
        Wrapper<T>::Instantiate();
        _asm push eax
        _asm push edx
        _asm call Wrapper<T>::Delete
        _asm add esp, 8
        _asm popad
        _asm retn
    }

    template<class PoolAdjusterDynamic>
    inline void __declspec(naked) release_eax()
    {
        typedef typename PoolAdjusterDynamic::pool_type T;
        _asm pushad
        Wrapper<T>::Instantiate();
        _asm push eax
        _asm call PoolAdjusterDynamic::pool
        _asm push eax
        _asm call Wrapper<T>::Delete
        _asm add esp, 8
        _asm popad
        _asm retn
    }
};



/*
	PoolAdjusterDynamic
		<>>>>>>>>>>>
		
*/
template<uintptr_t addr_call, uintptr_t addr_ppool, size_t sizeof_obj, uintptr_t addr_sizeofpool,
    uintptr_t addr_alloc, uintptr_t addr_release>
struct PoolAdjusterDynamic : PoolAdjuster<addr_call, addr_ppool>
{
    typedef typename std::aligned_storage<sizeof_obj, 1>::type T;

	typedef void* (__fastcall *func_type)(void* self, int dummy, size_t size, const char* name);
	typedef void* (__fastcall *func_type3)(void* self, int dummy, size_t size); // GTA3
	typedef injector::hook_back<func_type> hook_back;
    typedef CDynamicPool<T> pool_type;

    static pool_type* pool() // Pointer to this pool
    { 
        if(!addr_ppool) return nullptr;
        pool_type** x = injector::lazy_pointer<addr_ppool>::get();
        return *x;
    }

    static size_t& limit() { static size_t x; return x; }           // The new limit, as defined by the ini file
	static hook_back& hb() { static hook_back x; return x; }        // The old call pointer
	
	// Patches the CPool construction call
	static void Patch(size_t newLimit)
	{
		limit() = newLimit;
		
		if(Adjuster::IsIII())
			hb().fun = MakeCALL(addr_call, raw_ptr(HookedConstructor3)).get();
		else
			hb().fun = MakeCALL(addr_call, raw_ptr(HookedConstructor)).get();

        static_assert(sizeof(pool_type) < 127, "sizeof(CDynamicPool) too big");
        
        WriteMemory<char>(addr_sizeofpool, sizeof(pool_type), true);
        if(addr_alloc) MakeJMP(addr_alloc, Do_StaticAlloc);
        if(addr_release) MakeJMP(addr_release, Do_StaticRelease);

        if(Adjuster::IsSA()) SpecialPatch_SA();

        // TODO release
	}

    static void SpecialPatch_SA()
    {
        using namespace PoolAdjusterDynamic_HOOKS;

        switch(addr_ppool)
        {
            case 0xB74484:  // PtrNodeSingle
            {
                MakeCALL(0x55243B, release_eax<PoolAdjusterDynamic>);
                MakeNOP (0x55243B + 5, ((mem_ptr(0x55245B) - mem_ptr(0x55243B)).as_int() - 5));
                break;
            }
        }
    }














	
	// This will be called instead of the standard CPool constructor
	static void* __fastcall HookedConstructor(pool_type* self, int, size_t size, const char* name)
	{
        size = 1; // TODO FIX, AND OTHERS TOO
        self->Init(size, name);
        return self;
	}
	
	// This will be called instead of the standard CPool constructor, GTA3 do not have an name parameter
	static void* __fastcall HookedConstructor3(pool_type* self, int, size_t size)
	{
        return HookedConstructor(self, 0, size, "");
	}

    static T* __cdecl Do_StaticAlloc()
    {
        auto* w = pool();
        int x = sizeof(T);
        T* p = pool()->New();
        if(p == nullptr)
            p = nullptr;
        return p;
    }

    static void __cdecl Do_StaticRelease(T* obj)
    {
        auto* w = pool();
        return pool()->Delete(obj);
    }



    // Change pool limit
    virtual void ChangeLimit(size_t limit)
    {
        // TODO check if unlimited
        return Patch(limit);
    }
    
    // Get pool usage
    virtual bool GetUsage(std::string& output)
    {
        if(auto pool = this->pool())
        {
            auto usage = 0u;
            for(int i = 0; i < pool->m_Size; ++i)
                if(!pool->IsFreeSlotAtIndex(i)) ++usage;

            return Adjuster::GetUsage<uint32_t>(output, usage, pool->m_Size);
        }
        return false;
    }







};


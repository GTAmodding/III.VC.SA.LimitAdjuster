/*
 *  Utility for adjusting pools
 *  Copyright (C) 2014 LINK/2012 <dma_2012@hotmail.com>
 *  Licensed under the MIT License (http://opensource.org/licenses/MIT)
 */
#pragma once
#include <LimitAdjuster.h>
#include <CPool.h>
#include <injector/utility.hpp>

struct PoolAdjusterBase
{
    virtual void MakeUnlimited() = 0;
    virtual void ChangeLimit(size_t) = 0;
    virtual bool GetUsage(std::string& output) = 0;
};

/*
	PoolAdjuster
		This template will create hooks into the pool constructors and will replace the 'size' parameter with
		a custom one, defined by the ini file.
		
		@addr_call is the address of the 'call CPool::CPool'
*/
template<uintptr_t addr_call>
struct PoolAdjuster : PoolAdjusterBase
{
	typedef void* (__fastcall *func_type)(void* self, int dummy, size_t size, const char* name);
	typedef void* (__fastcall *func_type3)(void* self, int dummy, size_t size); // GTA3
	typedef injector::hook_back<func_type> hook_back;
    typedef CPool<char, char> pool_type;

    static pool_type*& pool() // Pointer to this pool
    { 
        static pool_type* x = nullptr;
        return x;
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
	}
	
	// This will be called instead of the standard CPool constructor
	static void* __fastcall HookedConstructor(pool_type* self, int, size_t, const char* name)
	{
        pool() = self;
		return hb().fun(self, 0, limit(), name);
	}
	
	// This will be called instead of the standard CPool constructor, GTA3 do not have an name parameter
	static void* __fastcall HookedConstructor3(pool_type* self, int, size_t)
	{
        pool() = self;
		return ((func_type3)(hb().fun))(self, 0, limit());
	}


    // Change pool limit
    virtual void ChangeLimit(size_t limit)
    {
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

    virtual void MakeUnlimited()
    {
        throw std::runtime_error("Cannot set this pool to be unlimited");
    }
};


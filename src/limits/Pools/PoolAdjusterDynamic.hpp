/*
 *  Utility for adjusting pools
 *  Copyright (C) 2014 LINK/2012 <dma_2012@hotmail.com>
 *  Licensed under the MIT License (http://opensource.org/licenses/MIT)
 */
#pragma once
#include "PoolAdjuster.hpp"
#include "DynamicPool.hpp"
#include <injector/assembly.hpp>


/*
	PoolAdjusterDynamic
		Dynamically resizeable pool adjuster

        @addr_call is the address of the 'call CPool::CPool'
        @addr_sizeofpool is the address of the size to new for the pool, since our pool struct will be larger we need to adjust the value
        @addr_sizeofobj is the size of each element our pool will store
*/
template<uintptr_t addr_call, uintptr_t addr_sizeofpool, size_t sizeof_obj>
struct PoolAdjusterDynamic : PoolAdjuster<addr_call>
{
    typedef typename std::aligned_storage<sizeof_obj, 1>::type T;   // U type of element the pool stores
    typedef CDynamicPool<T> pool_type;                              // The pool type

    // Pointer to this pool
    static pool_type*& pool()
    { 
        static pool_type* x = nullptr;
        return x;
    }
	
	// Patches the CPool to be dynamic
	static void PatchDynamic()
	{
        // Patches the constructor to construct our pool
		if(Adjuster::IsIII())
			hb().fun = MakeCALL(addr_call, raw_ptr(HookedConstructor3)).get();
		else
			hb().fun = MakeCALL(addr_call, raw_ptr(HookedConstructor)).get();

        // Make sure the sizeof CDynamicPool is handleable by the game and change the allocation size
        static_assert(sizeof(pool_type) < 127, "sizeof(CDynamicPool) too big");
        WriteMemory<char>(addr_sizeofpool, sizeof(pool_type), true);

        // Do very specific patching
        if(Adjuster::IsSA()) Patch_SA();
	}

    // Patches operator new and operator delete to our own
    static void PatchStaticND(uintptr_t new_ptr, uintptr_t del_ptr)
    {
        MakeJMP(new_ptr, Do_StaticAlloc);
        MakeJMP(del_ptr, Do_StaticRelease);
    }

    // Patches an delete(px->flags) to actually delete all our custom allocations
    static void PatchShutdown(uintptr_t ptr_to_call_to_delete_flags)
    {
        MakeCALL(ptr_to_call_to_delete_flags, HookedDestruction);
    }


    // San Andreas specific patching
    static void Patch_SA()
    {
        switch(addr_call)
        {
            case 0x55110D:  // ColModel
                PatchShutdown(0x551C30);
                PatchStaticND(0x40FC30, 0x40FC40);
                break;

            case 0x5511B2:  // PointRoute
                PatchShutdown(0x551CF3);
                PatchStaticND(0x41B7C0, 0x41B7D0);
                break;

            case 0x5511E7:  // PatrolRoute
                PatchShutdown(0x551D34);
                PatchStaticND(0x41B810, 0x41B820);
                break;

            case 0x55121C:  // NodeRoute
                PatchShutdown(0x551D75);
                PatchStaticND(0x41B860, 0x41B870);
                break;

            case 0x550F4C:  // PtrNodeSingle
                PatchShutdown(0x551A28);
                PatchStaticND(0x552380, 0x552390);
                injector::MakeInline<0x55243B, 0x55245B>(ReleaseEAX);
                break;

            case 0x550F88:  // PtrNodeDouble
                PatchShutdown(0x551A69);
                PatchStaticND(0x5523C0, 0x5523D0);
                injector::MakeInline<0x5524A9, 0x5524D8>(ReleaseEAX);
                break;

            case 0x550FC0:  // EntryInfoNode
                PatchShutdown(0x551AAA);
                PatchStaticND(0x536DC0, 0x536DD0);
                injector::MakeInline<0x536E49, 0x536E79>(ReleaseEAX);
                break;

            case 0x551145:  // Task
                PatchShutdown(0x551C71);
                PatchStaticND(0x61A5A0, 0x61A5B0);
                injector::MakeJMP(0x61A4B0, Do_IsExactlyContained);
                injector::MakeInline<0x61A5EF, 0x61A613>(ReleaseEAX);
                injector::MakeInline<0x61A631, 0x61A655>(ReleaseEAX);
                injector::MakeInline<0x61A670, 0x61A694>(ReleaseEAX);
                break;

            case 0x551251:  // TaskAllocator
                PatchShutdown(0x551DB6);
                //PatchStaticND(0x69D980, 0x69D990);  // Crashes on HOODLUM, crazy code obfuscation
                MakeJMP(0x69D8E0, Do_Alloc);          // Get over it by hooking at CPool method itself
                MakeJMP(0x69D990, Do_StaticRelease);  // No need to 'get over it' here
                injector::MakeInline<0x69D9D1, 0x69D9F5>(ReleaseESI);
                injector::MakeInline<0x69DA11, 0x69DA35>(ReleaseESI);
                injector::MakeInline<0x69DA51, 0x69DA75>(ReleaseESI);
                injector::MakeInline<0x69DA91, 0x69DAB5>(ReleaseESI);
                injector::MakeInline<0x69DB11, 0x69DB35>(ReleaseESI);
                injector::MakeInline<0x69DAD0, 0x69DAF4>(ReleaseEAX);
                break;

            case 0x5512BF:  // PedAttractors
                PatchShutdown(0x551E38);
                injector::MakeJMP(0x5EA9F0, Do_Alloc);
                injector::MakeJMP(0x5EAFE0, Do_StaticRelease);
                injector::MakeInline<0x5EC4C0, 0x5EC4C0 + 0x35>(ReleaseEDI);
                injector::MakeInline<0x5EE4F0, 0x5EE4F0 + 0x35>(ReleaseEDI);
                injector::MakeInline<0x5EE5C0, 0x5EE5C0 + 0x35>(ReleaseEDI);
                injector::MakeInline<0x5EE690, 0x5EE690 + 0x35>(ReleaseEDI);
                injector::MakeInline<0x5EE770, 0x5EE770 + 0x35>(ReleaseEDI);
                injector::MakeInline<0x5EE840, 0x5EE840 + 0x35>(ReleaseEDI);
                injector::MakeInline<0x5EE910, 0x5EE910 + 0x35>(ReleaseEDI);
                injector::MakeInline<0x5EE9E0, 0x5EE9E0 + 0x35>(ReleaseEDI);
                injector::MakeInline<0x5EEAB0, 0x5EEAB0 + 0x35>(ReleaseEDI);
                injector::MakeInline<0x5EEB80, 0x5EEB80 + 0x35>(ReleaseEDI);
                injector::MakeInline<0x5EEC50, 0x5EEC50 + 0x35>(ReleaseEDI);
                break;

            case 0x55117D:  // Events
                PatchShutdown(0x551CB2);
                injector::MakeJMP(0x4B5570, Do_Alloc);
                injector::MakeJMP(0x4B5630, Do_StaticRelease);
                injector::MakeInline<0x4B67B0, 0x4B67E5>(ReleaseEDI);
                injector::MakeInline<0x4B6450, 0x4B6485>(ReleaseEDI);
                injector::MakeInline<0x4B62C0, 0x4B62F5>(ReleaseEDI);
                injector::MakeInline<0x4B6130, 0x4B6165>(ReleaseEDI);
                injector::MakeInline<0x4B6360, 0x4B6395>(ReleaseEDI);
                injector::MakeInline<0x4B5F00, 0x4B5F35>(ReleaseEDI);
                injector::MakeInline<0x4B6580, 0x4B65B5>(ReleaseEDI);
                injector::MakeInline<0x4B69F0, 0x4B6A25>(ReleaseEDI);
                injector::MakeInline<0x4B6710, 0x4B6745>(ReleaseEDI);
                injector::MakeInline<0x4B5940, 0x4B5975>(ReleaseEDI);
                injector::MakeInline<0x4B6A40, 0x4B6A75>(ReleaseEDI);
                injector::MakeInline<0x4B5FA0, 0x4B5FD5>(ReleaseEDI);
                injector::MakeInline<0x4B6530, 0x4B6565>(ReleaseEDI);
                injector::MakeInline<0x4B63B0, 0x4B63E5>(ReleaseEDI);
                injector::MakeInline<0x4B6180, 0x4B61B5>(ReleaseEDI);
                injector::MakeInline<0x4B60E0, 0x4B6115>(ReleaseEDI);
                injector::MakeInline<0x4B5FF0, 0x4B6025>(ReleaseEDI);
                injector::MakeInline<0x4B59E0, 0x4B5A15>(ReleaseEDI);
                injector::MakeInline<0x4B5F50, 0x4B5F85>(ReleaseEDI);
                injector::MakeInline<0x4B6670, 0x4B66A5>(ReleaseEDI);
                injector::MakeInline<0x4B5E60, 0x4B5E95>(ReleaseEDI);
                injector::MakeInline<0x4B5EB0, 0x4B5EE5>(ReleaseEDI);
                injector::MakeInline<0x4B6A90, 0x4B6AC5>(ReleaseEDI);
                injector::MakeInline<0x4B6220, 0x4B6255>(ReleaseEDI);
                injector::MakeInline<0x4B61D0, 0x4B6205>(ReleaseEDI);
                injector::MakeInline<0x4B6040, 0x4B6075>(ReleaseEDI);
                injector::MakeInline<0x4B5990, 0x4B59C5>(ReleaseEDI);
                injector::MakeInline<0x4B58F0, 0x4B5925>(ReleaseEDI);
                injector::MakeInline<0x4B69A0, 0x4B69D5>(ReleaseEDI);
                injector::MakeInline<0x4B6400, 0x4B6435>(ReleaseEDI);
                injector::MakeInline<0x4B5D00, 0x4B5D35>(ReleaseEDI);
                injector::MakeInline<0x4B5A80, 0x4B5AB5>(ReleaseEDI);
                injector::MakeInline<0x4B5E10, 0x4B5E45>(ReleaseEDI);
                injector::MakeInline<0x4B6AE0, 0x4B6B15>(ReleaseEDI);
                injector::MakeInline<0x4B66C0, 0x4B66F5>(ReleaseEDI);
                injector::MakeInline<0x4B6800, 0x4B6835>(ReleaseEDI);
                injector::MakeInline<0x4B5A30, 0x4B5A65>(ReleaseEDI);
                injector::MakeInline<0x4B6270, 0x4B62A5>(ReleaseEDI);
                injector::MakeInline<0x4B6310, 0x4B6345>(ReleaseEDI);
                injector::MakeInline<0x4B684D, 0x4B6887>(ReleaseECX);
                injector::MakeInline<0x4B675D, 0x4B6797>(ReleaseECX);
                injector::MakeInline<0x4B7C8E, 0x4B7CC8>(ReleaseECX);
                injector::MakeInline<0x4B661D, 0x4B6657>(ReleaseECX);
                injector::MakeInline<0x4B65CD, 0x4B6607>(ReleaseECX);
                injector::MakeInline<0x4B608D, 0x4B60C7>(ReleaseECX);
                injector::MakeInline<0x4B64DD, 0x4B6517>(ReleaseECX);
                injector::MakeInline<0x4B694D, 0x4B6987>(ReleaseECX);
                injector::MakeInline<0x4B567D, 0x4B56B7>(ReleaseECX);
                injector::MakeInline<0x4B5DBD, 0x4B5DF7>(ReleaseECX);
                break;

            default:
                throw std::logic_error("Missing patcher for dynamic pool");
        }
    }





	
	// This will be called instead of the standard CPool constructor
	static void* __fastcall HookedConstructor(pool_type* self, int, size_t size, const char* name)
	{
#if defined(_DEBUG) && 0
        size = 1;       // To test if the dynamicity of the pool is working fine
#endif
        pool() = self;
        return new (self) pool_type(size, name);
	}
	
	// This will be called instead of the standard CPool constructor, GTA3 do not have an name parameter
	static void* __fastcall HookedConstructor3(pool_type* self, int, size_t size)
	{
        return HookedConstructor(self, 0, size, "");
	}
    
    // Called for destruction
    static void __cdecl HookedDestruction(void*)
    {
        pool()->~CDynamicPool();
    }

    // Allocates another element in the pool (operator new hook)
    static T* __cdecl Do_StaticAlloc()
    {
        return pool()->New();
    }

    // Releases another element in the pool (operator delete hook)
    static void __cdecl Do_StaticRelease(T* obj)
    {
        return pool()->Delete(obj);
    }

    // Allocates another element in the pool (CPool::New hook)
    static T* __fastcall Do_Alloc(pool_type* ppool)
    {
        return ppool->New();
    }

    // Checks if the object is exactly in the pool (...)
    static bool __cdecl Do_IsExactlyContained(T* obj)
    {
        return pool()->IsExactlyContained(obj);
    }

    // Deletes the element at the eax register
    static void ReleaseEAX(const injector::reg_pack& regs)
    {
        pool()->Delete(reinterpret_cast<T*>(regs.eax));
    }

    // Deletes the element at the ecx register
    static void ReleaseECX(const injector::reg_pack& regs)
    {
        pool()->Delete(reinterpret_cast<T*>(regs.ecx));
    }

    // Deletes the element at the edi register
    static void ReleaseEDI(const injector::reg_pack& regs)
    {
        pool()->Delete(reinterpret_cast<T*>(regs.edi));
    }

    // Deletes the element at the esi register
    static void ReleaseESI(const injector::reg_pack& regs)
    {
        pool()->Delete(reinterpret_cast<T*>(regs.esi));
    }



    // Patches this pool to be unlimited
    virtual void MakeUnlimited()
    {
        PatchDynamic();
    }

    // Get pool usage (needs to override because the pool structure and methods are different than base)
    virtual bool GetUsage(std::string& output)
    {
        if(auto pool = this->pool())
        {
            auto usage = 0u;
            for(int i = 0; i < pool->m_Size; ++i)
                if(!pool->IsFreeSlotAtIndex(i)) ++usage;

            return Adjuster::GetUsage<uint32_t>(output, usage, pool->m_Size);
        }

        // Not patched using the dynamic adjuster, try to get usage with the normal pool adjuster
        return PoolAdjuster::GetUsage(output);
    }
};


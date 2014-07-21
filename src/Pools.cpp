/*
 *  Dynamic Pools Limit Adjuster
 *  Copyright (C) 2013 ThirteenAG <>
 *  Copyright (C) 2014 LINK/2012 <dma_2012@hotmail.com>
 *  Licensed under the MIT License (http://opensource.org/licenses/MIT)
 */
#include "LimitAdjuster.h"
#include "injector/hooking.hpp"
using namespace injector;

/*
 *  Key: [...]
 *  Value: Integer
 *  Game: III, VC, SA
 */



// Pools initialization on each game happens at:
// 3  at 0x4A1770
// VC at 0x4C0270
// SA at 0x550F10

enum
{
	PtrNode,            // GTA3 / VC
	PtrNodeSingle,      // SA
	PtrNodeDouble,      // SA
	EntryInfoNode,
	Peds,
	Vehicles,
	Buildings,
	Objects,
	Dummys,
	ColModel,           // VC / SA
	
	// VC-only below
	Treadables,
	AudioScriptObj,

	// SA-only below
	Task,
	Event,
	PointRoute,
	PatrolRoute,
	NodeRoute,
	TaskAllocator,
	PedIntelligence,
	PedAttractors,
	
	MAX_POOLS
};


// It's necessary to know if the game is GTA3, because it's pool constructor prototype is different from VC/SA
static bool bIsGTA3 = false;

/*
	PoolAdjuster
		This template will create hooks into the pool constructors and will replace the 'size' parameter with
		a custom one, defined by the ini file.
		
		@addr1 is the address of the 'call CPool::CPool'
*/
template<uintptr_t addr1>
struct PoolAdjuster
{
	typedef void* (__fastcall *func_type)(void* self, int dummy, size_t size, const char* name);
	typedef void* (__fastcall *func_type3)(void* self, int dummy, size_t size); // GTA3
	typedef injector::hook_back<func_type> hook_back;

	static size_t& limit() { static size_t x; return x; }          // The new limit, as defined by the ini file
	static hook_back& hb() { static hook_back x; return x; }       // The old call pointer
	
	// Patches the CPool construction call
	static void Patch(size_t newLimit)
	{
		limit() = newLimit;
		
		if(bIsGTA3)
			hb().fun = MakeCALL(addr1, raw_ptr(HookedConstructor3)).get();
		else
			hb().fun = MakeCALL(addr1, raw_ptr(HookedConstructor)).get();
	}
	
	// This will be called instead of the standard CPool constructor
	static void* __fastcall HookedConstructor(void* self, int, size_t, const char* name)
	{
		return hb().fun(self, 0, limit(), name);
	}
	
	// This will be called instead of the standard CPool constructor, GTA3 do not have an name parameter
	static void* __fastcall HookedConstructor3(void* self, int, size_t)
	{
		return ((func_type3)(hb().fun))(self, 0, limit());
	}
};

// Adjusts the pool at address @addr1 to the limit @newLimit
template<uintptr_t addr1>
inline void AdjustPool(size_t newLimit)
{
	PoolAdjuster<addr1>::Patch(newLimit);
}


/*
 *  Pool Adjuster for GTA3
 */
struct PoolsAdjuster3 : public Adjuster
{
	// Sets the limit
	void ChangeLimit(int id, const std::string& value)
	{
		auto limit = std::stoul(value); 
		switch(id)
		{
			case PtrNode:       AdjustPool<0x4A1783>(limit); break;
			case EntryInfoNode: AdjustPool<0x4A17A0>(limit); break;
			case Peds:          AdjustPool<0x4A17BD>(limit); break;
			case Vehicles:      AdjustPool<0x4A17D7>(limit); break;
			case Buildings:     AdjustPool<0x4A17F4>(limit); break;
			case Objects:       AdjustPool<0x4A182E>(limit); break;
			case Dummys:        AdjustPool<0x4A184B>(limit); break;
		}
	}

	// Limit Names
	const Limit* GetLimits()
	{
		if(GetGVM().IsIII())
		{
			static Limit lm[] =
			{
				DEFINE_LIMIT(PtrNode),
				DEFINE_LIMIT(EntryInfoNode),
				DEFINE_LIMIT(Peds),
				DEFINE_LIMIT(Vehicles),
				DEFINE_LIMIT(Buildings),
				DEFINE_LIMIT(Objects),
				DEFINE_LIMIT(Dummys),
				FINISH_LIMITS()
			};
			
			bIsGTA3 = true;
			return lm;
		}
		return nullptr;
	}

// Instantiate the adjuster on the global scope
} adjuster_Pools_3;


/*
 *  Pool Adjuster for GTA VC
 */
struct PoolsAdjusterVC : public Adjuster
{
	// Sets the limit
	void ChangeLimit(int id, const std::string& value)
	{
		auto limit = std::stoul(value); 
		switch(id)
		{
			case PtrNode:       AdjustPool<0x4C0288>(limit); break;
			case EntryInfoNode: AdjustPool<0x4C02AA>(limit); break;
			case Peds:          AdjustPool<0x4C02CC>(limit); break;
			case Vehicles:      AdjustPool<0x4C02EB>(limit); break;
			case Buildings:     AdjustPool<0x4C030D>(limit); break;
			case Objects:       AdjustPool<0x4C034E>(limit); break;
			case Dummys:        AdjustPool<0x4C0370>(limit); break;
			case ColModel:      AdjustPool<0x4C03B4>(limit); break;
			case Treadables:    AdjustPool<0x4C032C>(limit); break;
			case AudioScriptObj:AdjustPool<0x4C0392>(limit); break;
		}
	}

	// Limit Names
	const Limit* GetLimits()
	{
		if(GetGVM().IsVC())
		{
			static Limit lm[] =
			{
				DEFINE_LIMIT(PtrNode),
				DEFINE_LIMIT(EntryInfoNode),
				DEFINE_LIMIT(Peds),
				DEFINE_LIMIT(Vehicles),
				DEFINE_LIMIT(Buildings),
				DEFINE_LIMIT(Objects),
				DEFINE_LIMIT(Dummys),
				DEFINE_LIMIT(ColModel),
				DEFINE_LIMIT(Treadables),
				DEFINE_LIMIT(AudioScriptObj),
				FINISH_LIMITS()
			};
			return lm;
		}
		return nullptr;
	}

// Instantiate the adjuster on the global scope
} adjuster_Pools_VC;


/*
 *  Pool Adjuster for GTA SA
 */
struct PoolsAdjusterSA : public Adjuster
{
	// Sets the limit
	void ChangeLimit(int id, const std::string& value)
	{
		auto limit = std::stoul(value); 
		switch(id)
		{
			case EntryInfoNode: if (!GetModuleHandle("samp")) { AdjustPool<0x550FC0>(limit); } break;
			case Peds:          AdjustPool<0x550FF8>(limit); break;
			case Vehicles:      AdjustPool<0x55102D>(limit); break;
			case Buildings:     if (!GetModuleHandle("samp")) { AdjustPool<0x551065>(limit); } break;
			case Objects:       AdjustPool<0x55109D>(limit); break;
			case Dummys:        AdjustPool<0x5510D5>(limit); break;
			case ColModel:      if (!GetModuleHandle("samp")) { AdjustPool<0x55110D>(limit); } break;
			case PtrNodeSingle: AdjustPool<0x550F4C>(limit); break;
			case PtrNodeDouble: AdjustPool<0x550F88>(limit); break;
			case Task:          AdjustPool<0x551145>(limit); break;
			case Event:         AdjustPool<0x55117D>(limit); break;
			case PointRoute:    AdjustPool<0x5511B2>(limit); break;
			case PatrolRoute:   AdjustPool<0x5511E7>(limit); break;
			case NodeRoute:     AdjustPool<0x55121C>(limit); break;
			case TaskAllocator: AdjustPool<0x551251>(limit); break;
			case PedIntelligence:AdjustPool<0x551289>(limit); break;
			case PedAttractors: AdjustPool<0x5512BF>(limit); break;
		}
	}

	// Limit Names
	const Limit* GetLimits()
	{
		if(GetGVM().IsSA())
		{
			static Limit lm[] =
			{
				DEFINE_LIMIT(PtrNodeSingle),
				DEFINE_LIMIT(PtrNodeDouble),
				DEFINE_LIMIT(EntryInfoNode),
				DEFINE_LIMIT(Peds),
				DEFINE_LIMIT(Vehicles),
				DEFINE_LIMIT(Buildings),
				DEFINE_LIMIT(Objects),
				DEFINE_LIMIT(Dummys),
				DEFINE_LIMIT(ColModel),
				DEFINE_LIMIT(Task),
				DEFINE_LIMIT(Event),
				DEFINE_LIMIT(PointRoute),
				DEFINE_LIMIT(PatrolRoute),
				DEFINE_LIMIT(NodeRoute),
				DEFINE_LIMIT(TaskAllocator),
				DEFINE_LIMIT(PedIntelligence),
				DEFINE_LIMIT(PedAttractors),
				FINISH_LIMITS()
			};
			return lm;
		}
		return nullptr;
	}

// Instantiate the adjuster on the global scope
} adjuster_Pools_SA;


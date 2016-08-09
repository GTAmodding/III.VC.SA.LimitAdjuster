/*
 *  Dynamic Pools Limit Adjuster
 *  Copyright (C) 2013 ThirteenAG <thirteenag@gmail.com>
 *  Copyright (C) 2014 LINK/2012 <dma_2012@hotmail.com>
 *  Licensed under the MIT License (http://opensource.org/licenses/MIT)
 */
#include <LimitAdjuster.h>
#include "PoolAdjuster.hpp"
#include "PoolAdjusterDynamic.hpp"
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
	Treadables,	    // GTA3 / VC
	AudioScriptObj,	    // GTA3 / VC
	PtrNodeSingle,      // SA
	PtrNodeDouble,      // SA
	EntryInfoNode,
	Peds,
	Vehicles,
	Buildings,
	Objects,
	Dummys,
	ColModel,           // VC / SA

	// SA-only below
	Task,
	Event,
	PointRoute,
	PatrolRoute,
	NodeRoute,
	TaskAllocator,
	PedIntelligence,
	PedAttractors,
	VehicleStructs,

	MAX_POOLS
};



/*
 *  Base for pools adjusters here
 *  Provides an ChangeLimit() and GetUsage() interface by taking the PoolAdjusterBase* (NOT >Pools<AdjusterBase) from the limit user data
 *  Use DEFINE_POOL_LIMIT on inherited objects to define a proper Limit with proper user data
 */
struct PoolsAdjusterBase : public Adjuster
{
    PoolAdjusterBase* GetUserData(int id)
    {
        for(auto* p = GetLimits(); p->name; ++p)
            if(p->id == id) return p->GetUserData<PoolAdjusterBase*>();
        return nullptr;
    }

	// Sets the limit
	void ChangeLimit(int id, const std::string& value)
	{
        if(id == EntryInfoNode || id == Buildings || id == ColModel)
            if(this->IsSA() && GetModuleHandleA("samp")) return;    // Bugzz

		if (id == PtrNode || id == Buildings)
			if (this->IsVC() && (injector::ReadMemory<int>(0x4C0284) > 50000 || injector::ReadMemory<int>(0x4C0309) > 50000)) return;    // Vice Cry 1.8 compat

        if(IsUnlimited(value))
            return GetUserData(id)->MakeUnlimited();
        else
            return GetUserData(id)->ChangeLimit(std::stoul(value));
	}

    bool GetUsage(int id, std::string& usagebuf)
    {
        return GetUserData(id)->GetUsage(usagebuf);
    }
};

#define DEFINE_POOL_LIMIT(limit) DEFINE_LIMIT_U(limit, (PoolAdjusterBase*) &m_##limit)



/*
 *  Pool Adjuster for GTA3
 */
struct PoolsAdjuster3 : public PoolsAdjusterBase
{
    PoolAdjuster<0x4A1783> m_PtrNode;
    PoolAdjuster<0x4A17A0> m_EntryInfoNode;
    PoolAdjuster<0x4A17BD> m_Peds;
    PoolAdjuster<0x4A17D7> m_Vehicles;
    PoolAdjuster<0x4A17F4> m_Buildings;
    PoolAdjuster<0x4A1811> m_Treadables;
    PoolAdjuster<0x4A182E> m_Objects;
    PoolAdjuster<0x4A184B> m_Dummys;
    PoolAdjuster<0x4A1868> m_AudioScriptObj;

	// Limit Names
	const Limit* GetLimits()
	{
		if(GetGVM().IsIII())
		{
			static Limit lm[] =
			{
				DEFINE_POOL_LIMIT(PtrNode),
				DEFINE_POOL_LIMIT(EntryInfoNode),
				DEFINE_POOL_LIMIT(Peds),
				DEFINE_POOL_LIMIT(Vehicles),
				DEFINE_POOL_LIMIT(Buildings),
				DEFINE_POOL_LIMIT(Treadables),
				DEFINE_POOL_LIMIT(Objects),
				DEFINE_POOL_LIMIT(Dummys),
				DEFINE_POOL_LIMIT(AudioScriptObj),
				FINISH_LIMITS()
			};
			return lm;
		}
		return nullptr;
	}

// Instantiate the adjuster on the global scope
} adjuster_Pools_3;


/*
 *  Pool Adjuster for GTA VC
 */
struct PoolsAdjusterVC : public PoolsAdjusterBase
{
    PoolAdjuster<0x4C0288> m_PtrNode;
    PoolAdjuster<0x4C02AA> m_EntryInfoNode;
    PoolAdjuster<0x4C02CC> m_Peds;
    PoolAdjuster<0x4C02EB> m_Vehicles;
    PoolAdjuster<0x4C030D> m_Buildings;
    PoolAdjuster<0x4C034E> m_Objects;
    PoolAdjuster<0x4C0370> m_Dummys;
    PoolAdjuster<0x4C03B4> m_ColModel;
    PoolAdjuster<0x4C032C> m_Treadables;
    PoolAdjuster<0x4C0392> m_AudioScriptObj;

	// Limit Names
	const Limit* GetLimits()
	{
		if(GetGVM().IsVC())
		{
			static Limit lm[] =
			{
				DEFINE_POOL_LIMIT(PtrNode),
				DEFINE_POOL_LIMIT(EntryInfoNode),
				DEFINE_POOL_LIMIT(Peds),
				DEFINE_POOL_LIMIT(Vehicles),
				DEFINE_POOL_LIMIT(Buildings),
				DEFINE_POOL_LIMIT(Objects),
				DEFINE_POOL_LIMIT(Dummys),
				DEFINE_POOL_LIMIT(ColModel),
				DEFINE_POOL_LIMIT(Treadables),
				DEFINE_POOL_LIMIT(AudioScriptObj),
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
struct PoolsAdjusterSA : public PoolsAdjusterBase
{
    // Non-Dynamic Pools, mostly because it'd break manyyyy mods and hard to adapt in the game itself
    PoolAdjuster<0x550FF8> m_Peds;
    PoolAdjuster<0x551289> m_PedIntelligence;   // Since Peds is here, let PedIntelligence too
    PoolAdjuster<0x55102D> m_Vehicles;
    PoolAdjuster<0x551065> m_Buildings;
    PoolAdjuster<0x55109D> m_Objects;
    PoolAdjuster<0x5510D5> m_Dummys;
    
    // Dynamic pools, dynamically resizes to fit more elements
    PoolAdjusterDynamic<0x550F4C, 0x550F26 + 1,  8>     m_PtrNodeSingle;
    PoolAdjusterDynamic<0x550F88, 0x550F59 + 1,  12>    m_PtrNodeDouble;
    PoolAdjusterDynamic<0x550FC0, 0x550F91 + 1,  20>    m_EntryInfoNode;
    PoolAdjusterDynamic<0x55110D, 0x5510DE + 1,  48>    m_ColModel;
    PoolAdjusterDynamic<0x551145, 0x551116 + 1,  128>   m_Task;
    PoolAdjusterDynamic<0x55117D, 0x55114E + 1,  68>    m_Event;
    PoolAdjusterDynamic<0x5511B2, 0x551186 + 1,  100>   m_PointRoute;
    PoolAdjusterDynamic<0x5511E7, 0x5511BB + 1,  420>   m_PatrolRoute;
    PoolAdjusterDynamic<0x55121C, 0x5511F0 + 1,  36>    m_NodeRoute;
    PoolAdjusterDynamic<0x551251, 0x551225 + 1,  32>    m_TaskAllocator;
    PoolAdjusterDynamic<0x5512BF, 0x551292 + 1,  196>   m_PedAttractors;
    PoolAdjusterDynamic<0x5B8FE7, 0x5B8FC3 + 1,  788>   m_VehicleStructs;

	// Limit Names
	const Limit* GetLimits()
	{
		if(GetGVM().IsSA())
		{
			static Limit lm[] =
			{
				DEFINE_POOL_LIMIT(PtrNodeSingle),
				DEFINE_POOL_LIMIT(PtrNodeDouble),
				DEFINE_POOL_LIMIT(EntryInfoNode),
				DEFINE_POOL_LIMIT(Peds),
				DEFINE_POOL_LIMIT(Vehicles),
				DEFINE_POOL_LIMIT(Buildings),
				DEFINE_POOL_LIMIT(Objects),
				DEFINE_POOL_LIMIT(Dummys),
				DEFINE_POOL_LIMIT(ColModel),
				DEFINE_POOL_LIMIT(Task),
				DEFINE_POOL_LIMIT(Event),
				DEFINE_POOL_LIMIT(PointRoute),
				DEFINE_POOL_LIMIT(PatrolRoute),
				DEFINE_POOL_LIMIT(NodeRoute),
				DEFINE_POOL_LIMIT(TaskAllocator),
				DEFINE_POOL_LIMIT(PedIntelligence),
				DEFINE_POOL_LIMIT(PedAttractors),
                DEFINE_POOL_LIMIT(VehicleStructs),
				FINISH_LIMITS()
			};
			return lm;
		}
		return nullptr;
	}

// Instantiate the adjuster on the global scope
} adjuster_Pools_SA;




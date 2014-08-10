/*
 *  Dynamic Pools Limit Adjuster
 *  Copyright (C) 2013 ThirteenAG <thirteenag@gmail.com>
 *  Copyright (C) 2014 LINK/2012 <dma_2012@hotmail.com>
 *  Licensed under the MIT License (http://opensource.org/licenses/MIT)
 */
#include <LimitAdjuster.h>
#include <utility/PoolAdjuster.hpp>
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
    PoolAdjuster<0x4A1783, 0x000000> m_PtrNode;
    PoolAdjuster<0x4A17A0, 0x000000> m_EntryInfoNode;
    PoolAdjuster<0x4A17BD, 0x000000> m_Peds;
    PoolAdjuster<0x4A17D7, 0x000000> m_Vehicles;
    PoolAdjuster<0x4A17F4, 0x000000> m_Buildings;
    PoolAdjuster<0x4A182E, 0x000000> m_Objects;
    PoolAdjuster<0x4A184B, 0x000000> m_Dummys;

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
				DEFINE_POOL_LIMIT(Objects),
				DEFINE_POOL_LIMIT(Dummys),
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
    PoolAdjuster<0x4C0288, 0x000000> m_PtrNode;
    PoolAdjuster<0x4C02AA, 0x000000> m_EntryInfoNode;
    PoolAdjuster<0x4C02CC, 0x000000> m_Peds;
    PoolAdjuster<0x4C02EB, 0x000000> m_Vehicles;
    PoolAdjuster<0x4C030D, 0x000000> m_Buildings;
    PoolAdjuster<0x4C034E, 0x000000> m_Objects;
    PoolAdjuster<0x4C0370, 0x000000> m_Dummys;
    PoolAdjuster<0x4C03B4, 0x000000> m_ColModel;
    PoolAdjuster<0x4C032C, 0x000000> m_Treadables;
    PoolAdjuster<0x4C0392, 0x000000> m_AudioScriptObj;

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
    PoolAdjuster<0x550FC0, 0xB7448C> m_EntryInfoNode;
    PoolAdjuster<0x550FF8, 0xB74490> m_Peds;
    PoolAdjuster<0x55102D, 0xB74494> m_Vehicles;
    PoolAdjuster<0x551065, 0xB74498> m_Buildings;
    PoolAdjuster<0x55109D, 0xB7449C> m_Objects;
    PoolAdjuster<0x5510D5, 0xB744A0> m_Dummys;
    PoolAdjuster<0x55110D, 0xB744A4> m_ColModel;
    PoolAdjuster<0x550F4C, 0xB74484> m_PtrNodeSingle;
    PoolAdjuster<0x550F88, 0xB74488> m_PtrNodeDouble;
    PoolAdjuster<0x551145, 0xB744A8> m_Task;
    PoolAdjuster<0x55117D, 0xB744AC> m_Event;
    PoolAdjuster<0x5511B2, 0xB744B0> m_PointRoute;
    PoolAdjuster<0x5511E7, 0xB744B4> m_PatrolRoute;
    PoolAdjuster<0x55121C, 0xB744B8> m_NodeRoute;
    PoolAdjuster<0x551251, 0xB744BC> m_TaskAllocator;
    PoolAdjuster<0x551289, 0xB744C0> m_PedIntelligence;
    PoolAdjuster<0x5512BF, 0xB744C4> m_PedAttractors;

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
				FINISH_LIMITS()
			};
			return lm;
		}
		return nullptr;
	}

// Instantiate the adjuster on the global scope
} adjuster_Pools_SA;


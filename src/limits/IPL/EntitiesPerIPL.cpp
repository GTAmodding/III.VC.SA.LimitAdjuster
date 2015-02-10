/*
* Entities Per IPL Adjuster
* Copyright (C) 2014 fastman92 <>
* Copyright (C) 2015 LINK/2012 <dma_2012@hotmail.com>
* Licensed under the MIT License (http://opensource.org/licenses/MIT)
*/
#include "LimitAdjuster.h"
#include "utility/StaticArrayAdjuster.hpp"
using namespace injector;

/*
    EntitiesPerIplBase
        Base for the entities per ipl pointers rellocation
        @pCallLoadObjectInstance is the address of the 'call CFileLoader::LoadObjectInstance' used to parse IPLs.
        @pDefaultArray is the address of the default array at the data segment
        @DefaultArraySize is the default size of that array
        @pArrayUsage is the address where the array usage is stored
*/
template<uintptr_t pCallLoadObjectInstance, uintptr_t pDefaultArray, size_t DefaultArraySize, uintptr_t pArrayUsage>
struct EntitiesPerIplBase : public StaticArrayAdjuster<void*>
{
    private:
        size_t& m_dwUsage;                                      // Reference to the array usage (pArrayUsage)
        void*(*LoadObjectInstance)(const char*);
        size_t m_dwMaxUsageEver;

        // Just to access it from the DoGrowFun functor, unique because of the template parameters
        static EntitiesPerIplBase*& Instance()
        {
             static EntitiesPerIplBase* myself;
             return myself;
        }

    public:

        EntitiesPerIplBase() :
            StaticArrayAdjuster(pDefaultArray, DefaultArraySize),
            m_dwUsage(*injector::mem_ptr(pArrayUsage).get<size_t>()), m_dwMaxUsageEver(0)
        {
            this->Instance() = this;
        }

        // Called when the limit is found on the ini
	    virtual void ChangeLimit(int, const std::string& value)
	    {
            if(Adjuster::IsUnlimited(value))
                this->LoadObjectInstance = MakeCALL(pCallLoadObjectInstance, &EntitiesPerIplBase::Grower).get();
            else
                this->RellocArray(std::stoi(value));
	    }

        // Gets the current usage of this visibility array
        virtual bool GetUsage(int, std::string& output)
        {
            if(m_dwMaxUsageEver)    // only works with unlimited
                return Adjuster::GetUsage(output, m_dwMaxUsageEver, this->GetArraySize());
            return false;
        }

        // Grows the array used for unlimited
        static void* Grower(const char* line)
        {
            auto m_dwUsage = Instance()->m_dwUsage;
            auto m_dwNewUse = m_dwUsage + 1;
            Instance()->EnsureHasCapacityFor(m_dwNewUse);
            Instance()->m_dwMaxUsageEver = Instance()->m_dwMaxUsageEver < m_dwNewUse? m_dwNewUse : Instance()->m_dwMaxUsageEver; 
            return Instance()->LoadObjectInstance(line);
        }
};


//
//
//

struct EntitiesPerIplSA : EntitiesPerIplBase<0x5B892A, 0xBCC0E0, 4096, 0xBCC0D8>    // <pCallLoadObjectInstance, pDefault, dwMax, dwUsage>
{
	const char* GetLimitName()
    {
        return IsSA()? "EntitiesPerIpl" : nullptr;
    }
	
    EntitiesPerIplSA()
    {
        this->AddPointer(0x5B51F0 + 3);
        this->AddPointer(0x5B51FF + 3);
        this->AddPointer(0x5B524B + 1);
        this->AddPointer(0x5B5258 + 3);
        this->AddPointer(0x5B531C + 1);
        this->AddPointer(0x5B5340 + 3);
        this->AddPointer(0x5B8938 + 3);
        this->AddPointer(0x5B8A50 + 1);
        this->AddPointer(0x5B8A50 + 1);
        this->AddPointer(0x5B8A6F + 3);
    }

} EntitiesPerIplSA;

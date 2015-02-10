/*
* Entity IPLs Adjuster
* Copyright (C) 2015 LINK/2012 <dma_2012@hotmail.com>
* Licensed under the MIT License (http://opensource.org/licenses/MIT)
*/
#include "LimitAdjuster.h"
#include "utility/StaticArrayAdjuster.hpp"
using namespace injector;

/*
    EntityIplBase
        Base for the entities per ipl pointers rellocation
        @pCallLoadObjectInstance is the address of the 'call CFileLoader::LoadObjectInstance' used to parse IPLs.
        @pDefaultArray is the address of the default array at the data segment
        @DefaultArraySize is the default size of that array
        @pArrayUsage is the address where the array usage is stored
*/
template<uintptr_t pCallNewIplEntityIndexArray, uintptr_t pDefaultArray, size_t DefaultArraySize, uintptr_t pArrayUsage>
struct EntityIplBase : public StaticArrayAdjuster<void**>
{
    private:
        size_t& m_dwUsage;                                      // Reference to the array usage (pArrayUsage)
        int(*NewIplEntityIndexArray)(int);

        // Just to access it from the DoGrowFun functor, unique because of the template parameters
        static EntityIplBase*& Instance()
        {
             static EntityIplBase* myself;
             return myself;
        }

    public:

        EntityIplBase() :
            StaticArrayAdjuster(pDefaultArray, DefaultArraySize),
            m_dwUsage(*injector::mem_ptr(pArrayUsage).get<size_t>())
        {
            this->Instance() = this;
        }

        // Called when the limit is found on the ini
	    virtual void ChangeLimit(int, const std::string& value)
	    {
            if(Adjuster::IsUnlimited(value))
                this->NewIplEntityIndexArray = MakeCALL(pCallNewIplEntityIndexArray, &EntityIplBase::Grower).get();
            else
                this->RellocArray(std::stoi(value));
	    }

        // Gets the current usage of this visibility array
        virtual bool GetUsage(int, std::string& output)
        {
            return Adjuster::GetUsage(output, m_dwUsage, this->GetArraySize());
        }

        // Grows the array used for unlimited
        static int Grower(int count)
        {
            auto m_dwUsage = Instance()->m_dwUsage;
            Instance()->EnsureHasCapacityFor(m_dwUsage + 1);
            return Instance()->NewIplEntityIndexArray(count);
        }
};


//
//
//

struct EntityIplSA : EntityIplBase<0x5B8A36, 0x8E3F08, 40, 0x8E3F00>    // <pCallNewEntityIndexArray, pDefault, dwMax, dwUsage>
{
	const char* GetLimitName()
    {
        return IsSA()? "EntityIpl" : nullptr;
    }
	
    EntityIplSA()
    {
        this->AddPointer(0x40479A);
        this->AddPointer(0x4047B7);
        this->AddPointer(0x405C3D);
        this->AddPointer(0x406033);
        this->AddPointer(0x40619B);
    }

} EntityIplSA;

/*
* Dynamic CStore<>
* Copyright (c) 2014 LINK/2012 <dma_2012@hotmail.com>
* Licensed under the MIT License (http://opensource.org/licenses/MIT)
*/
#pragma once

#pragma warning(push)
#pragma warning(disable : 4200) // MSVC warning about CDynamicStore<T>::Store having a flexible-sized array
                                // Well, it's on C99 (not sure if applied on C++11 by the C99 compatibility rule)
                                // and supported by MSVC, GCC and Clang as extensions for C++


/*
    CDynamicStore
        Dynamic version of CStore<>
        To get the backward compatible interface call GetStore()
        @T is the type of the element to store
*/
template<class T>
class CDynamicStore
{
    public:
        // Backward compatible interface
        struct Store
        {
            size_t m_nCount;
            T      m_Objects[];
        };

        // Constructs the store with a default initial size
        CDynamicStore(uintptr_t default_addr, size_t default_capacity) :
            m_nCapacity(default_capacity), m_Store((Store*)default_addr), m_bOwnsAllocation(false)
        {
            static_assert(sizeof(Store) == 4, "Wrong sizeof");
            //this->MakeSureHasBlocks(default_capacity);
        }

        //
        ~CDynamicStore()
        {
            if(m_bOwnsAllocation)
            {
                for(size_t i = 0; i < m_nCapacity; ++i)
                    m_Store->m_Objects[i].~T();
                operator delete(m_Store);
            }
        }

        // Ensures the store has capacity for another @count objects
        // Returns true if capacity changed
        bool EnsureHasCapacityForAnother(size_t n = 1)
        {
            if(this->m_nCapacity < (m_Store->m_nCount + n))
            {
                this->Grow();
                return true;
            }
            return false;
        }

        // Sets the total capacity for this store not considering the old CStore
        // Necessary to clean up any information about the original CStore because possibily
        // it hasn't initialized yet so we cannot std::move it
        void ForceCapacity(size_t n)
        {
            if(!this->m_bOwnsAllocation)
            {
                this->m_nCapacity = 0;
                this->m_Store     = nullptr;
            }
            this->MakeSureHasBlocks(n);
        }

        // Gets the backward compatible CStore<> interface
        Store* GetStore()
        {
            return this->m_Store;
        }

        size_t GetCapacity()
        {
            return this->m_nCapacity;
        }

        size_t GetUsage()
        {
            return this->m_Store->m_nCount;
        }

        bool IsUsingGameAllocation()
        {
            return !m_bOwnsAllocation;
        }

    private:
        size_t m_nCapacity;     // Current store capacity
        Store* m_Store;         // Store interface
        bool m_bOwnsAllocation; // Is me that owns the m_Store allocation or the game (.data seg)?

        // Make sure has @size capacity
        void MakeSureHasBlocks(size_t size)
        {
            if(this->m_nCapacity < size)
            {
                auto count = m_Store? m_Store->m_nCount : 0;

                // Allocate new memory block and move previous elements into there
                Store* pNewStore = (Store*) operator new(sizeof(Store) + (size * sizeof(T)));

                // Move previous existing objects
                for(size_t i = 0; i < m_nCapacity; ++i)
                {
                    new (&pNewStore->m_Objects[i]) T(std::move(m_Store->m_Objects[i]));
                    if(m_bOwnsAllocation) m_Store->m_Objects[i].~T();
                }

                // Construct newer objects
                for(size_t i = m_nCapacity; i < size; ++i)  
                    new (&pNewStore->m_Objects[i]) T();

                // Free previous piece of memory block
                if(m_bOwnsAllocation) operator delete(m_Store);

                // Setup new information
                this->m_Store           = pNewStore;
                this->m_Store->m_nCount = count;
                this->m_nCapacity       = size;
                this->m_bOwnsAllocation = true;
            }
        }


        // Grows the storer, so it can store more objects
        bool Grow()
        {
            try 
            {
                MakeSureHasBlocks(m_nCapacity? m_nCapacity * 2 : 16);
                return true;
            }
            catch(const std::bad_alloc&)
            {
                return false;
            }
        }
};



#pragma warning(pop)

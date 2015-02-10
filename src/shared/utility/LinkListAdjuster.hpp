/*
* Utility for adjusting CLinkList s
* Copyright (c) 2014 LINK/2012 <dma_2012@hotmail.com>
* Licensed under the MIT License (http://opensource.org/licenses/MIT)
*/
#pragma once
#include "LimitAdjuster.h"
#include "structs/CLinkList.h"
#include <memory>

/*
	LinkListAdjuster
		CLinkList Dynamic Adjuster
        @T is type TYPE associated with the CLinkList, as in CLinkList<T>
*/
template<class T>
class LinkListAdjuster : public SimpleAdjuster
{
    protected:
        static_assert(sizeof(T) > 1, "sizeof(T) is 1, is that a mistake?"); // common to do that in aligned_storage

        typedef typename CLinkList<T>::Link Link_t;
        typedef Link_t* (__fastcall *pInsertSorted_t)(CLinkList<T>* self, int, T* item);

        // The CLinkList associated with this adjuster, used in GetUsage
        CLinkList<T>* const m_pAttached;

    public:

        // Basic GetUsage() for this adjuster
        virtual bool GetUsage(int, std::string& output)
        {
            auto nUsed = m_pAttached->GetNumUsed();
            auto nFree = m_pAttached->GetNumFree();
            return Adjuster::GetUsage(output, nUsed, nUsed + nFree);
        }

    protected:
        
        // Constructor needs the linked list associated with this adjuster
        LinkListAdjuster(CLinkList<T>* list) : m_pAttached(list)
        {}

        // Adds the pointer to a 'call CLinkList<T>::InsertSorted' to the list of calls that must be checked to
        // grow the list if it returns null
        void AddInsertSortedPatch(uintptr_t addr)
        {
            pInsertSorted() = injector::MakeCALL(addr, &InsertSortedHook).get();
        }

        // Grows the linked list, so it can store more elements
        static Link_t* GrowLinkedList(CLinkList<T>& list)
        {
            // Allocates more links by 100% in relation to the current size of the list
            auto grow_by = int(list.GetNumFree() + list.GetNumUsed());
            Link_t *last = nullptr, *p = AllocateLinks(grow_by);

            // Adds more free links to the linked list
            for(int i = 0; i < grow_by; ++i)
            {
                last = &p[i];
                std::memset(last, 0, sizeof(Link_t));
                list.AddFree(*last);
            }
            return last;
        }

    private:

        // Stores a static pointer to the CLinkList<T>::InsertSorted function
        static pInsertSorted_t& pInsertSorted()
        {
            static pInsertSorted_t ptr;
            return ptr;
        }

        // This function replaces another CLinkList<T>::InsertSorted, the difference is that it'll never fail
        // it will always return a valid element because it grows by demand
        static Link_t* __fastcall InsertSortedHook(CLinkList<T>* self, int, T* item)
        {
            if(Link_t* result = pInsertSorted()(self, 0, item))
                return result;

            GrowLinkedList(*self);
            return pInsertSorted()(self, 0, item);
        }

        // Allocates @q more links and returns the array pointer
        static Link_t* AllocateLinks(int q)
        {
            static std::vector<std::unique_ptr<Link_t[]>> growz;
            growz.push_back(std::unique_ptr<typename CLinkList<T>::Link[]>( new Link_t[q] ));
            return growz.back().get();
        }
};

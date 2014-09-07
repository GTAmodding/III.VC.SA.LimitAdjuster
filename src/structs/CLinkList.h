/*
 * Copyright (C) 2014 LINK/2012 <dma_2012@hotmail.com>
 * Licensed under zlib license (http://opensource.org/licenses/Zlib)
 * 
 */
#pragma once

template<class T>
class CLinkList
{
    public:
        struct Link
        {
            T     m_Object;
            Link* m_pPrev;
            Link* m_pNext;
        };

        Link m_UsedListHead, m_UsedListTail;
        Link m_FreeListHead, m_FreeListTail;
        T* m_pArray;

        Link* Insert(const T& obj)
        {
            auto link = this->m_FreeListHead.m_pNext;
            if(link != &this->m_FreeListTail)
            {
                link->m_Object = *obj;
                link->m_pNext->m_pPrev = link->m_pPrev;
                link->m_pPrev->m_pNext = link->m_pNext;
                link->m_pNext = this->m_UsedListHead.m_pNext;
                this->m_UsedListHead.m_pNext->m_pPrev = link;
                link->m_pPrev = this->m_UsedListHead;
                this->m_UsedListHead.m_pNext = link;
                return link;
            }
            return nullptr;
        }


        void AddFree(Link& link)
        {
            auto& head = m_FreeListHead;
            link.m_pNext = head.m_pNext;
            head.m_pNext->m_pPrev = &link;
            link.m_pPrev = &head;
            head.m_pNext = &link;
        }


        uint32_t GetNumFree()
        {
            return GetLinkUsage(m_FreeListHead, m_FreeListTail);
        }

        uint32_t GetNumUsed()
        {
            return GetLinkUsage(m_UsedListHead, m_UsedListTail);
        }

    private:
        uint32_t GetLinkUsage(const Link& head, const Link& tail)
        {
            uint32_t count = 0;
            for(auto link = head.m_pNext; link != &tail; link = link->m_pNext) ++count;
            return count;
        }
};

static_assert(sizeof(CLinkList<int>) == 0x34, "...");

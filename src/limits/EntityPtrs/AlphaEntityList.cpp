/*
* Alpha Entity List Adjuster
* Copyright (c) 2014 ThirteenAG <thirteenag@gmail.com>
* Copyright (c) 2014 Silent <zdanio95@gmail.com>
* Copyright (c) 2014 LINK/2012 <dma_2012@hotmail.com>
* Copyright (c) 2026 isonisx <dblazheski96@gmail.com>
* Licensed under the MIT License (http://opensource.org/licenses/MIT)
*/
#include "LimitAdjuster.h"
#include "utility/LinkListAdjuster.hpp"
#include "utility/dummy_object.hpp"

typedef dummy_object<0x8> AlphaObjectInfoVC;
typedef dummy_object<0xC> AlphaObjectInfoSA;


class AlphaEntityListIII : public SimpleAdjuster
{
public:
	const char* GetLimitName() { return GetGVM().IsIII() ? "AlphaEntityList" : nullptr; }
	void ChangeLimit(int, const std::string& value)
	{
		injector::WriteMemory(0x527E6B + 1, std::stoi(value), true);
		if (*(DWORD*)0x943098 == 0x4CBEBC20)
		{
			auto CVisibilityPlugins__Shutdown = (void (__cdecl *)()) 0x527EA0;
			auto CVisibilityPlugins__Initialise = (void (__cdecl *)()) 0x527E50;

			CVisibilityPlugins__Shutdown();
			CVisibilityPlugins__Initialise();
		}
	}

} AlphaEntityListIII;

class AlphaEntityListVC : public SimpleAdjuster
{
public:
	const char* GetLimitName() { return GetGVM().IsVC() ? "AlphaEntityList" : nullptr; }
	void ChangeLimit(int, const std::string& value)
		{
			injector::WriteMemory(0x5828DB+1, std::stoi(value), true);

			if (*(DWORD*)0x97F30C == 0x4CBEBC20)
			{
				auto CVisibilityPlugins__Shutdown = (void (__cdecl *)()) 0x582870;
				auto CVisibilityPlugins__Initialise = (void (__cdecl *)()) 0x5828A0;

				CVisibilityPlugins__Shutdown();
				CVisibilityPlugins__Initialise();
			}
		}

} AlphaEntityListVC;


class AlphaEntityListSA : public LinkListAdjuster<AlphaObjectInfoSA>
{
    public:
        AlphaEntityListSA()
            : LinkListAdjuster(injector::lazy_ptr<0xC88120>().get())
        {}

        const char* GetLimitName()
        {
            return IsSA() ? "AlphaEntityList" : nullptr;
        }

        void ChangeLimit(int, const std::string& value)
	    {
            if(Adjuster::IsUnlimited(value))
            {
                AddInsertSortedPatch(0x733DF5); // m_alphaEntityList
                AddInsertSortedPatch(0x7345F2); // m_alphaEntityList
                AddInsertSortedPatch(0x733D35); // m_alphaList
                AddInsertSortedPatch(0x733D75); // m_alphaBoatAtomicList
                AddInsertSortedPatch(0x733DB5); // m_alphaUnderwaterEntityList
                AddInsertSortedPatch(0x7345D9); // m_alphaUnderwaterEntityList
                AddInsertSortedPatch(0x733E35); // m_alphaReallyDrawLastList
                AddInsertSortedPatch(0x733E75); // m_alphaReallyDrawLastList
                AddInsertSortedPatch(0x7345B6); // m_alphaReallyDrawLastList
            }
            else
            {
                // initializer is inlined in in GTA SA, needs to multiply by sizeof(CLinkList<AlphaObjectInfo>::Link)
                auto size = std::stoi(value) * 20;
	            injector::WriteMemory(0x733B05, size, true); // m_alphaEntityList
	            injector::WriteMemory(0x733B55, size, true); // m_alphaEntityList
	            injector::WriteMemory(0x733A22, size, true); // m_alphaList
	            injector::WriteMemory(0x733A5E, size, true); // m_alphaList
	            injector::WriteMemory(0x733A87, size, true); // m_alphaBoatAtomicList
	            injector::WriteMemory(0x733AD7, size, true); // m_alphaBoatAtomicList
	            injector::WriteMemory(0x733B85, size, true); // m_alphaUnderwaterEntityList
	            injector::WriteMemory(0x733BD5, size, true); // m_alphaUnderwaterEntityList
	            injector::WriteMemory(0x733C05, size, true); // m_alphaReallyDrawLastList
	            injector::WriteMemory(0x733C55, size, true); // m_alphaReallyDrawLastList
            }
	    }

} AlphaEntityListSA;

/*
* (In)Visibible Entities List Adjuster
* Copyright (c) 2014 ThirteenAG <thirteenag@gmail.com>
* Licensed under the MIT License (http://opensource.org/licenses/MIT)
*/
#include "LimitAdjuster.h"

static std::vector<void*> aInVisibleEntityPtrs;
static std::vector<void*> aVisibleEntityPtrs;

class InVisibleEntityPtrsIII : public SimpleAdjuster
{
public:
	const char* GetLimitName() { return GetGVM().IsIII() ? "InVisibleEntityPtrs" : nullptr; }

	void ChangeLimit(int, const std::string& value)
	{
		aInVisibleEntityPtrs.resize(std::stoi(value));

		injector::WriteMemory(0x4A7850 + 0x3, &aInVisibleEntityPtrs[0], true);
		injector::WriteMemory(0x4A78D3 + 0x3, &aInVisibleEntityPtrs[0], true);
		injector::WriteMemory(0x4A7950 + 0x3, &aInVisibleEntityPtrs[0], true);
		injector::WriteMemory(0x4A9300 + 0x33 + 0x3, &aInVisibleEntityPtrs[0], true);
		injector::WriteMemory(0x4A9BB0 + 0xB8 + 0x3, &aInVisibleEntityPtrs[0], true);
		injector::WriteMemory(0x4A9E30 + 0xC5 + 0x3, &aInVisibleEntityPtrs[0], true);
		injector::WriteMemory(0x4AA0A0 + 0x6C + 0x3, &aInVisibleEntityPtrs[0], true);
	}

} InVisibleEntityPtrsIII;

class VisibleEntityPtrsIII : public SimpleAdjuster
{
public:
	const char* GetLimitName() { return GetGVM().IsIII() ? "VisibleEntityPtrs" : nullptr; }

		void ChangeLimit(int, const std::string& value)
		{
			aVisibleEntityPtrs.resize(std::stoi(value));

			injector::WriteMemory(0x4A7870 + 0x3, &aVisibleEntityPtrs[0], true);
			injector::WriteMemory(0x4A9BB0 + 0x1C7 + 0x3, &aVisibleEntityPtrs[0], true);
			injector::WriteMemory(0x4A9E30 + 0x1D7 + 0x3, &aVisibleEntityPtrs[0], true);
			injector::WriteMemory(0x4AA0A0 + 0xE3 + 0x3, &aVisibleEntityPtrs[0], true);
		}
} VisibleEntityPtrsIII;

class InVisibleEntityPtrsVC : public SimpleAdjuster
{
public:
	const char* GetLimitName() { return GetGVM().IsVC() ? "InVisibleEntityPtrs" : nullptr; }

	void ChangeLimit(int, const std::string& value)
		{
		aInVisibleEntityPtrs.resize(std::stoi(value));

		injector::WriteMemory(0x4C7560 + 0xBE + 0x3, &aInVisibleEntityPtrs[0], true);
		injector::WriteMemory(0x4C7740 + 0xB8 + 0x3, &aInVisibleEntityPtrs[0], true);
		injector::WriteMemory(0x4C8540 + 0x78 + 0x3, &aInVisibleEntityPtrs[0], true);
		injector::WriteMemory(0x4C9F80 + 0x3, &aInVisibleEntityPtrs[0], true);
		injector::WriteMemory(0x4CA1B8 + 0x3, &aInVisibleEntityPtrs[0], true);
		injector::WriteMemory(0x4CA200 + 0x3, &aInVisibleEntityPtrs[0], true);
		}
} InVisibleEntityPtrsVC;

class VisibleEntityPtrsVC : public SimpleAdjuster
{
public:
	const char* GetLimitName() { return GetGVM().IsVC() ? "VisibleEntityPtrs" : nullptr; }
	void ChangeLimit(int, const std::string& value)
	{
		aVisibleEntityPtrs.resize(std::stoi(value));

		injector::WriteMemory(0x4C7560 + 0x185 + 0x3, &aVisibleEntityPtrs[0], true);
		injector::WriteMemory(0x4C7740 + 0x17F + 0x3, &aVisibleEntityPtrs[0], true);
		injector::WriteMemory(0x4CA220 + 0x3, &aVisibleEntityPtrs[0], true);
	}
} VisibleEntityPtrsVC;

class InVisibleEntityPtrsSA : public SimpleAdjuster
{
public:
	const char* GetLimitName() { return GetGVM().IsSA() ? "InVisibleEntityPtrs" : nullptr; }
	
	void ChangeLimit(int, const std::string& value)
	{
		aInVisibleEntityPtrs.resize(std::stoi(value));

		injector::WriteMemory(0x5534B0 + 0x42 + 0x3, &aInVisibleEntityPtrs[0], true);
		injector::WriteMemory(0x553920 + 0x3, &aInVisibleEntityPtrs[0], true);
		injector::WriteMemory(0x553CB0 + 0x3, &aInVisibleEntityPtrs[0], true);
	}

    // TODO GetUsage

} InVisibleEntityPtrsSA;

class VisibleEntityPtrsSA : public SimpleAdjuster
{
public:
	const char* GetLimitName() { return GetGVM().IsSA() ? "VisibleEntityPtrs" : nullptr; }
	
	void ChangeLimit(int, const std::string& value)
	{
		aVisibleEntityPtrs.resize(std::stoi(value));

		injector::WriteMemory(0x5534B0 + 0x76 + 0x3, &aVisibleEntityPtrs[0], true);
		injector::WriteMemory(0x553941 + 0x3, &aVisibleEntityPtrs[0], true);
		injector::WriteMemory(0x553A50 + 0x3, &aVisibleEntityPtrs[0], true);
		injector::WriteMemory(0x553B00 + 0x3, &aVisibleEntityPtrs[0], true);
	}

    // TODO GetUsage

} VisibleEntityPtrsSA;

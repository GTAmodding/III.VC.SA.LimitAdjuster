/*
* TimeModelInfo Adjuster for GTA VC
* Copyright (c) 2014 ThirteenAG <thirteenag@gmail.com>
* Licensed under the MIT License (http://opensource.org/licenses/MIT)
*/
#include "LimitAdjuster.h"
std::vector<char> stTimeModelInfo;
DWORD jmpAddr;
DWORD IIIvalue;

void __declspec(naked) asm_50BC78()
{
	_asm
	{
		add     esp, 0Ch
		push    IIIvalue
		push    58h
		mov jmpAddr, 0x50BC7F
		jmp jmpAddr
	}
}

void __declspec(naked) asm_50B9C0()
{
	_asm
	{
		mov eax, IIIvalue
		cmp     ds : [0x94076C], eax
		mov jmpAddr, 0x50B9C7
		jmp jmpAddr
	}
}

class TimeModelInfoIII : public SimpleAdjuster
{
public:
	const char* GetLimitName() { return GetGVM().IsIII() ? "TimeModelInfo" : nullptr; }
	void ChangeLimit(int, const std::string& value)
	{
		IIIvalue = std::stoi(value);
		stTimeModelInfo.resize(IIIvalue * 88 * 4 * 10);
		injector::MakeJMP(0x50BC78, asm_50BC78, true);  //injector::WriteMemory(0x50BC7B + 1, std::stoi(value), true);

		injector::MakeJMP(0x50B9C0, asm_50B9C0, true);
		/*injector::WriteMemory(0x55FEFF + 0x1, std::stoi(value), true);
		injector::WriteMemory(0x55F6E0 + 0x6, std::stoi(value), true);
		injector::WriteMemory(0x55F6F2 + 0x1, std::stoi(value), true);
		injector::WriteMemory(0x560246 + 0x1, std::stoi(value), true);*/

		injector::WriteMemory(0x50BC89 + 0x1, &stTimeModelInfo[0] + 4, true);

		injector::WriteMemory(0x50B310 + 0xE6 + 0x2, &stTimeModelInfo[0], true);
		injector::WriteMemory(0x50B5B0 + 0x8B + 0x2, &stTimeModelInfo[0], true);
		injector::WriteMemory(0x50B5B0 + 0x94 + 0x1, &stTimeModelInfo[0], true);
		injector::WriteMemory(0x50B5B0 + 0xB1 + 0x2, &stTimeModelInfo[0], true);
		injector::WriteMemory(0x50B5B0 + 0x23E + 0x2, &stTimeModelInfo[0], true);
		//injector::WriteMemory(0x50B9C0 + 0x2, &stTimeModelInfo[0], true); //inject @asm_50B9C0
		injector::WriteMemory(0x50B9DD + 0x2, &stTimeModelInfo[0], true);
		injector::WriteMemory(0x50B9C0 + 0x23 + 0x2, &stTimeModelInfo[0], true);
		injector::WriteMemory(0x50B9C0 + 0x2C + 0x2, &stTimeModelInfo[0], true);
		injector::WriteMemory(0x50BC00 + 0x96 + 0x2, &stTimeModelInfo[0], true);
		injector::WriteMemory(0x50BC00 + 0xAA + 0x1, &stTimeModelInfo[0], true);

		if (*(DWORD*)0x943098 == 0x4CBEBC20)
		{
			auto CModelInfo_CModelInfo = (void(__cdecl *)()) 0x50BC00;
			CModelInfo_CModelInfo();
		}
	}

} TimeModelInfoIII;

class TimeModelInfoVC : public SimpleAdjuster
{
public:
	const char* GetLimitName() { return GetGVM().IsVC() ? "TimeModelInfo" : nullptr; }
	void ChangeLimit(int, const std::string& value)
	{
		stTimeModelInfo.resize(std::stoi(value) * 80 * 4);
		injector::WriteMemory(0x5828DB + 0x1, std::stoi(value), true);

		injector::WriteMemory(0x55FEFF + 0x1, std::stoi(value), true);
		injector::WriteMemory(0x55F6E0 + 0x6, std::stoi(value), true);
		injector::WriteMemory(0x55F6F2 + 0x1, std::stoi(value), true);
		injector::WriteMemory(0x560246 + 0x1, std::stoi(value), true);

		injector::WriteMemory(0x55FF10 + 0x1, &stTimeModelInfo[0] + 4, true);

		injector::WriteMemory(0x55F6E0 + 0x2, &stTimeModelInfo[0], true);
		injector::WriteMemory(0x55F703 + 0x2, &stTimeModelInfo[0], true);
		injector::WriteMemory(0x55F6E0 + 0x29 + 0x2, &stTimeModelInfo[0], true);
		injector::WriteMemory(0x55F6E0 + 0x32 + 0x2, &stTimeModelInfo[0], true);
		injector::WriteMemory(0x55F820 + 0x4C + 0x2, &stTimeModelInfo[0], true);
		injector::WriteMemory(0x55F820 + 0x55 + 0x1, &stTimeModelInfo[0], true);
		injector::WriteMemory(0x55F820 + 0x71 + 0x2, &stTimeModelInfo[0], true);
		injector::WriteMemory(0x55F820 + 0x1AC + 0x2, &stTimeModelInfo[0], true);
		injector::WriteMemory(0x55FA40 + 0x154 + 0x2, &stTimeModelInfo[0], true);
		injector::WriteMemory(0x55FEC0 + 0x5D + 0x2, &stTimeModelInfo[0], true);
		injector::WriteMemory(0x55FEC0 + 0x71 + 0x1, &stTimeModelInfo[0], true);

		if (*(DWORD*)0x97F30C == 0x4CBEBC20)
		{
			auto CModelInfo_CModelInfo = (void(__cdecl *)()) 0x55FEC0;
			CModelInfo_CModelInfo();
		}
	}

} TimeModelInfoVC;

class TimeModelInfoSA : public SimpleAdjuster
{
public:
	const char* GetLimitName() { return GetGVM().IsSA() ? "TimeModelInfo" : nullptr; }

	void ChangeLimit(int, const std::string& value)
	{
		stTimeModelInfo.resize(std::stoi(value) * 36 * 4 * 10);

		injector::WriteMemory(0x4C5DDB + 0x1, std::stoi(value), true);

		injector::WriteMemory(0x4C66BA + 0x3, &stTimeModelInfo[0] + 4, true);
		injector::WriteMemory(0x4C6463 + 0x1, &stTimeModelInfo[0] + 4, true);

		injector::WriteMemory(0x4C6458 + 0x1, &stTimeModelInfo[0], true);
		injector::WriteMemory(0x4C63E0 + 0x8F + 0x1, &stTimeModelInfo[0], true);
		injector::WriteMemory(0x4C63E0 + 0x20A + 0x2, &stTimeModelInfo[0], true);
		injector::WriteMemory(0x4C66B0 + 0x1, &stTimeModelInfo[0], true);
		injector::WriteMemory(0x4C66B0 + 0x11 + 0x1, &stTimeModelInfo[0], true);
		injector::WriteMemory(0x4C6810 + 0x29 + 0x2, &stTimeModelInfo[0], true);
		injector::WriteMemory(0x84BC50 + 0x1, &stTimeModelInfo[0], true);
		injector::WriteMemory(0x856260 + 0x1, &stTimeModelInfo[0], true);
	}
} TimeModelInfoSA;
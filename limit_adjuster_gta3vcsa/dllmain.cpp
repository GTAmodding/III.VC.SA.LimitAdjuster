#include "stdafx.h"
#include "CPatch.h"
#include <stdio.h>
#include <string>
#include <Windows.h>
#include "CIniReader\IniReader.h"
using namespace std;

#define gGameVersion (*(unsigned int *)0x601048)
#define GTA_3_1_0     0x3A83126F
#define GTA_3_1_1     0x3F8CCCCD
#define GTA_3_STEAM     0x47BDA5
#define GTA_VC_1_0     0x53FF1B8B
#define GTA_VC_STEAM     0xF04F883
#define GTA_VC_1_1     0x783EE8
#define GTA_SA_1_0     0xCE8B168B
#define GTA_SA_1_1     0x0
#define GTA_SA_STEAM     0x1




int Thread()
{

	CIniReader iniReader("");
	
		switch(gGameVersion)
		{
			case GTA_3_1_0:
				CPatch::SetInt(0x4A177E+0x1, iniReader.ReadInteger("GTA3LIMITS", "PtrNode", 30000));
				CPatch::SetInt(0x4A179B+0x1, iniReader.ReadInteger("GTA3LIMITS", "EntryInfoNode", 5400));
				CPatch::SetInt(0x4A17B8+0x1, iniReader.ReadInteger("GTA3LIMITS", "Peds", 140));
				CPatch::SetChar(0x4A17D5+0x1, iniReader.ReadInteger("GTA3LIMITS", "Vehicles", 110));
				CPatch::SetInt(0x4A17EF+0x1, iniReader.ReadInteger("GTA3LIMITS", "Buildings", 5500));
				CPatch::SetInt(0x4A1829+0x1, iniReader.ReadInteger("GTA3LIMITS", "Objects", 450));
				CPatch::SetInt(0x4A1846+0x1, iniReader.ReadInteger("GTA3LIMITS", "Dummys", 2802));
			break;

			case GTA_3_1_1:
				return 0;
			break;

			case GTA_3_STEAM:
				return 0;
			break;

			case GTA_VC_1_0:
				CPatch::SetInt(0x4C0283+0x1, iniReader.ReadInteger("VCLIMITS", "PtrNode", 50000));
				CPatch::SetInt(0x4C02A5+0x1, iniReader.ReadInteger("VCLIMITS", "EntryInfoNode", 3200));
				CPatch::SetInt(0x4C02C7+0x1, iniReader.ReadInteger("VCLIMITS", "Peds", 140));
				CPatch::SetChar(0x4C02E9+0x1, iniReader.ReadInteger("VCLIMITS", "Vehicles", 110));
				CPatch::SetInt(0x4C0308+0x1, iniReader.ReadInteger("VCLIMITS", "Buildings", 7000));
				CPatch::SetChar(0x4C032A+0x1, iniReader.ReadInteger("VCLIMITS", "Treadables", 1));
				CPatch::SetInt(0x4C0349+0x1, iniReader.ReadInteger("VCLIMITS", "Objects", 460));
				CPatch::SetInt(0x4C036B+0x1, iniReader.ReadInteger("VCLIMITS", "Dummys", 2340));
				CPatch::SetInt(0x4C038D+0x1, iniReader.ReadInteger("VCLIMITS", "AudioScriptObj", 192));
				CPatch::SetInt(0x4C03AF+0x1, iniReader.ReadInteger("VCLIMITS", "ColModel", 4400));
			break;

			case GTA_VC_1_1:
				return 0;
			break;

			case GTA_VC_STEAM:
				return 0;
			break;	

			case GTA_SA_1_0:
				CPatch::SetInt(0x550F45+0x1, iniReader.ReadInteger("SALIMITS", "PtrNodeSingle", 70000));
				CPatch::SetInt(0x550F81+0x1, iniReader.ReadInteger("SALIMITS", "PtrNodeDouble", 3200));
				CPatch::SetInt(0x550FB9+0x1, iniReader.ReadInteger("SALIMITS", "EntryInfoNode", 500));
				CPatch::SetInt(0x550FF1+0x1, iniReader.ReadInteger("SALIMITS", "Peds", 140));
				CPatch::SetChar(0x551029+0x1, iniReader.ReadInteger("SALIMITS", "Vehicles", 110));
				CPatch::SetInt(0x55105E+0x1, iniReader.ReadInteger("SALIMITS", "Buildings", 13000));
				CPatch::SetInt(0x551096+0x1, iniReader.ReadInteger("SALIMITS", "Objects", 350));
				CPatch::SetInt(0x5510CE+0x1, iniReader.ReadInteger("SALIMITS", "Dummys", 2500));
				CPatch::SetInt(0x551106+0x1, iniReader.ReadInteger("SALIMITS", "ColModel", 10150));
				CPatch::SetInt(0x55113E+0x1, iniReader.ReadInteger("SALIMITS", "Task", 500));
				CPatch::SetInt(0x551176+0x1, iniReader.ReadInteger("SALIMITS", "Event", 200));
				CPatch::SetChar(0x5511AE+0x1, iniReader.ReadInteger("SALIMITS", "PointRoute", 64));
				CPatch::SetChar(0x5511E3+0x1, iniReader.ReadInteger("SALIMITS", "PatrolRoute", 32));
				CPatch::SetChar(0x551218+0x1, iniReader.ReadInteger("SALIMITS", "NodeRoute", 64));
				CPatch::SetChar(0x55124D+0x1, iniReader.ReadInteger("SALIMITS", "TaskAllocator", 16));
				CPatch::SetInt(0x551282+0x1, iniReader.ReadInteger("SALIMITS", "PedIntelligence", 140));
				CPatch::SetChar(0x5512BB+0x1, iniReader.ReadInteger("SALIMITS", "PedAttractors", 64));
				CPatch::SetInt(0x54F3A0 + 0x1, iniReader.ReadInteger("SALIMITS", "StaticMatrices", 900));
			break;	

			case GTA_SA_1_1:
				return 0;
			break;

			case GTA_SA_STEAM:
				return 0;
			break;
		}
return 1;
}



BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if(reason==DLL_PROCESS_ATTACH)
    {
		Thread();
    }
    return TRUE;
}
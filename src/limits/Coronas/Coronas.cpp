/*
* Copyright (c) 2014 Wesser
* Copyright (c) 2014 ThirteenAG
* Copyright (c) 2014 Silent <zdanio95@gmail.com>
* Licensed under the MIT License (http://opensource.org/licenses/MIT)
*/
#include "LimitAdjuster.h"
#include <vector>
#include <map>
#include <math.h>
#include "yacl/General.h"
#include "yacl/Coronas.h"
#include "yacl/Camera.h"
#include "CPatch.h"

int NUM_CORONAS;

#define WRAPPER __declspec(naked)
#define EAXJMP(a) { _asm mov eax, a _asm jmp eax }

std::map<unsigned int, CCoronasLinkedListNode*>	CCoronas::UsedMap;
CCoronasLinkedListNode							CCoronas::FreeList, CCoronas::UsedList;
std::vector<CCoronasLinkedListNode>				CCoronas::aLinkedList;
std::vector<CRegisteredCorona>					CCoronas::aCoronas;
int&											CCoronas::bChangeBrightnessImmediately = *(int*)0xC3E034;
float&											CCoronas::ScreenMult = *(float*)0x8D4B5C;

WRAPPER void CRegisteredCorona::Update() { EAXJMP(0x6FABF0); }
WRAPPER void CEntity::RegisterReference(CEntity** pAddress) { (pAddress); EAXJMP(0x571B70); }
WRAPPER int CCamera::GetLookDirection() { EAXJMP(0x50AE90); }

CCamera&     TheCamera = *(CCamera*)0xB6F028;

void CCoronas::RegisterCorona(unsigned int nID, CEntity* pAttachTo, unsigned char R, unsigned char G, unsigned char B, unsigned char A, const CVector& Position, float Size, float Range, RwTexture* pTex, unsigned char flareType, unsigned char reflectionType, unsigned char LOSCheck, unsigned char unused, float normalAngle, bool bNeonFade, float PullTowardsCam, bool bFadeIntensity, float FadeSpeed, bool bOnlyFromBelow, bool bWhiteCore)
{
    UNREFERENCED_PARAMETER(unused);

    CVector		vecPosToCheck;
    if (pAttachTo)
    {
        // TODO: AllocateMatrix
        vecPosToCheck = *pAttachTo->GetMatrix() * Position;
    }
    else
        vecPosToCheck = Position;

    CVector*	pCamPos = TheCamera.GetCoords();
    if (Range * Range >= (pCamPos->x - vecPosToCheck.x)*(pCamPos->x - vecPosToCheck.x) + (pCamPos->y - vecPosToCheck.y)*(pCamPos->y - vecPosToCheck.y))
    {
        if (bNeonFade)
        {
            float		fDistFromCam = CVector(*pCamPos - vecPosToCheck).Magnitude();

            if (fDistFromCam < 35.0f)
                return;
            if (fDistFromCam < 50.0f)
                A *= static_cast<unsigned char>((fDistFromCam - 35.0f) * (2.0f / 3.0f));
        }

        // Is corona already present?
        CRegisteredCorona*		pSuitableSlot;
        auto it = UsedMap.find(nID);

        if (it != UsedMap.end())
        {
            pSuitableSlot = it->second->GetFrom();

            if (pSuitableSlot->FadedIntensity == 0 && A == 0)
            {
                // Mark as free
                it->second->GetFrom()->Identifier = 0;
                it->second->Add(&FreeList);
                UsedMap.erase(nID);
                return;
            }
        }
        else
        {
            if (!A)
                return;

            // Adding a new element
            auto	pNewEntry = FreeList.First();
            if (!pNewEntry)
            {
                MessageBoxA(0, "ERROR: Not enough space for coronas!", "ERROR: Not enough space for coronas!", 0);
                return;
            }

            pSuitableSlot = pNewEntry->GetFrom();

            // Add to used list and push this index to the map
            pNewEntry->Add(&UsedList);
            UsedMap[nID] = pNewEntry;

            pSuitableSlot->FadedIntensity = bFadeIntensity ? 255 : 0;
            pSuitableSlot->OffScreen = true;
            pSuitableSlot->JustCreated = true;
            pSuitableSlot->Identifier = nID;
        }

        pSuitableSlot->Red = R;
        pSuitableSlot->Green = G;
        pSuitableSlot->Blue = B;
        pSuitableSlot->Intensity = A;
        pSuitableSlot->Coordinates = Position;
        pSuitableSlot->Size = Size;
        pSuitableSlot->NormalAngle = normalAngle;
        pSuitableSlot->Range = Range;
        pSuitableSlot->pTex = pTex;
        pSuitableSlot->FlareType = flareType;
        pSuitableSlot->ReflectionType = reflectionType;
        pSuitableSlot->LOSCheck = LOSCheck;
        pSuitableSlot->RegisteredThisFrame = true;
        pSuitableSlot->PullTowardsCam = PullTowardsCam;
        pSuitableSlot->FadeSpeed = FadeSpeed;

        pSuitableSlot->NeonFade = bNeonFade;
        pSuitableSlot->OnlyFromBelow = bOnlyFromBelow;
        pSuitableSlot->WhiteCore = bWhiteCore;

        if (pAttachTo)
        {
            pSuitableSlot->bIsAttachedToEntity = true;
            pSuitableSlot->pEntityAttachedTo = pAttachTo;

            pAttachTo->RegisterReference(&pSuitableSlot->pEntityAttachedTo);
        }
        else
        {
            pSuitableSlot->bIsAttachedToEntity = false;
            pSuitableSlot->pEntityAttachedTo = nullptr;
        }
    }
}

void CCoronas::Update()
{
    ScreenMult = min(1.0f, (float)*(DWORD*)0xB7CB5C * 0.03f + ScreenMult);

    static unsigned int		nSomeHackyMask = 0;
    unsigned int			nThisHackyMask = 0;

    if (TheCamera.Cams[TheCamera.ActiveCam].LookingLeft)
        nThisHackyMask |= 1;

    if (TheCamera.Cams[TheCamera.ActiveCam].LookingRight)
        nThisHackyMask |= 2;

    if (TheCamera.Cams[TheCamera.ActiveCam].LookingBehind)
        nThisHackyMask |= 4;

    if (TheCamera.GetLookDirection())
        nThisHackyMask |= 8;

    if (nSomeHackyMask == nThisHackyMask)
        bChangeBrightnessImmediately = max(0, bChangeBrightnessImmediately - 1);
    else
    {
        bChangeBrightnessImmediately = 3;
        nSomeHackyMask = nThisHackyMask;
    }

    auto pNode = UsedList.First();
    if (pNode)
    {
        while (pNode != &UsedList)
        {
            unsigned int	nIndex = pNode->GetFrom()->Identifier;
            auto			pNext = pNode->GetNextNode();

            pNode->GetFrom()->Update();

            // Did it become invalid?
            if (!pNode->GetFrom()->Identifier)
            {
                // Remove from used list
                pNode->Add(&FreeList);
                UsedMap.erase(nIndex);
            }

            pNode = pNext;
        }
    }
}

void CCoronas::UpdateCoronaCoors(unsigned int nID, const CVector& vecPosition, float fMaxDist, float fNormalAngle)
{
    CVector*	pCamPos = TheCamera.GetCoords();
    if (fMaxDist * fMaxDist >= (pCamPos->x - vecPosition.x)*(pCamPos->x - vecPosition.x) + (pCamPos->y - vecPosition.y)*(pCamPos->y - vecPosition.y))
    {
        auto	it = UsedMap.find(nID);

        if (it != UsedMap.end())
        {
            it->second->GetFrom()->Coordinates = vecPosition;
            it->second->GetFrom()->NormalAngle = fNormalAngle;
        }
    }
}

void CCoronas::Init()
{
    // Initialise the lists
    FreeList.Init();
    UsedList.Init();

    for (int i = 0; i < NUM_CORONAS; i++)
    {
        aLinkedList[i].Add(&FreeList);
        aLinkedList[i].SetEntry(&aCoronas[i]);
    }
}

void CCoronas::Inject()
{
    CCoronas::aLinkedList.resize(NUM_CORONAS);
    CCoronas::aCoronas.resize(NUM_CORONAS);
    CRegisteredCorona* aCoronas = &CCoronas::aCoronas[0];    // avoid exception on vector for acessing [NUM_CORONAS]

    // CCoronas::RenderReflections
    CPatch::SetPointer(0x6FB648, &aCoronas[0].JustCreated + 1);
    CPatch::SetPointer(0x6FB6CF, &aCoronas[0].FadedIntensity);

    // CCoronas::Render
    CPatch::SetPointer(0x6FAF42, &aCoronas[0].pEntityAttachedTo);

    // CCoronas::RenderReflections
    CPatch::SetPointer(0x6FB657, &aCoronas[NUM_CORONAS].JustCreated + 1);
    CPatch::SetPointer(0x6FB9B8, &aCoronas[NUM_CORONAS].FadedIntensity);

    // CCoronas::Render
    CPatch::SetInt(0x6FAF4A, NUM_CORONAS);

    CPatch::RedirectJump(0x6FC180, CCoronas::RegisterCorona);
    CPatch::RedirectJump(0x6FC4D0, CCoronas::UpdateCoronaCoors);
    CPatch::RedirectJump(0x6FAAD9, CCoronas::Init);
    CPatch::RedirectCall(0x53C13B, CCoronas::Update);
}

class CoronasSA : public SimpleAdjuster
{
public:
    const char* GetLimitName() { return GetGVM().IsSA() ? "Coronas" : nullptr; }
    void ChangeLimit(int, const std::string& value)
    {
        NUM_CORONAS = std::stoi(value);
        CCoronas::Inject();
    }
} CoronasSA;

/////////////////////////////////////////////GTA 3|VC/////////////////////////////////////////////
DWORD _eax;
DWORD _ebx;
std::vector<char> aCoronas;
DWORD jmpAddress;
int CORONAS_LIMIT;

/////////////////////////////////////////////GTA Vice City/////////////////////////////////////////////
void patch_542434();
void patch_5425B7();
void patch_5425F4();
void patch_542955();
void patch_54310C();
void patch_543497();
void patch_543E0C();
void patch_544086();
void patch_544214();

// 0x0054243A - 0x00815134
// 0x005425BD - 0x00815134
// 0x005425FA - 0x00815134
// 0x00542959 - 0x00815134
// 0x0054310F - 0x0081515C

void PatchCoronasVC()
{
    aCoronas.resize(CORONAS_LIMIT * 104);

    //CPatch::AdjustPointer(0x400000+0x14243A, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x14244E, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x14245C, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x142467, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x142470, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x142479, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x142483, &aCoronas[0], 0x815128, 0x81518F);
    //CPatch::AdjustPointer(0x400000+0x1425BD, &aCoronas[0], 0x815128, 0x81518F);
    //CPatch::AdjustPointer(0x400000+0x1425FA, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x142619, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x142623, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x142643, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x142653, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x14265A, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x142661, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x142668, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x14266F, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x142676, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x142686, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x14269D, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x1426C1, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x1426C7, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x1426D1, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x1426DB, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x1426E5, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x1426F2, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x1426FB, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x142704, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x14270E, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x142718, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x142720, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x142727, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x14272D, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x142737, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x142741, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x142747, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x142784, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x142884, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x1428A4, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x1428DE, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x1428C1, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x1428FB, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x142918, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x142935, &aCoronas[0], 0x815128, 0x81518F);
    //CPatch::AdjustPointer(0x400000+0x142959, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x14301C, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143049, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x14305F, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143069, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x14307F, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143089, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x14309F, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x1430A9, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x1430BF, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x1430C9, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x1430DF, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x1430E9, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x14303F, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143102, &aCoronas[0], 0x815128, 0x81518F);
    //CPatch::AdjustPointer(0x400000+0x14310F, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x14317E, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143190, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x14319D, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x1431A6, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x1431B3, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143206, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143210, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143243, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x14324D, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x14326B, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x14328D, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x1432B4, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143306, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x1433F4, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143428, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143439, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143459, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143573, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x1435B7, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x1435C4, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x1435D5, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x1435DD, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x1435E8, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x1435F1, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x1435FA, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143603, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x14360C, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143615, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143626, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143636, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143642, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143649, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143656, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x14365F, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143720, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143741, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x1437F5, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143810, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143817, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x14381D, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143823, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143829, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x14382F, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143835, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x14383B, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143842, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x14384F, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143856, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x14387A, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143881, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x1438AD, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x1438B4, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x1438C2, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143991, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x1439CB, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143A09, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143A14, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143A28, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143A87, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143AC0, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143AFA, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143B3B, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143BA3, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143BD8, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143C11, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143C52, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143CA2, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143D14, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143D28, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143D36, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143E76, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143EB0, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143EC8, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143F61, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143F71, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143F81, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143F97, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143FAD, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143FBA, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x143FF5, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x14400C, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x144018, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x144043, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x1441FB, &aCoronas[0], 0x815128, 0x81518F);
    CPatch::AdjustPointer(0x400000+0x144200, &aCoronas[0], 0x815128, 0x81518F);

    CPatch::RedirectJump(0x542434, patch_542434);
    CPatch::RedirectJump(0x5425B7, patch_5425B7);
    CPatch::RedirectJump(0x5425F4, patch_5425F4);
    CPatch::SetUShort(0x542616, 0xDA8B); // mov ebx, edx
    CPatch::Nop(0x542618, 1);
    CPatch::SetUShort(0x542680, 0xC28B); // mov eax, edx
    CPatch::Nop(0x542682, 1);
    CPatch::SetUShort(0x5426B0, 0xCA8B); // mov ecx, edx
    CPatch::Nop(0x5426B2, 1);
    CPatch::SetUShort(0x542895, 0xDA8B); // mov ebx, edx
    CPatch::Nop(0x542897, 1);
    CPatch::SetUShort(0x5428B2, 0xD38B); // mov edx, ebx
    CPatch::Nop(0x5428B4, 1);
    CPatch::SetUShort(0x5428CF, 0xDA8B); // mov ebx, edx
    CPatch::Nop(0x5428D1, 1);
    CPatch::SetUShort(0x5428EC, 0xD38B); // mov edx, ebx
    CPatch::Nop(0x5428EE, 1);
    CPatch::SetUShort(0x542909, 0xDA8B); // mov ebx, edx
    CPatch::Nop(0x54290B, 1);
    CPatch::SetUShort(0x542926, 0xD38B); // mov edx, ebx
    CPatch::Nop(0x542928, 1);
    CPatch::SetUShort(0x542943, 0xDA8B); // mov ebx, edx
    CPatch::Nop(0x542945, 1);
    CPatch::RedirectJump(0x542955, patch_542955);
    CPatch::RedirectJump(0x54310C, patch_54310C);
    CPatch::RedirectJump(0x543497, patch_543497);
    CPatch::RedirectJump(0x543E0C, patch_543E0C);
    CPatch::RedirectJump(0x544086, patch_544086);
    CPatch::RedirectJump(0x544214, patch_544214);
}

void __declspec(naked)patch_542434()
{
    __asm
    {
        cmp eax, CORONAS_LIMIT
        jnb loc_542442
        mov _eax, eax
        mov eax, [aCoronas]
        add eax, 0xC
        cmp edx, dword ptr ds : [eax + ebx]
        mov eax, _eax
        jne loc_542430
loc_542442:
        cmp eax, CORONAS_LIMIT
        je loc_542489
        mov jmpAddress, 0x54244B
        jmp jmpAddress
loc_542489:
        mov jmpAddress, 0x542489
        jmp jmpAddress
loc_542430:
        mov jmpAddress, 0x542430
        jmp jmpAddress
    }
}

void __declspec(naked)patch_5425B7()
{
    __asm
    {
        cmp edx, CORONAS_LIMIT
        jnb loc_5425C5
        mov _eax, eax
        mov eax, [aCoronas]
        add eax, 0xC
        cmp esi, dword ptr ds : [eax + ecx]
        mov eax, _eax
        jne loc_5425B3
loc_5425C5:
        cmp edx, CORONAS_LIMIT
        jne loc_542680
        mov jmpAddress, 0x5425CF
        jmp jmpAddress
loc_542680:
        mov jmpAddress, 0x542680
        jmp jmpAddress
loc_5425B3:
        mov jmpAddress, 0x5425B3
        jmp jmpAddress
    }
}

void __declspec(naked)patch_5425F4()
{
    __asm
    {
        cmp edx, CORONAS_LIMIT
        jnb loc_542603
        mov _ebx, ebx
        mov ebx, [aCoronas]
        add ebx, 0xC
        cmp dword ptr ds : [ebx + eax], 0
        mov ebx, _ebx
        jne loc_5425F0
loc_542603:
        cmp edx, CORONAS_LIMIT
        jne loc_542616
        mov jmpAddress, 0x542609
        jmp jmpAddress
loc_542616:
        mov jmpAddress, 0x542616
        jmp jmpAddress
loc_5425F0:
        mov jmpAddress, 0x5425F0
        jmp jmpAddress
    }
}

void __declspec(naked)patch_542955()
{
    __asm
    {
        cmp eax, CORONAS_LIMIT
        mov _eax, eax
        mov eax, [aCoronas]
        add eax, 0xC
        mov dword ptr ds : [eax + edx * 8h], 0
        mov eax, _eax
        mov jmpAddress, 0x542964
        jmp jmpAddress
    }
}

void __declspec(naked)patch_54310C()
{
    __asm
    {
        cmp eax, CORONAS_LIMIT
        mov _eax, eax
        mov eax, [aCoronas]
        add eax, 0x34
        mov byte ptr ds : [eax + esi * 8], cl
        mov eax, _eax
        mov jmpAddress, 0x543116
        jmp jmpAddress
    }
}

void __declspec(naked)patch_543497()
{
    __asm
    {
        add ebp, 68h
        mov _eax, eax
        mov eax, CORONAS_LIMIT
        cmp dword ptr [esp], eax
        mov eax, _eax
        mov jmpAddress, 0x54349E
        jmp jmpAddress
    }
}

void __declspec(naked)patch_543E0C()
{
    __asm
    {
        mov _eax, eax
        mov eax, CORONAS_LIMIT
        cmp dword ptr[esp + 0x38], eax
        mov eax, _eax
        mov jmpAddress, 0x543E12
        jmp jmpAddress
    }
}

void __declspec(naked)patch_544086()
{
    __asm
    {
        mov _eax, eax
        mov eax, CORONAS_LIMIT
        cmp dword ptr[esp + 0x34], eax
        mov eax, _eax
        mov jmpAddress, 0x54408C
        jmp jmpAddress
    }
}

void __declspec(naked)patch_544214()
{
    __asm
    {
        add esi, 68h
        cmp ebx, CORONAS_LIMIT
        mov jmpAddress, 0x54421A
        jmp jmpAddress
    }
}



class CoronasVC : public SimpleAdjuster
{
public:
    const char* GetLimitName() { return GetGVM().IsVC() ? "Coronas" : nullptr; }
    void ChangeLimit(int, const std::string& value)
    {
        CORONAS_LIMIT = std::stoi(value);
        PatchCoronasVC();
    }
} CoronasVC;

/////////////////////////////////////////////GTA 3/////////////////////////////////////////////////////
void patch_4F8FA7();
void patch_4F9860();
void patch_4F9ABD();
void patch_4F9BA7();
void patch_4F9F2A();
void patch_4FA03B();
void patch_4FA12C();
void patch_4FA15B();
void patch_4FA324();

DWORD ext_4F8F90 = 0x4F8F90;
DWORD ext_4F8FAC = 0x4F8FAC;
DWORD ext_4F9866 = 0x4F9866;
DWORD ext_4F9AC3 = 0x4F9AC3;
DWORD ext_4F9B66 = 0x4F9B66;
DWORD ext_4F9BAD = 0x4F9BAD;
DWORD ext_4F9F2F = 0x4F9F2F;
DWORD ext_4F9FE2 = 0x4F9FE2;
DWORD ext_4FA041 = 0x4FA041;
DWORD ext_4FA128 = 0x4FA128;
DWORD ext_4FA13E = 0x4FA13E;
DWORD ext_4FA157 = 0x4FA157;
DWORD ext_4FA16E = 0x4FA16E;
DWORD ext_4FA320 = 0x4FA320;
DWORD ext_4FA33B = 0x4FA33B;
DWORD ext_4FA379 = 0x4FA379;

/*
    Excluded patchable addresses because of hooks:

    0x004FA132 - 0x0072E518
    0x004FA161 - 0x0072E518
    0x004FA32A - 0x0072E518
*/

void PatchCoronasIII()
{
    aCoronas.resize(CORONAS_LIMIT * 128);

    CPatch::AdjustPointer(0x400000+0xF8F8B, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF8F90, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9023, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF906C, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF907C, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9083, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF908A, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9097, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF909E, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF90A7, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF90B0, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF90B9, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF90C2, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF90CB, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF90E2, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF90EE, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF90F5, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9102, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF910B, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF91D0, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF91F7, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9263, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF926D, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9273, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9279, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF927F, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9285, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF928B, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9291, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9297, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF929E, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF92A7, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF92AD, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF92D4, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF92DA, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9301, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9308, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9318, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF93E5, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF941F, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9468, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF947C, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF94DB, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9514, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF954E, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF958F, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF95F7, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF962C, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9665, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF96A6, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF96F6, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF976D, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9781, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF978F, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF98CA, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF990B, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9923, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF99BC, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF99C9, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF99DA, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF99E7, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF99F4, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9A1F, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9A28, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9A43, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9A4F, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9A7A, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9B69, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9B70, &aCoronas[1], 0x72E598, 0x72E617);
    CPatch::AdjustPointer(0x400000+0xF9B77, &aCoronas[2], 0x72E618, 0x72E697);
    CPatch::AdjustPointer(0x400000+0xF9B7E, &aCoronas[3], 0x72E698, 0x72E717);
    CPatch::AdjustPointer(0x400000+0xF9B85, &aCoronas[4], 0x72E718, 0x72E797);
    CPatch::AdjustPointer(0x400000+0xF9B8C, &aCoronas[5], 0x72E798, 0x72E817);
    CPatch::AdjustPointer(0x400000+0xF9B93, &aCoronas[6], 0x72E818, 0x72E897);
    CPatch::AdjustPointer(0x400000+0xF9B9A, &aCoronas[7], 0x72E898, 0x72E918);
    CPatch::AdjustPointer(0x400000+0xF9C0A, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9C17, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9C24, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9C2D, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9C3A, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9C47, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9C91, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9C9B, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9CD1, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9CDB, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9CE1, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9CE8, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9CF5, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9D17, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9D3E, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9D90, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9E84, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9EB8, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9EC9, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9EE9, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9FE5, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xF9FEF, &aCoronas[1], 0x72E598, 0x72E617);
    CPatch::AdjustPointer(0x400000+0xF9FF9, &aCoronas[2], 0x72E618, 0x72E697);
    CPatch::AdjustPointer(0x400000+0xFA003, &aCoronas[3], 0x72E698, 0x72E717);
    CPatch::AdjustPointer(0x400000+0xFA00D, &aCoronas[4], 0x72E718, 0x72E797);
    CPatch::AdjustPointer(0x400000+0xFA017, &aCoronas[5], 0x72E798, 0x72E817);
    CPatch::AdjustPointer(0x400000+0xFA021, &aCoronas[6], 0x72E818, 0x72E897);
    CPatch::AdjustPointer(0x400000+0xFA02B, &aCoronas[7], 0x72E898, 0x72E918);
    //CPatch::AdjustPointer(0x400000+0xFA132, &aCoronas[0], 0x72E518, 0x72E597);
    //CPatch::AdjustPointer(0x400000+0xFA161, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xFA179, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xFA183, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xFA19D, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xFA1A4, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xFA1B4, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xFA1BB, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xFA1C2, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xFA1C9, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xFA1D0, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xFA1D7, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xFA1E6, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xFA1F9, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xFA20D, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xFA215, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xFA21B, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xFA227, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xFA231, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xFA23B, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xFA247, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xFA250, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xFA259, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xFA263, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xFA26D, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xFA275, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xFA27F, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xFA285, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xFA28F, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xFA299, &aCoronas[0], 0x72E518, 0x72E597);
    //CPatch::AdjustPointer(0x400000+0xFA32A, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xFA33E, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xFA34C, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xFA357, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xFA360, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xFA369, &aCoronas[0], 0x72E518, 0x72E597);
    CPatch::AdjustPointer(0x400000+0xFA373, &aCoronas[0], 0x72E518, 0x72E597);

    CPatch::RedirectJump(0x4F8FA7, patch_4F8FA7);
    CPatch::RedirectJump(0x4F9860, patch_4F9860);
    CPatch::RedirectJump(0x4F9ABD, patch_4F9ABD);
    CPatch::RedirectJump(0x4F9BA7, patch_4F9BA7);
    CPatch::SetUShort(0x4F9C50, 0x8B90); // mov eax, dword ptr [esp]
    CPatch::RedirectJump(0x4F9F2A, patch_4F9F2A);
    CPatch::RedirectJump(0x4FA03B, patch_4FA03B);
    CPatch::RedirectJump(0x4FA12C, patch_4FA12C);
    CPatch::RedirectJump(0x4FA15B, patch_4FA15B);
    CPatch::SetUShort(0x4FA176, 0xDA8B); // mov ebx, edx
    CPatch::Nop(0x4FA178, 1);
    CPatch::SetUShort(0x4FA1E0, 0xC28B); // mov eax, edx
    CPatch::Nop(0x4FA1E2, 1);
    CPatch::Nop(0x4FA206, 3);
    CPatch::RedirectJump(0x4FA324, patch_4FA324);
}

void __declspec(naked) patch_4F8FA7()
{
    __asm
    {
        cmp ebx, CORONAS_LIMIT
        jb loc_4F8F90
        jmp ext_4F8FAC
loc_4F8F90:
        jmp ext_4F8F90
    }
}

void __declspec(naked) patch_4F9860()
{
    __asm
    {
        mov _eax, eax
        mov eax, CORONAS_LIMIT
        cmp dword ptr [esp+3Ch], eax
        mov eax, _eax
        jmp ext_4F9866
    }
}

void __declspec(naked) patch_4F9ABD()
{
    __asm
    {
        mov _eax, eax
        mov eax, CORONAS_LIMIT
        cmp dword ptr [esp+34h], eax
        mov eax, _eax
        jmp ext_4F9AC3
    }
}

void __declspec(naked) patch_4F9BA7()
{
    __asm
    {
        cmp eax, CORONAS_LIMIT
        jb loc_4F9B66
        jmp ext_4F9BAD
loc_4F9B66:
        jmp ext_4F9B66
    }
}

void __declspec(naked) patch_4F9F2A()
{
    __asm
    {
        mov _eax, eax
        mov eax, CORONAS_LIMIT
        cmp dword ptr [esp], eax
        mov eax, _eax
        jmp ext_4F9F2F
    }
}

void __declspec(naked) patch_4FA03B()
{
    __asm
    {
        cmp eax, CORONAS_LIMIT
        jb loc_4F9FE2
        jmp ext_4FA041
loc_4F9FE2:
        jmp ext_4F9FE2
    }
}

void __declspec(naked) patch_4FA12C()
{
    __asm
    {
        cmp edx, CORONAS_LIMIT
        jnb loc_4FA13A
        mov _eax, eax
        mov eax, [aCoronas]
        cmp edi, dword ptr ds:[eax + ebx]
        mov eax, _eax
        jne loc_4FA128
loc_4FA13A:
        cmp edx, CORONAS_LIMIT
        jmp ext_4FA13E
loc_4FA128:
        jmp ext_4FA128
    }
}

void __declspec(naked) patch_4FA15B()
{
    __asm
    {
        cmp edx, CORONAS_LIMIT
        jnb loc_4FA16A
        mov _ebx, ebx
        mov ebx, [aCoronas]
        cmp dword ptr ds:[ebx + eax], 0
        mov ebx, _ebx
        jne loc_4FA157
loc_4FA16A:
        cmp edx, CORONAS_LIMIT
        jmp ext_4FA16E
loc_4FA157:
        jmp ext_4FA157
    }
}

void __declspec(naked) patch_4FA324()
{
    __asm
    {
        cmp eax, CORONAS_LIMIT
        jnb loc_4FA332
        mov _eax, eax
        mov eax, [aCoronas]
        cmp edx, dword ptr ds:[eax + ebx]
        mov eax, _eax
        jne loc_4FA320
loc_4FA332:
        cmp eax, CORONAS_LIMIT
        je loc_4FA379
        jmp ext_4FA33B
loc_4FA379:
        jmp ext_4FA379
loc_4FA320:
        jmp ext_4FA320
    }
}

class CoronasIII : public SimpleAdjuster
{
public:
    const char* GetLimitName() { return GetGVM().IsIII() ? "Coronas" : nullptr; }
    void ChangeLimit(int, const std::string& value)
    {
        CORONAS_LIMIT = std::stoi(value);
        PatchCoronasIII();
    }
} CoronasIII;
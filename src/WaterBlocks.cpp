/*
 *  Outside World Water Blocks Limit Adjuster
 *  Copyright (C) 2014 LINK/2012 <dma_2012@hotmail.com>
 *  Licensed under the MIT License (http://opensource.org/licenses/MIT)
 */
#include "LimitAdjuster.h"
#include <stdexcept>
 
using namespace injector;

// Assembly related stuff
extern "C" void ASM_OutsideWorldWaterBlocks();
extern "C" int  LIMIT_OutsideWorldWaterBlocks;
extern "C" void* ASM_OutsideWorldWaterBlocks_ret1;
extern "C" void* ASM_OutsideWorldWaterBlocks_ret2;

 
/*
 *  Key: OutsideWorldWaterBlocks
 *  Value: Integer, Maximum value ~500
 *  Game: SA
 */
struct OutsideWorldWaterBlocks : public SimpleAdjuster
{
	uint16_t *a, *aX, *aY;
	
	// Constructs initializing array to null
	OutsideWorldWaterBlocks() : a(nullptr)
	{}
	
	// Destructs deleting array
	~OutsideWorldWaterBlocks()
	{ this->Free(); }
	
	// Limit Name
	const char* GetLimitName()
	{
		return (GetGVM().IsSA()? "OutsideWorldWaterBlocks" : nullptr);
	}

	// Sets the water blocks limit
	void ChangeLimit(int, const std::string& value)
	{
		auto limit = std::stoi(value);
		
		// This maximum value for the limit is related to the maximum number of vertices/indices the temporary vertices/indices array
		// for immediate mode can handle...
		// If this value is too high, it will end up overwriting game info at .data segment, after the temporary immediate arrays
		if(limit > 500) throw std::runtime_error("Maximum value for OutsideWorldWaterBlocks is 500");
		
		// Allocate array to store X and Y position of the outside world water blocks
		this->a  = this->ReAlloc(limit);
		this->aX = &a[0];
		this->aY = &a[limit];
		LIMIT_OutsideWorldWaterBlocks = limit;

		// Patch the necessary references
		MakeJMP(0x6E6CE9, ASM_OutsideWorldWaterBlocks);
		WriteMemory(0x6E6CEE + 0x4, aX, true);
		WriteMemory(0x6EF6E0 + 0x4, aX, true);
		WriteMemory(0x6EFE82 + 0x4, aX, true);
		WriteMemory(0x6E6CF6 + 0x4, aY, true);
		WriteMemory(0x6EF6E8 + 0x4, aY, true);
		WriteMemory(0x6EFE95 + 0x4, aY, true);
		WriteMemory(0x6EFEAF + 0x4, aY, true);
		
		// Find real offsets for the assembly hook
		ASM_OutsideWorldWaterBlocks_ret1 = lazy_pointer<0x6E6CEE>::get();
		ASM_OutsideWorldWaterBlocks_ret2 = lazy_pointer<0x6E6D04>::get();
	}

    // Usage counter
    bool GetUsage(int, std::string& output)
    {
        int max = HasPatched()? LIMIT_OutsideWorldWaterBlocks : ReadMemory<char>(0x6E6CE9 + 2, true);
        return Adjuster::GetUsage(output, ReadMemory<int>(0xC215EC), max);
    }

    // Frees the current outside world water block array
    void Free()
    {
        if(this->a)
        {
            delete[] a;
            this->a = nullptr;
        }
    }

    // Reallocates the outside world water block array
    uint16_t* ReAlloc(int limit)
    {
        this->Free();
        return (this->a  = new uint16_t[limit * 2]);
    }

    bool HasPatched()
    {
        return this->a != nullptr;
    }

// Instantiate the adjuster on the global scope
} adjuster_OutsideWorldWaterBlocks;



/*
 *  Assembly patches
 */

int LIMIT_OutsideWorldWaterBlocks;
void* ASM_OutsideWorldWaterBlocks_ret1;
void* ASM_OutsideWorldWaterBlocks_ret2;

void __declspec(naked) ASM_OutsideWorldWaterBlocks()
{
	__asm
	{
			cmp     eax, LIMIT_OutsideWorldWaterBlocks
			jge     _Full
			push    ASM_OutsideWorldWaterBlocks_ret1
			ret
		_Full:
			push    ASM_OutsideWorldWaterBlocks_ret2
			ret
	}
}

/////////////////////////////////////////////GTA Vice City/////////////////////////////////////////////
class OutsideWorldWaterBlocksVC : public SimpleAdjuster
{
//Mod for GTA VC versions 1.0 and 1.1 increases visibility of water to 10000x10000
//Author: xanser
//e-mail: xanser@mail.ru
public:
    const char* GetLimitName() { return GetGVM().IsVC() ? "OutsideWorldWaterBlocks" : nullptr; }
    void ChangeLimit(int, const std::string& value)
    {
        BYTE * WaterRender = (BYTE *)0x5C1710;					// функция рендера воды
        if (*WaterRender == 0xDE) WaterRender += 0x20;			// VC 1.1
        BYTE   WaterLines = std::stoi(value);                                // количество линий внешней воды 5
        //        *(float *)0x69CC58 = 0;                               // сдвиг всей воды на запад 400
        *(float *)0x69CD7C = 10000;                             // радиус видимости внутренней воды ниже 60 метров (z-15)*800*0.022+1200
        *(float *)0x69CD80 = 10000;                             // радиус видимости внутренней воды выше 60 метров 2000
        *(float *)0x69CDCC = static_cast<float>(WaterLines * 256);                    // сдвиг на количество внешней воды 1280 = 5*256
        injector::WriteMemory<unsigned char>(WaterRender + 0xDC1, WaterLines, true);				// количество внешней воды севера и юга
        injector::WriteMemory<unsigned char>(WaterRender + 0xDCB, (WaterLines - 5) * 2 + 26, true);     // расширение внешней воды севера и юга на восток
        injector::WriteMemory<unsigned char>(WaterRender + 0xDD3, WaterLines, true);             // сдвиг внешней воды запада и востока на север
        injector::WriteMemory<unsigned char>(WaterRender + 0x11D1, WaterLines, true);				// количество внешней воды запада и востока
        injector::WriteMemory<unsigned char>(WaterRender + 0x11DB, WaterLines + 16, true);          // расширение внешней воды запада и востока на север
    }
} OutsideWorldWaterBlocksVC;

/////////////////////////////////////////////GTA 3/////////////////////////////////////////////////////
class OutsideWorldWaterBlocksIII : public SimpleAdjuster
{
public:
    const char* GetLimitName() { return GetGVM().IsIII() ? "OutsideWorldWaterBlocks" : nullptr; }
    void ChangeLimit(int, const std::string& value)
    {
        int WaterCount = std::stoi(value);

        injector::WriteMemory<float>(0x602AE8, 10000.0f, true);
        injector::WriteMemory<float>(0x602AF0, 10000.0f, true);
        injector::WriteMemory<float>(0x602B48, static_cast<float>(WaterCount)* 256.0f, true);
        injector::WriteMemory<unsigned char>(0x5564BD + 0x2, static_cast<unsigned char>(WaterCount));
        injector::WriteMemory<unsigned char>(0x5C24D9 + 0x3, (static_cast<unsigned char>(WaterCount)-5) * 2 + 26);
        injector::WriteMemory<unsigned char>(0x5564D0 + 0x1, static_cast<unsigned char>(WaterCount));
        injector::WriteMemory<unsigned char>(0x55676D + 0x2, static_cast<unsigned char>(WaterCount));
        injector::WriteMemory<unsigned char>(0x556777 + 0x2, static_cast<unsigned char>(WaterCount)+16);
    }
} OutsideWorldWaterBlocksIII;
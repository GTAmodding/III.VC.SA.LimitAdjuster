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

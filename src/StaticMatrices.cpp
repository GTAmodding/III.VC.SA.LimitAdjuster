/*
 *  Static Matrices Limit Adjuster
 *  Copyright (C) 2014 LINK/2012 <dma_2012@hotmail.com>
 *  Licensed under the MIT License (http://opensource.org/licenses/MIT)
 */
#include "LimitAdjuster.h"

/*
 *  Key: StaticMatrices
 *  Value: Integer
 *  Game: SA
 */
struct StaticMatrices : public SimpleAdjuster
{
    // Limit Name
    const char* GetLimitName()
    {
        return (GetGVM().IsSA()? "StaticMatrices" : nullptr);
    }

    // Sets the limit
    void ChangeLimit(int, const std::string& value)
    {
        injector::WriteMemory(0x54F3A0 + 0x1, std::stoi(value), true);
    }

// Instantiate the adjuster on the global scope
} adjuster_StaticMatrices;


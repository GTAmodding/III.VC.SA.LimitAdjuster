/*
 *  Static Matrices Limit Adjuster
 *  Copyright (C) 2014 LINK/2012 <dma_2012@hotmail.com>
 *  Licensed under the MIT License (http://opensource.org/licenses/MIT)
 */
#include "LimitAdjuster.h"

/*
 *  Key: StreamingObjectInstancesListSize
 *  Value: Integer
 *  Game: SA
 */
struct StreamingObjectInstancesListSize : public SimpleAdjuster
{
	// Limit Name
	const char* GetLimitName()
	{
		return (GetGVM().IsSA()? "StreamingObjectInstancesListSize" : nullptr);
	}

	// Sets the limit
	void ChangeLimit(int, const std::string& value)
	{
		injector::WriteMemory(0x5B8E55, std::stoi(value), true);
		injector::WriteMemory(0x5B8EB0, std::stoi(value), true);
	}

// Instantiate the adjuster on the global scope
} adjuster_StreamingObjectInstancesListSize;


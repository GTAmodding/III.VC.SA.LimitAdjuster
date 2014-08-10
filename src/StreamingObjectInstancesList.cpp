/*
 *  Streaming Object Instance List Limit Adjuster
 *  Copyright (C) 2014 ThirteenAG <thirteenag@gmail.com>
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
		return (GetGVM().IsSA()? "StreamingObjectInstancesList" : nullptr);
	}

	// Sets the limit
	void ChangeLimit(int, const std::string& value)
	{
        auto size = std::stoi(value) * 12;
		injector::WriteMemory(0x5B8E55, size, true);
		injector::WriteMemory(0x5B8EB0, size, true);
	}

    // TODO GetUsage

// Instantiate the adjuster on the global scope
} adjuster_StreamingObjectInstancesListSize;


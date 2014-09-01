/*
 *  Streaming Object Instance List Limit Adjuster
 *  Copyright (C) 2014 ThirteenAG <thirteenag@gmail.com>
 *  Licensed under the MIT License (http://opensource.org/licenses/MIT)
 */
#include "LimitAdjuster.h"
#include "CLinkList.h"


/*
 *  Key: StreamingObjectInstancesList
 *  Value: Integer
 *  Game: SA
 *
 *  Note: Not reliable to have it unlimited!!! Will eat way unecesary memory!!!
 *        Plus would need another unreliable code fix to the game.
 *
 */
struct StreamingObjectInstancesList : public SimpleAdjuster
{
    typedef CLinkList<void*> CStreamingLinkList;

	// Limit Name
	const char* GetLimitName()
	{
		return (GetGVM().IsSA()? "StreamingObjectInstancesList" : nullptr);
	}

	// Sets the limit
	void ChangeLimit(int, const std::string& value)
	{
        auto size = std::stoi(value) * sizeof(CStreamingLinkList::Link);
	    injector::WriteMemory(0x5B8E55, size, true);
	    injector::WriteMemory(0x5B8EB0, size, true);
	}

    // Gets the rwObjectInstances usage
    bool GetUsage(int, std::string& output)
    {
        auto& rwObjectInstances = *injector::lazy_pointer<0x9654F0>().get<CStreamingLinkList>();
        auto nUsed = rwObjectInstances.GetNumUsed();
        auto nFree = rwObjectInstances.GetNumFree();
        return Adjuster::GetUsage(output, nUsed, nUsed + nFree);
    }

// Instantiate the adjuster on the global scope
} adjuster_StreamingObjectInstancesList;

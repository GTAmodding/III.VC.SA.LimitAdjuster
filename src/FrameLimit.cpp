/*
 *  FrameLimit Adjuster
 *
 */
#include "LimitAdjuster.h"
 
class FrameLimitSA : public SimpleAdjuster
{
    public:
        const char* GetLimitName() { return GetGVM().IsSA()? "FrameLimit" : nullptr; }
        void ChangeLimit(int, const std::string& value) 
        { 
            injector::WriteMemory(0x619626, std::stoi(value), true); 
            injector::WriteMemory(0xC1704C, std::stoi(value), true);
        }
        
} FrameLimitSA;

class FrameLimitVC : public SimpleAdjuster
{
public:
    const char* GetLimitName() { return GetGVM().IsVC() ? "FrameLimit" : nullptr; }
    void ChangeLimit(int, const std::string& value) 
    { 
        injector::WriteMemory(0x602D68, std::stoi(value), true); 
        injector::WriteMemory(0x9B48EC, std::stoi(value), true);
    }

} FrameLimitVC;

class FrameLimitIII : public SimpleAdjuster
{
public:
    const char* GetLimitName() { return GetGVM().IsIII() ? "FrameLimit" : nullptr; }
    void ChangeLimit(int, const std::string& value) 
    { 
        injector::WriteMemory(0x584C78, std::stoi(value), true); 
        injector::WriteMemory(0x8F4374, std::stoi(value), true);
    }

} FrameLimitIII;
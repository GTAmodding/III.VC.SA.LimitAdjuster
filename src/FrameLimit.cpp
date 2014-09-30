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
            injector::WriteMemory(0x619626, std::stof(value), true); 
            injector::WriteMemory(0xC1704C, std::stof(value), true);
        }
        
} FrameLimitSA;

class FrameLimitVC : public SimpleAdjuster
{
public:
    const char* GetLimitName() { return GetGVM().IsVC() ? "FrameLimit" : nullptr; }
    void ChangeLimit(int, const std::string& value) 
    { 
        injector::WriteMemory(0x602D68, std::stof(value), true); 
        injector::WriteMemory(0x9B48EC, std::stof(value), true);
    }

} FrameLimitVC;

class FrameLimitIII : public SimpleAdjuster
{
public:
    const char* GetLimitName() { return GetGVM().IsIII() ? "FrameLimit" : nullptr; }
    void ChangeLimit(int, const std::string& value) 
    { 
        injector::WriteMemory(0x584C78, std::stof(value), true); 
        injector::WriteMemory(0x8F4374, std::stof(value), true);
    }

} FrameLimitIII;
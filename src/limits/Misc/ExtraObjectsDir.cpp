#include "LimitAdjuster.h"

using namespace injector;

class ExtraObjectsDirIII : public SimpleAdjuster
{
    public:
        const char* GetLimitName() { return GetGVM().IsIII()? "ExtraObjectsDir" : nullptr; }
        void ChangeLimit(int, const std::string& value)
	{
		int n = std::stoi(value);
		WriteMemory(0x4069D2 + 1, n, true);
	}
} ExtraObjectsDirIII;

class ExtraObjectsDirVC : public SimpleAdjuster
{
    public:
        const char* GetLimitName() { return GetGVM().IsVC()? "ExtraObjectsDir" : nullptr; }
        void ChangeLimit(int, const std::string& value)
	{
		int n = std::stoi(value);
		WriteMemory(0x4106E9 + 1, n, true);
	}
} ExtraObjectsDirVC;

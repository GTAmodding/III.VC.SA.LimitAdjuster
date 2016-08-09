#include "LimitAdjuster.h"

using namespace injector;

class TxdStoreIII : public SimpleAdjuster
{
    public:
        const char* GetLimitName() { return GetGVM().IsIII()? "TxdStore" : nullptr; }
        void ChangeLimit(int, const std::string& value)
	{
		int n = std::stoi(value);
		WriteMemory(0x406977 + 2, n, true);
		WriteMemory(0x527457 + 1, n, true);
		WriteMemory(0x5274CF + 2, n, true);
		WriteMemory(0x592C98 + 1, n, true);
		WriteMemory(0x592CE4 + 1, n, true);
		WriteMemory(0x592D3D + 1, n, true);
		WriteMemory(0x592E32 + 1, n, true);
		WriteMemory(0x592F22 + 2, n, true);
		WriteMemory(0x592F59 + 1, n, true);        
	}
} TxdStoreIII;

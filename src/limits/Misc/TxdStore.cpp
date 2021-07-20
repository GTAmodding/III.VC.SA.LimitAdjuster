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

class TxdStoreVC : public SimpleAdjuster {
public:
	const char* GetLimitName() { return GetGVM().IsVC() ? "TxdStore" : nullptr; }
	void ChangeLimit(int, const std::string& value)
	{
		int n = std::stoi(value);
		WriteMemory(0x58102D + 0, n, true);
		WriteMemory(0x41065D + 2, n, true);
		WriteMemory(0x580FD4 + 2, n, true);
		WriteMemory(0x61DAA5 + 1, n, true);
		WriteMemory(0x61DCFA + 2, n, true);
		WriteMemory(0x61DD39 + 1, n, true);
		WriteMemory(0x61D98B + 1, n, true);

	}
} TxdStoreVC;

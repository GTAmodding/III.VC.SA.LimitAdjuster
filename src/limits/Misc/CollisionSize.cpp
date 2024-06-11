#include "LimitAdjuster.h"

using namespace injector;

class CollisionSizeVC : public SimpleAdjuster
{
	public:
		const char* GetLimitName() { return GetGVM().IsVC() ? "CollisionSize" : nullptr; }
		void ChangeLimit(int, const std::string& value)
		{
			// Collision size is stored in bytes, so we need to convert the input value to bytes.
			int size = std::stoi(value) * 1024;
			char* colbuf = new char[size]();

			WriteMemory(0x0048AAFB + 1, colbuf, true);
			WriteMemory(0x0048AB30 + 1, colbuf, true);
			WriteMemory(0x0048AB60 + 1, colbuf, true);

		}
} CollisionSizeVC;
namespace Hook
{
	inline unsigned long Protect(int address, int size)
	{
		unsigned long vp;
		VirtualProtect((void *)address, size, PAGE_EXECUTE_READWRITE, &vp);
		return vp;
	}
	inline void Unprotect(int address, int size, unsigned long vp)
	{
		VirtualProtect((void *)address, size, vp, nullptr);
	}
	inline void Patch(int address, void *data, int size)
	{
		unsigned long vp;
		vp = Protect(address, size);
		memcpy((void *)address, data, size);
		Unprotect(address, size, vp);
	}
	inline void Nop(int address, int size)
	{
		unsigned long vp;
		vp = Protect(address, size);
		memset((void *)address, 0x90, size);
		Unprotect(address, size, vp);
	}
	inline void FillWithZeroes(int address, int size)
	{
		unsigned long vp;
		vp = Protect(address, size);
		memset((void *)address, 0x00, size);
		Unprotect(address, size, vp);
	}
	inline int GetOffset(int address, void *func)
	{
		return (int)func - (address + 4);
	}
	inline int GetOffset(int address, int loc)
	{
		return loc - (address + 4);
	}
	inline char GetShortOffset(int address, int loc)
	{
		return loc - (address + 1);
	}
	inline void RedirectOffset(int address, void *func)
	{
		int temp = GetOffset(address, func);
		Patch(address, &temp, 4);
	}
	inline void RedirectOffset(int address, int loc)
	{
		int temp = GetOffset(address, loc);
		Patch(address, &temp, 4);
	}
	inline void RedirectShortOffset(int address, int loc)
	{
		unsigned char temp = GetShortOffset(address, loc);
		Patch(address, &temp, 1);
	}
	inline void RedirectCall(int address, void *func)
	{
		unsigned char temp = 0xE8;
		Patch(address++, &temp, 1);
		RedirectOffset(address, func);
	}
	inline void RedirectJump(int address, void *func)
	{
		unsigned char temp = 0xE9;
		Patch(address++, &temp, 1);
		RedirectOffset(address, func);
	}
	inline void RedirectJump(int address, int loc)
	{
		unsigned char temp = 0xE9;
		Patch(address++, &temp, 1);
		RedirectOffset(address, loc);
	}
	inline void RedirectShortJump(int address, int loc)
	{
		unsigned char temp = 0xEB;
		Patch(address++, &temp, 1);
		RedirectShortOffset(address, loc);
	}
	inline void SetChar(int address, char value)
	{
		Patch(address, &value, 1);
	}
	inline void SetUChar(int address, unsigned char value)
	{
		Patch(address, &value, 1);
	}
	inline void SetShort(int address, short value)
	{
		Patch(address, &value, 2);
	}
	inline void SetUShort(int address, unsigned short value)
	{
		Patch(address, &value, 2);
	}
	inline void SetInt(int address, int value)
	{
		Patch(address, &value, 4);
	}
	inline void SetUInt(int address, unsigned int value)
	{
		Patch(address, &value, 4);
	}
	inline void SetFloat(int address, float value)
	{
		Patch(address, &value, 4);
	}
	inline void SetPointer(int address, void *value)
	{
		Patch(address, &value, 4);
	}
	inline void SetPointer(int address, int value)
	{
		Patch(address, &value, 4);
	}
	inline void AdjustPointer(int address, void *value, int start, int end)
	{
		int lastaddr = address + 12, content, result;
		do
		{
			content = *(int *)address;
			if(content >= start && content < end)
			{
				result = (int)value + (content - start);
				Patch(address, &result, 4);
				break;
			}
		}
		while(++address < lastaddr);
	}
	inline void AdjustPointer(int address, void *value, int start, int end, int offset)
	{
		int lastaddr = address + 12, content, result;
		do
		{
			content = *(int *)address;
			if(content >= start && content < end)
			{
				result = (int)value + (content - start) + offset;
				Patch(address, &result, 4);
				break;
			}
		}
		while(++address < lastaddr);
	}
	inline void AdjustPointer(int address, void *value, int start, int end, int oldsize, int newsize)
	{
		int lastaddr = address + 12, content, result;
		do
		{
			content = *(int *)address;
			if(content >= start && content < end)
			{
				result = (int)value + (content - start) + (newsize - oldsize);
				Patch(address, &result, 4);
				break;
			}
		}
		while(++address < lastaddr);
	}
};

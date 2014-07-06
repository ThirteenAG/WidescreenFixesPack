#pragma once
#include <Windows.h>

class CPatch
{
public:
	inline static void Patch(void* address, void* data, int size)
	{
		unsigned long protect[2];
		VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &protect[0]);
		memcpy(address, data, size);
		VirtualProtect(address, size, protect[0], &protect[1]);
	}

	inline static void Patch2(int address, void* data, int size)
	{
		unsigned long protect[2];
		VirtualProtect((void *)address, size, PAGE_EXECUTE_READWRITE, &protect[0]);
		memcpy((void *)address, data, size);
		VirtualProtect((void *)address, size, protect[0], &protect[1]);
	}

	inline static void Unprotect(int address, int size)
	{
		unsigned long protect[2];
		VirtualProtect((void *)address, size, PAGE_EXECUTE_READWRITE, &protect[0]);
	}
	inline static void Nop(int address, int size)
	{
		unsigned long protect[2];
		VirtualProtect((void *)address, size, PAGE_EXECUTE_READWRITE, &protect[0]);
		memset((void *)address, 0x90, size);
		VirtualProtect((void *)address, size, protect[0], &protect[1]);
	}
	inline static void FillWithZeroes(int address, int size)
	{
		unsigned long protect[2];
		VirtualProtect((void *)address, size, PAGE_EXECUTE_READWRITE, &protect[0]);
		memset((void *)address, 0x00, size);
		VirtualProtect((void *)address, size, protect[0], &protect[1]);
	}
	inline static void RedirectCall(int address, void *func)
	{
		int temp = 0xE8;
		Patch((void *)address, &temp, 1);
		temp = (int)func - ((int)address + 5);
		Patch((void *)((int)address + 1), &temp, 4);
	}
	inline static void RedirectJump(int address, void *func)
	{
		int temp = 0xE9;
		Patch((void *)address, &temp, 1);
		temp = (int)func - ((int)address + 5);
		Patch((void *)((int)address + 1), &temp, 4);
	}
	inline static void SetChar(int address, char value)
	{
		Patch((void *)address, &value, 1);
	}
	inline static void SetUChar(int address, unsigned char value)
	{
		Patch((void *)address, &value, 1);
	}
	inline static void SetShort(int address, short value)
	{
		Patch((void *)address, &value, 2);
	}
	inline static void SetUShort(int address, unsigned short value)
	{
		Patch((void *)address, &value, 2);
	}
	inline static void SetInt(int address, int value)
	{
		Patch((void *)address, &value, 4);
	}
	inline static void SetUInt(int address, unsigned int value)
	{
		Patch((void *)address, &value, 4);
	}
	inline static void SetUIntWithCheck(int address, unsigned int value, unsigned int expectedValue)
	{
		if (*(unsigned int *)address == expectedValue)
		Patch((void *)address, &value, 4);
	}
	inline static void SetFloat(int address, float value)
	{
		Patch((void *)address, &value, 4);
	}
	inline static void SetDouble(int address, double value)
	{
		Patch((void *)address, &value, 8);
	}
	inline static void SetPointer(int address, void *value)
	{
		Patch((void *)address, &value, 4);
	}

	inline static void AdjustPointer(int address, void *value, DWORD offset, DWORD end)
	{
		int result;
		if((DWORD)*(DWORD*)address >= offset && (DWORD)*(DWORD*)address <= end) {
		result = (DWORD)value + (DWORD)*(DWORD*)address - (DWORD)offset;
		Patch((void *)address, &result, 4);
		} else {
		address = address + 1;
			if((DWORD)*(DWORD*)address >= offset && (DWORD)*(DWORD*)address <= end) {
			result = (DWORD)value + (DWORD)*(DWORD*)address - (DWORD)offset;
			Patch((void *)address, &result, 4);
			} else {
				address = address + 1;
				if((DWORD)*(DWORD*)address >= offset && (DWORD)*(DWORD*)address <= end) {
				result = (DWORD)value + (DWORD)*(DWORD*)address - (DWORD)offset;
				Patch((void *)address, &result, 4);
				} else {
					address = address + 1;
					if((DWORD)*(DWORD*)address >= offset && (DWORD)*(DWORD*)address <= end) {
				result = (DWORD)value + (DWORD)*(DWORD*)address - (DWORD)offset;
				Patch((void *)address, &result, 4);
				} else {
					address = address + 1;
					if((DWORD)*(DWORD*)address >= offset && (DWORD)*(DWORD*)address <= end) {
						result = (DWORD)value + (DWORD)*(DWORD*)address - (DWORD)offset;
						Patch((void *)address, &result, 4);
					} else {
						address = address + 1;
						if((DWORD)*(DWORD*)address >= offset && (DWORD)*(DWORD*)address <= end) {
							result = (DWORD)value + (DWORD)*(DWORD*)address - (DWORD)offset;
							Patch((void *)address, &result, 4);
						}
					}
					}
				}
			}
		} 
	}
	
	inline static bool FileExists(const TCHAR *fileName)
	{
    DWORD       fileAttr;
    fileAttr = GetFileAttributes(fileName);
    if (0xFFFFFFFF == fileAttr && GetLastError()==ERROR_FILE_NOT_FOUND)
        return false;
    return true;
	}
};
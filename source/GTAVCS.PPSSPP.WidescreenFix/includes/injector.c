#include "injector.h"

void WriteMemoryRaw(uintptr_t addr, void* value, size_t size)
{
    memcpy((addr + injector.base_addr), value, size);
}

void ReadMemoryRaw(uintptr_t addr, void* ret, size_t size)
{
    memcpy(ret, (addr + injector.base_addr), size);
}

void MemoryFill(uintptr_t addr, uint8_t value, size_t size)
{
    memset((addr + injector.base_addr), value, size);
}

void WriteMemory8(uintptr_t addr, uint8_t value)
{
    *(uint8_t*)(addr + injector.base_addr) = value;
}

void WriteMemory16(uintptr_t addr, uint16_t value)
{
    *(uint16_t*)(addr + injector.base_addr) = value;
}

void WriteMemory32(uintptr_t addr, uint32_t value)
{
    *(uint32_t*)(addr + injector.base_addr) = value;
}

void WriteMemory64(uintptr_t addr, uint64_t value)
{
    *(uint64_t*)(addr + injector.base_addr) = value;
}

void WriteMemoryFloat(uintptr_t addr, float value)
{
    *(float*)(addr + injector.base_addr) = value;
}

void WriteMemoryDouble(uintptr_t addr, double value)
{
    *(double*)(addr + injector.base_addr) = value;
}

uint8_t ReadMemory8(uintptr_t addr)
{
    return *(uint8_t*)(addr + injector.base_addr);
}

uint16_t ReadMemory16(uintptr_t addr)
{
    return *(uint16_t*)(addr + injector.base_addr);
}

uint32_t ReadMemory32(uintptr_t addr)
{
    return *(uint32_t*)(addr + injector.base_addr);
}

uint64_t ReadMemory64(uintptr_t addr)
{
    return *(uint64_t*)(addr + injector.base_addr);
}

float ReadMemoryFloat(uintptr_t addr)
{
    return *(float*)(addr + injector.base_addr);
}

double ReadMemoryDouble(uintptr_t addr)
{
    return *(double*)(addr + injector.base_addr);
}

uintptr_t GetAbsoluteOffset(int rel_value, uintptr_t start_of_instruction)
{
    return start_of_instruction + rel_value;
}

int GetRelativeOffset(uintptr_t abs_value, uintptr_t end_of_instruction)
{
    return (uintptr_t)(abs_value - end_of_instruction);
}

uintptr_t ReadRelativeOffset(uintptr_t at)
{
    return (GetAbsoluteOffset(ReadMemory32(at) & 0x03ffffff, at));
}

void MakeRelativeOffset(uintptr_t at, uintptr_t dest)
{
    return; //TODO
}

uintptr_t GetBranchDestination(uintptr_t at)
{
    return ReadRelativeOffset(at);
}

void MakeJMP(uintptr_t at, uintptr_t dest)
{
    WriteMemory32(at + injector.base_addr, (0x08000000 | (((u32)(dest) & 0x0FFFFFFC) >> 2)));
}

void MakeJAL(uintptr_t at, uintptr_t dest)
{
    WriteMemory32(at + injector.base_addr, (0x0C000000 | ((dest) >> 2)));
}

void MakeCALL(uintptr_t at, uintptr_t dest)
{
    WriteMemory32(at + injector.base_addr, (0x0C000000 | (((unsigned int)(dest) >> 2) & 0x03FFFFFF)));
}

void MakeNOP(uintptr_t at)
{
    MemoryFill(at, 0x00, 4);
}

void MakeNOPWithSize(uintptr_t at, size_t size)
{
    MemoryFill(at, 0x00, size);
}

void MakeRangedNOP(uintptr_t at, uintptr_t until)
{
    return MakeNOPWithSize(at, (size_t)(until - at));
}

struct injector_t injector =
{
    .base_addr = 0,
    .WriteMemoryRaw = WriteMemoryRaw,
    .ReadMemoryRaw = ReadMemoryRaw,
    .MemoryFill = MemoryFill,
    .WriteMemory8 = WriteMemory8,
    .WriteMemory16 = WriteMemory16,
    .WriteMemory32 = WriteMemory32,
    .WriteMemory64 = WriteMemory64,
    .WriteMemoryFloat = WriteMemoryFloat,
    .WriteMemoryDouble = WriteMemoryDouble,
    .ReadMemory8 = ReadMemory8,
    .ReadMemory16 = ReadMemory16,
    .ReadMemory32 = ReadMemory32,
    .ReadMemory64 = ReadMemory64,
    .ReadMemoryFloat = ReadMemoryFloat,
    .ReadMemoryDouble = ReadMemoryDouble,
    .GetAbsoluteOffset = GetAbsoluteOffset,
    .GetRelativeOffset = GetRelativeOffset,
    .ReadRelativeOffset = ReadRelativeOffset,
    .MakeRelativeOffset = MakeRelativeOffset,
    .GetBranchDestination = GetBranchDestination,
    .MakeJMP = MakeJMP,
    .MakeJAL = MakeJAL,
    .MakeCALL = MakeCALL,
    .MakeNOP = MakeNOP,
    .MakeNOPWithSize = MakeNOPWithSize,
    .MakeRangedNOP = MakeRangedNOP
};
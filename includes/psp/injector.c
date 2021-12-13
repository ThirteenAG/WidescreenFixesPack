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

void WriteInstr(uintptr_t addr, uint32_t value)
{
    *(uint32_t*)(addr + injector.base_addr) = value;
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
    WriteMemory32(at, (0x08000000 | (((u32)(dest) & 0x0FFFFFFC) >> 2)));
}

void MakeJAL(uintptr_t at, uintptr_t dest)
{
    WriteMemory32(at, (0x0C000000 | ((dest) >> 2)));
}

void MakeCALL(uintptr_t at, uintptr_t dest)
{
    WriteMemory32(at, (0x0C000000 | (((unsigned int)(dest) >> 2) & 0x03FFFFFF)));
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

void MakeInline(uintptr_t at, uintptr_t dest)
{
    injector.MakeJAL(at, dest);
}

uintptr_t MakeCallStub(uintptr_t numInstr) {
    SceUID block_id = sceKernelAllocPartitionMemory(PSP_MEMORY_PARTITION_USER, "", PSP_SMEM_High, numInstr * sizeof(uintptr_t), NULL);
    uintptr_t stub = (uintptr_t)sceKernelGetBlockHeadAddr(block_id);
    return stub;
}

void MakeLUIORI(uintptr_t at, RegisterID reg, float imm)
{
    uintptr_t functor = MakeCallStub(4); // lui ori j nop
    injector.WriteMemory32(functor + 0 - injector.base_addr, lui(reg, HIWORD(imm)));
    injector.WriteMemory32(functor + 4 - injector.base_addr, ori(reg, reg, LOWORD(imm)));
    injector.MakeJMP(functor + 8 - injector.base_addr, at + 4 + injector.base_addr);
    injector.MakeNOP(functor + 12 - injector.base_addr);
    injector.MakeJMP(at, functor);
}

void MakeLI(uintptr_t at, RegisterID reg, int32_t imm)
{
    uintptr_t functor = MakeCallStub(4); // lui ori j nop
    injector.WriteMemory32(functor + 0 - injector.base_addr, lui(reg, HIWORD(imm)));
    injector.WriteMemory32(functor + 4 - injector.base_addr, ori(reg, reg, LOWORD(imm)));
    injector.MakeJMP(functor + 8 - injector.base_addr, at + 4 + injector.base_addr);
    injector.MakeNOP(functor + 12 - injector.base_addr);
    injector.MakeJMP(at, functor);
}

uint32_t parseCommand(uint32_t command, uint32_t from, uint32_t to)
{
    uint32_t mask = ((1 << (to - from + 1)) - 1) << from;
    return (command & mask) >> from;
}

void MakeInlineLUIORI(uintptr_t at, float imm)
{
    static const uint32_t instr_len = 4;

    uint8_t LUI = parseCommand(lui(0, 0), 26, 32);
    uint8_t ORI = parseCommand(ori(0, 0, 0), 26, 32);

    uint32_t prev_instr = parseCommand(ReadMemory32(at - instr_len), 26, 32);
    uint32_t bytes = ReadMemory32(at);
    uint8_t instr = parseCommand(bytes, 26, 32);
    uint8_t reg_lui = parseCommand(bytes, 16, 21);

    if (instr == LUI)
    {
        for (uintptr_t i = at + instr_len; i <= (at + instr_len + (5 * instr_len)); i += instr_len)
        {
            bytes = ReadMemory32(i);
            instr = parseCommand(bytes, 26, 32);
            uint8_t reg_ori = parseCommand(bytes, 16, 21);

            if (instr == LUI)
                break;
            else if (instr == ORI && reg_lui == reg_ori)
            {
                if ((prev_instr == 0x01) || (prev_instr >= 0x04 && prev_instr <= 0x07) || (prev_instr >= 0x14 && prev_instr <= 0x17)) //beq and such
                {
                    WriteMemory16(i, LOWORD(imm));
                    break;
                }
                else
                    return MakeLUIORI(i, reg_ori, imm);
            }
        }
        if ((prev_instr >= 0x01 && prev_instr <= 0x07) || (prev_instr >= 0x14 && prev_instr <= 0x17)) //beq and such
            WriteMemory16(at, HIWORD(imm));
        else
            return MakeLUIORI(at, reg_lui, imm);
    }
}

void MakeInlineLI(uintptr_t at, int32_t imm)
{
    static const uint32_t instr_len = 4;

    uint8_t ORI = parseCommand(ori(0, 0, 0), 26, 32);
    uint8_t ZERO = parseCommand(ori(0, 0, 0), 21, 26);

    uint32_t prev_instr = parseCommand(ReadMemory32(at - instr_len), 26, 32);
    uint32_t bytes = ReadMemory32(at);
    uint8_t instr = parseCommand(bytes, 26, 32);
    uint8_t reg_ori = parseCommand(bytes, 16, 21);
    uint8_t reg_zero = parseCommand(bytes, 21, 26);

    if (instr == ORI && reg_zero == ZERO)
    {
        if ((prev_instr >= 0x01 && prev_instr <= 0x07) || (prev_instr >= 0x14 && prev_instr <= 0x17)) //beq and such
            return WriteMemory16(at, LOWORD(imm));
        else
            return MakeLI(at, reg_ori, imm);
    }
}

struct injector_t injector =
{
    .base_addr = 0,
    .WriteMemoryRaw = WriteMemoryRaw,
    .ReadMemoryRaw = ReadMemoryRaw,
    .MemoryFill = MemoryFill,
    .WriteInstr = WriteInstr,
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
    .MakeRangedNOP = MakeRangedNOP,
    .MakeInline = MakeInline,
    .MakeInlineLUIORI = MakeInlineLUIORI,
    .MakeInlineLI = MakeInlineLI
};
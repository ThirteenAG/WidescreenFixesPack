#include "injector.h"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

void SetModuleBaseAddress(uintptr_t addr, size_t size)
{
    injector.module_addr = addr;
    injector.module_size = size;
}

void SetGameBaseAddress(uintptr_t addr, size_t size)
{
    injector.base_addr = addr;
    injector.base_size = size;
}

uintptr_t adjustAddress(uintptr_t addr)
{
    if (addr >= injector.base_addr && addr < injector.base_addr + injector.base_size)
        return addr;
    else if (addr >= injector.module_addr && addr < injector.module_addr + injector.module_size)
        return addr;
    else
    {
        uintptr_t m = MIN(injector.base_addr, injector.module_addr);
        if (addr >= m)
            return addr;
        else
            return addr + injector.base_addr;
    }
}

void WriteMemoryRaw(uintptr_t addr, void* value, size_t size)
{
    memcpy(adjustAddress(addr), value, size);
}

void ReadMemoryRaw(uintptr_t addr, void* ret, size_t size)
{
    memcpy(ret, adjustAddress(addr), size);
}

void MemoryFill(uintptr_t addr, uint8_t value, size_t size)
{
    memset(adjustAddress(addr), value, size);
}

void WriteInstr(uintptr_t addr, uint32_t value)
{
    *(uint32_t*)(adjustAddress(addr)) = value;
}

void WriteMemory8(uintptr_t addr, uint8_t value)
{
    *(uint8_t*)(adjustAddress(addr)) = value;
}

void WriteMemory16(uintptr_t addr, uint16_t value)
{
    *(uint16_t*)(adjustAddress(addr)) = value;
}

void WriteMemory32(uintptr_t addr, uint32_t value)
{
    *(uint32_t*)(adjustAddress(addr)) = value;
}

void WriteMemory64(uintptr_t addr, uint64_t value)
{
    *(uint64_t*)(adjustAddress(addr)) = value;
}

void WriteMemoryFloat(uintptr_t addr, float value)
{
    *(float*)(adjustAddress(addr)) = value;
}

void WriteMemoryDouble(uintptr_t addr, double value)
{
    *(double*)(adjustAddress(addr)) = value;
}

uint8_t ReadMemory8(uintptr_t addr)
{
    return *(uint8_t*)(adjustAddress(addr));
}

uint16_t ReadMemory16(uintptr_t addr)
{
    return *(uint16_t*)(adjustAddress(addr));
}

uint32_t ReadMemory32(uintptr_t addr)
{
    return *(uint32_t*)(adjustAddress(addr));
}

uint64_t ReadMemory64(uintptr_t addr)
{
    return *(uint64_t*)(adjustAddress(addr));
}

float ReadMemoryFloat(uintptr_t addr)
{
    return *(float*)(adjustAddress(addr));
}

double ReadMemoryDouble(uintptr_t addr)
{
    return *(double*)(adjustAddress(addr));
}

void MakeJMP(uintptr_t at, uintptr_t dest)
{
    WriteMemory32(adjustAddress(at), (0x08000000 | ((adjustAddress(dest) & 0x0FFFFFFC) >> 2)));
}

void MakeJAL(uintptr_t at, uintptr_t dest)
{
    WriteMemory32(adjustAddress(at), (0x0C000000 | ((adjustAddress(dest)) >> 2)));
}

void MakeCALL(uintptr_t at, uintptr_t dest)
{
    WriteMemory32(adjustAddress(at), (0x0C000000 | (((adjustAddress(dest)) >> 2) & 0x03FFFFFF)));
}

void MakeNOP(uintptr_t at)
{
    MemoryFill(adjustAddress(at), 0x00, 4);
}

void MakeNOPWithSize(uintptr_t at, size_t size)
{
    MemoryFill(adjustAddress(at), 0x00, size);
}

void MakeRangedNOP(uintptr_t at, uintptr_t until)
{
    return MakeNOPWithSize(adjustAddress(at), (size_t)(until - at));
}

uintptr_t MakeInline(size_t instrCount, uintptr_t at, ...)
{
    uintptr_t functor = MakeCallStub(instrCount * 4 + 8);
    size_t i;

    va_list ap;
    va_start(ap, instrCount);
    for (i = 0; i < instrCount; i++) {
        uint32_t arg = va_arg(ap, uint32_t);
        WriteInstr(functor + i * 4, arg);
    }
    va_end(ap);

    injector.MakeJMP(functor + i * 4, at + 4);
    i++;
    injector.MakeNOP(functor + i * 4);
    injector.MakeJMP(at, functor);
    return functor;
}

uintptr_t MakeCallStub(uintptr_t numInstr) {
    SceUID block_id = sceKernelAllocPartitionMemory(PSP_MEMORY_PARTITION_USER, "", PSP_SMEM_High, numInstr * sizeof(uintptr_t), NULL);
    uintptr_t stub = (uintptr_t)sceKernelGetBlockHeadAddr(block_id);
    return stub;
}

void MakeLUIORI(uintptr_t at, RegisterID reg, float imm)
{
    uintptr_t functor = MakeCallStub(4); // lui ori j nop
    injector.WriteMemory32(functor + 0, lui(reg, HIWORD(imm)));
    injector.WriteMemory32(functor + 4, ori(reg, reg, LOWORD(imm)));
    injector.MakeJMP(functor + 8, at + 4);
    injector.MakeNOP(functor + 12);
    injector.MakeJMP(at, functor);
}

void MakeLI(uintptr_t at, RegisterID reg, int32_t imm)
{
    uintptr_t functor = MakeCallStub(4); // lui ori j nop
    injector.WriteMemory32(functor + 0, lui(reg, HIWORD(imm)));
    injector.WriteMemory32(functor + 4, ori(reg, reg, LOWORD(imm)));
    injector.MakeJMP(functor + 8, at + 4);
    injector.MakeNOP(functor + 12);
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
    .base_size = 0,
    .module_addr = 0,
    .module_size = 0,
    .SetModuleBaseAddress = SetModuleBaseAddress,
    .SetGameBaseAddress = SetGameBaseAddress,
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
/*
 *  Injectors - Base Header
 *
 *  Copyright (C) 2012-2014 LINK/2012 <dma_2012@hotmail.com>
 *
 *  This software is provided 'as-is', without any express or implied
 *  warranty. In no event will the authors be held liable for any damages
 *  arising from the use of this software.
 *
 *  Permission is granted to anyone to use this software for any purpose,
 *  including commercial applications, and to alter it and redistribute it
 *  freely, subject to the following restrictions:
 *
 *     1. The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software. If you use this software
 *     in a product, an acknowledgment in the product documentation would be
 *     appreciated but is not required.
 *
 *     2. Altered source versions must be plainly marked as such, and must not be
 *     misrepresented as being the original software.
 *
 *     3. This notice may not be removed or altered from any source
 *     distribution.
 *
 */
#pragma once
#include <pspsdk.h>
#include <pspkernel.h>
#include <stdint.h>
#include <string.h>
#include "mips.h"

#define LOWORD(l)           ((uint16_t)((*(uint32_t*)(&l)) & 0xffff))
#define HIWORD(l)           ((uint16_t)(((*(uint32_t*)(&l)) >> 16) & 0xffff))

uintptr_t MakeCallStub(uintptr_t numInstr);

struct injector_t {
    uintptr_t base_addr;
    void (*WriteMemoryRaw)(uintptr_t addr, void* value, size_t size);
    void (*ReadMemoryRaw)(uintptr_t addr, void* ret, size_t size);
    void (*MemoryFill)(uintptr_t addr, uint8_t value, size_t size);
    void (*WriteInstr)(uintptr_t addr, uint32_t value);
    void (*WriteMemory8)(uintptr_t addr, uint8_t value);
    void (*WriteMemory16)(uintptr_t addr, uint16_t value);
    void (*WriteMemory32)(uintptr_t addr, uint32_t value);
    void (*WriteMemory64)(uintptr_t addr, uint64_t value);
    void (*WriteMemoryFloat)(uintptr_t addr, float value);
    void (*WriteMemoryDouble)(uintptr_t addr, double value);
    uint8_t(*ReadMemory8)(uintptr_t addr);
    uint16_t(*ReadMemory16)(uintptr_t addr);
    uint32_t(*ReadMemory32)(uintptr_t addr);
    uint64_t(*ReadMemory64)(uintptr_t addr);
    float (*ReadMemoryFloat)(uintptr_t addr);
    double (*ReadMemoryDouble)(uintptr_t addr);
    uintptr_t(*GetAbsoluteOffset)(int rel_value, uintptr_t start_of_instruction);
    int (*GetRelativeOffset)(uintptr_t abs_value, uintptr_t end_of_instruction);
    uintptr_t(*ReadRelativeOffset)(uintptr_t at);
    void (*MakeRelativeOffset)(uintptr_t at, uintptr_t dest);
    uintptr_t(*GetBranchDestination)(uintptr_t at);
    void (*MakeJMP)(uintptr_t at, uintptr_t dest);
    void (*MakeJAL)(uintptr_t at, uintptr_t dest);
    void (*MakeCALL)(uintptr_t at, uintptr_t dest);
    void (*MakeNOP)(uintptr_t at);
    void (*MakeNOPWithSize)(uintptr_t at, size_t count);
    void (*MakeRangedNOP)(uintptr_t at, uintptr_t until);
    void (*MakeInline)(uintptr_t at, uintptr_t functor);
    void (*MakeInlineLUIORI)(uintptr_t at, float imm);
    void (*MakeInlineLI)(uintptr_t at, int32_t imm);
};

extern struct injector_t injector;
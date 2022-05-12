#pragma once
#include <pspsdk.h>
#include <pspkernel.h>
#include <pspmodulemgr.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include "mips.h"

#define LOWORD(l)           ((uint16_t)((*(uint32_t*)(&l)) & 0xffff))
#define HIWORD(l)           ((uint16_t)(((*(uint32_t*)(&l)) >> 16) & 0xffff))

uintptr_t MakeCallStub(uintptr_t numInstr);

struct injector_t {
    uintptr_t base_addr;
    size_t base_size;
    uintptr_t module_addr;
    size_t module_size;
    int  (*AllocMemBlock)(int size, int* id);
    void (*FreeMemBlock)();
    void*(*GetGP)();
    void (*SetModuleBaseAddress)(uintptr_t addr, size_t size);
    void (*SetGameBaseAddress)(uintptr_t addr, size_t size);
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
    void (*MakeJMP)(uintptr_t at, uintptr_t dest);
    void (*MakeJMPwNOP)(uintptr_t at, uintptr_t dest);
    void (*MakeJAL)(uintptr_t at, uintptr_t dest);
    void (*MakeCALL)(uintptr_t at, uintptr_t dest);
    void (*MakeNOP)(uintptr_t at);
    void (*MakeNOPWithSize)(uintptr_t at, size_t count);
    void (*MakeRangedNOP)(uintptr_t at, uintptr_t until);
    uintptr_t(*MakeInline)(size_t instrCount, uintptr_t at, ...);
    void (*MakeInlineLUIORI)(uintptr_t at, float imm);
    void (*MakeInlineLI)(uintptr_t at, int32_t imm);
};

extern struct injector_t injector;

#ifndef __INTELLISENSE__
#define PP_NARG(...) \
         PP_NARG_(__VA_ARGS__,PP_RSEQ_N())
#define PP_NARG_(...) \
         PP_128TH_ARG(__VA_ARGS__)
#define PP_128TH_ARG( \
          _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
         _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
         _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
         _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
         _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
         _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
         _61,_62,_63,_64,_65,_66,_67,_68,_69,_70, \
         _71,_72,_73,_74,_75,_76,_77,_78,_79,_80, \
         _81,_82,_83,_84,_85,_86,_87,_88,_89,_90, \
         _91,_92,_93,_94,_95,_96,_97,_98,_99,_100, \
         _101,_102,_103,_104,_105,_106,_107,_108,_109,_110, \
         _111,_112,_113,_114,_115,_116,_117,_118,_119,_120, \
         _121,_122,_123,_124,_125,_126,_127,N,...) N
#define PP_RSEQ_N() \
         127,126,125,124,123,122,121,120, \
         119,118,117,116,115,114,113,112,111,110, \
         109,108,107,106,105,104,103,102,101,100, \
         99,98,97,96,95,94,93,92,91,90, \
         89,88,87,86,85,84,83,82,81,80, \
         79,78,77,76,75,74,73,72,71,70, \
         69,68,67,66,65,64,63,62,61,60, \
         59,58,57,56,55,54,53,52,51,50, \
         49,48,47,46,45,44,43,42,41,40, \
         39,38,37,36,35,34,33,32,31,30, \
         29,28,27,26,25,24,23,22,21,20, \
         19,18,17,16,15,14,13,12,11,10, \
         9,8,7,6,5,4,3,2,1,0
#endif

#define MakeInlineWrapper(at, ...) MakeInline(PP_NARG(__VA_ARGS__), at, __VA_ARGS__)
#define MakeInlineWrapperWithNOP(at, ...) MakeNOP(at + 4); MakeInline(PP_NARG(__VA_ARGS__), at, __VA_ARGS__)

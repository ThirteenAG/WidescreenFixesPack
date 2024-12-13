#pragma once
#include <stdafx.h>
#include "ppc.hpp"
#include <utility/Scan.hpp>

class Dolphin
{
public:
    static inline uintptr_t GameMemoryStart = 0;
    static inline uintptr_t GameMemoryEnd = 0;
    static inline uintptr_t ImageBase = 0;
private:
    static inline auto _SetIsThrottlerTempDisabled = (void(__fastcall*)(bool disable))(nullptr);
    static inline auto _MenuBarClearCache = (void(__fastcall*)())(nullptr);

public:
    //static hook::pattern Pattern()
    //{
    //    return hook::pattern("0F B6 C8 E8 ? ? ? ? 33 D2");
    //}

    static inline void SetIsThrottlerTempDisabled(bool disable)
    {
        __try
        {
            [&disable]()
            {
                if (!_SetIsThrottlerTempDisabled)
                {
                    const auto current_module = GetModuleHandleW(NULL);
                    auto candidate_string = utility::scan_string(current_module, "Fog: {}");
                    if (!candidate_string) candidate_string = utility::scan_string(current_module, "Copy EFB: {}");
                    if (candidate_string)
                    {
                        auto candidate_stringref = utility::scan_displacement_reference(current_module, *candidate_string);
                        if (candidate_stringref)
                        {
                            for (size_t i = 0; i < 8000; ++i)
                            {
                                const auto mov = utility::scan_mnemonic(*candidate_stringref + i, 5, "MOV");
                                if (mov)
                                {
                                    if (injector::ReadMemory<uint32_t>(*mov + 1, true) == 19 || injector::ReadMemory<uint32_t>(*mov + 1, true) == 20)
                                    {
                                        const auto next_fn_call1 = utility::scan_mnemonic(*mov, 100, "CALL");
                                        if (next_fn_call1)
                                        {
                                            const auto next_fn_call2 = utility::scan_mnemonic(*next_fn_call1 + 5, 100, "CALL");
                                            if (next_fn_call2)
                                            {
                                                const auto next_mov = utility::scan_mnemonic(*next_fn_call2, 100, "MOV");
                                                if (next_mov)
                                                {
                                                    if (injector::ReadMemory<uint8_t>(*next_mov + 1, true) == 1 || injector::ReadMemory<uint32_t>(*next_mov + 1, true) == 17 || injector::ReadMemory<uint32_t>(*next_mov + 1, true) == 18)
                                                    {
                                                        _SetIsThrottlerTempDisabled = (void(__fastcall*)(bool disable))(injector::GetBranchDestination(*next_fn_call2).as_int());
                                                        return _SetIsThrottlerTempDisabled(disable);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                else
                    return _SetIsThrottlerTempDisabled(disable);
            }();
        }
        __except ((GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
        }
    }

    static inline void MenuBarClearCache()
    {
        __try
        {
            []()
            {
                if (!_MenuBarClearCache)
                {
                    const auto current_module = GetModuleHandleW(NULL);
                    const auto candidate_string = utility::scan_string(current_module, "Clear Cache");
                    if (candidate_string)
                    {
                        auto candidate_stringref = utility::scan_displacement_references(current_module, *candidate_string);
                        if (!candidate_stringref.empty())
                        {
                            auto ref = candidate_stringref.back();
                            ref -= 4;
                            for (size_t i = 0; i < 100; ++i)
                            {
                                const auto disp = utility::resolve_displacement(ref - i);
                                if (disp)
                                {
                                    _MenuBarClearCache = (void(__fastcall*)())*disp;
                                    return _MenuBarClearCache();
                                }
                            }
                        }
                    }
                }
                else
                    return _MenuBarClearCache();
            }();
        }
        __except ((GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
        }
    }

    static inline void FindEmulatorMemory()
    {
        uintptr_t curAddr = 0;
        MEMORY_BASIC_INFORMATION MemoryInf;
        while (VirtualQuery((LPCVOID)curAddr, &MemoryInf, sizeof(MemoryInf)))
        {
            if (MemoryInf.AllocationProtect == PAGE_READWRITE && MemoryInf.State == MEM_COMMIT &&
                MemoryInf.Protect == PAGE_READWRITE && MemoryInf.Type == MEM_MAPPED && MemoryInf.RegionSize == 0x2000000)
            {
                if (GameMemoryStart == 0)
                {
                    GameMemoryStart = (uintptr_t)MemoryInf.BaseAddress;
                    GameMemoryEnd = GameMemoryStart + 0x01800000;
                    ImageBase = 0x80000000;
                    break;
                }
            }
            curAddr += MemoryInf.RegionSize;
        }
    }

    static inline bool MemoryValid()
    {
        static MEMORY_BASIC_INFORMATION MemoryInf;
        if (GameMemoryStart == 0 || VirtualQuery((LPCVOID)GameMemoryStart, &MemoryInf, sizeof(MemoryInf)) == 0 || MemoryInf.AllocationProtect != PAGE_READWRITE)
        {
            GameMemoryStart = 0;
            FindEmulatorMemory();
            return false;
        }
        return true;
    }

    template <typename T>
    static inline bool MemoryAddrValid(T addr)
    {
        static MEMORY_BASIC_INFORMATION MemoryInf;
        if (addr == 0 || VirtualQuery((LPCVOID)addr, &MemoryInf, sizeof(MemoryInf)) == 0 || MemoryInf.AllocationProtect != PAGE_READWRITE)
            return false;
        return true;
    }

    static inline std::string_view GameID()
    {
        static auto default_id = "";
        if (MemoryAddrValid(GameMemoryStart))
            return std::string_view(reinterpret_cast<char*>(GameMemoryStart));
        else
            return default_id;
    }
};

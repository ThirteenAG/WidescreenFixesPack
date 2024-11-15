#pragma once
#define _WINTERNL_
#include <utility/Scan.hpp>
#include <utility/Module.hpp>

namespace utility {
    static inline std::vector<uintptr_t> scan_references(uintptr_t start, size_t length, uintptr_t ptr)
    {
        std::vector<uintptr_t> results{};
        const auto end = (start + length) - sizeof(void*);

        for (auto ref = utility::scan_ptr(start, length, ptr); ref; ref = utility::scan_ptr(*ref + sizeof(void*), end - (*ref + sizeof(void*)), ptr)) {
            results.push_back(*ref);
        }

        return results;
    }

    static inline std::vector<uintptr_t> scan_references(HMODULE module, uintptr_t ptr)
    {
        const auto module_size = utility::get_module_size(module);

        if (!module_size) {
            return {};
        }

        return scan_references((uintptr_t)module, *module_size, ptr);
    }

    template<class T, class S, class UnaryPred>
    static T GetSymbolFromFuncStart(S& str, UnaryPred predicate)
    {
        auto func = utility::find_function_from_string_ref(utility::get_executable(), str, true);
        if (func)
        {
            INSTRUX ix{};
            auto ip = utility::find_function_start_with_call(func.value()).value_or(func.value());

            while (ip)
            {
                const auto status = NdDecodeEx(&ix, (ND_UINT8*)ip, 1000, ND_CODE_64, ND_DATA_64);

                if (!ND_SUCCESS(status)) {
                    break;
                }

                if (predicate(ix))
                {
                    auto disp = utility::resolve_displacement(ip);
                    if (disp)
                    {
                        return reinterpret_cast<T>(disp.value());
                        break;
                    }
                }

                ip += ix.Length;
            }
        }
        return T{};
    }

    template<class T, class S>
    static T GetFunctionCallAfterStringParam(S& str, int callnum = 0)
    {
        auto scan_res = utility::scan_string(utility::get_executable(), str, true);
        if (scan_res)
        {
            auto xref = utility::scan_reference(utility::get_executable(), scan_res.value());
            if (xref)
            {
                INSTRUX ix{};
                auto ip = xref.value() + 4;
                auto counter = 0;

                while (true)
                {
                    const auto status = NdDecodeEx(&ix, (ND_UINT8*)ip, 1000, ND_CODE_64, ND_DATA_64);

                    if (!ND_SUCCESS(status)) {
                        break;
                    }

                    if (ix.Instruction == ND_INS_CALLNR)
                    {
                        auto disp = utility::resolve_displacement(ip);
                        if (disp && counter == callnum)
                        {
                            return reinterpret_cast<T>(disp.value());
                            break;
                        }
                        counter++;
                    }

                    ip += ix.Length;
                }
            }
        }
        return T{};
    }
}
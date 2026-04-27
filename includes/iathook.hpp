#pragma once
#include <windows.h>
#include <string>
#include <map>
#include <future>
#include <tuple>
#include <string_view>
#include <thread>
#include <chrono>

class IATHook
{
public:
    template <class... Ts>
    static auto Replace(HMODULE target_module, std::string_view dll_name, Ts&& ... inputs)
    {
        std::map<std::string, std::future<void*>> originalPtrs;

        const DWORD_PTR instance = reinterpret_cast<DWORD_PTR>(target_module);
        const PIMAGE_NT_HEADERS ntHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(instance + reinterpret_cast<PIMAGE_DOS_HEADER>(instance)->e_lfanew);
        PIMAGE_IMPORT_DESCRIPTOR pImports = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(instance + ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
        DWORD dwProtect[2];

        // Regular imports
        for (; pImports->Name != 0; pImports++)
        {
            if (_stricmp(reinterpret_cast<const char*>(instance + pImports->Name), dll_name.data()) == 0)
            {
                if (pImports->OriginalFirstThunk != 0)
                {
                    const PIMAGE_THUNK_DATA pThunk = reinterpret_cast<PIMAGE_THUNK_DATA>(instance + pImports->OriginalFirstThunk);

                    for (ptrdiff_t j = 0; pThunk[j].u1.AddressOfData != 0; j++)
                    {
                        auto pAddress = reinterpret_cast<void**>(instance + pImports->FirstThunk) + j;
                        if (!pAddress) continue;
                        VirtualProtect(pAddress, sizeof(void*), PAGE_EXECUTE_READWRITE, &dwProtect[0]);
                        ([&]
                        {
                            auto name = std::string_view(std::get<0>(inputs));
                            auto num = std::string("-1");
                            if (name.contains("@"))
                            {
                                num = name.substr(name.find_last_of("@") + 1);
                                name = name.substr(0, name.find_last_of("@"));
                            }

                            if (pThunk[j].u1.Ordinal & IMAGE_ORDINAL_FLAG)
                            {
                                try
                                {
                                    if (IMAGE_ORDINAL(pThunk[j].u1.Ordinal) == std::stoi(num.data()))
                                    {
                                        originalPtrs[std::get<0>(inputs)] = std::async(std::launch::deferred, [&]() -> void* { return *pAddress; });
                                        originalPtrs[std::get<0>(inputs)].wait();
                                        *pAddress = std::get<1>(inputs);
                                    }
                                } catch (...) {}
                            }
                            else if ((*pAddress && *pAddress == (void*)GetProcAddress(GetModuleHandleA(dll_name.data()), name.data())) ||
                            (strcmp(reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(instance + pThunk[j].u1.AddressOfData)->Name, name.data()) == 0))
                            {
                                originalPtrs[std::get<0>(inputs)] = std::async(std::launch::deferred, [&]() -> void* { return *pAddress; });
                                originalPtrs[std::get<0>(inputs)].wait();
                                *pAddress = std::get<1>(inputs);
                            }
                        } (), ...);
                        VirtualProtect(pAddress, sizeof(void*), dwProtect[0], &dwProtect[1]);
                    }
                }
                else
                {
                    auto pFunctions = reinterpret_cast<void**>(instance + pImports->FirstThunk);

                    for (ptrdiff_t j = 0; pFunctions[j] != nullptr; j++)
                    {
                        auto pAddress = &pFunctions[j];
                        VirtualProtect(pAddress, sizeof(void*), PAGE_EXECUTE_READWRITE, &dwProtect[0]);
                        ([&]
                        {
                            if (*pAddress && *pAddress == (void*)GetProcAddress(GetModuleHandleA(dll_name.data()), std::get<0>(inputs)))
                            {
                                originalPtrs[std::get<0>(inputs)] = std::async(std::launch::deferred, [&]() -> void* { return *pAddress; });
                                originalPtrs[std::get<0>(inputs)].wait();
                                *pAddress = std::get<1>(inputs);
                            }
                        } (), ...);
                        VirtualProtect(pAddress, sizeof(void*), dwProtect[0], &dwProtect[1]);
                    }
                }
            }
        }

        // Delay imports
        PIMAGE_DELAYLOAD_DESCRIPTOR pDelayed = reinterpret_cast<PIMAGE_DELAYLOAD_DESCRIPTOR>(instance + ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].VirtualAddress);
        if (pDelayed && ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].VirtualAddress != 0 && ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].Size != 0)
        {
            for (; pDelayed->DllNameRVA != 0; pDelayed++)
            {
                if (_stricmp(reinterpret_cast<const char*>(instance + pDelayed->DllNameRVA), dll_name.data()) == 0)
                {
                    if (pDelayed->ImportAddressTableRVA != 0)
                    {
                        const PIMAGE_THUNK_DATA pThunk = reinterpret_cast<PIMAGE_THUNK_DATA>(instance + pDelayed->ImportNameTableRVA);
                        const PIMAGE_THUNK_DATA pFThunk = reinterpret_cast<PIMAGE_THUNK_DATA>(instance + pDelayed->ImportAddressTableRVA);

                        for (ptrdiff_t j = 0; pThunk[j].u1.AddressOfData != 0; j++)
                        {
                            auto pAddress = reinterpret_cast<void**>(&pFThunk[j].u1.Function);
                            if (!pAddress) continue;
                            VirtualProtect(pAddress, sizeof(void*), PAGE_EXECUTE_READWRITE, &dwProtect[0]);
                            ([&]
                            {
                                auto name = std::string_view(std::get<0>(inputs));
                                auto num = std::string("-1");
                                if (name.contains("@"))
                                {
                                    num = name.substr(name.find_last_of("@") + 1);
                                    name = name.substr(0, name.find_last_of("@"));
                                }

                                if (pThunk[j].u1.Ordinal & IMAGE_ORDINAL_FLAG)
                                {
                                    try
                                    {
                                        if (IMAGE_ORDINAL(pThunk[j].u1.Ordinal) == std::stoi(num.data()))
                                        {
                                            originalPtrs[std::get<0>(inputs)] = std::async(std::launch::async,
                                            [](void** pAddress, void* value, PVOID instance) -> void*
                                            {
                                                DWORD dwProtect[2];
                                                VirtualProtect(pAddress, sizeof(void*), PAGE_EXECUTE_READWRITE, &dwProtect[0]);
                                                MEMORY_BASIC_INFORMATION mbi;
                                                mbi.AllocationBase = instance;
                                                do
                                                {
                                                    VirtualQuery(*pAddress, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
                                                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                                                } while (mbi.AllocationBase == instance);
                                                auto r = *pAddress;
                                                *pAddress = value;
                                                VirtualProtect(pAddress, sizeof(void*), dwProtect[0], &dwProtect[1]);
                                                return r;
                                            }, pAddress, std::get<1>(inputs), (PVOID)instance);
                                        }
                                    } catch (...) {}
                                }
                                else if (strcmp(reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(instance + pThunk[j].u1.AddressOfData)->Name, name.data()) == 0)
                                {
                                    originalPtrs[std::get<0>(inputs)] = std::async(std::launch::async,
                                    [](void** pAddress, void* value, PVOID instance) -> void*
                                    {
                                        DWORD dwProtect[2];
                                        VirtualProtect(pAddress, sizeof(void*), PAGE_EXECUTE_READWRITE, &dwProtect[0]);
                                        MEMORY_BASIC_INFORMATION mbi;
                                        mbi.AllocationBase = instance;
                                        do
                                        {
                                            VirtualQuery(*pAddress, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
                                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                                        } while (mbi.AllocationBase == instance);
                                        auto r = *pAddress;
                                        *pAddress = value;
                                        VirtualProtect(pAddress, sizeof(void*), dwProtect[0], &dwProtect[1]);
                                        return r;
                                    }, pAddress, std::get<1>(inputs), (PVOID)instance);
                                }
                            } (), ...);
                            VirtualProtect(pAddress, sizeof(void*), dwProtect[0], &dwProtect[1]);
                        }
                    }
                }
            }
        }

        // Fallback section scan (e.g. re5dx9.exe steam)
        if (originalPtrs.empty())
        {
            static auto getSection = [](const PIMAGE_NT_HEADERS nt_headers, unsigned section) -> PIMAGE_SECTION_HEADER
            {
                return reinterpret_cast<PIMAGE_SECTION_HEADER>(
                    (UCHAR*)nt_headers->OptionalHeader.DataDirectory +
                    nt_headers->OptionalHeader.NumberOfRvaAndSizes * sizeof(IMAGE_DATA_DIRECTORY) +
                    section * sizeof(IMAGE_SECTION_HEADER));
            };

            for (auto i = 0; i < ntHeader->FileHeader.NumberOfSections; i++)
            {
                auto sec = getSection(ntHeader, i);
                auto pFunctions = reinterpret_cast<void**>(instance + std::max(sec->PointerToRawData, sec->VirtualAddress));

                for (ptrdiff_t j = 0; j < 300; j++)
                {
                    auto pAddress = &pFunctions[j];
                    VirtualProtect(pAddress, sizeof(void*), PAGE_EXECUTE_READWRITE, &dwProtect[0]);
                    ([&]
                    {
                        auto name = std::string_view(std::get<0>(inputs));
                        auto num = std::string("-1");
                        if (name.contains("@"))
                        {
                            num = name.substr(name.find_last_of("@") + 1);
                            name = name.substr(0, name.find_last_of("@"));
                        }

                        if (*pAddress && *pAddress == (void*)GetProcAddress(GetModuleHandleA(dll_name.data()), name.data()))
                        {
                            originalPtrs[std::get<0>(inputs)] = std::async(std::launch::deferred, [&]() -> void* { return *pAddress; });
                            originalPtrs[std::get<0>(inputs)].wait();
                            *pAddress = std::get<1>(inputs);
                        }
                    } (), ...);
                    VirtualProtect(pAddress, sizeof(void*), dwProtect[0], &dwProtect[1]);
                }

                if (!originalPtrs.empty())
                    return originalPtrs;
            }
        }

        return originalPtrs;
    }
};
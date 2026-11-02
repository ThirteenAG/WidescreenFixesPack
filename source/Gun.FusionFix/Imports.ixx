module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"

export module Imports;


export namespace GunImports
{
    template<typename T>
    T Replace(const char* library, const char* name, T replacement)
    {
        auto module = GetModuleHandleA(library);
        auto target = module ? GetProcAddress(module, name) : nullptr;
        if (!target)
            return nullptr;
        auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
        auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
        auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
        auto imports = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(base + nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
        for (; imports->Name; ++imports)
        {
            if (_stricmp(reinterpret_cast<const char*>(base + imports->Name), library) != 0)
                continue;
            auto addresses = reinterpret_cast<IMAGE_THUNK_DATA*>(base + imports->FirstThunk);
            for (size_t i = 0; addresses[i].u1.Function; ++i)
            {
                auto slot = reinterpret_cast<T*>(&addresses[i].u1.Function);
                if (reinterpret_cast<FARPROC>(*slot) != target)
                    continue;
                DWORD protect;
                if (!VirtualProtect(slot, sizeof(*slot), PAGE_READWRITE, &protect))
                    return nullptr;
                auto original = *slot;
                *slot = replacement;
                VirtualProtect(slot, sizeof(*slot), protect, &protect);
                return original;
            }
        }
        // Packed executables rebuild their imports separately from the PE import directory.
        auto sections = IMAGE_FIRST_SECTION(nt);
        for (unsigned section = 0; section < nt->FileHeader.NumberOfSections; ++section)
        {
            if (!(sections[section].Characteristics & IMAGE_SCN_MEM_EXECUTE))
                continue;
            auto start = base + sections[section].VirtualAddress;
            auto size = std::min<size_t>(sections[section].Misc.VirtualSize,
                nt->OptionalHeader.SizeOfImage - sections[section].VirtualAddress);
            for (size_t i = 0; i + 6 <= size; ++i)
            {
                auto instruction = reinterpret_cast<const uint8_t*>(start + i);
                if (instruction[0] != 0xFF || (instruction[1] != 0x15 && instruction[1] != 0x25))
                    continue;
                uint32_t address;
                memcpy(&address, instruction + 2, sizeof(address));
                if (address < base || address > base + nt->OptionalHeader.SizeOfImage - sizeof(T))
                    continue;
                auto slot = reinterpret_cast<T*>(address);
                if (reinterpret_cast<FARPROC>(*slot) != target)
                    continue;
                DWORD protect;
                if (!VirtualProtect(slot, sizeof(*slot), PAGE_READWRITE, &protect))
                    return nullptr;
                auto original = *slot;
                *slot = replacement;
                VirtualProtect(slot, sizeof(*slot), protect, &protect);
                return original;
            }
        }
        return nullptr;
    }
}

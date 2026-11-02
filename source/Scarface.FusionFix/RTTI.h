#pragma once

#include <windows.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <unordered_map>

namespace ScarfaceRTTI
{
    // MSVC x86 layout, as used by kananlib: vtable[-1] -> Complete Object
    // Locator -> TypeDescriptor. Every dword of the image's sections is a candidate
    // slot; values that point outside the image are not RTTI.
    inline std::unordered_map<std::string, uintptr_t> ScanVtables(HMODULE module)
    {
        static_assert(sizeof(void*) == 4);
        std::unordered_map<std::string, uintptr_t> result;
        const auto base = reinterpret_cast<uintptr_t>(module);
        const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(base + dos->e_lfanew);
        const auto end = base + nt->OptionalHeader.SizeOfImage;
        const auto inImage = [&](uintptr_t address, size_t size)
        {
            return address >= base && address < end && size <= end - address;
        };
        const auto sections = IMAGE_FIRST_SECTION(nt);
        for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; ++i)
        {
            const auto& section = sections[i];
            if (!(section.Characteristics & IMAGE_SCN_MEM_READ))
                continue;
            const auto first = base + section.VirtualAddress;
            const auto last = std::min<uintptr_t>(first + section.Misc.VirtualSize, end);
            for (auto slot = (first + 3) & ~uintptr_t(3); slot + 8 <= last; slot += 4)
            {
                const auto locator = *reinterpret_cast<const uint32_t*>(slot);
                if (!inImage(locator, 20))
                    continue;
                const auto col = reinterpret_cast<const uint32_t*>(locator); // signature, offset, cdOffset, type, hierarchy
                if (col[0] != 0 || col[1] != 0 || !inImage(col[4], 16) || !inImage(col[3], 12))
                    continue;
                // Only primary vtables are needed by Scarface's hooks.
                const auto hierarchy = reinterpret_cast<const uint32_t*>(col[4]);
                if (hierarchy[0] != 0 || !hierarchy[2])
                    continue;
                const auto name = reinterpret_cast<const char*>(col[3] + 8);
                if (name[0] != '.' || name[1] != '?')
                    continue;
                const auto terminator = static_cast<const char*>(memchr(name, 0, end - (col[3] + 8)));
                if (!terminator || !memchr(name, '@', terminator - name))
                    continue;
                result.try_emplace(std::string(name, terminator), slot + 4);
            }
        }
        return result;
    }

    inline uintptr_t FindVtable(std::string_view rawName)
    {
        static const auto vtables = ScanVtables(GetModuleHandleW(nullptr));
        return vtables.at(std::string(rawName));
    }
}

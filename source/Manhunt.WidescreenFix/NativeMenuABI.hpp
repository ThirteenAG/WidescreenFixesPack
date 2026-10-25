#pragma once
#include "NativeMenuContracts.hpp"
#include "FixSettings.hpp"
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>

namespace MHMenu::ABI
{
    template<class F> F Function(uintptr_t address)
    {
        return reinterpret_cast<F>(address);
    }

    template<class T> T& Data(uintptr_t address)
    {
        return *reinterpret_cast<T*>(address);
    }

    template<class T> void Store(uintptr_t address, const T& value)
    {
        ProtectedGameRef<T>{ reinterpret_cast<T*>(address) } = value;
    }

    inline bool Readable(const void* address, size_t size)
    {
        auto begin = reinterpret_cast<uintptr_t>(address);
        if (!begin || size > UINTPTR_MAX - begin) return false;
        const uintptr_t end = begin + static_cast<uintptr_t>(size);
        while (begin < end)
        {
            MEMORY_BASIC_INFORMATION region{};
            if (!VirtualQuery(reinterpret_cast<const void*>(begin), &region, sizeof(region))
                || region.State != MEM_COMMIT || (region.Protect & (PAGE_GUARD | PAGE_NOACCESS))) return false;
            const uintptr_t next = reinterpret_cast<uintptr_t>(region.BaseAddress) + static_cast<uintptr_t>(region.RegionSize);
            if (next <= begin) return false;
            begin = (std::min)(next, end);
        }
        return true;
    }

    inline bool Supported()
    {
        auto module = GetModuleHandleW(nullptr);
        if (reinterpret_cast<uintptr_t>(module) != 0x400000 || !Readable(module, sizeof(IMAGE_DOS_HEADER))) return false;
        auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(module);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < 0 || dos->e_lfanew > 0x100000) return false;
        auto nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(reinterpret_cast<const uint8_t*>(module) + dos->e_lfanew);
        if (!Readable(nt, sizeof(*nt)) || nt->Signature != IMAGE_NT_SIGNATURE
            || nt->FileHeader.Machine != IMAGE_FILE_MACHINE_I386
            || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC
            || nt->OptionalHeader.SizeOfImage != Contracts::ImageSize) return false;
        for (const auto& prefix : Contracts::Code)
        {
            auto address = reinterpret_cast<const void*>(prefix.address);
            if (!Readable(address, prefix.bytes.size()) || std::memcmp(address, prefix.bytes.data(), prefix.bytes.size())) return false;
        }
        return Readable(reinterpret_cast<void*>(0x7D6764), 8)
            && Readable(reinterpret_cast<void*>(0x7D6914), 8)
            && Data<uintptr_t>(0x7D6764) == 0x602800 && Data<uintptr_t>(0x7D6768) == 0x6029B0
            && Data<uintptr_t>(0x7D6914) == 0x603290 && Data<uintptr_t>(0x7D6918) == 0x603480;
    }

    inline int CurrentMenu() { return Data<int32_t>(0x7C86F8); }
    inline bool Paused() { return Function<int(__cdecl*)()>(0x5EA2B0)() == 2; }
    inline int& SettingsSelection() { return Data<int32_t>(0x7C89E8); }
    inline int& VideoSelection() { return Data<int32_t>(0x7C8A38); }
    inline float X() { return Data<float>(0x7C8718); }
    inline float Y() { return Data<float>(0x7C871C); }
    inline float ScaleX() { return Data<float>(0x7C8720); }
    inline float ScaleY() { return Data<float>(0x7C8724); }
    inline float Spacing() { return Data<float>(0x7C8728); }

    inline bool Up() { return Function<int(__cdecl*)()>(0x5D8A10)() != 0; }
    inline bool Down() { return Function<int(__cdecl*)()>(0x5D8AA0)() != 0; }
    inline bool Left() { return Function<int(__cdecl*)()>(0x5D8890)() != 0; }
    inline bool Right() { return Function<int(__cdecl*)()>(0x5D8950)() != 0; }
    inline bool Enter() { return Function<int(__cdecl*)()>(0x5D8B30)() != 0; }
    inline bool Escape() { return Function<int(__cdecl*)()>(0x5D8C60)() != 0; }
    inline bool MouseMode() { return Function<int(__cdecl*)()>(0x5DACB0)() != 0; }
    inline bool HoverActive() { return Function<int(__cdecl*)()>(0x5DACC0)() != 0; }
    inline int Hovered() { return Function<int(__cdecl*)()>(0x5DAD00)(); }
    inline int MouseValue() { return Function<int(__cdecl*)()>(0x5DAD10)(); }
    inline bool FooterBack() { return Data<int32_t>(0x7C8F9C) != 0; }
    inline bool FooterAccept() { return Data<int32_t>(0x7C8FA0) != 0; }

    inline void Switch(int page)
    {
        Function<void(__cdecl*)(int)>(0x5D7A40)(page);
    }

    inline const wchar_t* Text(const char* key)
    {
        auto result = Function<const wchar_t*(__thiscall*)(void*, const char*)>(0x4937E0)(reinterpret_cast<void*>(0x725A38), key);
        return result ? result : L"";
    }

    inline double Width(const std::wstring& text, float scale, int style = 2)
    {
        return Function<double(__cdecl*)(const wchar_t*, float, int)>(0x5E56F0)(text.c_str(), scale, style);
    }

    inline void Header(const wchar_t* text)
    {
        Function<int(__cdecl*)(const wchar_t*)>(0x5D5740)(text);
    }

    inline void Row(const wchar_t* text, float y, bool selected)
    {
        // This is also the game's mouse-hit collector. Keep its main-row path.
        Store<int32_t>(0x7C8F80, 1);
        Function<int(__cdecl*)(const wchar_t*, float, float, float, float, int)>(0x5D55C0)(
            text, X(), y, ScaleX(), ScaleY(), selected ? 1 : 0);
        Function<int(__cdecl*)(const wchar_t*, float, float, float, float)>(0x5D5B30)(
            text, X(), y, ScaleX(), ScaleY());
    }

    inline void Footer(const wchar_t* extra = L"")
    {
        Function<int(__cdecl*)(const wchar_t*, const wchar_t*, const wchar_t*, const wchar_t*)>(0x5D5BB0)(
            L"", L"", Text("IBACK"), extra);
    }

    inline std::string Language()
    {
        auto selected = FixSettings().Wanted("MAIN.TextLanguage");
        for (auto tag : { "en", "fr", "de", "it", "es" })
            if (selected == tag) return selected;
        // The frontend text loader selects GAME, GER, FRE, ITA and SPA in this order.
        static constexpr const char* languages[] = { "en", "de", "fr", "it", "es" };
        auto language = Data<int32_t>(0x7D6AF0);
        return language >= 0 && language < 5 ? languages[language] : "en";
    }
}

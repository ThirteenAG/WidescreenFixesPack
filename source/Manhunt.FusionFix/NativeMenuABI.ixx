module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <windows.h>
#include <cmath>
#include <cstdint>
#include <string>

export module NativeMenuABI;

import ComVars;


export namespace MHMenu::ABI
{
    template<class F> F Function(uintptr_t address)
    {
        return reinterpret_cast<F>(address);
    }

    template<class T> void Store(uintptr_t address, const T& value)
    {
        injector::WriteMemory<T>(reinterpret_cast<T*>(address), value, true);
    }

    // Code address found by pattern
    inline uintptr_t Code(const char* signature, ptrdiff_t offset = 0)
    {
        return reinterpret_cast<uintptr_t>(hook::get_pattern(signature, offset));
    }

    // Absolute address read from an instruction operand
    inline uintptr_t Operand(const char* signature, ptrdiff_t offset)
    {
        return *hook::get_pattern<uintptr_t>(signature, offset);
    }

    namespace Address
    {
        inline GameRef<int32_t>& CurrentMenu()
        {
            static const auto bound = []()
            {
                menuCurrentMenu.SetAddress(Operand("83 3D ? ? ? ? 07 55 74 05", 2));
                return true;
            }();
            return menuCurrentMenu;
        }
        inline GameRef<int32_t>& SettingsSelection()
        {
            static const auto bound = []()
            {
                menuSettingsSelection.SetAddress(Operand("BB ? ? ? ? C7 05 ? ? ? ? 01 00 00 00", 1));
                return true;
            }();
            return menuSettingsSelection;
        }
        inline GameRef<int32_t>& SettingsRows()
        {
            static const auto bound = []()
            {
                menuSettingsRows.SetAddress(Operand("83 3D ? ? ? ? 01 59 75 5E", 2));
                return true;
            }();
            return menuSettingsRows;
        }
        inline GameRef<int32_t>& SettingsPaused()
        {
            static const auto bound = []()
            {
                menuSettingsPaused.SetAddress(Operand("83 3D ? ? ? ? 01 75 14 6A 08", 2));
                return true;
            }();
            return menuSettingsPaused;
        }
        inline GameRef<int32_t>& VideoSelection()
        {
            static const auto bound = []()
            {
                menuVideoSelection.SetAddress(Operand("BF ? ? ? ? 75 07 BB 02 00 00 00", 1));
                return true;
            }();
            return menuVideoSelection;
        }
        inline GameRef<float>& X()
        {
            static const auto bound = []()
            {
                menuX.SetAddress(Operand("D9 05 ? ? ? ? D8 25 ? ? ? ? 53", 2));
                return true;
            }();
            return menuX;
        }
        inline GameRef<float>& Y()
        {
            static const auto bound = []()
            {
                menuY.SetAddress(Operand("8B 0D ? ? ? ? A1 ? ? ? ? 56", 2));
                return true;
            }();
            return menuY;
        }
        inline GameRef<float>& ScaleX()
        {
            static const auto bound = []()
            {
                menuScaleX.SetAddress(Operand("A1 ? ? ? ? 57 55 83 EC 1C", 1));
                return true;
            }();
            return menuScaleX;
        }
        inline GameRef<float>& ScaleY()
        {
            static const auto bound = []()
            {
                menuScaleY.SetAddress(Operand("8B 0D ? ? ? ? 89 44 24 40", 2));
                return true;
            }();
            return menuScaleY;
        }
        inline GameRef<float>& Spacing()
        {
            static const auto bound = []()
            {
                menuSpacing.SetAddress(Operand("8B 15 ? ? ? ? 89 44 24 14", 2));
                return true;
            }();
            return menuSpacing;
        }
        inline GameRef<int32_t>& FooterBack()
        {
            static const auto bound = []()
            {
                menuFooterBack.SetAddress(Operand("83 3D ? ? ? ? 00 74 0F 6A 07", 2));
                return true;
            }();
            return menuFooterBack;
        }
        inline GameRef<int32_t>& FooterAccept()
        {
            static const auto bound = []()
            {
                menuFooterAccept.SetAddress(Operand("83 3D ? ? ? ? 01 75 05 BE 01 00 00 00", 2));
                return true;
            }();
            return menuFooterAccept;
        }
        inline GameRef<int32_t>& MainRow()
        {
            static const auto bound = []()
            {
                menuMainRow.SetAddress(Operand("83 3D ? ? ? ? 00 74 19 83 3D ? ? ? ? 00", 2));
                return true;
            }();
            return menuMainRow;
        }
        inline GameRef<int32_t>& Language()
        {
            static const auto bound = []()
            {
                menuLanguage.SetAddress(Operand("8B 15 ? ? ? ? 83 FA 04 77 57", 2));
                return true;
            }();
            return menuLanguage;
        }
        inline uintptr_t TextTable() { static auto a = Operand("B9 ? ? ? ? D8 05 ? ? ? ?", 1); return a; }
        inline GameRef<float>& DetailRows()
        {
            static const auto bound = []()
            {
                menuDetailRows.SetAddress(Operand("D8 0D ? ? ? ? B9 ? ? ? ? D9 5C 24 0C D9 04 24 D8 44 24 0C D9 1C 24 68 ? ? ? ? E8 ? ? ? ? 83 FB 03", 2));
                return true;
            }();
            return menuDetailRows;
        }
        inline GameRef<int32_t>& ResolutionCount()
        {
            static const auto bound = []()
            {
                menuResolutionCount.SetAddress(Operand("C7 05 ? ? ? ? 04 00 00 00 C7 05 ? ? ? ? 04 00 00 00 C3", 2));
                return true;
            }();
            return menuResolutionCount;
        }
        inline GameRef<int32_t>& DetailCount()
        {
            static const auto bound = []()
            {
                menuDetailCount.SetAddress(Operand("C7 05 ? ? ? ? 04 00 00 00 C3", 2));
                return true;
            }();
            return menuDetailCount;
        }
        // Video page values: resolution, depth, windowed and two detail levels
        inline GameRef<int32_t>* VideoValues()
        {
            static const auto bound = []()
            {
                const uintptr_t values[] =
                {
                    Operand("B9 ? ? ? ? BF ? ? ? ? 75 07", 1),
                    Operand("A1 ? ? ? ? C3 90 90 90 90 90 90 90 90 90 90 A1 ? ? ? ? A3 ? ? ? ?", 1),
                    Operand("83 3D ? ? ? ? 01 75 07 E8 ? ? ? ?", 2),
                    Operand("A1 ? ? ? ? C3 90 90 90 90 90 90 90 90 90 90 A1 ? ? ? ? C3 90 90 90 90 90 90 90 90 90 90 A1 ? ? ? ? C3 90 90 90 90 90 90 90 90 90 90 A1 ? ? ? ? A3 ? ? ? ?", 1),
                    Operand("A1 ? ? ? ? C3 90 90 90 90 90 90 90 90 90 90 A1 ? ? ? ? C3 90 90 90 90 90 90 90 90 90 90 A1 ? ? ? ? A3 ? ? ? ?", 1),
                };
                for (size_t i = 0; i < menuVideoValues.size(); ++i)
                    menuVideoValues[i].SetAddress(values[i]);
                return true;
            }();
            return menuVideoValues.data();
        }
        // Menu page callback slots: input, then draw
        inline uintptr_t SettingsPage() { static auto a = Operand("C7 03 ? ? ? ? E8 ? ? ? ? 89 D8 5B C3 90 90 90 90 90 90 90 90 90 90 90 90 90 90 53 55 31 ED", 2); return a; }
        inline uintptr_t VideoPage() { static auto a = Operand("C7 03 ? ? ? ? E8 ? ? ? ? 89 D8 5B C3 90 90 90 90 90 90 90 90 90 90 90 90 90 90 53 56 57 55 31 ED", 2); return a; }

        inline uintptr_t SettingsInput() { static auto a = Code("53 55 31 ED C7 05 ? ? ? ? 01 00 00 00"); return a; }
        inline uintptr_t SettingsDraw() { static auto a = Code("53 8B 15 ? ? ? ? 8B 0D ? ? ? ? A1 ? ? ? ? 56"); return a; }
        inline uintptr_t VideoInput() { static auto a = Code("53 56 57 55 31 ED 31 F6 83 3D ? ? ? ? 00"); return a; }
        inline uintptr_t VideoDraw() { static auto a = Code("53 56 57 8B 15 ? ? ? ? 8B 0D ? ? ? ?"); return a; }
        inline uintptr_t VideoApply() { static auto a = Code("B8 ? ? ? ? 8B 15 ? ? ? ? 89 10"); return a; }
        inline uintptr_t VideoRestore() { static auto a = Code("83 3D ? ? ? ? 00 75 07 E8 ? ? ? ? 89 C0 83 3D ? ? ? ? 01"); return a; }
        inline uintptr_t DefaultsConfirmed() { static auto a = Code("E8 ? ? ? ? 6A 00 E8 ? ? ? ? 59 E8 ? ? ? ? E8 ? ? ? ? 89 C0"); return a; }
        inline uintptr_t PauseState() { static auto a = Code("A1 ? ? ? ? C3 90 90 90 90 90 90 90 90 90 90 80 3D ? ? ? ? 00"); return a; }
        // Direction keys, found by the key code each one compares
        inline uintptr_t Up() { static auto a = Code("8B 90 ? ? ? ? 81 FA FB 03 00 00", -0x30); return a; }
        inline uintptr_t Down() { static auto a = Code("8B 90 ? ? ? ? 81 FA FC 03 00 00", -0x30); return a; }
        inline uintptr_t Left() { static auto a = Code("8B 90 ? ? ? ? 81 FA FD 03 00 00", -0x60); return a; }
        inline uintptr_t Right() { static auto a = Code("8B 90 ? ? ? ? 81 FA FE 03 00 00", -0x60); return a; }
        inline uintptr_t Enter() { static auto a = Code("53 56 55 31 DB E8 ? ? ? ? 85 C0"); return a; }
        inline uintptr_t Escape() { static auto a = Code("53 55 31 ED 31 DB E8 ? ? ? ? 85 C0"); return a; }
        inline uintptr_t MouseMode() { static auto a = Code("A1 ? ? ? ? C3 90 90 90 90 90 90 90 90 90 90 83 3D ? ? ? ? 00 74 17"); return a; }
        inline uintptr_t HoverActive() { static auto a = Code("83 3D ? ? ? ? 00 74 17 E8 ? ? ? ?"); return a; }
        inline uintptr_t Hovered() { static auto a = Code("A1 ? ? ? ? C3 90 90 90 90 90 90 90 90 90 90 A1 ? ? ? ? C3 90 90 90 90 90 90 90 90 90 90 C7 05 ? ? ? ? 00 00 00 00"); return a; }
        inline uintptr_t MouseValue() { static auto a = Code("A1 ? ? ? ? C3 90 90 90 90 90 90 90 90 90 90 C7 05 ? ? ? ? 00 00 00 00"); return a; }
        inline uintptr_t Switch() { static auto a = Code("8B 44 24 04 85 C0 75 05 A1 ? ? ? ?"); return a; }
        inline uintptr_t Text() { static auto a = Code("57 55 8B 6C 24 0C 55 E8 ? ? ? ?"); return a; }
        inline uintptr_t Width() { static auto a = Code("53 56 55 83 EC 04 8B 5C 24 14 8B 74 24 1C"); return a; }
        inline uintptr_t Header() { static auto a = Code("8B 15 ? ? ? ? 8B 0D ? ? ? ? 53 56 57 55 83 EC 24"); return a; }
        inline uintptr_t RowText() { static auto a = Code("53 56 57 55 83 EC 0C 8B 3D ? ? ? ?"); return a; }
        inline uintptr_t RowHit() { static auto a = Code("53 31 DB E8 ? ? ? ? 83 F8 02 75 05"); return a; }
        inline uintptr_t Footer() { static auto a = Code("8B 0D ? ? ? ? 53 56 55 83 EC 18"); return a; }
    }

    inline int CurrentMenu() { return Address::CurrentMenu().get(); }
    inline bool Paused() { return Function<int(__cdecl*)()>(Address::PauseState())() == 2; }
    inline int& SettingsSelection() { return Address::SettingsSelection().get(); }
    inline int& VideoSelection() { return Address::VideoSelection().get(); }
    inline float X() { return Address::X().get(); }
    inline float Y() { return Address::Y().get(); }
    inline float ScaleX() { return Address::ScaleX().get(); }
    inline float ScaleY() { return Address::ScaleY().get(); }
    inline float Spacing() { return Address::Spacing().get(); }

    inline bool Up() { return Function<int(__cdecl*)()>(Address::Up())() != 0; }
    inline bool Down() { return Function<int(__cdecl*)()>(Address::Down())() != 0; }
    inline bool Left() { return Function<int(__cdecl*)()>(Address::Left())() != 0; }
    inline bool Right() { return Function<int(__cdecl*)()>(Address::Right())() != 0; }
    inline bool Enter() { return Function<int(__cdecl*)()>(Address::Enter())() != 0; }
    inline bool Escape() { return Function<int(__cdecl*)()>(Address::Escape())() != 0; }
    inline bool MouseMode() { return Function<int(__cdecl*)()>(Address::MouseMode())() != 0; }
    inline bool HoverActive() { return Function<int(__cdecl*)()>(Address::HoverActive())() != 0; }
    inline int Hovered() { return Function<int(__cdecl*)()>(Address::Hovered())(); }
    inline int MouseValue() { return Function<int(__cdecl*)()>(Address::MouseValue())(); }
    inline bool FooterBack() { return Address::FooterBack().get() != 0; }
    inline bool FooterAccept() { return Address::FooterAccept().get() != 0; }

    inline void Switch(int page)
    {
        Function<void(__cdecl*)(int)>(Address::Switch())(page);
    }

    inline const wchar_t* Text(const char* key)
    {
        auto result = Function<const wchar_t*(__thiscall*)(void*, const char*)>(Address::Text())(reinterpret_cast<void*>(Address::TextTable()), key);
        return result ? result : L"";
    }

    inline double Width(const std::wstring& text, float scale, int style = 2)
    {
        return Function<double(__cdecl*)(const wchar_t*, float, int)>(Address::Width())(text.c_str(), scale, style);
    }

    inline void Header(const wchar_t* text)
    {
        Function<int(__cdecl*)(const wchar_t*)>(Address::Header())(text);
    }

    inline void Row(const wchar_t* text, float y, bool selected)
    {
        // This is also the game's mouse-hit collector. Keep its main-row path.
        injector::WriteMemory<int32_t>(Address::MainRow().get_ptr(), 1, true);
        Function<int(__cdecl*)(const wchar_t*, float, float, float, float, int)>(Address::RowText())(
            text, X(), y, ScaleX(), ScaleY(), selected ? 1 : 0);
        Function<int(__cdecl*)(const wchar_t*, float, float, float, float)>(Address::RowHit())(
            text, X(), y, ScaleX(), ScaleY());
    }

    inline void Footer(const wchar_t* extra = L"")
    {
        Function<int(__cdecl*)(const wchar_t*, const wchar_t*, const wchar_t*, const wchar_t*)>(Address::Footer())(
            L"", L"", Text("IBACK"), extra);
    }

    inline std::string Language()
    {
        auto selected = FixSettings().Wanted("MAIN.TextLanguage");
        for (auto tag : { "en", "fr", "de", "it", "es" })
            if (selected == tag) return selected;
        // The frontend text loader selects GAME, GER, FRE, ITA and SPA in this order.
        static constexpr const char* languages[] = { "en", "de", "fr", "it", "es" };
        auto language = Address::Language().get();
        return language >= 0 && language < 5 ? languages[language] : "en";
    }
}

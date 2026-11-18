module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

// Update notice: one line at the top of the game's menu screens, in the game's subtitle font

export module Updates;

import ComVars;

export namespace SniperUpdates
{
    inline std::wstring Line;
    inline void(__cdecl* SetFont)(int32_t) = nullptr;
    inline char(__cdecl* Print)(const wchar_t*, const float*, float, float, float, float, uint32_t, uint32_t) = nullptr;

    inline void WINAPI Exit(UINT code)
    {
        RWGTA::UpdateCheck::OnExit();
        ExitProcess(code);
    }

    // From the menu drawing, with the menu's 4:3 text layout active
    inline void Draw()
    {
        if (Line.empty() || !SetFont || !UpdateFont.is_initialized() || !Print)
            return;
        static constexpr float area[] = { 0.02f, 0.015f, 0.98f, 0.08f }; //the free strip above the menus
        SetFont(UpdateFont);
        Print(Line.c_str(), area, 0.0f, 0.0f, 0.75f, 0.5f, 0xFFFFFFFF, 0x1C5);
    }

    // MAIN.TextLanguage: Automatic follows the language of the game's text files
    inline void SelectLanguage()
    {
        auto language = FixSettings().String("MAIN.TextLanguage");
        if (language.empty() || language == "auto")
        {
            language = "en";
            static constexpr std::pair<const char*, const char*> files[] =
            {
                { "Text/PC_Text_Fre.asr", "fr" }, { "Text/PC_Text_Fra.asr", "fr" },
                { "Text/PC_Text_Ger.asr", "de" }, { "Text/PC_Text_Deu.asr", "de" },
                { "Text/PC_Text_Ita.asr", "it" }, { "Text/PC_Text_Spa.asr", "es" },
                { "Text/PC_Text_Esp.asr", "es" },
            };
            for (const auto& [file, tag] : files)
            {
                if (GetFileAttributesA(file) != INVALID_FILE_ATTRIBUTES)
                    language = tag;
            }
        }
        FixSettings().GetTexts().Select(language);
    }

    inline void Init(decltype(Print) print)
    {
        SelectLanguage();
        FixSettings().OnChange("MAIN.TextLanguage", SelectLanguage);
        auto font = hook::pattern("E8 ? ? ? ? D8 2D ? ? ? ? A1 ? ? ? ? 50 D9 5C 24 08"); //475092
        auto setFont = hook::pattern("8B 44 24 04 83 F8 FF 74 ? 8B 0D ? ? ? ? 83 3C 81 00"); //40E5F0
        Print = print;
        if (auto* fontAddress = *font.get_first<int32_t*>(12))
            UpdateFont.SetAddress(fontAddress);
        SetFont = reinterpret_cast<decltype(SetFont)>(setFont.get_first());
        IATHook::Replace(GetModuleHandleW(nullptr), "KERNEL32.dll", std::forward_as_tuple("ExitProcess", Exit));
        RWGTA::UpdateCheck::Config config;
        config.pluginId = "SniperElite.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.playable = []() { return true; };
        config.show = [](const std::wstring& line) { Line = line; };
        RWGTA::UpdateCheck::Init(std::move(config));
    }
}

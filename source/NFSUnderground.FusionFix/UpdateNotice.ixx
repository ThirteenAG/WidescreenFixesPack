module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"


export module UpdateNotice;

import ComVars;

export namespace NFSUpdateNotice
{
    inline void Tick() { RWGTA::UpdateCheck::Tick(); }
    inline void OnExit() { RWGTA::UpdateCheck::OnExit(); }

    inline void SetText(void* screen, void* fn, const char* value)
    {
        __asm
        {
            push ebx
            mov ebx, screen
            push value
            call fn            // the setter pops its argument
            pop ebx
        }
    }

    // The dialog's text box shows one line per row; the notice is split at spaces into rows of a few words
    inline std::string Wrap(const std::string& text, size_t width)
    {
        std::string result, line;
        size_t at = 0;
        while (at < text.size())
        {
            auto end = text.find(' ', at);
            if (end == std::string::npos)
                end = text.size();
            auto word = text.substr(at, end - at);
            at = end + 1;
            if (!line.empty() && line.size() + 1 + word.size() > width)
            {
                result += line + "\n";
                line.clear();
            }
            line += (line.empty() ? "" : " ") + word;
        }
        return result + line;
    }

    inline bool Install(asi_settings::Plugin& settings, std::function<bool()> frontend,
        std::function<uint8_t*(const char*)> package, std::function<std::string(const std::string&)> encode)
    {
        auto dialog = hook::pattern("51 55 8B 6C 24 10 56 57 68 ? ? ? ? 55 E8 ? ? ? ? 8B 44 24 24 83 C4 08 E8 ? ? ? ? 8B 70 18 89 44 24 0C");
        auto text = hook::pattern("8B 43 44 85 C0 75 16 6A 00 6A 00 6A 00 68 00 01 00 00 E8 ? ? ? ? 83 C4 10 89 43 44 8B 43 44 56 8B 74 24 08");
        static auto showDialog = reinterpret_cast<void* (__cdecl*)(uint32_t*, const char*, const char*)>(dialog.get_first());
        static auto setText = text.get_first();
        RWGTA::UpdateCheck::Config config;
        config.pluginId = "NFSUnderground.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &settings;
        config.playable = [frontend, package]()
        {
            return frontend() && package("MU_Main_PC.fng") && !package("GenericDialog.fng") && !package("GenericOKDialog.fng");
        };
        config.show = [encode](const std::wstring& notice)
        {
            // The game's OK dialog owns its text buffer and returns to the main menu on dismissal.
            uint32_t parameters[4] = { 0, 0xDE02CF84, 0x12340000, 0 };
            auto screen = showDialog(parameters, "GenericOKDialog.fng", "MU_Main_PC.fng");
            if (!screen)
                return;
            auto text = Wrap(encode(asi_settings::text::Narrow(notice)), 40);
            auto value = text.c_str();
            auto fn = setText;
            SetText(screen, fn, value);
        };
        RWGTA::UpdateCheck::Init(std::move(config));
        return true;
    }
}

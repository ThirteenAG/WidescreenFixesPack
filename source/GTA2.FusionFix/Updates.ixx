module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Updates;

import ComVars;
import Options;
import MenuText;

// MAIN.CheckForUpdates: see Shared/RWGTA/UpdateCheck.hpp. The notice is shown on the frontend's main page, in its
// footer font, below the menu.
export namespace GTA2Updates
{
    std::vector<std::wstring> Notice;

    void Install()
    {
        RWGTA::UpdateCheck::Config config;
        config.pluginId = "GTA2.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.playable = [] { return GTA2Options::Menu && *reinterpret_cast<uint16_t*>(GTA2Options::Menu + GTA2Options::CurrentPage) == 0; };
        config.show = [](const std::wstring& line)
        {
            if (!GTA2Options::TextWidth)
                return;
            Notice = GTA2MenuText::Wrap(GTA2MenuText::Native(line), 320, 49, [](const std::wstring& text)
            {
                return GTA2Options::TextWidth(text.c_str(), wFooterFont);
            });
            if (Notice.size() > 3)
                Notice.resize(3);
        };
        RWGTA::UpdateCheck::Init(std::move(config));
    }

    // Each frontend frame, after its input
    void Frame(char* menu)
    {
        RWGTA::UpdateCheck::Tick();
        if (Notice.empty() || *reinterpret_cast<uint16_t*>(menu + GTA2Options::CurrentPage) != 0)
            return;
        // The main page has no static text of its own: the notice lines are its static records
        auto page = menu + GTA2Options::PageBase;
        *reinterpret_cast<uint16_t*>(page + 2) = uint16_t(Notice.size());
        for (size_t i = 0; i < Notice.size(); ++i)
        {
            auto record = page + 1304 + i * 110;
            std::memset(record, 0, 110);
            record[0] = record[1] = 1;
            *reinterpret_cast<int16_t*>(record + 2) = 300;
            *reinterpret_cast<int16_t*>(record + 4) = int16_t(392 + 16 * int(i));
            wcsncpy_s(reinterpret_cast<wchar_t*>(record + 6), 50, Notice[i].c_str(), _TRUNCATE);
            *reinterpret_cast<uint16_t*>(record + 106) = wFooterFont;
            *reinterpret_cast<uint16_t*>(record + 108) = 0xFFFF;
        }
    }
}

class Updates
{
public:
    Updates()
    {
        // Each frontend frame, after its input: the options page first, then the notice on page 0
        WFP::onMenuDrawingEvent() += []()
        {
            GTA2Options::Process(FrontendMenu);
            GTA2Updates::Frame(FrontendMenu);
        };
    }
} Updates;

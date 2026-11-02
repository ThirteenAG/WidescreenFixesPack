module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/GTAClassic/InputDevices.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Updates;

import ComVars;
import Options;
import MenuText;

// MAIN.CheckForUpdates: see Shared/RWGTA/UpdateCheck.hpp. The notice is written above the frontend's footer in its
// small font.
export namespace GTA1Updates
{
    std::vector<std::string> Notice;

    // Any frontend page but the options pages
    bool MainMenu()
    {
        return InputDevices::Frontend && !GTA1Options::Showing();
    }

    void Install()
    {
        RWGTA::UpdateCheck::Config config;
        config.pluginId = "GTA1.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.playable = [] { return MainMenu(); };
        config.show = [](const std::wstring& line)
        {
            if (!GTA1Options::TextWidth)
                return;
            int font = nSmallFont;
            Notice = GTA1MenuText::Wrap(GTA1Options::FontText(GTA1Options::Utf8(line.c_str())), 400, [font](const std::string& text)
            {
                return GTA1Options::TextWidth(font, text.c_str());
            });
            if (Notice.size() > 2)
                Notice.resize(2);
        };
        RWGTA::UpdateCheck::Init(std::move(config));
    }

    // Before a frontend frame is presented
    void Draw()
    {
        RWGTA::UpdateCheck::Tick();
        if (Notice.empty() || !MainMenu() || !GTA1Options::Text)
            return;
        int font = nSmallFont;
        int height = std::max(8, int(reinterpret_cast<uint8_t*>(font)[1]));
        // Centred between the logos in the bottom corners
        for (size_t i = 0; i < Notice.size(); ++i)
            GTA1Options::Text(font, 320 - GTA1Options::TextWidth(font, Notice[i].c_str()) / 2, 480 - height * int(Notice.size() + 1 - i), Notice[i].c_str());
    }
}

class Updates
{
public:
    Updates()
    {
        // Before a frontend frame is presented
        WFP::onMenuDrawingEvent() += []()
        {
            GTA1Updates::Draw();
        };

        WFP::onShutdownEvent() += []()
        {
            RWGTA::UpdateCheck::OnExit();
        };
    }
} Updates;

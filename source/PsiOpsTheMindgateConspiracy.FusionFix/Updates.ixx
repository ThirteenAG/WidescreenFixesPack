module;
#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Updates;
import NativeMenu;
import Window;


export namespace PsiOpsUpdates
{
    inline void WINAPI Exit(UINT code)
    {
        RWGTA::UpdateCheck::OnExit();
        ExitProcess(code);
    }

    inline void Install()
    {
        const bool available = PsiOpsMenu::eventHook && PsiOpsMenu::drawTextHook && PsiOpsWindow::createD3DHook;
        FixSettings().Availability("MAIN.CheckForUpdates", [available](std::string_view value)
        {
            return value == "0" || available;
        });
        if (!available)
            return;
        IATHook::Replace(GetModuleHandleW(nullptr), "KERNEL32.dll", std::forward_as_tuple("ExitProcess", Exit));
        RWGTA::UpdateCheck::Config config;
        config.pluginId = "PsiOpsTheMindgateConspiracy.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.playable = []()
        {
            const auto screen = PsiOpsMenu::current;
            return screen && screen->open && !screen->cleaning && screen->view.notice.empty();
        };
        config.format = [](const std::string&, bool hub)
        {
            return asi_settings::text::Widen(PsiOpsMenu::Text(hub ? "update.availablehub" : "update.availablesite"));
        };
        config.show = [](const std::wstring& text)
        {
            auto screen = PsiOpsMenu::current;
            if (!screen || !screen->open)
                return;
            screen->view = {};
            screen->view.notice = asi_settings::text::Narrow(text);
            screen->selected = 0;
            PsiOpsMenu::Build(*screen);
            PsiOpsMenu::Focus(*screen, screen->selected);
        };
        RWGTA::UpdateCheck::Init(std::move(config));
        const auto render = PsiOpsWindow::frame;
        PsiOpsWindow::frame = [render](IDirect3DDevice9* device)
        {
            if (render)
                render(device);
            RWGTA::UpdateCheck::Tick();
        };
    }
}


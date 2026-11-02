module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/MaxPayne/SettingsMenu.hpp"
#include "Shared/MaxPayne/NativeIcons.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Menu;

import ComVars;
import input;

// The fix's settings next to the game's options; the rest is on its page in Plugins
std::vector<MaxPayneMenu::Placement> MenuPlacements()
{
    using MaxPayneMenu::Spot;
    return
    {
        { "GCID_VIDEOU", Spot::BeforeLastMenu, 2, { "MAIN.WindowMode", "MAIN.FOVFactor", "MAIN.HudAspectRatioConstraint", "GRAPHICS.AntiAliasing", "GRAPHICS.ConsoleGamma", "GRAPHICS.VSync", "MAIN.FPSLimit" } },
        { "GCID_GENERICU", Spot::BeforeFirstItem, 0, { "MAIN.CutsceneBorders", "MAIN.AnimatedCutsceneBorders", "MAIN.GraphicNovelMode" } },
        { "GCID_CONTROLSU", Spot::BeforeLastMenu, 3, { "MAIN.GamepadIcons", "MAIN.GamepadCameraSpeed", "MAIN.InvertGamepadY", "MAIN.Vibration" } },
        { "GCID_MOUSESETTINGSU", Spot::BeforeBack, 3, { "MAIN.RawInputMouse", "MAIN.MouseSmoothing" } },
    };
}

// The fix's rows in the game's Options menu, see source/Shared/MaxPayne/SettingsMenu.hpp. MaxPayne_MenuMode
// answers the rows through its virtual functions setItem, isItemEnabled, setSlider and getSlider; the item and slider
// lines keep their name (P_Text) at +4 and their ID at +12, and update in slot 3.
export void InitControlIcons()
{
    if (!MaxPayneIcons::Ready || !MaxPayneMenu::Get().host)
        return;
    MaxPayneMenu::Get().tipSource = MaxPayneIcons::Source;
    MaxPayneIcons::HookControls(MaxPayneMenu::FindVTable(GetModuleHandleW(nullptr), ".?AVX_MenuInputConfig@@"),
        3, 12, 34, GetControlIcon);
}

export void InitMenu()
{
    HMODULE hModule = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&InitMenu, &hModule);
    if (!MaxPayneMenu::Init(hModule, GetModuleHandleW(L"rlmfc"), GetModuleHandleW(L"e2mfc"), MenuPlacements()))
        return;

    auto exe = GetModuleHandleW(nullptr);
    MaxPayneMenu::HookMenu(MaxPayneMenu::FindVTable(exe, ".?AVMaxPayne_MenuMode@@"), { 21, 22, 28, 29, 14, 347, 302 },
        MaxPayneMenu::FindVTable(exe, ".?AVX_MenuItem@@"), MaxPayneMenu::FindVTable(exe, ".?AVX_MenuSlider@@"), { 4, 12, 3 });
    InitControlIcons();
}

namespace Updates
{
    SafetyHookInline ExitHook;

    int __fastcall Exit(void* app, void* edx)
    {
        RWGTA::UpdateCheck::OnExit();
        return ExitHook.unsafe_fastcall<int>(app, edx);
    }

    void Init()
    {
        auto pattern = hook::pattern("56 8B F1 E8 ? ? ? ? E8 ? ? ? ? 68 85 00 00 00 6A 00 6A 00 6A 00 FF 15 ? ? ? ? 8B CE E8 ? ? ? ? 5E C3");
        ExitHook = safetyhook::create_inline(pattern.get_first(), Exit);
        RWGTA::UpdateCheck::Config config;
        config.pluginId = "MaxPayne.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.playable = []()
        {
            return CurrentGameMode == "menu" && MaxPayneMenu::Get().tooltip;
        };
        config.show = [](const std::wstring& line)
        {
            MaxPayneMenu::Get().notification = MaxPayneMenu::ToFont(asi_settings::text::Narrow(line));
        };
        RWGTA::UpdateCheck::Init(std::move(config));
        FixSettings().OnChange("MAIN.CheckForUpdates", []()
        {
            if (!FixSettings().Bool("MAIN.CheckForUpdates"))
                MaxPayneMenu::Get().notification.clear();
        });
    }
}

class Menu
{
public:
    Menu()
    {
        WFP::onInitEvent() += []()
        {
            InitMenu();
            Updates::Init();
        };

        WFP::onGameProcessEvent() += []()
        {
            RWGTA::UpdateCheck::Tick();
        };
    }
} Menu;

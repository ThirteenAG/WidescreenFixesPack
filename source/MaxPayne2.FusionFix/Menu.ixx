module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/MaxPayne/SettingsMenu.hpp"
#include "Shared/MaxPayne/NativeIcons.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"
#include <stdexcept>

export module Menu;

import ComVars;
import x_inputmfc;

// The fix's settings next to the game's options; the rest is on its page in Plugins
std::vector<MaxPayneMenu::Placement> MenuPlacements()
{
    using MaxPayneMenu::Spot;
    return
    {
        { "GCID_VIDEOU", Spot::BeforeLastMenu, 2, { "MAIN.WindowMode", "MAIN.FOVFactor", "MAIN.HudAspectRatioConstraint", "GRAPHICS.AntiAliasing", "GRAPHICS.ConsoleGamma", "GRAPHICS.VSync", "MAIN.FPSLimit" } },
        { "GCID_GENERICU", Spot::BeforeFirstItem, 0, { "MAIN.CutsceneBorders", "MAIN.AnimatedCutsceneBorders", "MAIN.FMVWidescreenMode", "MAIN.GraphicNovelMode" } },
        { "GCID_CONTROLSU", Spot::BeforeLastMenu, 3, { "MAIN.GamepadIcons", "MAIN.GamepadCameraSpeed", "MAIN.InvertGamepadY", "MAIN.Vibration" } },
        { "GCID_MOUSESETTINGSU", Spot::BeforeBack, 3, { "MAIN.RawInputMouse", "MAIN.MouseSmoothing" } },
    };
}

// The fix's rows in the game's Options menu, see source/Shared/MaxPayne/SettingsMenu.hpp. MP_MenuMode
// answers the rows through its virtual functions setItem, isItemEnabled, setSlider and getSlider; the item and slider
// lines (X_BasicModesMFC) keep their name (P_Text) at +4 and their ID at +12, and update in slot 3.
export void InitMenu()
{
    HMODULE hModule = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&InitMenu, &hModule);
    auto basicModes = GetModuleHandleW(L"X_BasicModesMFC");
    if (!basicModes || !MaxPayneMenu::Init(hModule, GetModuleHandleW(L"rlmfc"), GetModuleHandleW(L"e2mfc"), MenuPlacements()))
        return;

    auto exe = GetModuleHandleW(nullptr);
    auto menu = MaxPayneMenu::FindVTable(exe, ".?AVMP_MenuMode@@");
    auto crt = GetModuleHandleW(L"msvcr71.dll");
    auto allocate = crt ? reinterpret_cast<decltype(MaxPayneMenu::Get().allocate)>(GetProcAddress(crt, "malloc")) : nullptr;
    bool footer = menu && allocate;
    if (footer)
    {
        auto& state = MaxPayneMenu::Get();
        state.allocate = allocate;
        state.textSize = 436;
        state.textLayerOffset = 284;
        state.fontHeightOffset = 80;
    }

    MaxPayneMenu::MenuModeSlots slots = { 21, 22, 29, 30 };
    if (footer)
        slots = { 21, 22, 29, 30, 14, 0x17A, 0x14D };
    MaxPayneMenu::HookMenu(menu, slots,
        MaxPayneMenu::FindVTable(basicModes, ".?AVX_MenuItem@@"), MaxPayneMenu::FindVTable(basicModes, ".?AVX_MenuSlider@@"), { 4, 12, 3 });
    auto controls = MaxPayneMenu::FindVTable(basicModes, ".?AVX_MenuInputConfig@@");
    if (MaxPayneIcons::Ready && controls)
    {
        MaxPayneMenu::Get().tipSource = MaxPayneIcons::Source;
        MaxPayneIcons::HookControlsByObject(controls, 3, 0x2e, [](uint8_t* item)
        {
            auto value = item + 0x56;
            auto length = *reinterpret_cast<uint32_t*>(value + 0x14);
            auto capacity = *reinterpret_cast<uint32_t*>(value + 0x18);
            auto text = capacity >= 16 ? *reinterpret_cast<const char**>(value + 4) : reinterpret_cast<const char*>(value + 4);
            return text && length <= 100 ? GetControlIcon(std::string_view(text, length)) : std::string();
        });
    }
}

namespace Updates
{
    SafetyHookInline QuitHook;
    SafetyHookInline ExitProcessHook;

    void WINAPI ExitProcessDetour(UINT code)
    {
        RWGTA::UpdateCheck::OnExit();
        ExitProcessHook.stdcall<void>(code);
    }

    void __cdecl Quit()
    {
        RWGTA::UpdateCheck::OnExit();
        QuitHook.ccall<void>();
    }

    void Init()
    {
        auto pattern = hook::pattern("6A 00 FF 15 ? ? ? ? C3 CC CC CC CC CC CC CC 56 57");
        QuitHook = safetyhook::create_inline(pattern.get_first(), Quit);
        auto kernel = GetModuleHandleW(L"kernel32.dll");
        auto exitProcess = kernel ? GetProcAddress(kernel, "ExitProcess") : nullptr;
        if (exitProcess)
            ExitProcessHook = safetyhook::create_inline(exitProcess, ExitProcessDetour);
        if (!QuitHook || !ExitProcessHook)
        {
            QuitHook.reset();
            ExitProcessHook.reset();
            throw std::runtime_error("Update exit hooks unavailable");
        }
        RWGTA::UpdateCheck::Config config;
        config.pluginId = "MaxPayne2.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.playable = []() { return CurrentGameMode == "menu" && MaxPayneMenu::Get().tooltip; };
        config.show = [](const std::wstring& line) { MaxPayneMenu::Get().notification = MaxPayneMenu::ToFont(asi_settings::text::Narrow(line)); };
        RWGTA::UpdateCheck::Init(std::move(config));
        WFP::onGameProcessEvent() += []() { RWGTA::UpdateCheck::Tick(); };
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
            Updates::Init();
        };
    }
} Menu;

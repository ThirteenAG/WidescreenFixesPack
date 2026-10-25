#include "stdafx.h"
#include "FixSettings.hpp"
#include "NativeMenu.hpp"
#include "Camera.hpp"
#include "Hud.hpp"
#include "Intro.hpp"
#include "Window.hpp"
#include "Movie.hpp"

void Init()
{
    WindowedModeWrapper::EnableDpiAwareness();
    auto& settings = FixSettings();
    asi_settings::LiveLink::Start();
    if (!DeerMovie::Init())
        OutputDebugStringA("Deer Avenger 4: native movie scaling contracts unavailable.\n");
    if (!DeerWindow::Init(settings.Int("MAIN.WindowMode")))
        OutputDebugStringA("Deer Avenger 4: native window mode contracts unavailable.\n");
    if (!DeerIntro::Init(settings.Bool("MAIN.SkipIntro")))
        OutputDebugStringA("Deer Avenger 4: startup video decision unavailable; native preference retained.\n");
    if (!DeerHud::Init(settings.Bool("MAIN.FixHUD")))
        OutputDebugStringA("Deer Avenger 4: gameplay HUD correction unavailable; native layout retained.\n");
    if (!DeerCamera::Init(settings.Bool("MAIN.FixAspectRatio")))
        OutputDebugStringA("Deer Avenger 4: camera/FOV correction incomplete; native signatures or hook unavailable.\n");
}

void InitWithMenu()
{
    Init();
    HMODULE owner = nullptr;
    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&Init), &owner))
        DeerMenu::Init(owner);
}

ASI_SETTINGS_EXPORT(FixSettings())

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
            CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(InitWithMenu, hook::pattern("83 EC 58 53 56 57 89 65 E8"));
        });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    return TRUE;
}
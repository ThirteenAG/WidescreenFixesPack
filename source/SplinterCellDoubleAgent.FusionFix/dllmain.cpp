#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/WidescreenFixConflict.hpp"
#include "Shared/SplinterCell/SaveLocation.hpp"

ASI_SETTINGS_EXPORT(FixSettings())

import ComVars;
import Settings;
import FrameRate;
import LED;
import Window;
import Engine;
import D3DDrv;
import EchelonMenus;
import Echelon;
import Core;
import WinDrv;
import Xidi;
import Online;
import Eax;
import GUI;
import WidescreenHUD;
import NativeMenu;
import Updates;

// MAIN.SaveLocation: the profiles and saves in Documents or the game folder instead of ProgramData
std::wstring SaveFolder;
SafetyHookInline shGetSaveFolder = {};
int __fastcall GetSaveFolder(void* fileManager, void* edx, wchar_t* out)
{
    SHCreateDirectoryExW(nullptr, SaveFolder.c_str(), nullptr);
    wcscpy_s(out, MAX_PATH, (SaveFolder + L"\\").c_str());
    return 1;
}

void InitSaveLocation()
{
    switch (FixSettings().Int("MAIN.SaveLocation"))
    {
        case 1:
            SaveFolder = SCSaveLocation::Documents(L"Splinter Cell Double Agent").wstring();
            break;
        case 2:
            SaveFolder = (SCSaveLocation::GameFolder() / L"Save").wstring();
            break;
        default:
            return;
    }
    // No Documents folder: the game keeps its own
    if (SaveFolder.empty())
        return;
    auto pattern = hook::pattern("55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 81 EC 38 05 00 00");
    shGetSaveFolder = safetyhook::create_inline(pattern.get_first(), GetSaveFolder);
}

void Init()
{
    InitSaveLocation();
    WFP::onInitEvent().executeAll();

    //for test only (steam version)
    //CallbackHandler::RegisterCallback([]()
    //{
    //	auto pattern = hook::pattern("89 85 D8 61 00 00");
    //	struct StartupHook
    //	{
    //		void operator()(injector::reg_pack& regs)
    //		{
    //			*(uint32_t*)(regs.ebp + 0x61D8) = regs.eax;
    //			MessageBox(0, 0, L"test", 0);
    //		}
    //	}; injector::MakeInline<StartupHook>(pattern.get_first(0), pattern.get_first(6));
    //}, hook::pattern("89 85 D8 61 00 00").count_hint(1).empty(), 0x1100);
}

// Ensure Gear registers the drive containing the game when its 10-prefix limit is reached.
void InitGear()
{
    static std::string gameDrive = GetExeModulePath().root_path().string();

    if (gameDrive.size() != 3 || gameDrive[1] != ':' ||
        (gameDrive[2] != '\\' && gameDrive[2] != '/'))
    {
        return;
    }

    gameDrive[2] = '\\';

    auto pattern = hook::module_pattern(
        GetModuleHandle(L"gear_RD"),
        "8D 9E A4 02 00 00 89 5D FC C7 45 F8 34 04 00 00"
    );

    static auto GearDrivePrefixHook =
        safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        auto* device = reinterpret_cast<uint8_t*>(regs.esi);
        auto* prefixCount = reinterpret_cast<uint32_t*>(device + 0x14);
        auto* prefixes = reinterpret_cast<const char**>(device + 0x18);

        if (*prefixCount == 0 || *prefixCount > 10)
        {
            return;
        }

        for (uint32_t i = 0; i < *prefixCount; ++i)
        {
            if (prefixes[i] != nullptr &&
                _stricmp(prefixes[i], gameDrive.c_str()) == 0)
            {
                return;
            }
        }

        if (*prefixCount < 10)
        {
            prefixes[*prefixCount] = gameDrive.c_str();
            ++(*prefixCount);
        }
        else
        {
            prefixes[9] = gameDrive.c_str();
        }
    });
}


CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        if (WidescreenFixConflict::Found())
            return;
        // Real pixels on scaled desktops: the desktop resolution, and a full screen picture not drawn into a corner
        WindowedModeWrapper::EnableDpiAwareness();
        FixSettings();
        asi_settings::LiveLink::Start();
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("74 ? 68 ? ? ? ? 53 FF D7"));
        CallbackHandler::RegisterCallback(L"Window.dll", InitWindow);
        CallbackHandler::RegisterCallback(L"Engine.dll", InitEngine);
        CallbackHandler::RegisterCallback(L"D3DDrv.dll", InitD3DDrv);
        CallbackHandler::RegisterCallback(L"gear_RD.dll", InitGear);
        CallbackHandler::RegisterCallback(L"EchelonMenus.dll", [] { InitEchelonMenus(); DAMenu::Init(); NativeWindowsMenuHelp = DAMenu::WindowsHelp; });
        CallbackHandler::RegisterCallback(L"Echelon.dll", InitEchelon);
        CallbackHandler::RegisterCallback(L"Core.dll", []() { InitCore(); InitUpdateCheck(); });
        CallbackHandler::RegisterCallback(L"WinDrv.dll", InitWinDrv);
        CallbackHandler::RegisterCallback(L"Xidi.32.dll", InitXidi);
        CallbackHandler::RegisterCallback(L"eax.dll", InitEAX);

        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(InitOnline, hook::pattern("8B F9 8B 47 30 85 C0 74 18"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        WFP::onShutdownEvent().executeAll();
    }
    return TRUE;
}

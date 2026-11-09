module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <shlobj.h>
#include <shlwapi.h>
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")

export module Misc;

import ComVars;
import Resolution;
import Camera;
import Widescreen;
import Windowed;
import Canvas;
import FrameRate;
import Updates;
import Cursor;
import Hud;
import Intro;

namespace Shadows
{
    void Init()
    {
        auto& settings = FixSettings();

        // Each character is drawn from above into its own shadow texture, which is then projected onto the ground.
        // Shadows are enlarged in that projection, so the texture keeps the whole silhouette; zooming into the
        // texture would cut off whatever lies near its edges. The size grows by half the setting's distance from 1
        // (1.7 makes the shadows about 1.35 times larger).
        const float scale = std::clamp(settings.Float("MISC.ShadowScale"), 0.1f, 10.0f);
        if (scale != 1.0f)
        {
            const float growth = 1.0f + (scale - 1.0f) * 0.5f;
            auto projection = *hook::get_pattern<float*>("68 ? ? ? ? 8D 4C 24 44 51 8B D1 52 E8", 1);
            injector::WriteMemory(&projection[0], projection[0] / growth, true);
            injector::WriteMemory(&projection[5], projection[5] / growth, true);
        }

        // The character shadows are drawn into a 256x256 texture; every place that knows that size follows the setting
        const int size = std::clamp(settings.Int("GRAPHICS.ShadowResolution"), 16, 8192);
        if (size == 256)
            return;
        const float fsize = float(size);
        injector::WriteMemory(hook::get_pattern("68 00 01 00 00 E8 ? ? ? ? 83 C4 04 85 C0 89 06 74 59", 1), size, true);
        injector::WriteMemory(hook::get_pattern("B8 FC 00 00 00 6A 00 6A 00 89 44 24 18", 1), size - 4, true);
        const auto shadowSetup = *hook::get_pattern<uintptr_t>("68 ? ? ? ? E8 ? ? ? ? 83 C4 04 BF", 1);
        injector::WriteMemory(shadowSetup + 0x14, size, true);
        auto quad = hook::pattern("C7 44 24 3C 00 00 00 00 C7 44 24 40 00 00 80 43 C7 44 24 58 00 00 80 43 C7 44 24 5C 00 00 00 00 "
            "C7 44 24 74 00 00 80 43 C7 44 24 78 00 00 80 43");
        for (const int offset : { 0x0C, 0x14, 0x24, 0x2C })
            injector::WriteMemory(quad.get_first(offset), fsize, true);
    }
}

// MISC.CustomUserFilesDirectoryInGameDir: saves and sonic_h.ini go to a folder in the game folder instead of
// %APPDATA%\SEGA\SONICHEROES
namespace SaveFolder
{
    std::filesystem::path Folder;

    HRESULT WINAPI GetFolderPath(HWND window, int folder, HANDLE token, DWORD flags, LPSTR path)
    {
        if ((folder & 0xFF) != CSIDL_APPDATA)
            return SHGetFolderPathA(window, folder, token, flags, path);
        const auto text = Folder.string();
        if (text.size() >= MAX_PATH)
            return E_FAIL;
        std::error_code error;
        std::filesystem::create_directories(Folder, error);
        strcpy_s(path, MAX_PATH, text.c_str());
        return S_OK;
    }

    // The game appends its own SEGA\SONICHEROES folder to the application data folder; the chosen folder replaces both
    BOOL WINAPI PathAppend(LPSTR path, LPCSTR more)
    {
        std::string_view folder = more ? more : "";
        if (folder.starts_with(".\\"))
            folder.remove_prefix(2);
        if (folder.size() >= 16 && _strnicmp(folder.data(), "SEGA\\SONICHEROES", 16) == 0 && folder.find_first_not_of('\\', 16) == std::string_view::npos)
        {
            PathAddBackslashA(path); // the game adds file names right after it
            return TRUE;
        }
        return PathAppendA(path, more);
    }

    void Init()
    {
        const auto name = FixSettings().String("MISC.CustomUserFilesDirectoryInGameDir");
        if (name.empty() || name == "0")
            return;
        Folder = GetExeModulePath<std::filesystem::path>() / name;
        const auto module = GetModuleHandleW(nullptr);
        IATHook::Replace(module, "SHFOLDER.dll", std::forward_as_tuple("SHGetFolderPathA", GetFolderPath));
        IATHook::Replace(module, "SHLWAPI.dll", std::forward_as_tuple("PathAppendA", PathAppend));
    }
}

namespace Options
{
    // DirectInput mouse: never created, so it can't move the camera or wake the cursor
    HRESULT __stdcall NoMouse(void*, const GUID*, void** device, void*)
    {
        if (device)
            *device = nullptr;
        return E_FAIL;
    }

    SafetyHookMid DemoHook;
    SafetyHookInline ObjectsHook;
    float ObjectScale = 1.0f;
    int ObjectMinimum = 0;
    uintptr_t ObjectTable = 0;

    // The game sets the objects' visibility distances when a stage loads; the setting scales them afterwards
    void __cdecl ScaleObjects()
    {
        ObjectsHook.ccall();
        auto entry = *reinterpret_cast<uint8_t**>(ObjectTable);
        if (!entry)
            return;
        for (int i = 0; i < 2048; ++i, entry += 48)
        {
            auto& distance = entry[43];
            distance = uint8_t(std::clamp(int(distance * ObjectScale), ObjectMinimum, 255));
        }
    }

    void Init()
    {
        auto& settings = FixSettings();

        if (settings.Bool("MISC.DisableMouseInput"))
        {
            auto create = hook::pattern("FF 51 0C 85 C0 0F 8C ? ? ? ? A1 ? ? ? ? 8B 10 68");
            const auto skip = create.get_first<uint8_t>(11) + *create.get_first<int32_t>(7); // the jl's target
            injector::MakeCALL(create.get_first(), NoMouse, true);
            injector::MakeJMP(create.get_first(5), skip, true);
        }

        // Castle and mansion switches: the PlayStation 2 blend of the flash (6 is the PC's)
        if (const int flash = std::clamp(settings.Int("MISC.WarpSwitchEffectMode"), 0, 2); flash != 0)
            injector::WriteMemory<uint8_t>(hook::get_pattern("6A 02 6A 0A E8 ? ? ? ? 6A ? EB 0B 6A 05", 0x0A), uint8_t(flash), true);

        // Always one game step per frame at the configured rate, instead of catching up with skipped frames
        if (settings.Bool("MISC.DisableFrameSkipping"))
            injector::WriteMemory<uint8_t>(hook::get_pattern("84 C9 74 24 A1 ? ? ? ? 2B 05 ? ? ? ? ? 89 87 78 02 00 00", 2), 0xEB, true);

        // The title screen's attract mode cycles through all six demos instead of only the first
        if (settings.Bool("MISC.RestoreDemos"))
        {
            DemoHook = safetyhook::create_mid(hook::get_pattern("89 50 3C 41 3B CB 7E 02 33 C9 88 58 27 88 58 1E", 0x0A), [](SafetyHookContext& context)
            {
                const int current = *reinterpret_cast<int8_t*>(context.eax + 0x19);
                context.ecx = uint32_t((current + 1) % 6);
            });
        }

        // The retail game looks for its disc; the data in the game folder is used instead
        {
            auto search = hook::pattern("E8 ? ? ? ? A0 ? ? ? ? 84 C0 0F 85 ? ? ? ? 8B 1D");
            injector::MakeNOP(search.get_first(), 5, true);
            injector::WriteMemory<uint16_t>(search.get_first(12), 0xE990, true);
        }

        // Closing the window quits without the question, as if it was answered with yes
        if (settings.Bool("MISC.DisableQuitDialog"))
        {
            static auto answer = *hook::get_pattern<DLGPROC>("68 ? ? ? ? 50 68 83 00 00 00 52 FF D7", 1);
            static auto ask = [](HINSTANCE instance, LPCSTR name, HWND parent, DLGPROC procedure, LPARAM parameter) -> INT_PTR
            {
                if (procedure == answer && name == MAKEINTRESOURCEA(0x83))
                {
                    procedure(nullptr, WM_COMMAND, IDOK, 0);
                    return IDOK;
                }
                return DialogBoxParamA(instance, name, parent, procedure, parameter);
            };
            IATHook::Replace(GetModuleHandleW(nullptr), "USER32.dll",
                std::forward_as_tuple("DialogBoxParamA", static_cast<INT_PTR(WINAPI*)(HINSTANCE, LPCSTR, HWND, DLGPROC, LPARAM)>(ask)));
        }

        // Loading screens: the game loads without drawing its animated loading screen in between
        if (settings.Bool("MISC.DisableLoadingTimer"))
            injector::WriteMemory<std::array<uint8_t, 3>>(hook::get_pattern("A1 ? ? ? ? 3B C6 77 27 8B CE 2B C8 85 C9", -0x2F), { 0x33, 0xC0, 0xC3 }, true);

        if (settings.Bool("MISC.DisableSubtitles"))
            injector::WriteMemory<std::array<uint8_t, 5>>(hook::get_pattern("A1 ? ? ? ? 83 EC 14 53 8A 58 11 8B 40 38 33 C9"), { 0x33, 0xC0, 0xC2, 0x1C, 0x00 }, true);

        if (settings.Bool("MISC.IncreaseObjectDistance"))
        {
            ObjectScale = std::clamp(settings.Float("GRAPHICS.DrawDistance"), 0.0f, 255.0f);
            ObjectMinimum = std::clamp(settings.Int("MISC.MinObjDistance"), 0, 255);
            auto objects = hook::pattern("A1 ? ? ? ? 53 55 56 57 33 F6 8D 48 2B BF 00 08 00 00");
            ObjectTable = *objects.get_first<uintptr_t>(1);
            ObjectsHook = safetyhook::create_inline(objects.get_first(), &ScaleObjects);
        }

        // Clip_Range 3 in sonic_h.ini: the custom camera clip range
        const auto clipRanges = *hook::get_pattern<float*>("D8 0C 85 ? ? ? ? A1 ? ? ? ? 83 F8 14", 3);
        injector::WriteMemory(&clipRanges[3], std::clamp(settings.Float("MISC.ClipRange"), 0.01f, 100.0f), true);
    }
}

class Misc
{
public:
    Misc()
    {
        WFP::onInitEvent() += []()
        {
            asi_settings::LiveLink::Start();
            SonicCursor::Initialize();

            Resolution::Init(Windowed::CoversMonitor());
            Windowed::Init();
            Widescreen::Init();
            Canvas::Init();
            Shadows::Init();
            SaveFolder::Init();
            Options::Init();
            FrameRate::Init();

            SonicHud::Init(&Screen.fWidth, &Screen.fHeight, &Screen.fHudScale, &GameplayFov::Factor, &GameplayFov::ProjectionAdjusted);
            SonicIntro::Init();
            InitUpdates();
        };
    }
} Misc;

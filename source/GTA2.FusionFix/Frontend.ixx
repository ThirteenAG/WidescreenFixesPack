module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/GTAClassic/InputDevices.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Frontend;

import ComVars;
import Hud;
import Camera;
import Input;
import Images;
import Options;
import WindowProc;

// The frontend: its art and text scaled into the 4:3 box, the intro movie, the credits and the Options page
SafetyHookInline frontendPageHook;

void InstallOptions(void* setPage)
{
    using namespace GTA2Options;
    using namespace InputDevices;
    ResX = &nResX; ResY = &nResY;
    auto reload = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? 84 C0 75 09 A0 ? ? ? ? 84 C0 74 11");
    ReloadVideoMode = reinterpret_cast<decltype(ReloadVideoMode)>(reload.get_first());
    ApplyResolution = []
    {
        // The video mode is re-read from the patched settings and reset, as after alt-tab.
        if (fullscreenWidth.is_initialized()) fullscreenWidth = nResX;
        if (fullscreenHeight.is_initialized()) fullscreenHeight = nResY;
        ReloadVideoMode();
        return true;
    };
    // Original GTA2 Manager settings applied at once where the game allows it.
    static void* sound = nullptr;
    static void (__fastcall* setMusicVolume)(void*, int, int) = nullptr;
    static void (__fastcall* setSoundVolume)(void*, int, int) = nullptr;
    auto volume = hook::pattern("68 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? 50 B9 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? 50 B9 ? ? ? ? E8");
    {
        sound = *volume.get_first<void*>(17);
        setMusicVolume = reinterpret_cast<decltype(setMusicVolume)>(injector::GetBranchDestination(volume.get_first(21)).get<void>());
        setSoundVolume = reinterpret_cast<decltype(setSoundVolume)>(injector::GetBranchDestination(volume.get_first(47)).get<void>());
    }
    auto gamma = hook::pattern("C7 05 ? ? ? ? 1E 00 00 00 C3");
    nGammaRefresh.SetAddress(*gamma.get_first<int*>(2));

    // Languages: the text files present in data (e.gxt, f.gxt, ...).
    static std::vector<std::pair<char, std::wstring>> languages;
    const std::pair<char, const wchar_t*> names[] = { {'e', L"ENGLISH"}, {'f', L"FRENCH"}, {'g', L"GERMAN"}, {'i', L"ITALIAN"},
        {'s', L"SPANISH"}, {'j', L"JAPANESE"} };
    for (auto [code, name] : names)
        if (std::filesystem::exists(std::filesystem::path("data") / (std::string(1, code) + ".gxt"))) languages.emplace_back(code, name);
    Entry language{ L"LANGUAGE", []
        {
            auto code = ReadText("Option", "Language", "e");
            for (auto& [c, name] : languages) if (code.size() && c == code[0]) return name;
            return Upper(std::wstring(code.begin(), code.end()));
        },
        [](int step)
        {
            if (languages.empty()) return;
            auto code = ReadText("Option", "Language", "e");
            size_t index = 0;
            for (size_t i = 0; i < languages.size(); ++i) if (code.size() && languages[i].first == code[0]) index = i;
            index = (index + languages.size() + (step < 0 ? -1 : 1)) % languages.size();
            WriteText("Option", "Language", std::string(1, languages[index].first));
        }, NoAction, nullptr, "SETUP.Option.Language" };

    static const std::vector<float> times = { 0.0f, 0.1f, 0.15f, 0.2f, 0.25f, 0.35f, 0.5f, 0.75f, 1.0f };
    enum { Root, Display, Graphics, Sound, CameraPage, ControlsPage, Keys, MoreKeys, ClassicKeys, ClassicKeys2, GamePage };
    Sections = {
        { Open(L"DISPLAY", Display), Open(L"GRAPHICS", Graphics), Open(L"SOUND", Sound), Open(L"CAMERA", CameraPage),
          Open(L"CONTROLS", ControlsPage), Open(L"GAME", GamePage), Back() },
        { Resolution(),
          Entry{ L"WINDOW MODE", [] { return std::wstring(windowMode == 1 ? L"WINDOWED" : windowMode == 2 ? L"BORDERLESS" : L"FULLSCREEN"); },
              [](int step) { SaveInt("MAIN", "WindowMode", (windowMode + (step < 0 ? 2 : 1)) % 3); },
              NoAction, nullptr, "MAIN.WindowMode" },
          Toggle(L"FILL MENU BACKGROUND", "MAIN", "FillFrontendBackground", bFillBackground),
          Entry{ L"HUD ASPECT RATIO", [] { auto& value = FixSettings().String("MAIN.HudAspectRatioConstraint"); return Upper(std::wstring(value.begin(), value.end())); },
              [](int step)
              {
                  static const char* values[] = { "Auto", "4:3", "16:9", "21:9", "32:9" };
                  auto& current = FixSettings().String("MAIN.HudAspectRatioConstraint");
                  int index = 0;
                  for (int i = 0; i < int(std::size(values)); ++i) if (_stricmp(current.c_str(), values[i]) == 0) index = i;
                  index = (index + int(std::size(values)) + (step < 0 ? -1 : 1)) % int(std::size(values));
                  FixSettings().Set("MAIN.HudAspectRatioConstraint", values[index]);
              }, NoAction, nullptr, "MAIN.HudAspectRatioConstraint" },
          RegToggle(L"TRIPLE BUFFER", "Screen", "tripple_buffer", 0), Back() },
        { RegRange(L"GAMMA", "Screen", "gamma", 0, 20, 1, 10, [](int) { nGammaRefresh = 1; }),
          RegToggle(L"LIGHTING", "Screen", "lighting", 1),
          RegToggle(L"EXPLODING SCORES", "Screen", "exploding_on", 1),
          RegToggle(L"FRAME LIMITER", "Screen", "max_frame_rate", 1),
          RegToggle(L"FRAME SKIP", "Screen", "min_frame_rate", 0),
          RegToggle(L"SPECIAL RECOGNITION", "Screen", "special_recognition", 1), Back() },
        { RegRange(L"MUSIC VOLUME", "Sound", "CDVol", 0, 127, 8, 127, [](int value) { if (setMusicVolume) setMusicVolume(sound, 0, value); }),
          RegRange(L"SFX VOLUME", "Sound", "SFXVol", 0, 127, 8, 127, [](int value) { if (setSoundVolume) setSoundVolume(sound, 0, value); }),
          RegToggle(L"3D SOUND", "Sound", "do_3d_sound", 0), Back() },
        { Toggle(L"ROTATING CAMERA", "CAMERA", "RotateWithPlayer", GTA2Camera::Enabled),
          Toggle(L"ROTATE IN VEHICLES", "CAMERA", "RotateInVehicles", GTA2Camera::RotateVehicle),
          Toggle(L"ROTATE ON FOOT", "CAMERA", "RotateOnFoot", GTA2Camera::RotateFoot),
          Choice(L"ROTATION SMOOTHING", "CAMERA", "RotationSmoothing", GTA2Camera::Smoothing, times),
          Toggle(L"SMOOTH FOLLOW", "CAMERA", "SmoothFollow", GTA2Camera::SmoothFollow),
          Choice(L"FOLLOW TIME", "CAMERA", "FollowTime", GTA2Camera::FollowTime, times),
          Choice(L"HEIGHT TIME", "CAMERA", "HeightTime", GTA2Camera::HeightTime, times),
          Choice(L"MAX LOOK AHEAD", "CAMERA", "MaxLookAhead", GTA2Camera::MaxLead, { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f }), Back() },
        { Toggle(L"MODERN CONTROLS", "INPUT", "ModernControls", ModernControls),
          Toggle(L"GAMEPAD", "INPUT", "Gamepad", Gamepad),
          Toggle(L"STICK THROTTLE", "INPUT", "StickThrottle", StickThrottle),
          Toggle(L"MOUSE AIM", "INPUT", "MouseAim", GTA2Input::MouseAim),
          Choice(L"STICK DEADZONE", "INPUT", "StickDeadzone", Deadzone, { 0.05f, 0.1f, 0.15f, 0.2f, 0.25f, 0.3f, 0.4f }),
          Open(L"MODERN KEYS", Keys), Open(L"MORE MODERN KEYS", MoreKeys),
          Open(L"CLASSIC KEYS", ClassicKeys), Open(L"MORE CLASSIC KEYS", ClassicKeys2), Back() },
        { Key(L"MOVE UP", "INPUT", "MoveUpKey", MoveUpKey), Key(L"MOVE DOWN", "INPUT", "MoveDownKey", MoveDownKey),
          Key(L"MOVE LEFT", "INPUT", "MoveLeftKey", MoveLeftKey), Key(L"MOVE RIGHT", "INPUT", "MoveRightKey", MoveRightKey),
          Key(L"FIRE", "INPUT", "FireKey", FireKey), Key(L"ENTER VEHICLE", "INPUT", "EnterVehicleKey", EnterVehicleKey),
          Key(L"JUMP", "INPUT", "JumpKey", JumpKey), Key(L"SPECIAL", "INPUT", "SpecialKey", SpecialKey), Back() },
        { Key(L"PREVIOUS WEAPON", "INPUT", "PreviousWeaponKey", PreviousWeaponKey),
          Key(L"NEXT WEAPON", "INPUT", "NextWeaponKey", NextWeaponKey), Key(L"PAUSE", "INPUT", "PauseKey", PauseKey),
          Key(L"QUICKSAVE", "MISC", "QuicksaveKey", nQuicksaveKey), Key(L"ZOOM IN", "MISC", "ZoomIncreaseKey", nZoomIncreaseKey),
          Key(L"ZOOM OUT", "MISC", "ZoomDecreaseKey", nZoomDecreaseKey),
          Key(L"RECOVER PLAYER", "MISC", "RecoverPlayerKey", GTA2Input::RecoveryKey),
          Key(L"TOGGLE ROTATION", "CAMERA", "ToggleRotationKey", GTA2Camera::ToggleKey), Back() },
        { NativeKey(L"UP", 0), NativeKey(L"DOWN", 1), NativeKey(L"LEFT", 2), NativeKey(L"RIGHT", 3),
          NativeKey(L"ATTACK", 4), NativeKey(L"ENTER/EXIT", 5), NativeKey(L"JUMP", 6), Back() },
        { NativeKey(L"PREVIOUS WEAPON", 7), NativeKey(L"NEXT WEAPON", 8), NativeKey(L"SPECIAL 1", 9),
          NativeKey(L"SPECIAL 2", 10), NativeKey(L"SPECIAL 3", 11), Back() },
        { language, RegRange(L"TEXT SPEED", "Option", "text_speed", 1, 3, 1, 3),
          RegToggle(L"PLAY INTRO MOVIE", "Screen", "do_play_movie", 1),
          Toggle(L"SKIP INTRO MOVIE", "MAIN", "SkipIntro", bSkipMovie),
          Toggle(L"SKIP CREDITS", "MISC", "SkipCredits", bSkipCredits),
          RegToggle(L"SHOW PLAYER NAMES", "Network", "show_player_names", 1), Back() },
    };
    GTA2Options::Install(setPage);
}

int __fastcall SetFrontendPage(void* menu, int, uint16_t page)
{
    GTA2Options::Opening(static_cast<char*>(menu), page);
    // Page 9 is shared by Quit, Escape from the menu, and name entry.
    // Request normal frontend shutdown, allowing the engine to clean up.
    if (page == 9)
        RWGTA::UpdateCheck::OnExit();
    if (page == 9 && bSkipCredits)
    {
        *reinterpret_cast<int*>(static_cast<char*>(menu) + 264) = 1;
        return 0;
    }
    return frontendPageHook.thiscall<int>(menu, page);
}

void ReposAndScaleFontCall(void* addr)
{
    static injector::hook_back<void(__stdcall*)(const wchar_t*, int, int, int, int, const int*, int, bool, int)> printString;
    auto f = [](const wchar_t* str, int x, int y, int style, int scale, const int* mode, int palette, bool enableAlpha, int alpha) {
        Repos(x, y);
        Rescale(scale);
        printString.fun(str, x, y, style, scale, mode, palette, enableAlpha, alpha);
    };
    printString.fun = injector::GetBranchDestination(addr).get();
    injector::MakeCALL(addr, (void(__stdcall*)(const wchar_t*, int, int, int, int, const int*, int, bool, int))f);
}

void ReposAndScaleSpriteCall(void* addr)
{
    static injector::hook_back<void(__stdcall*)(int, int, int, int, int, int, const int*, int, int, int, int)> drawSprite;
    auto f = [](int id1, int id2, int x, int y, int angle, int scale, const int* mode, int enableAlpha, int alpha, int a10, int lightFlag) {
        Repos(x, y);
        Rescale(scale);
        drawSprite.fun(id1, id2, x, y, angle, scale, mode, enableAlpha, alpha, a10, lightFlag);
    };
    drawSprite.fun = injector::GetBranchDestination(addr).get();
    injector::MakeCALL(addr, (void(__stdcall*)(int, int, int, int, int, int, const int*, int, int, int, int))f);
}

export void InitD3DDLL()
{
    auto module = GetModuleHandleW(L"d3ddll.dll");
    GTA2Images::Install(module);
    SettingsReader iniReader("");
    auto pattern = hook::module_pattern(module, "89 7C 24 28 89 74 24 2C");
    struct gbh_BlitImageHook
    {
        void operator()(injector::reg_pack& regs)
        {
            RECT& dest = (*(RECT*)(regs.esp + 0x30 - 0x10));
            RECT& source = *reinterpret_cast<RECT*>(regs.esp + 0x10);
            // The frontend splits its 640x480 art into adjacent TGA panels.
            // Crop them as one canvas so the seam stays aligned at every aspect.
            if (bFillBackground && regs.ebx == 0 && regs.esi == 480 &&
                source.top == 0 && source.bottom == 480)
            {
                int width = int(screen_width()), height = int(screen_height());
                double scale = std::max(width / 640.0, height / 480.0);
                double ox = (width - 640 * scale) / 2;
                double oy = (height - 480 * scale) / 2;
                double left = std::max(0.0, regs.ebp * scale + ox);
                double right = std::min(double(width), regs.edi * scale + ox);
                source.left += LONG(std::lround((left - ox) / scale - regs.ebp));
                source.right -= LONG(std::lround(regs.edi - (right - ox) / scale));
                source.top = std::clamp<LONG>(LONG(std::lround(-oy / scale)), 0, 479);
                source.bottom = std::clamp<LONG>(LONG(std::lround((height - oy) / scale)), source.top + 1, 480);
                dest = {LONG(std::lround(left)), 0, LONG(std::lround(right)), height};
                return;
            }
            dest.left = static_cast<LONG>(std::lround(regs.ebp * hud_scale()));
            dest.top = static_cast<LONG>(std::lround(regs.ebx * hud_scale()));
            dest.right = static_cast<LONG>(std::lround(regs.edi * hud_scale()));
            dest.bottom = static_cast<LONG>(std::lround(regs.esi * hud_scale()));

            dest.left += (int32_t)(hud_offset() / 2);
            dest.right += (int32_t)(hud_offset() / 2);
        }
    }; injector::MakeInline<gbh_BlitImageHook>(pattern.get_first(0), pattern.get_first(8));
}

export namespace GTA2Frontend
{
    void Install()
    {
        // Frontend
        auto pattern = hook::pattern("E8 ? ? ? ? 8B 44 24 1C 8B 54 24 20");
        ReposAndScaleFontCall(pattern.get_first(0)); // 0x453799

        pattern = hook::pattern("E8 ? ? ? ? FE 44 24 11");
        ReposAndScaleFontCall(pattern.get_first(-9210)); // 0x453A1D

        pattern = hook::pattern("E8 ? ? ? ? 8B 44 24 30 8B 4C 24 18");
        ReposAndScaleFontCall(pattern.get_first(0)); // 0x4567DC

        pattern = hook::pattern("E8 ? ? ? ? 66 A1 ? ? ? ? 68 ? ? ? ?");
        ReposAndScaleFontCall(pattern.get_first(-4085)); // 0x456A17

        pattern = hook::pattern("E8 ? ? ? ? 8B 7C 24 1C 8A 44 24 30");
        ReposAndScaleFontCall(pattern.get_first(0)); // 0x4570A7

        pattern = hook::pattern("EB 7E 66 8B 6D 6C");
        ReposAndScaleFontCall(pattern.get_first(128)); // 0x4580C1

        pattern = hook::pattern("E8 ? ? ? ? 8B 44 24 18 40");
        ReposAndScaleFontCall(pattern.get_first(0)); // 0x458421

        pattern = hook::pattern("E8 ? ? ? ? E9 ? ? ? ? 68 ? ? ? ? 57");
        ReposAndScaleSpriteCall(pattern.get_first(0)); // 0x453705

        pattern = hook::pattern(" E8 ? ? ? ? EB 44 8B 44 24 10");
        ReposAndScaleSpriteCall(pattern.get_first(0)); // 0x453753

        pattern = hook::pattern("E8 ? ? ? ? E9 ? ? ? ? 66 8B 5D 6A");
        ReposAndScaleSpriteCall(pattern.get_first(0)); // 0x4582EE

        pattern = hook::pattern("8B 15 ? ? ? ? 6A 06 52 8B 08 50"); //0x4B4FB8
        auto hwnd = *pattern.get_first<HWND*>(2);
        GTA2WindowProc::WatchWindow(hwnd);

        if (bSkipMovie)
        {
            //skip movie to prevent windowed crash
            pattern = hook::pattern("8A 88 ? ? ? ? 88 88 ? ? ? ? 40 84 C9 ? ? B8 ? ? ? ? C3");
            injector::WriteMemory<uint8_t>(*pattern.get_first<uint8_t*>(2), '_', true); //0x459695+2
        }

        pattern = hook::pattern("81 EC 00 01 00 00 55 56 57 8B F1 E8 ? ? ? ? 8B AC 24 10 01 00 00");
        frontendPageHook = safetyhook::create_inline(pattern.get_first(), SetFrontendPage);
        InstallOptions(pattern.get_first());
    }
}

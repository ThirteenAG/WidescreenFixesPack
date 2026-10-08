#include "stdafx.h"
#include "Log.h"
#include "InputDevices.h"
#include "GameRegistry.h"
#include "Desktop.h"
#include "Camera.h"
#include "Input.h"
#include "Movies.h"
#include "Images.h"

uint32_t* window_width;
uint32_t* window_height;

int RenderDimension(int offset, uint32_t* configured, int fallback)
{
    // A bordered desktop-sized window can have a smaller client area. Match
    // the surface actually allocated by the video backend, including resize.
    auto video = GTA2Movies::Video ? *GTA2Movies::Video : nullptr;
    if (video && *reinterpret_cast<void**>(video + 312))
    {
        int value = *reinterpret_cast<int*>(video + offset);
        if (value > 0 && value <= 7680) return value;
    }
    return configured ? int(*configured) : fallback;
}

#define default_screen_width (640.0f)
#define default_screen_height (480.0f)
#define default_aspect_ratio (default_screen_width / default_screen_height)

#define screen_width (float(RenderDimension(72, window_width, 640)))
#define screen_height (float(RenderDimension(76, window_height, 480)))
#define screen_aspect_ratio (screen_width / screen_height)

#define fone (16384.0f)
#define one (16384)
#define camera_scale (screen_width / default_screen_width)
#define hud_scale (screen_height / default_screen_height)
#define default_hud_scale (screen_width / default_screen_width)

#define hud_offset (screen_width - screen_height * default_aspect_ratio)

typedef int Sint32;
typedef Sint32 SCR_f;
typedef unsigned int undefined4;
typedef unsigned int uint;
typedef unsigned short undefined2;

struct CameraPos
{
    SCR_f x;
    SCR_f y;
    SCR_f z;
    int zoom;
};

struct WorldRect
{
    SCR_f left;
    SCR_f right;
    SCR_f top;
    SCR_f bottom;
};

struct CameraOrPhysics
{
    CameraPos cameraPosTarget2;
    CameraPos cameraPosTarget;
    WorldRect cameraBoundaries;
    enum PLAYER_PHYSICS_MOVEMENT movementBitmask;
    struct Ped* ped;
    undefined4 field_0x38;
    int followedPedID;
    int targetElevation;
    int flyTimerMaybe;
    SCR_f altMovingPosX;
    SCR_f altMovingPosY;
    SCR_f altMovingStateUp;
    SCR_f altMovingStateDown;
    SCR_f altMovingStateLeft;
    SCR_f altMovingStateRight;
    uint altMovingArrowsRelated;
    undefined2 field_0x64;
    short altMovingLimit;
    int screenPxWidth;
    int screenPxHeight;
    int screenPxCenterX;
    int screenPxCenter;
    WorldRect cameraBoundariesNonNegative;
    CameraPos cameraPos2;
    CameraPos cameraPos;
    int uiScale;
    CameraPos cameraVelocity;
};

int nResX;
int nResY;
bool bExtendHud;
int32_t nQuicksaveKey;
int32_t nZoomIncreaseKey;
int32_t nZoomDecreaseKey;
float fZoom;
bool bSkipMovie = true, bSkipCredits = true, bNoSampManDelay = true, bFillBackground = false;
WNDPROC wndProcOld = NULL;
SafetyHookInline frontendPageHook;
SafetyHookInline colorDepthHook;
SafetyHookInline screenSettingHook;
bool windowedMode = true;
int __stdcall ReadScreenSetting(const char* name, int fallback)
{
    auto value = screenSettingHook.stdcall<int>(name, fallback);
    if (windowedMode && strcmp(name, "start_mode") == 0) return 0;
    return value;
}
uint32_t* rendererReady = nullptr;
void __cdecl SetColourDepth()
{
    // WM_SETFOCUS can arrive inside DirectDraw mode creation, before gbh_InitDLL.
    if (*rendererReady) colorDepthHook.call<void>();
}
#include "Options.h"
int* fullscreenWidth = nullptr, * fullscreenHeight = nullptr;
void (__cdecl* ReloadVideoMode)() = nullptr;

void InstallOptions(void* setPage)
{
    using namespace GTA2Options;
    using namespace InputDevices;
    ResX = &nResX; ResY = &nResY;
    if (auto reload = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? 84 C0 75 09 A0 ? ? ? ? 84 C0 74 11"); reload.size() == 1)
        ReloadVideoMode = reinterpret_cast<decltype(ReloadVideoMode)>(reload.get_first());
    ApplyResolution = []
    {
        // The video mode is re-read from the patched settings and reset, as after alt-tab.
        if (fullscreenWidth) injector::WriteMemory(fullscreenWidth, nResX, true);
        if (fullscreenHeight) injector::WriteMemory(fullscreenHeight, nResY, true);
        if (ReloadVideoMode) ReloadVideoMode();
    };
    // Original GTA2 Manager settings applied at once where the game allows it.
    static void* sound = nullptr;
    static void (__fastcall* setMusicVolume)(void*, int, int) = nullptr;
    static void (__fastcall* setSoundVolume)(void*, int, int) = nullptr;
    if (auto volume = hook::pattern("68 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? 50 B9 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? 50 B9 ? ? ? ? E8"); volume.size() == 1)
    {
        sound = *volume.get_first<void*>(17);
        setMusicVolume = reinterpret_cast<decltype(setMusicVolume)>(injector::GetBranchDestination(volume.get_first(21)).get<void>());
        setSoundVolume = reinterpret_cast<decltype(setSoundVolume)>(injector::GetBranchDestination(volume.get_first(47)).get<void>());
    }
    static int* gammaRefresh = nullptr;
    if (auto gamma = hook::pattern("C7 05 ? ? ? ? 1E 00 00 00 C3"); gamma.size() == 1) gammaRefresh = *gamma.get_first<int*>(2);

    // Languages: the text files present in data (e.gxt, f.gxt, ...).
    static std::vector<std::pair<char, std::wstring>> languages;
    const std::pair<char, const wchar_t*> names[] = { {'e', L"ENGLISH"}, {'f', L"FRENCH"}, {'g', L"GERMAN"}, {'i', L"ITALIAN"},
        {'s', L"SPANISH"}, {'j', L"JAPANESE"} };
    for (auto [code, name] : names)
        if (std::filesystem::exists(std::filesystem::path("data") / (std::string(1, code) + ".gxt"))) languages.emplace_back(code, name);
    Entry language{ L"LANGUAGE (RESTART)", []
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
        } };

    static const std::vector<float> times = { 0.0f, 0.1f, 0.15f, 0.2f, 0.25f, 0.35f, 0.5f, 0.75f, 1.0f };
    enum { Root, Display, Graphics, Sound, CameraPage, ControlsPage, Keys, MoreKeys, ClassicKeys, ClassicKeys2, GamePage };
    Sections = {
        { Open(L"DISPLAY", Display), Open(L"GRAPHICS", Graphics), Open(L"SOUND", Sound), Open(L"CAMERA", CameraPage),
          Open(L"CONTROLS", ControlsPage), Open(L"GAME", GamePage), Back() },
        { Resolution(), Toggle(L"WINDOWED MODE", "MAIN", "WindowedMode", windowedMode, true),
          Toggle(L"FILL MENU BACKGROUND", "MAIN", "FillFrontendBackground", bFillBackground),
          Toggle(L"WIDE HUD", "MAIN", "ExtendHud", bExtendHud),
          RegToggle(L"TRIPLE BUFFER", "Screen", "tripple_buffer", 0, L" (RESTART)"), Back() },
        { RegRange(L"GAMMA", "Screen", "gamma", 0, 20, 1, 10, [](int) { if (gammaRefresh) *gammaRefresh = 1; }),
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
          Toggle(L"SKIP INTRO MOVIE", "MISC", "SkipMovie", bSkipMovie, true),
          Toggle(L"SKIP CREDITS", "MISC", "SkipCredits", bSkipCredits),
          RegToggle(L"SHOW PLAYER NAMES", "Network", "show_player_names", 1),
          Toggle(L"NO SOUND DELAY", "MISC", "NoSampManDelay", bNoSampManDelay, true), Back() },
    };
    GTA2Options::Install(setPage);
}

int __fastcall SetFrontendPage(void* menu, int, uint16_t page)
{
    GTA2Options::Opening(static_cast<char*>(menu), page);
    // Page 9 is shared by Quit, Escape from the menu, and name entry.
    // Request normal frontend shutdown, allowing the engine to clean up.
    if (page == 9 && bSkipCredits)
    {
        *reinterpret_cast<int*>(static_cast<char*>(menu) + 264) = 1;
        return 0;
    }
    return frontendPageHook.thiscall<int>(menu, page);
}

LRESULT APIENTRY WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    InputDevices::KeyboardMessage(uMsg, wParam);
    // Rebinding a key in the options page.
    int pressed = uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN ? int(wParam) : uMsg == WM_LBUTTONDOWN ? VK_LBUTTON :
        uMsg == WM_RBUTTONDOWN ? VK_RBUTTON : uMsg == WM_MBUTTONDOWN ? VK_MBUTTON :
        uMsg == WM_XBUTTONDOWN ? (GET_XBUTTON_WPARAM(wParam) == XBUTTON1 ? VK_XBUTTON1 : VK_XBUTTON2) : 0;
    if (pressed && GTA2Options::KeyDown(pressed)) return 0;
    switch (uMsg)
    {
    case WM_KEYDOWN:
        GTA2Input::QueueMenuKey(wParam);
        GTA2Camera::KeyDown(wParam);
        if (wParam == nZoomIncreaseKey) {
            fZoom += 1.0f;
        }
        else if (wParam == nZoomDecreaseKey) {
            fZoom -= 1.0f;
        }

        fZoom = std::max(0.0f, std::min(fZoom, 10.0f));
        break;
    case WM_KEYUP:
        break;
    case WM_KILLFOCUS:
        GTA2Input::MenuKeys = 0;
        GTA2Input::TextKey = 0;
        break;
    }

    return CallWindowProc(wndProcOld, hwnd, uMsg, wParam, lParam);
}

DWORD WINAPI WindowCheck(LPVOID hWnd)
{
    while (*(HWND*)hWnd == NULL)
        Sleep(10);

    wndProcOld = (WNDPROC)GetWindowLong(*(HWND*)hWnd, GWL_WNDPROC);
    InputDevices::Window = *(HWND*)hWnd;
    SetWindowLong(*(HWND*)hWnd, GWL_WNDPROC, (LONG)WndProc);
    return 0;
}

int posType = 0; // 0 = default, 1 = centered, 2 = unscaled

void RePositionElement(void* addr) {
    static injector::hook_back<int* (__fastcall*)(int*, int, int*, int*)> rePositionElement;
    auto f = [](int* in, int, int* out, int* scale) {
        out = rePositionElement.fun(in, 0, out, scale);

        float x = *out / fone;
        if (posType == 0) {
            // Shift this element approximatively.
            if (bExtendHud) {
                float f = screen_height / default_screen_height;
                if (x <= 140.0f * f) {
                }
                else if (x > 140.0f * f && x < 490.0f * f) {
                    x += hud_offset / 2;
                }
                else if (x >= 490.0f * f) {
                    x += hud_offset;
                }
            }
            else
                goto centered;
        }
        else if (posType == 1) {
centered:
            x += (hud_offset / 2);
        }
        else if (posType == 2) {
            x /= hud_scale;
            x *= default_hud_scale;
        }
        posType = 0;

        *out = (uint32_t)(x * one);
        return out;
    };
    rePositionElement.fun = injector::GetBranchDestination(addr).get();
    injector::MakeCALL(addr, (int*(__fastcall*)(int*, int, int*, int*))f);
}

void Rescale(int& scale) {
    float fs = (scale / fone) * hud_scale;
    scale = (int32_t)(fs * one);
}

void Repos(int& x, int& y) {
    float fx = (x / fone) * hud_scale;
    float fy = (y / fone) * hud_scale;
    fx += ((int32_t)(hud_offset / 2));

    x = (int32_t)(fx * one);
    y = (int32_t)(fy * one);
}

void Repos3d(int& x, int& y) {
    float fx = (x / fone) * hud_scale;
    float fy = (y / fone);
    //fx += ((int32_t)(hud_offset / 2));

    x = (int32_t)(fx * one);
    y = (int32_t)(fy * one);
}

template<int te>
void SetPosType(void* addr)
{
    static injector::hook_back<int* (__fastcall*)(int*, int, int)> encodeFlt;
    auto f = [](int* a, int, int b) {
        encodeFlt.fun(a, 0, b);
        posType = te;
    };
    encodeFlt.fun = injector::GetBranchDestination(addr).get();
    injector::MakeCALL(addr, (void(__fastcall*)(int*, int, int))f);
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

void ReposSpriteCall3D(void* addr)
{
    static injector::hook_back<void(__stdcall*)(int, int, int, int, int, int, const int*, int, int, int, int)> drawSprite;
    auto f = [](int id1, int id2, int x, int y, int angle, int scale, const int* mode, int enableAlpha, int alpha, int a10, int lightFlag) {
        Repos3d(x, y);
        drawSprite.fun(id1, id2, x, y, angle, scale, mode, enableAlpha, alpha, a10, lightFlag);
    };
    drawSprite.fun = injector::GetBranchDestination(addr).get();
    injector::MakeCALL(addr, (void(__stdcall*)(int, int, int, int, int, int, const int*, int, int, int, int))f);
}

void Init()
{
    Log::Write("GTA2 patches initializing.");
    SetProcessDPIAware();

    CIniReader iniReader("");
    nResX = iniReader.ReadInteger("MAIN", "ResX", 0);
    nResY = iniReader.ReadInteger("MAIN", "ResY", 0);
    bExtendHud = iniReader.ReadBoolean("MAIN", "ExtendHud", 0);

    bSkipMovie = iniReader.ReadInteger("MISC", "SkipMovie", 1) != 0;
    bSkipCredits = iniReader.ReadInteger("MISC", "SkipCredits", 1) != 0;
    bNoSampManDelay = iniReader.ReadInteger("MISC", "NoSampManDelay", 1) != 0;
    bFillBackground = iniReader.ReadBoolean("MAIN", "FillFrontendBackground", false);

    nQuicksaveKey = iniReader.ReadInteger("MISC", "QuicksaveKey", VK_F5);
    nZoomIncreaseKey = iniReader.ReadInteger("MISC", "ZoomIncreaseKey", VK_OEM_PLUS);
    nZoomDecreaseKey = iniReader.ReadInteger("MISC", "ZoomDecreaseKey", VK_OEM_MINUS);

    Log::Read(iniReader);
    InputDevices::Read(iniReader);
    Desktop::Resolution(nResX, nResY);
    auto versionProbe = hook::get_pattern("50 51 E8 ? ? ? ? 81 7C 24 74 01 06 00 00 73 2E", 2);
    injector::MakeCALL(versionProbe, GTA2Images::DetectDirectX);
    rendererReady = *hook::pattern("83 3D ? ? ? ? 00 75 04 33 C0 EB 17 C7 05").get_first<uint32_t*>(2);
    colorDepthHook = safetyhook::create_inline(hook::get_pattern("FF 15 ? ? ? ? E9 25 FF FF FF"), SetColourDepth);
    windowedMode = iniReader.ReadBoolean("MAIN", "WindowedMode", true);
    // Several registry readers share a prologue; take the one the screen setup calls for start_mode.
    auto screenSetting = hook::get_pattern("6A 01 68 ? ? ? ? B9 ? ? ? ? A3 ? ? ? ? E8 ? ? ? ? 8B 0D", 17);
    screenSettingHook = safetyhook::create_inline(injector::GetBranchDestination(screenSetting).get<void>(), ReadScreenSetting);
    if (windowedMode)
    {
        // The bundled DirectDraw wrapper supplies the legacy pixel format.
        auto call = hook::get_pattern("83 EC 20 E8 ? ? ? ? 84 C0 75 04 83 C4 20 C3", 3);
        auto windowAllowed = injector::GetBranchDestination(call).get();
        uint8_t alwaysAllowed[] = { 0xB0, 0x01, 0xC3 };
        injector::WriteMemoryRaw(windowAllowed, alwaysAllowed, sizeof(alwaysAllowed), true);
        // The renderer's windowed capability is also checked on focus loss, where
        // a "no" minimizes the window (alt-tab, clicking another window).
        auto windowCapable = hook::pattern("A1 ? ? ? ? 8B 0D ? ? ? ? 50 51 FF 15 ? ? ? ? 85 C0 74 09 F6");
        if (windowCapable.size() == 1) injector::WriteMemoryRaw(windowCapable.get_first(), alwaysAllowed, sizeof(alwaysAllowed), true);

        // Borderless: the windowed mode uses WS_OVERLAPPEDWINDOW; use a popup and
        // centre it on its monitor (a desktop-sized window covers the screen).
        auto style = hook::pattern("68 00 00 CF 10 6A F0 50 FF 15");
        if (style.size() == 1) injector::WriteMemory<uint32_t>(style.get_first(1), WS_POPUP | WS_VISIBLE, true);
        auto position = hook::pattern("8B 0D ? ? ? ? 03 C2 8B 15 ? ? ? ? 50 A1 ? ? ? ? 51 52 6A 00 50");
        auto clientWidth = hook::pattern("A1 ? ? ? ? 03 D1 8B 4C 24 18");
        auto clientHeight = hook::pattern("8B 15 ? ? ? ? 68 16 03 00 00");
        if (position.size() == 1 && clientWidth.size() == 1 && clientHeight.size() == 1)
        {
            static auto windowX = *position.get_first<int*>(10);
            static auto windowY = *position.get_first<int*>(2);
            static auto window = *position.get_first<HWND*>(16);
            static auto configuredWidth = *clientWidth.get_first<int*>(1);
            static auto configuredHeight = *clientHeight.get_first<int*>(2);
            // The resize passes SWP_NOMOVE; allow it to move the window to the centre.
            injector::WriteMemory<uint32_t>(clientHeight.get_first(7), 0x316 & ~SWP_NOMOVE, true);
            static SafetyHookMid positionHook = safetyhook::create_mid(position.get_first(), [](SafetyHookContext&)
            {
                int width = *configuredWidth, height = *configuredHeight;
                MONITORINFO monitor{ sizeof(monitor) };
                if (!GetMonitorInfoW(MonitorFromWindow(*window, MONITOR_DEFAULTTOPRIMARY), &monitor)) return;
                *windowX = std::max(int(monitor.rcMonitor.left), int(monitor.rcMonitor.left + monitor.rcMonitor.right - width) / 2);
                *windowY = std::max(int(monitor.rcMonitor.top), int(monitor.rcMonitor.top + monitor.rcMonitor.bottom - height) / 2);
            });
        }
    }
    GTA2Camera::Install(iniReader);
    GTA2Input::Install(iniReader);
    
    window_width = *hook::pattern("8B 3D ? ? ? ? 8B 4C 24 1C").get_first<uint32_t*>(2);
    window_height = *hook::pattern("8B 0D ? ? ? ? 2B F8").get_first<uint32_t*>(2);

    // Res change
    if (nResX && nResY) {
        auto pattern = hook::pattern("8B 2D ? ? ? ? 56 8B 35 ? ? ? ? 57 8B 3D"); //0x4CB29F
        static auto dword_6732E0 = *pattern.get_first<uint32_t*>(2);
        pattern = hook::pattern("89 0D ? ? ? ? A3 ? ? ? ? 89 0D ? ? ? ? EB 5F"); //0x4CB2D5
        static auto dword_673578 = *pattern.get_first<uint32_t*>(2);
        static auto dword_6732E8 = *pattern.get_first<uint32_t*>(7);
        static auto dword_6732E4 = *pattern.get_first<uint32_t*>(13);
        struct SetResHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.eax = nResY;
                *dword_673578 = nResX;
                *dword_6732E8 = nResY;
                *dword_6732E4 = nResX;
                *dword_6732E0 = nResY;
            }
        }; injector::MakeInline<SetResHook>(pattern.get_first(-12), pattern.get_first(17));

        pattern = hook::pattern("74 49 A3 ? ? ? ? ? ? ? ? ? ? ? ? ? ? E8 ? ? ? ? 84 C0");
        injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true); //0x4CC61E
        injector::WriteMemory<uint8_t>(pattern.get_first(24), 0xEB, true); //0x4CC636
        pattern = hook::pattern("74 2B A1 ? ? ? ? B9 ? ? ? ? 50 68");
        injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true); //0x4CB692
        pattern = hook::pattern("B8 ? ? ? ? 3B C8 74 2B 6A 10 68");
        fullscreenWidth = pattern.get_first<int>(1); fullscreenHeight = pattern.get_first<int>(12);
        injector::WriteMemory(pattern.get_first(1), nResX, true); //0x4CB59C + 1
        injector::WriteMemory<uint8_t>(pattern.get_first(10), 32, true);
        injector::WriteMemory(pattern.get_first(12), nResY, true); //0x4CB5A7 + 1
        pattern = hook::pattern("6A ? 50 51 52 32 DB"); //0x4CB583
        //injector::WriteMemory<uint8_t>(pattern.get_first(1), 32, true); // causes green menu
        injector::MakeNOP(pattern.get_first(32), 2, true); //0x4CB5A3
    }

    auto pattern = hook::pattern("B9 2F 00 00 00 F3 A5"); //0x4A80CD 0x4A6257
    struct CameraZoom
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx = 0x2F;

            *(int32_t*)(regs.ebp + 0x138) = (uint32_t)(hud_scale * one);
            *(int32_t*)(regs.ebp + 0x2B0) = (uint32_t)(hud_scale * one); // for Zaibatsu [It was an Accident!] mission

            *(int32_t*)(regs.esi + 0x8) += (uint32_t)(fZoom * one);
        }
    }; injector::MakeInline<CameraZoom>(pattern.count(2).get(1).get<void*>(0));

    // Ped Shadows
    pattern = hook::pattern("A1 ? ? ? ? 8B 48 38 81 C1 ? ? ? ? E8 ? ? ? ? 84 C0"); //0x5EB4FC
    static auto ptrToGame = *pattern.get_first<uint32_t*>(1);

    pattern = hook::pattern("81 C1 ? ? ? ? E8 ? ? ? ? 8B C8 E8 ? ? ? ? D9 05"); //0x4BE38E
    injector::MakeNOP(pattern.get_first(0), 6, true);
    static auto PedShadowsHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        auto ph1 = PtrWalkthrough<CameraOrPhysics>(ptrToGame, 0x38, 0x90);
        
        if (ph1)
        {
            static uint32_t shadowsDummyUIScale = 0;
            auto playerph1 = *ph1;
            auto cameraZPos = (float)playerph1.cameraPos.z / 16384.0f;
            auto shadowsDistance = (1.0f / (cameraZPos + 7.0f)) * 8.0f;
            shadowsDummyUIScale = shadowsDistance * playerph1.uiScale;
            regs.ecx = (uintptr_t)&shadowsDummyUIScale;
            return;
        }

        regs.ecx += 0x0A8;
    });

    // Hud
    pattern = hook::pattern("E8 ? ? ? ? 2B FB");
    RePositionElement(pattern.get_first(-5962)); // 0x4C72AA

    pattern = hook::pattern("E8 ? ? ? ? 8B 76 54");
    RePositionElement(pattern.get_first(-3233)); // 0x4BA2F4

    pattern = hook::pattern("E8 ? ? ? ? 81 3B ? ? ? ?");
    RePositionElement(pattern.get_first(-7509)); // 0x4C71DA

    pattern = hook::pattern("E8 ? ? ? ? C6 44 24 ? ? EB 7C");
    RePositionElement(pattern.get_first(-359)); // 0x44B4BA

    pattern = hook::pattern("E8 ? ? ? ? 81 C6 ? ? ? ? 56");
    SetPosType<1>(pattern.get_first(0)); // Big messages 0x4C8A7D

    pattern = hook::pattern("E8 ? ? ? ? 66 0F B6 86 ? ? ? ?");
    SetPosType<1>(pattern.get_first(0)); // Subtitle sprite 0x4C946F

    pattern = hook::pattern("6A 40 E8 ? ? ? ? 56");
    SetPosType<1>(pattern.get_first(2)); // Subtitle text 0x4C94D7

    pattern = hook::pattern("E8 ? ? ? ? A0 ? ? ? ? 84 C0 74 0B 8B 0D ? ? ? ? E8 ? ? ? ? FF 15 ? ? ? ?");
    SetPosType<1>(pattern.get_first(450950)); // Quit text 1 0x4C87A5

    pattern = hook::pattern("E8 ? ? ? ? A0 ? ? ? ? 84 C0 74 0B 8B 0D ? ? ? ? E8 ? ? ? ? FF 15 ? ? ? ?");
    SetPosType<1>(pattern.get_first(451046)); // Quit text 2 0x4C8805

    pattern = hook::pattern("50 E8 ? ? ? ? 57 E8 ? ? ? ? 5F");
    SetPosType<1>(pattern.get_first(1)); // Quit text 3 0x4C8867

    pattern = hook::pattern("E8 ? ? ? ? 56 53");
    SetPosType<1>(pattern.get_first(0)); // Stats sprite 0x4CA1BC

    pattern = hook::pattern("57 E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 5F");
    SetPosType<1>(pattern.get_first(1)); // Stats text 0x4CA40C

    pattern = hook::pattern("E8 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 8B CB E8 ? ? ? ? 5D");
    SetPosType<2>(pattern.get_first(-14347)); // 3d Text 0x4BAD41

    pattern = hook::pattern("E8 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 8B CB E8 ? ? ? ? 5D");
    SetPosType<2>(pattern.get_first(-14202)); // 3d Text 0x4BADD2

    pattern = hook::pattern("E8 ? ? ? ? A1 ? ? ? ? 89 5C 24 18");
    SetPosType<2>(pattern.get_first(0)); // 3d Text 0x4BAE7B

    pattern = hook::pattern("E8 ? ? ? ? 8B 15 ? ? ? ? 8B 44 24 1C");
    SetPosType<2>(pattern.get_first(0)); // 3d Text 0x4BAF3D

    pattern = hook::pattern("E8 ? ? ? ? 8B CE E8 ? ? ? ? 8D 8E ? ? ? ? E8 ? ? ? ? 8D 8E ? ? ? ?");
    SetPosType<1>(pattern.get_first(-2986)); // Zone name 0x4C98F0

    pattern = hook::pattern("55 E8 ? ? ? ? 68 ? ? ? ?");
    SetPosType<1>(pattern.get_first(1)); // Zone name 0x4C9933

    pattern = hook::pattern("E8 ? ? ? ? 8B CE E8 ? ? ? ? 8D 8E ? ? ? ? E8 ? ? ? ? 8D 8E ? ? ? ?");
    SetPosType<1>(pattern.get_first(-2844)); // Zone name 0x4C997E

    pattern = hook::pattern("E8 ? ? ? ? 8B CE E8 ? ? ? ? 8D 8E ? ? ? ? E8 ? ? ? ? 8D 8E ? ? ? ?");
    SetPosType<1>(pattern.get_first(-2774)); // Zone name 0x4C99C4

    pattern = hook::pattern("E8 ? ? ? ? 03 F5 56");
    SetPosType<1>(pattern.get_first(0)); // Zone name 0x4C9A29

    pattern = hook::pattern("E8 ? ? ? ? 8B 46 04 6A 00");
    ReposSpriteCall3D(pattern.get_first(-495)); // Money messages 0x4B9424

    pattern = hook::pattern("74 7B 68 ? ? ? ?");
    injector::MakeNOP(pattern.get_first(0), 2, true); // 0x4B93AC

    pattern = hook::pattern("74 4D 8D 4C 24 14");
    injector::MakeNOP(pattern.get_first(0), 2, true); // 0x4B93DA

    // Frontend
    pattern = hook::pattern("E8 ? ? ? ? 8B 44 24 1C 8B 54 24 20");
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
    CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&WindowCheck, (LPVOID)hwnd, 0, NULL);

    if (bSkipMovie)
    {
        //skip movie to prevent windowed crash
        pattern = hook::pattern("8A 88 ? ? ? ? 88 88 ? ? ? ? 40 84 C9 ? ? B8 ? ? ? ? C3");
        injector::WriteMemory<uint8_t>(*pattern.get_first<void*>(2), '_', true); //0x459695+2
    }

    pattern = hook::pattern("81 EC 00 01 00 00 55 56 57 8B F1 E8 ? ? ? ? 8B AC 24 10 01 00 00");
    frontendPageHook = safetyhook::create_inline(pattern.get_first(), SetFrontendPage);
    InstallOptions(pattern.get_first());

    if (bNoSampManDelay)
    {
        pattern = hook::pattern("FF D5 6A 01");
        injector::MakeNOP(pattern.get_first(2), 8, true); //0x4B6821+2
    }

    // Intro crash fix
    pattern = hook::pattern("74 66 E8 ? ? ? ?");
    injector::MakeNOP(pattern.get_first(0), 2, true);

    GTA2Movies::Install();
    Log::Write("GTA2 presentation and input hooks installed.");

    {
        //code from NTAuthority
        static uint16_t oldState = 0;
        static uint16_t curState = 0;

        //injector::WriteMemory(0x47FEDC, 0, true);
        //injector::WriteMemory(0x47FEF5, 500, true);

        static uint32_t dword_45E510 = (uint32_t)hook::get_pattern("8D 81 00 03 00 00 C3", 0);
        static uint32_t dword_5EC070 = *(uint32_t*)hook::get_pattern("B9 ? ? ? ? E8 ? ? ? ? 66 0F B6", 1);
        static uint32_t dword_47EF40 = (uint32_t)hook::get_pattern("83 EC 18 53 8B 5C 24 20 55 8B", 0);
        static uint32_t dword_6644BC = *(uint32_t*)hook::get_pattern("8B 15 ? ? ? ? 8B 82 38 03 00", 2);
        static uint32_t dword_4C6750 = (uint32_t)hook::get_pattern("8B 44 24 08 8B 54 24 04 6A FF 50 52", 0);
        static uint32_t dword_672F40 = *(uint32_t*)hook::get_pattern("8B 0D ? ? ? ? 56 68 ? ? ? ? 6A 01", 2);
        static uint32_t dword_673E2C = *(uint32_t*)hook::get_pattern("A1 ? ? ? ? 85 C0 75 ? 8A 41 30", 1);

        pattern = hook::pattern("8B 73 04 33 FF 3B F7 66 89 BB E8"); //0x481380
        struct QuicksaveHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.esi = *(uint32_t*)(regs.ebx + 4);
                regs.edi = 0;

                if (!nQuicksaveKey || !InputDevices::Focused()) { oldState = 0; return; }
                curState = GetAsyncKeyState(nQuicksaveKey) & 0x8000;

                if (!curState && oldState)
                {
                    uint32_t missionFlag = **(uint32_t**)(*(uint32_t*)(dword_6644BC)+0x344);
                    uint32_t isMP = *(uint32_t*)dword_673E2C;
                    if (!missionFlag && !isMP)
                    {
                        //injector::thiscall<int(int, int)>::call(0x4105B0, 0x5D85A0, 0x3D); //sfx

                        auto i = injector::thiscall<uint32_t(uint32_t)>::call(dword_45E510, dword_5EC070); //save
                        injector::thiscall<uint32_t(uint32_t, uint32_t)>::call(dword_47EF40, *(uint32_t*)dword_6644BC, i);

                        injector::thiscall<uint32_t(uint32_t, uint32_t, const char*)>::call(dword_4C6750, *(uintptr_t*)dword_672F40 + 0xE4, 1, "svdone"); // text display
                    }
                }

                oldState = curState;
            }
        }; injector::MakeInline<QuicksaveHook>(pattern.get_first(0));
    }
}

void InitD3DDim()
{
    auto pattern = hook::module_pattern(GetModuleHandle(L"d3dim"), "B8 00 08 00 00 39");
    if (!pattern.count_hint(2).empty())
        injector::WriteMemory(pattern.get(0).get<void>(1), -1, true);
}

void InitD3DDim700()
{
    auto pattern = hook::module_pattern(GetModuleHandle(L"d3dim700"), "B8 00 08 00 00 39");
    if (!pattern.count_hint(2).empty())
        injector::WriteMemory(pattern.get(0).get<void>(1), -1, true);
}

void InitD3DDLL() 
{
    auto module = GetModuleHandleW(L"d3ddll.dll");
    GTA2Images::Install(module);
    CIniReader iniReader("");
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
                int width = int(screen_width), height = int(screen_height);
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
            dest.left = static_cast<LONG>(std::lround(regs.ebp * hud_scale));
            dest.top = static_cast<LONG>(std::lround(regs.ebx * hud_scale));
            dest.right = static_cast<LONG>(std::lround(regs.edi * hud_scale));
            dest.bottom = static_cast<LONG>(std::lround(regs.esi * hud_scale));

            dest.left += (int32_t)(hud_offset / 2);
            dest.right += (int32_t)(hud_offset / 2);
        }
    }; injector::MakeInline<gbh_BlitImageHook>(pattern.get_first(0), pattern.get_first(8));
}

// Run from the game's folder: the game opens its files relative to it.
static void SetGameDirectory()
{
    wchar_t path[MAX_PATH];
    if (GetModuleFileNameW(nullptr, path, MAX_PATH))
        SetCurrentDirectoryW(std::filesystem::path(path).parent_path().c_str());
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        SetGameDirectory();
        GameRegistry::Install();
        Log::Install();
        // The manager uses this ASI through the same local DirectDraw loader.
        // Keep its 16-bit format and minimum-size checks; remove only 4:3 filtering.
        auto managerCount = hook::pattern("DF E0 F6 C4 44 7A 01 43 8B 49 38");
        auto managerFill = hook::pattern("DF E0 F6 C4 44 7A 41 81 FF 80 02 00 00");
        if (managerCount.size() == 1 && managerFill.size() == 1)
        {
            injector::MakeNOP(managerCount.get_first(5), 2, true);
            injector::MakeNOP(managerFill.get_first(5), 2, true);
            return;
        }
        GameRegistry::DefaultString("Software\\DMA Design Ltd\\GTA2\\screen", "rendername", "d3ddll.dll");
        GameRegistry::DefaultString("Software\\DMA Design Ltd\\GTA2\\screen", "videoname", "dmavideo.dll");
        GameRegistry::DefaultValue("Software\\DMA Design Ltd\\GTA2\\screen", "start_mode", 0);
        GameRegistry::DefaultValue("Software\\DMA Design Ltd\\GTA2\\screen", "renderdevice", 1);
        GameRegistry::DefaultValue("Software\\DMA Design Ltd\\GTA2\\screen", "videodevice", 1);
        const int keys[] = { 0xC8, 0xD0, 0xCB, 0xCD, 0x1D, 0x1C, 0x39, 0x2C, 0x2D, 0x0F, 0x38, 0x36 };
        for (int i = 0; i < 12; ++i)
            GameRegistry::DefaultValue("Software\\DMA Design Ltd\\GTA2\\Control", std::to_string(i).c_str(), keys[i]);
        CallbackHandler::RegisterCallback(Init, hook::pattern("83 EC 68 55 56 8B 74 24 74"));
        CallbackHandler::RegisterCallback(L"d3dim.dll", InitD3DDim);       // crash fix for
        CallbackHandler::RegisterCallback(L"d3dim700.dll", InitD3DDim700); // resolutions > 2048
        CallbackHandler::RegisterCallback(L"d3ddll.dll", InitD3DDLL); // frontend background scale
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

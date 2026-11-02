module;

#include <stdafx.h>
#include <charconv>
#include "FixSettings.hpp"
#include "SetupFallback.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"
#include "Shared/WindowCursor.hpp"

#pragma comment(lib, "winhttp.lib")

export module Misc;

import ComVars;
import Movies;
import Focus;
import Hud;
import Cutscene;

std::atomic<HHOOK> CursorAttachHook{};
std::atomic<HWND> CursorAttachWindow{};
std::mutex CursorAttachMutex;
constexpr WPARAM CursorAttachToken{ 0x48443243 };

LRESULT CALLBACK AttachCursorOnMessage(int code, WPARAM wParam, LPARAM lParam)
{
    if (code >= 0)
    {
        const auto message = reinterpret_cast<MSG*>(lParam);
        const std::lock_guard lock(CursorAttachMutex);
        if (message->hwnd == CursorAttachWindow.load() && message->message == WM_NULL && message->wParam == CursorAttachToken)
        {
            WindowCursor::Attach(message->hwnd);
            if (WindowCursor::Window != message->hwnd)
                CursorAttachWindow.store(nullptr);
            if (const auto hook = CursorAttachHook.exchange(nullptr))
                UnhookWindowsHookEx(hook);
        }
    }
    return CallNextHookEx(nullptr, code, wParam, lParam);
}

void AttachCursor(HWND window)
{
    const std::lock_guard lock(CursorAttachMutex);
    if (CursorAttachWindow.exchange(window) == window)
        return;
    if (const auto previous = CursorAttachHook.exchange(nullptr))
        UnhookWindowsHookEx(previous);
    const auto thread = GetWindowThreadProcessId(window, nullptr);
    const auto hook = SetWindowsHookExW(WH_GETMESSAGE, AttachCursorOnMessage, nullptr, thread);
    if (!hook)
    {
        auto expected = window;
        CursorAttachWindow.compare_exchange_strong(expected, nullptr);
        return;
    }
    CursorAttachHook.store(hook);
    if (!PostMessageW(window, WM_NULL, CursorAttachToken, 0))
    {
        auto expectedHook = hook;
        if (CursorAttachHook.compare_exchange_strong(expectedHook, nullptr))
            UnhookWindowsHookEx(hook);
        auto expectedWindow = window;
        CursorAttachWindow.compare_exchange_strong(expectedWindow, nullptr);
    }
}

void PlaceWindow(HWND window)
{
    WindowedModeWrapper::GameHWND = window;
    WindowedModeWrapper::bBorderlessWindowed = WindowMode == 2;
    WindowedModeWrapper::desiredClientWidth = Screen.Width;
    WindowedModeWrapper::desiredClientHeight = Screen.Height;
    WindowedModeWrapper::afterCreateWindow();
    if (WindowMode == 2)
    {
        const auto monitor = WindowedModeWrapper::WindowMonitor();
        SetWindowPos(window, nullptr, monitor.rcMonitor.left, monitor.rcMonitor.top, Screen.Width, Screen.Height,
            SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }
}

// Camera projection: the horizontal half angle is widened for the screen's aspect (Hor+); cutscene cameras keep the
// 16:9 framing of their viewport, the gameplay camera also gets the FOV factor.
constexpr float ProjectionMaxHalf{ 1.5533430f }; // 178 degrees, divided by two
float ProjectionScale{ 1.0f };
float ProjectionAspect{ 4.0f / 3.0f };
std::atomic<float> ProjectionFactor{ 1.0f };
std::atomic<bool> ProjectionDirty{};
uintptr_t ProjectionReturn[2]{};
void(__stdcall* SetNativeFOV)(void*, float){};

float __stdcall ProjectionHalfAngle(uint8_t* camera, float fov)
{
    auto& aspect = *reinterpret_cast<float*>(camera + 0x138);
    auto scale = ProjectionScale;
    auto factor = 1.0f;
    if (HD2Cutscene::Active() && camera == HD2Cutscene::SceneCamera())
    {
        aspect = HD2Cutscene::Aspect(Screen.Width, Screen.Height);
        scale = aspect / HD2Cutscene::Authored;
    }
    else if (camera == GameCamera.get())
    {
        aspect = ProjectionAspect;
        factor = ProjectionFactor.load(std::memory_order_relaxed);
    }
    else if (std::fabs(aspect - 4.0f / 3.0f) < 0.002f)
    {
        // Scripted and other 3D cameras that kept the default
        aspect = ProjectionAspect;
    }
    // The factor widens the 4:3 view first, so it stays usable on ultrawide screens
    const auto half = std::min(fov * 0.5f * factor, ProjectionMaxHalf);
    return std::atan(std::tan(half) * scale);
}

void __declspec(naked) ProjectionFOV0()
{
    __asm
    {
        push eax
        push ecx
        push edx
        sub esp, 4
        fstp dword ptr [esp]
        push edx
        call ProjectionHalfAngle
        pop edx
        pop ecx
        pop eax
        jmp dword ptr [ProjectionReturn]
    }
}

void __declspec(naked) ProjectionFOV1()
{
    __asm
    {
        push eax
        push ecx
        push edx
        sub esp, 4
        fstp dword ptr [esp]
        push edx
        call ProjectionHalfAngle
        pop edx
        pop ecx
        pop eax
        jmp dword ptr [ProjectionReturn + 4]
    }
}

struct S_matrix
{
    float a1 = 1.0f;
    float a2 = 0.0f;
    float a3 = 0.0f;

    uint32_t pad1 = 0;

    float a4 = 0.0f;
    float a5 = 1.0f;
    float a6 = 0.0f;

    uint32_t pad2 = 0;

    float a7 = 0.0f;
    float a8 = 0.0f;
    float a9 = 1.0f;

    float a10 = 0.0f;
    float a11 = 0.0f;
    float a12 = 0.0f;
    float a13 = 0.0f;
    float a14 = 1.0f;
} mat;

// Menus, loading screens and the HUD are scenes seen through orthographic cameras set up while the game builds them:
// their view keeps the 4:3 frame (centred, full height; fitted to the width on screens narrower than 4:3)

void __stdcall S_matrix_Identity(S_matrix* _this)
{
    memcpy(_this, &mat, sizeof(S_matrix));
    auto camera = reinterpret_cast<uint8_t*>(_this) - 0x150;
    if (UiDepth > 0 && *reinterpret_cast<bool*>(camera + 0x13C))
    {
        // The orthographic height follows the camera's aspect; the view's y scale brings it back to the 4:3 frame
        const auto aspect = *reinterpret_cast<float*>(camera + 0x138);
        auto self = reinterpret_cast<S_matrix*>(camera + 0x40);
        self->a1 = Screen.fHudCameraX;
        self->a5 = Screen.fHudCameraY * (aspect > 0.0f ? (4.0f / 3.0f) / aspect : 1.0f);
    }
}

// Calls the original with the same registers and stack arguments, counted in UiDepth
#define UI_SCENE_BUILDER(name, argumentBytes) \
    SafetyHookInline name##Hook; \
    uintptr_t name##Original{}; \
    void __declspec(naked) name() \
    { \
        __asm inc UiDepth \
        __asm mov eax, argumentBytes \
        __asm copy: \
        __asm test eax, eax \
        __asm jz call_original \
        __asm push dword ptr [esp + argumentBytes] \
        __asm sub eax, 4 \
        __asm jmp copy \
        __asm call_original: \
        __asm call dword ptr [name##Original] \
        __asm dec UiDepth \
        __asm ret argumentBytes \
    }

UI_SCENE_BUILDER(MenuScene, 4)
UI_SCENE_BUILDER(LoadingScene, 4)
UI_SCENE_BUILDER(BriefingScene, 20)
UI_SCENE_BUILDER(MissionScene, 0)
UI_SCENE_BUILDER(StateSwitch, 8)
UI_SCENE_BUILDER(StateStart, 4)

void InstallSceneBuilder(SafetyHookInline& hook, uintptr_t& original, void* target, void(*thunk)())
{
    hook = safetyhook::create_inline(target, reinterpret_cast<void*>(thunk));
    original = hook.trampoline().address();
}

SafetyHookInline FontHeightHook;

int64_t __fastcall FitFontHeight(void* text, void*)
{
    const auto height = FontHeightHook.thiscall<int64_t>(text);
    return std::llround(static_cast<double>(height) * Screen.fHudCameraY);
}

SafetyHookInline StartupMovieHook;

char __fastcall StartMovie(void* owner, void*, const char* name, int stage)
{
    if (name && stage >= 1 && stage <= 3)
    {
        std::string_view file{ name };
        if (auto slash = file.find_last_of("\\/"); slash != file.npos)
            file.remove_prefix(slash + 1);
        const auto logo = std::string("Logo") + static_cast<char>('0' + stage) + ".bik";
        if (file.size() == logo.size() && std::equal(file.begin(), file.end(), logo.begin(),
            [](unsigned char a, unsigned char b) { return std::tolower(a) == std::tolower(b); }))
        {
            // Stage 4 is the intro movie, followed by the main menu; its normal movie object and cleanup are kept
            return StartupMovieHook.thiscall<char>(owner, "Intro.bik", 4);
        }
    }
    return StartupMovieHook.thiscall<char>(owner, name, stage);
}

// The intro movie (also in the Films menu) ends at its first update, as when it is skipped with Esc
SafetyHookInline StartupMovieUpdateHook;
bool IntroSkipped{};

char __fastcall UpdateStartupMovie(uint8_t* owner, void*, int time)
{
    if (!IntroSkipped && *reinterpret_cast<int*>(owner + 0x7C) == 4)
    {
        IntroSkipped = true;
        *reinterpret_cast<int*>(owner + 4) = 2;
    }
    return StartupMovieUpdateHook.thiscall<char>(owner, time);
}

void InstallLogoSkip()
{
    if (StartupMovieHook || FixSettings().Get("MAIN.SkipIntro") == "0")
        return;
    auto movie = hook::pattern("51 8B 44 24 0C 53 56 6A 1C 89 4C 24 0C 89 41 7C E8 ? ? ? ? 33 DB 83 C4 04 3B C3 74 20");
    StartupMovieHook = safetyhook::create_inline(movie.get_first(), StartMovie);
    auto update = hook::pattern("8B 44 24 04 56 50 8B F1 E8 ? ? ? ? 83 7E 04 02 75 ? C7 46 04 0D 00 00 00 5E C2 04 00");
    StartupMovieUpdateHook = safetyhook::create_inline(update.get_first(), UpdateStartupMovie);
}

std::atomic<int> MovieMode{};
void(__thiscall* UpdateMovieGeometry)(void*){};
SafetyHookMid MovieFrameHook;
SafetyHookMid GameMovieFrameHook;

void FitMovieFrame(uint8_t* movie)
{
    auto bink = *reinterpret_cast<uint32_t**>(movie + 28);
    if (!bink || Screen.Width <= 0 || Screen.Height <= 0 || !bink[0] || !bink[1])
        return;

    const auto fitted = FitMovie(Screen.Width, Screen.Height, bink[0], bink[1], MovieMode.load(std::memory_order_relaxed));
    auto rect = reinterpret_cast<int*>(movie + 44);
    if (rect[0] != fitted.x || rect[1] != fitted.y || rect[2] != fitted.width || rect[3] != fitted.height)
    {
        rect[0] = fitted.x; rect[1] = fitted.y; rect[2] = fitted.width; rect[3] = fitted.height;
        UpdateMovieGeometry(movie);
    }
}

// The version line of the menus shows the update notice instead
namespace HD2Update
{
    inline std::mutex lock;
    inline std::string notice;
    SafetyHookInline textHook;
    inline void(__cdecl* makeText)(void**, const char*, ...){};

    // Text 2006 is the version line ("... ver. %d.%d.%d") of the menus and the briefing
    inline void** __fastcall Text(void* table, void*, void** out, int id)
    {
        if (id == 2006)
        {
            std::string line;
            {
                std::lock_guard guard(lock);
                line = notice;
            }
            if (!line.empty())
            {
                std::string escaped;
                for (auto c : line)
                {
                    escaped += c;
                    if (c == '%')
                        escaped += '%';
                }
                makeText(out, "%s", escaped.c_str());
                return out;
            }
        }
        return textHook.thiscall<void**>(table, out, id);
    }

    inline void Init()
    {
        textHook = safetyhook::create_inline(hook::pattern(
            "51 56 57 8B 7C 24 14 8B F1 57 C7 44 24 0C 00 00 00 00 E8 ? ? ? ? 83 F8 FF 74").get_first(), Text);
        auto text = hook::pattern("68 ? ? ? ? 50 E8 ? ? ? ? 83 C4 08 6A 0D 68 14 00 00 01 6A 00 56 E8");
        makeText = reinterpret_cast<decltype(makeText)>(injector::GetBranchDestination(text.get_first(6)).as_int());

        RWGTA::UpdateCheck::Config update;
        update.pluginId = "HiddenandDangerous2.FusionFix";
        update.fileVersion = rsc_FileVersion;
        update.settings = &FixSettings();
        update.playable = []() { return true; };
        // The version line is short: name and version only, the hub opens after the game
        update.format = [](const std::string& latest, bool)
        {
            auto line = FixSettings().GetTexts().Resolve("@update.menu");
            if (auto at = line.find("{version}"); at != line.npos)
                line.replace(at, 9, latest);
            return asi_settings::text::Widen(line);
        };
        update.show = [](const std::wstring& line)
        {
            {
                std::lock_guard guard(lock);
                notice = asi_settings::text::Narrow(line);
            }
            // FusionFixHub waits for the game to close before it opens
            RWGTA::UpdateCheck::OnExit();
        };
        RWGTA::UpdateCheck::Init(std::move(update));
    }
}

// The film preview of the Films menu is placed from its widget's screen rectangle: map it into the 4:3 frame first
SafetyHookInline PreviewHook;

void __fastcall PlacePreview(uint8_t* widget, void*)
{
    auto& left = *reinterpret_cast<float*>(widget + 112);
    auto& top = *reinterpret_cast<float*>(widget + 124);
    auto& width = *reinterpret_cast<float*>(widget + 128);
    auto& height = *reinterpret_cast<float*>(widget + 132);
    const float saved[4]{ left, top, width, height };
    if (Screen.fAspectRatio < 4.0f / 3.0f)
    {
        top = top * Screen.fHudCameraY + (Screen.fHeight - Screen.fHeight * Screen.fHudCameraY) * 0.5f;
        height *= Screen.fHudCameraY;
    }
    else
    {
        left = left * Screen.fHudScale + Screen.fHudOffset;
        width *= Screen.fHudScale;
    }
    PreviewHook.thiscall<void>(widget);
    left = saved[0];
    top = saved[1];
    width = saved[2];
    height = saved[3];
}

float ParseAspect(std::string_view text)
{
    auto number = [](std::string_view part)
    {
        while (!part.empty() && part.front() == ' ') part.remove_prefix(1);
        while (!part.empty() && part.back() == ' ') part.remove_suffix(1);
        float value = 0.0f;
        const auto parsed = std::from_chars(part.data(), part.data() + part.size(), value);
        return part.empty() || parsed.ec != std::errc() || parsed.ptr != part.data() + part.size() || !std::isfinite(value) || value <= 0.0f ? 0.0f : value;
    };
    const auto colon = text.find(':');
    float value = number(colon == std::string_view::npos ? text : text.substr(0, colon));
    if (colon != std::string_view::npos)
    {
        const auto height = number(text.substr(colon + 1));
        value = height > 0.0f ? value / height : 0.0f;
    }
    return std::isfinite(value) && value > 0.0f ? std::clamp(value, 4.0f / 3.0f, 32.0f / 9.0f) : 0.0f;
}

void StartSettings()
{
    // Called from whichever init runs first
    static std::once_flag once;
    std::call_once(once, []()
    {
        FixSettings();
        asi_settings::LiveLink::Start();
    });
}

export void InitSettings()
{
    StartSettings();
    auto first = hook::pattern("0F 8A ? ? ? ? 81 FA 80 02 00 00 0F 82 ? ? ? ? 39 11 75 09");
    auto second = hook::pattern("0F 84 ? ? ? ? 81 FA 80 02 00 00 0F 82 ? ? ? ? A1 ? ? ? ? 6A 02 89 57 F8");
    injector::MakeNOP(first.get_first(), 6, true);
    injector::MakeNOP(second.get_first(), 6, true);
}

void Install()
{
    hook::pattern pattern;
    auto graphics = GetModuleHandleW(L"LS3DF.dll");
    StartSettings();
    InstallLogoSkip();
    HD2Setup::Init();
    HD2Focus::Init(graphics);
    auto& settings = FixSettings();
    WindowMode = std::clamp(std::stoi(settings.Get("MAIN.WindowMode")), 0, 2);
    const auto updateMovieMode = []()
    {
        MovieMode.store(std::clamp(std::stoi(FixSettings().Get("MAIN.FMVWidescreenMode")), 0, 1), std::memory_order_relaxed);
    };
    updateMovieMode();
    settings.OnChange("MAIN.FMVWidescreenMode", updateMovieMode);
    const auto updateFOV = []()
    {
        ProjectionFactor.store(std::clamp(std::stof(FixSettings().Get("MAIN.FOVFactor")), 0.5f, 2.0f), std::memory_order_relaxed);
        ProjectionDirty.store(true, std::memory_order_release);
    };
    updateFOV();
    settings.OnChange("MAIN.FOVFactor", updateFOV);
    const auto updateHud = []()
    {
        HD2Hud::boxAspect.store(ParseAspect(FixSettings().Get("MAIN.HudAspectRatioConstraint")), std::memory_order_relaxed);
    };
    updateHud();
    settings.OnChange("MAIN.HudAspectRatioConstraint", updateHud);

    pattern = hook::pattern("0F BE 05 ? ? ? ? 8B 15 ? ? ? ? 8B 0D ? ? ? ? 8B 35 ? ? ? ? 89 45 BC A1 ? ? ? ? 52 89 4D B8");
    SetupByte.SetAddress(*pattern.get_first<int8_t*>(3));
    SetupHeight.SetAddress(*pattern.get_first<int32_t*>(9));
    SetupWidth.SetAddress(*pattern.get_first<int32_t*>(29));
    struct WindowResolutionHook
    {
        void operator()(injector::reg_pack& regs)
        {
            if (WindowMode == 2)
            {
                auto [width, height] = GetDesktopRes();
                if (width <= 0 || height <= 0)
                    std::tie(width, height) = std::make_tuple(640, 480);
                injector::WriteMemory<int32_t>(SetupWidth.get_ptr(), width, true);
                injector::WriteMemory<int32_t>(SetupHeight.get_ptr(), height, true);
            }
            regs.eax = static_cast<int32_t>(SetupByte.get());
        }
    }; injector::MakeInline<WindowResolutionHook>(pattern.get_first(), pattern.get_first(7));

    pattern = hook::pattern("E8 ? ? ? ? 8B 08 8D 55 B8 52 50 FF 11 85 C0");
    static auto getGraph = reinterpret_cast<void*(__cdecl*)()>(injector::GetBranchDestination(pattern.get_first()).as_int());
    struct WindowFlagsHook
    {
        void operator()(injector::reg_pack& regs)
        {
            auto& flags = *reinterpret_cast<uint32_t*>(regs.ebp - 0x1C);
            flags |= 0x20u;
            if (WindowMode == 0)
                flags |= 2;
            else
                flags &= ~6u;
            regs.eax = reinterpret_cast<uint32_t>(getGraph());
        }
    }; injector::MakeInline<WindowFlagsHook>(pattern.get_first(), pattern.get_first(5));

    pattern = hook::pattern("E8 ? ? ? ? 8B 08 50 FF 51 04 8B 50 2C");
    struct WindowPlacementHook
    {
        void operator()(injector::reg_pack& regs)
        {
            auto graph = getGraph();
            regs.eax = reinterpret_cast<uint32_t>(graph);
            auto table = *reinterpret_cast<uintptr_t**>(graph);
            auto window = reinterpret_cast<HWND(__stdcall*)(void*)>(table[18])(graph);
            if (!window)
                return;
            AttachCursor(window);
            if (WindowMode == 0)
            {
                using SetAttribute = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
                static auto setAttribute = []()
                {
                    auto dwm = LoadLibraryExW(L"dwmapi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
                    return dwm ? reinterpret_cast<SetAttribute>(GetProcAddress(dwm, "DwmSetWindowAttribute")) : nullptr;
                }();
                const int preference = 1;
                if (setAttribute)
                    setAttribute(window, 33, &preference, sizeof(preference));
                return;
            }
            PlaceWindow(window);
        }
    }; injector::MakeInline<WindowPlacementHook>(pattern.get_first(), pattern.get_first(5));

    pattern = hook::pattern("33 F6 33 C0 66 8B 35 ? ? ? ? 66 A1 ? ? ? ? C1 E6 02 8B 0C 85 ? ? ? ? C7 45 F8 ? ? ? ? 8D BE ? ? ? ? 89 4D FC 89 7D 08");
    PostWindowValue.SetAddress(*pattern.get_first<uint16_t*>(7));
    struct WindowRestoreHook
    {
        void operator()(injector::reg_pack& regs)
        {
            if (WindowMode != 0 && WindowedModeWrapper::GameHWND)
                PlaceWindow(WindowedModeWrapper::GameHWND);
            regs.esi = PostWindowValue.get();
            regs.eax = 0;
        }
    }; injector::MakeInline<WindowRestoreHook>(pattern.get_first(), pattern.get_first(11));

    // Aspect the game gives the scene camera at the start and end of a scripted cutscene
    static auto ar1 = hook::pattern("68 ? ? ? ? 8B 80 5C 01 00 00 50 8B 08 FF 91 8C 00 00 00 E8 ? ? ? ? 8B 90 BC 00 00 00").get_first<float>(1);
    static auto ar2 = hook::pattern("68 ? ? ? ? 8B 80 5C 01 00 00 50 8B 08 FF 91 8C 00 00 00 E8 ? ? ? ? 8B 10 6A 00 6A 01 6A").get_first<float>(1);
    pattern = hook::pattern("C7 45 EC ? ? ? ? FF D6"); //0x6DB137
    static auto resolutionCallback = *pattern.get_first<uint32_t>(3);
    struct GetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *reinterpret_cast<uint32_t*>(regs.ebp - 0x14) = resolutionCallback;
            Screen.Width = regs.eax;
            Screen.Height = regs.edx;
            if (Screen.Width <= 0 || Screen.Height <= 0)
                std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
            if (Screen.Width <= 0 || Screen.Height <= 0)
                std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);
            Screen.fWidth = static_cast<float>(Screen.Width);
            Screen.fHeight = static_cast<float>(Screen.Height);
            Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
            ProjectionAspect = Screen.fAspectRatio;
            Screen.Width43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
            Screen.fWidth43 = static_cast<float>(Screen.Width43);
            Screen.fHudScale = (1.0f * ((4.0f / 3.0f) / (Screen.fAspectRatio)));
            Screen.fHudOffset = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
            Screen.fHudCameraX = std::min(1.0f, Screen.fHudScale);
            Screen.fHudCameraY = std::min(1.0f, Screen.fAspectRatio / (4.0f / 3.0f));
            Screen.fCursorScaleX = Screen.fHudCameraX / 1024.0f;
            ProjectionScale = Screen.fAspectRatio / (4.0f / 3.0f);

            injector::WriteMemory<float>(ar1, Screen.fAspectRatio, true);
            injector::WriteMemory<float>(ar2, Screen.fAspectRatio, true);
        }
    }; injector::MakeInline<GetResHook>(pattern.get_first(0), pattern.get_first(7));

    // Projection: FOV, aspect and the cutscene framing
    GameCamera.SetAddress(*hook::get_pattern<uintptr_t>("6A 0F B9 ? ? ? ? E8 ? ? ? ? C3", 3) + 0x240);
    SetNativeFOV = reinterpret_cast<decltype(SetNativeFOV)>(hook::module_pattern(graphics,
        "D9 44 24 08 D8 15 ? ? ? ? DF E0 F6 C4 05 7B ? D8 15 ? ? ? ? DF E0 F6 C4 01 75 ? D8 1D ? ? ? ? DF E0 F6 C4 05 7A ? D9 05 ? ? ? ? EB ? D9 05 ? ? ? ? 8B 4C 24 04 8A 81 3C 01 00 00 D9 99 20 01 00 00").get_first());
    const char* projectionPatterns[] = {
        "D9 82 20 01 00 00 D8 0D ? ? ? ? D9 82 38 01 00 00 D8 1D ? ? ? ? DF E0 F6 C4 41 75 ? D9 C0 DE C1 D8 15 ? ? ? ? DF E0 F6 C4 41 75 ? DD D8 D9 05 ? ? ? ? D9 C0",
        "D9 82 2C 01 00 00 D8 0D ? ? ? ? D9 82 38 01 00 00 D8 1D ? ? ? ? DF E0 F6 C4 41 75 ? D9 C0 DE C1 D8 15 ? ? ? ? DF E0 F6 C4 41 75 ? DD D8 D9 05 ? ? ? ? D9 C0"
    };
    for (size_t i = 0; i < std::size(projectionPatterns); ++i)
    {
        auto site = hook::module_pattern(graphics, projectionPatterns[i]).get_first<uint8_t>(6);
        ProjectionReturn[i] = reinterpret_cast<uintptr_t>(site + 0x32);
        injector::MakeRangedNOP(site, site + 0x32, true);
        injector::MakeJMP(site, i == 0 ? ProjectionFOV0 : ProjectionFOV1, true);
    }
    HD2Cutscene::Init(&Screen.Width, &Screen.Height);

    // New cameras get the aspect of the 4:3 frame (the 2D scenes rely on it); 3D views get the screen's aspect
    // when their projection is built
    auto graphWidth = hook::module_pattern(graphics, "78 ? A1 ? ? ? ? C2 04 00 90 90 90 90 90 90 90 90 90 90 A0 ? ? ? ? 84 C0");
    static auto cameraWidthCall = reinterpret_cast<uint32_t>(hook::module_pattern(graphics, "FF 51 18 89 44 24 0C A1 ? ? ? ? DB 44 24 0C").get_first());
    struct GraphWidthHook
    {
        void operator()(injector::reg_pack& regs)
        {
            const auto caller = *reinterpret_cast<uint32_t*>(regs.esp) - 3;
            regs.eax = caller == cameraWidthCall ? Screen.Width43 : Screen.Width;
        }
    }; injector::MakeInline<GraphWidthHook>(graphWidth.get_first(), graphWidth.get_first(7));

    // HUD scene and the 2D views
    HD2Hud::Install(hook::pattern("55 8B EC 83 E4 F0 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 48 53 56 8B F1 57 F6 46 0C 01").get_first());
    auto identity = hook::module_pattern(graphics, "E8 ? ? ? ? 8B 43 40 8B 4B 50 8B 53 60 89 06");
    injector::MakeCALL(identity.get_first(0), S_matrix_Identity, true);
    InstallSceneBuilder(MenuSceneHook, MenuSceneOriginal, hook::pattern(
        "55 8B EC 83 E4 F0 83 EC 18 8B 45 08 56 8B F1 83 F8 FE 57 B9 0B 00 00 00 89 86 F0 00 00 00").get_first(), MenuScene);
    InstallSceneBuilder(LoadingSceneHook, LoadingSceneOriginal, hook::pattern(
        "55 8B EC 83 EC 10 56 57 8B F9 B8 01 00 00 00 39 47 04 74 ? 39 47 1C 0F 85").get_first(), LoadingScene);
    InstallSceneBuilder(BriefingSceneHook, BriefingSceneOriginal, hook::pattern(
        "55 8B EC 83 E4 F0 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 6C 56 8B F1 57 89 74 24 20 E8 ? ? ? ? 8A 45 10 8A 4D 14").get_first(), BriefingScene);
    InstallSceneBuilder(MissionSceneHook, MissionSceneOriginal, hook::pattern(
        "55 8B EC 83 E4 F0 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 81 EC 48 04 00 00").get_first(), MissionScene);
    InstallSceneBuilder(StateSwitchHook, StateSwitchOriginal, hook::pattern(
        "6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 10 53 55 56 8B 74 24 2C 33 DB").get_first(), StateSwitch);
    InstallSceneBuilder(StateStartHook, StateStartOriginal, hook::pattern(
        "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 81 EC 10 01 00 00 56 8B 75 08").get_first(), StateStart);

    // Scenes that don't set a background colour clear to black (the sides of the menus) instead of green
    auto background = hook::module_pattern(graphics, "C7 44 24 ? 00 00 00 3F 8B 54 24 ? 89 50 04 8B 54 24 ? C7 44 24 ? CD CC CC 3E");
    injector::WriteMemory<float>(background.get_first<float>(4), 0.0f, true);
    injector::WriteMemory<float>(background.get_first<float>(23), 0.0f, true);

    // Game thread, every frame: FOV changes, cutscene framing and HUD layout
    auto gameLoop = hook::pattern("A1 ? ? ? ? 8B C8 83 E1 07 80 F9 07 74 ? A8 01 74 ? A0 ? ? ? ? 84 C0");
    static SafetyHookMid frameHook = safetyhook::create_mid(gameLoop.get_first(), [](SafetyHookContext&)
    {
        static std::once_flag once;
        std::call_once(once, []() { WFP::onGameInitEvent().executeAll(); });
        WFP::onGameProcessEvent().executeAll();
        if (ProjectionDirty.exchange(false, std::memory_order_acq_rel))
        {
            if (auto camera = GameCamera.get())
                SetNativeFOV(camera, *reinterpret_cast<float*>(camera + 0x120));
            else
                ProjectionDirty.store(true, std::memory_order_release);
        }
        HD2Cutscene::Frame(SetNativeFOV);
        HD2Hud::Frame(Screen.Width, Screen.Height);
        RWGTA::UpdateCheck::Tick();
    });

    // Movies keep their own aspect: Original inside the 4:3 frame, Fill as large as the screen allows
    UpdateMovieGeometry = reinterpret_cast<void(__thiscall*)(void*)>(
        hook::module_pattern(graphics, "8B 41 1C 83 EC 0C 33 D2 3B C2 56 74 25 8B 30 DB 41 34").get_first());
    auto movieFrame = hook::module_pattern(graphics, "8B 16 6A 00 56 FF 52 50 A1 ? ? ? ? 8B 08 50 FF 51 6C");
    MovieFrameHook = safetyhook::create_mid(movieFrame.get_first(), [](SafetyHookContext& regs)
    {
        FitMovieFrame(reinterpret_cast<uint8_t*>(regs.esi));
    });
    auto gameMovieFrame = hook::pattern("8B 46 20 6A 00 50 8B 10 FF 52 50 E8 ? ? ? ? 8B 08 50 FF 51 6C");
    GameMovieFrameHook = safetyhook::create_mid(gameMovieFrame.get_first(), [](SafetyHookContext& regs)
    {
        FitMovieFrame(*reinterpret_cast<uint8_t**>(regs.esi + 0x20));
    });

    PreviewHook = safetyhook::create_inline(hook::pattern(
        "83 EC 20 8D 54 24 10 56 8B F1 52 8B 46 6C 50 8B 08 FF 51 28 DB 44 24 20").get_first(), PlacePreview);

    // Texts placed on the 2D scenes follow their 4:3 frame
    FontHeightHook = safetyhook::create_inline(
        hook::pattern("51 8B 41 2C 83 C4 04 0F BF 49 06").get_first(), FitFontHeight);
    pattern = hook::pattern("88 46 72 88 46 70 5F"); //0x644B64
    struct TextHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint8_t*)(regs.esi + 0x72) = 1;
            *(uint8_t*)(regs.esi + 0x70) = 1;

            if (Screen.fAspectRatio < 4.0f / 3.0f)
            {
                const auto top = (Screen.fHeight - Screen.fHeight * Screen.fHudCameraY) * 0.5f;
                *(float*)(regs.esi + 0x0C) = *(float*)(regs.esi + 0x0C) * Screen.fHudCameraY + top;
                *(float*)(regs.esi + 0x14) = *(float*)(regs.esi + 0x14) * Screen.fHudCameraY + top;
                *(float*)(regs.esi + 0x1C) *= Screen.fHudCameraY;
            }
            else
            {
                *(float*)(regs.esi + 0x08) = *(float*)(regs.esi + 0x08) * Screen.fHudScale + Screen.fHudOffset;
                *(float*)(regs.esi + 0x10) = *(float*)(regs.esi + 0x10) * Screen.fHudScale + Screen.fHudOffset;
                *(float*)(regs.esi + 0x18) *= Screen.fHudScale;
            }
        }
    }; injector::MakeInline<TextHook>(pattern.get_first(0), pattern.get_first(6));

    // Mouse picking in the 2D scenes, and the cursor drawn with square pixels
    curs_pos_y.SetAddress(*(float**)hook::get_pattern("D9 05 ? ? ? ? D8 71 28 8B 44 24 04 D9 E0", 2)); //0x66A120 //0x840664
    curs_pos_x.SetAddress(curs_pos_y.get_ptr() - 1); //0x840660
    pattern = hook::pattern("C7 44 24 28 00 00 00 00 52"); //0x66A801
    struct MouseFix
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)(regs.esp + 0x28) = 0.0f;

            if (Screen.fAspectRatio < 4.0f / 3.0f)
            {
                const auto top = (Screen.fHeight - Screen.fWidth * 0.75f) * 0.5f;
                *(float*)(regs.esp + 0x38 + 0x00) = curs_pos_x.get() / Screen.fWidth;
                *(float*)(regs.esp + 0x38 + 0x04) = -(curs_pos_y.get() - top) / Screen.fWidth;
            }
            else
            {
                *(float*)(regs.esp + 0x38 + 0x00) = (curs_pos_x.get() - Screen.fHudOffset) / Screen.fWidth43;
                *(float*)(regs.esp + 0x38 + 0x04) = (curs_pos_y.get() / Screen.fHeight) * -0.75f;
            }
            *(float*)(regs.esp + 0x38 + 0x08) = -1.0f;
        }
    }; injector::MakeInline<MouseFix>(pattern.get_first(0), pattern.get_first(8));
    pattern = hook::pattern("D9 46 24 D8 0D ? ? ? ? 89 44 24 08 89 4C 24 04");
    injector::WriteMemory<float*>(pattern.get_first<float*>(5), &Screen.fCursorScaleX, true);

    pattern = hook::pattern("8B F1 8B 46 04 85 C0 74 5D"); //0x4AD441
    struct BloodyMoviesFix
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.esi = regs.ecx;
            regs.eax = *(uint32_t*)(regs.esi + 0x04);

            *(uint32_t*)(regs.esp + 0x08) = 0;
        }
    }; injector::MakeInline<BloodyMoviesFix>(pattern.get_first(0));

    pattern = hook::pattern("8B C7 5F F7 D8 1B C0 5E 25 ? ? ? ? C3"); //0x731C30
    struct InsertKey
    {
        void operator()(injector::reg_pack& regs)
        {
            static constexpr auto key = "1234-5678-9abc-dddf";
            if (!regs.eax)
                regs.eax = (uint32_t)key;
        }
    }; injector::MakeInline<InsertKey>(pattern.get_first(13));
    injector::MakeRET(pattern.get_first(13 + 5));

    HD2Update::Init();
}

class Misc
{
public:
    Misc()
    {
        WFP::onInitEvent() += []() { Install(); };
    }
} Misc;


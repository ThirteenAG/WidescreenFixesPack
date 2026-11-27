module;

#include <stdafx.h>
#include <atomic>
#include "FixSettings.hpp"
#include "overlay/OverlayD3D9.hpp"
#include "Shared/TonyHawk/Display.hpp"
#include "Shared/TonyHawk/PadPrompts.hpp"
#include "Shared/TonyHawk/ModeSwitch.hpp"

#pragma comment(lib, "version.lib")

export module Window;

import ComVars;

// The developer overlay on the game's Direct3D 9 device, and the game's window procedure (overlay input, focus)
export namespace THUG2Device
{
    inline asi_overlay::D3D9Overlay Overlay;
    inline SafetyHookInline CreateHook, PresentHook, ResetHook, WindowHook, KeyboardHook;
    inline std::array<SafetyHookMid, 2> ReleaseResourcesHooks, RestoreResourcesHooks;
    inline std::array<uintptr_t, 2> ResetAddresses{};
    inline bool ResetResourcesReleased = false;
    inline std::atomic<HWND> Window{ nullptr };
    inline UINT PauseSettingChanged = 0;
    inline bool HotkeyDown = false;
    inline std::atomic<int> FrameLimit{ 60 };
    inline std::mutex FrameLimitMutex;
    inline void (*MouseFrame)(HWND, bool) = nullptr;
    inline void (*MouseMessage)(HWND, UINT, LPARAM) = nullptr;

    inline void UpdateFrameLimit(HWND window)
    {
        std::lock_guard lock(FrameLimitMutex);
        int limit = std::atoi(FixSettings().Get("MAIN.FPSLimit").c_str());
        if (limit == -1)
        {
            MONITORINFOEXW monitor{};
            monitor.cbSize = sizeof(monitor);
            const auto display = MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY);
            DEVMODEW mode{};
            mode.dmSize = sizeof(mode);
            const bool knownMonitor = GetMonitorInfoW(display, &monitor) != FALSE;
            if (EnumDisplaySettingsW(knownMonitor ? monitor.szDevice : nullptr, ENUM_CURRENT_SETTINGS, &mode) && mode.dmDisplayFrequency > 1)
                limit = static_cast<int>(std::min<DWORD>(mode.dmDisplayFrequency, 60));
            else
                limit = 60;
        }
        else
            limit = limit > 0 ? std::clamp(limit, 30, 60) : 0;
        FrameLimit.store(limit, std::memory_order_relaxed);
    }

    inline void LimitFrameRate()
    {
        struct Pacer
        {
            HANDLE timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
            double frequency = []() { LARGE_INTEGER value{}; QueryPerformanceFrequency(&value); return static_cast<double>(value.QuadPart); }();
            double next = 0.0;
            int previous = 0;
            ~Pacer() { if (timer) CloseHandle(timer); }
            double Now() const { LARGE_INTEGER value{}; QueryPerformanceCounter(&value); return static_cast<double>(value.QuadPart); }
        };
        static Pacer pacer;
        const int limit = FrameLimit.load(std::memory_order_relaxed);
        if (limit <= 0 || pacer.frequency <= 0.0)
        {
            pacer.next = 0.0;
            pacer.previous = 0;
            return;
        }
        if (pacer.previous != limit)
        {
            pacer.next = 0.0;
            pacer.previous = limit;
        }
        const double period = pacer.frequency / limit;
        const double now = pacer.Now();
        if (pacer.next == 0.0 || now > pacer.next + period)
            pacer.next = now + period;
        const double seconds = (pacer.next - now) / pacer.frequency;
        constexpr double spinTime = 0.0015;
        if (seconds > spinTime)
        {
            LARGE_INTEGER due{};
            due.QuadPart = -static_cast<LONGLONG>((seconds - spinTime) * 10000000.0);
            if (pacer.timer && SetWaitableTimer(pacer.timer, &due, 0, nullptr, nullptr, FALSE))
                WaitForSingleObject(pacer.timer, INFINITE);
            else
                Sleep(static_cast<DWORD>((seconds - spinTime) * 1000.0));
        }
        while (pacer.Now() < pacer.next)
            YieldProcessor();
        pacer.next += period;
    }


    inline HMODULE ThisModule()
    {
        HMODULE module = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&ThisModule), &module);
        return module;
    }

    inline std::string BuildTime()
    {
        wchar_t path[MAX_PATH]{};
        GetModuleFileNameW(ThisModule(), path, DWORD(std::size(path)));
        DWORD ignored = 0;
        const auto size = GetFileVersionInfoSizeW(path, &ignored);
        std::vector<uint8_t> data(size);
        if (!size || !GetFileVersionInfoW(path, 0, size, data.data()))
            return "Unknown";
        wchar_t* value = nullptr;
        UINT length = 0;
        if (!VerQueryValueW(data.data(), L"\\StringFileInfo\\040004b0\\BuildTimeUtc", reinterpret_cast<void**>(&value), &length) || !value)
            return "Unknown";
        return asi_settings::text::Narrow(value);
    }

    inline HRESULT WINAPI Present(IDirect3DDevice9* device, const RECT* source, const RECT* destination, HWND window, const RGNDATA* dirty)
    {
        if (GameDevice.is_initialized() && device == GameDevice.get())
        {
            static const bool page = []()
            {
                asi_overlay::Overlay::AddPage("Debug", "Info", [](asi_overlay::OverlayUI& ui)
                {
                    static const auto buildTime = BuildTime();
                    ui.Value("Version", rsc_ProductVersion);
                    ui.Value("BuildTimeUtc", buildTime.c_str());
                    ui.Value("Graphics API", "Direct3D 9");
                    IDirect3DSurface9* surface = nullptr;
                    if (GameDevice.get() && SUCCEEDED((GameDevice.get())->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surface)))
                    {
                        D3DSURFACE_DESC desc{};
                        surface->GetDesc(&desc);
                        surface->Release();
                        ui.Value("Resolution", std::to_string(desc.Width) + " x " + std::to_string(desc.Height));
                    }
                });
                return true;
            }();
            if constexpr (WFP::DeveloperOverlay)
            {
                // The hotkey is read here too: the game's DirectInput keyboard can take the key messages
                const bool down = Window && GetForegroundWindow() == Window &&
                    (GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(VK_F10) & 0x8000);
                if (down && !HotkeyDown)
                    Overlay.SetOpen(!Overlay.IsOpen());
                HotkeyDown = down;
                Overlay.Present(device, ThisModule());
            }
            if (MouseFrame)
                MouseFrame(Window, Overlay.BlocksInput());
            TonyPad::Frame();
            LimitFrameRate();
        }
        const auto result = PresentHook.unsafe_stdcall<HRESULT>(device, source, destination, window, dirty);
        if (GameDevice.is_initialized() && device == GameDevice.get())
        {
            TonyModeSwitch::Presented(result);
            TonyModeSwitch::Update(device, Window);
        }
        return result;
    }

    inline HRESULT WINAPI Reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* parameters)
    {
        if (GameDevice.is_initialized() && device == GameDevice.get())
            Overlay.BeforeReset();
        return ResetHook.unsafe_stdcall<HRESULT>(device, parameters);
    }

    inline int __cdecl Create()
    {
        auto result = CreateHook.unsafe_ccall<int>();
        if (!GameDevice.get() || PresentHook)
            return result;
        D3DDEVICE_CREATION_PARAMETERS parameters{};
        if (SUCCEEDED((GameDevice.get())->GetCreationParameters(&parameters)))
        {
            Window = parameters.hFocusWindow;
            UpdateFrameLimit(parameters.hFocusWindow);
        }
        auto table = *reinterpret_cast<void***>(GameDevice.get());
        PresentHook = safetyhook::create_inline(table[17], Present);
        if constexpr (WFP::DeveloperOverlay)
            ResetHook = safetyhook::create_inline(table[16], Reset);
        return result;
    }

    inline LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (MouseMessage)
            MouseMessage(window, message, lParam);
        if (window == Window && (message == WM_WINDOWPOSCHANGED || message == WM_DISPLAYCHANGE))
            UpdateFrameLimit(window);
        LRESULT result = 0;
        if (WFP::DeveloperOverlay && (message == WM_KEYDOWN || message == WM_SYSKEYDOWN || message == WM_KEYUP || message == WM_SYSKEYUP) &&
            wParam == VK_F10 && ((GetAsyncKeyState(VK_CONTROL) & 0x8000) || HotkeyDown))
            return 0;
        if (TonyModeSwitch::Message(window, message, wParam, lParam))
            return 0;
        if (Overlay.Message(window, message, wParam, lParam, result))
            return result;
        const bool pauseSettingMessage = PauseSettingChanged && message == PauseSettingChanged;
        const bool deactivating = message == WM_ACTIVATE && LOWORD(wParam) == WA_INACTIVE;
        if (TonyDisplay::Mode != 0 && (deactivating || pauseSettingMessage))
        {
            if (pauseSettingMessage && GetForegroundWindow() == window)
                return 0;
            if (FixSettings().Get("MAIN.PauseOnFocusLoss") == "0")
            {
                if (pauseSettingMessage)
                {
                    AppActive = 1;
                    return 0;
                }
                return DefWindowProcA(window, message, wParam, lParam);
            }
            if (pauseSettingMessage)
                return WindowHook.unsafe_stdcall<LRESULT>(window, WM_ACTIVATE, WA_INACTIVE, 0);
        }
        return WindowHook.unsafe_stdcall<LRESULT>(window, message, wParam, lParam);
    }

    // The game reads the keyboard through DirectInput once per frame; the open overlay keeps the keys for itself
    inline void __cdecl ReadKeyboard()
    {
        KeyboardHook.unsafe_ccall<void>();
        if (Overlay.BlocksInput())
            std::memset(KeyBuffer.get().data(), 0, 256);
        TonyModeSwitch::KeysRead(KeyBuffer.get().data());
        TonyPad::KeysRead(KeyBuffer.get().data(), PreviousKeys.get().data());
    }

    template<size_t Index>
    void ReleaseResetResources(SafetyHookContext& regs)
    {
        if (std::exchange(ResetResourcesReleased, true))
            regs.eip = ResetAddresses[Index];
    }

    inline void Install()
    {
        // The recovery routine and main loop both retry Reset. The native cleanup retains buffer
        // identities for the rebuild, so only release once until a successful reset rebuilds them.
        auto recovery = hook::pattern(
            "E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 8B 10 68 ? ? ? ? 50 FF 52 40 "
            "85 C0 7C 16 E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? C6 05 ? ? ? ? 00").count(2);
        constexpr std::array release{ &ReleaseResetResources<0>, &ReleaseResetResources<1> };
        for (size_t i = 0; i < ReleaseResourcesHooks.size(); ++i)
        {
            const auto site = recovery.get(i);
            ResetAddresses[i] = reinterpret_cast<uintptr_t>(site.get<void>(15));
            ReleaseResourcesHooks[i] = safetyhook::create_mid(site.get<void>(), release[i]);
            RestoreResourcesHooks[i] = safetyhook::create_mid(site.get<void>(50), [](SafetyHookContext&)
            {
                ResetResourcesReleased = false;
            });
            if (!ReleaseResourcesHooks[i] || !RestoreResourcesHooks[i])
                std::terminate();
        }

        UpdateFrameLimit(Window);
        FixSettings().OnChange("MAIN.FPSLimit", []() { UpdateFrameLimit(Window); });
        PauseSettingChanged = RegisterWindowMessageW(L"THUG2.FusionFix.PauseSettingChanged");
        // Apply background setting changes on the window thread without activating the window.
        FixSettings().OnChange("MAIN.PauseOnFocusLoss", []()
        {
            if (const auto window = Window.load(); window && PauseSettingChanged)
                PostMessageA(window, PauseSettingChanged, 0, 0);
        });
        auto create = hook::pattern("83 EC 58 A1 ? ? ? ? 89 44 24 54 A0 ? ? ? ? 53 33 DB 84 C0");
        auto window = hook::pattern("8B 44 24 08 3D 00 01 00 00 0F 87 ? ? ? ? 0F 84");
        auto keyboard = hook::pattern("A1 ? ? ? ? 56 57 68 ? ? ? ? B9 40 00 00 00 BE");
        auto deactivate = hook::pattern("6A 01 6A 00 6A 00 C6 05 ? ? ? ? 00 C6 05 ? ? ? ? 01 FF 15");
        AppActive.SetAddress(*deactivate.get_first<uint8_t*>(8));
        KeyBuffer.SetAddress(*keyboard.get_first<uint8_t*>(8));
        PreviousKeys.SetAddress(*keyboard.get_first<uint8_t*>(23));
        CreateHook = safetyhook::create_inline(create.get_first(), Create);
        WindowHook = safetyhook::create_inline(window.get_first(), WindowProc);
        KeyboardHook = safetyhook::create_inline(keyboard.get_first(), ReadKeyboard);
    }
}

export namespace THUG2Display
{
    inline std::atomic<bool> StartupMoviesFinished{ false };

    void Init(int width, int height)
    {
        auto& settings = FixSettings();
        auto mode = std::clamp(settings.Int("MAIN.WindowMode"), 0, 2);
        TonyModeSwitch::Install("A0 ? ? ? ? 84 C0 74 ? A1 ? ? ? ? 8B 08 50 FF 51 0C 3D 68 08 76 88 74 ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 8B 10 68 ? ? ? ? 50 FF 52 40 85 C0 7C ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? C6 05 ? ? ? ? 00 C3", 1, 0x32, mode, width, height);
        WindowedModeWrapper::EnableDpiAwareness();
        TonyDisplay::Window(0, width, height);
        if (mode != 0)
        {
            auto pattern = hook::pattern("A3 ? ? ? ? B8 01 00 00 00 89 1D ? ? ? ? A3 ? ? ? ? 89 74 24 10 76");
            NativeWindowed.SetAddress(*pattern.get_first<int*>(12));
            static auto windowedHook = safetyhook::create_mid(pattern.get_first(16), [](SafetyHookContext&)
            {
                NativeWindowed = 1;
            });
            TonyDisplay::Window(mode, width, height);
        }
        if (settings.Bool("MAIN.SkipIntro"))
        {
            // The startup logos and the intro movie play once before the main menu; Options > Movies still plays them
            auto pattern = hook::pattern("51 8B 4C 24 08 6A 01 8D 44 24 04 50 6A 00 E8 ? ? ? ? 84 C0 74 10 8B 0C 24 51 E8 ? ? ? ? 83 C4 04 B0 01 59 C3 32 C0 59 C3");
            static const uintptr_t resume = reinterpret_cast<uintptr_t>(pattern.get_first(32));
            static auto movieHook = safetyhook::create_mid(pattern.get_first(27), [](SafetyHookContext& regs)
            {
                if (StartupMoviesFinished.load(std::memory_order_relaxed))
                    return;
                constexpr const char* startup[]{ "Intel_intro", "Dolby_Digital", "atvi", "nslogo", "Beenox_Shift", "intro", "introf" };
                std::string_view name(*reinterpret_cast<const char**>(regs.esp));
                if (auto slash = name.find_last_of("/\\"); slash != std::string_view::npos)
                    name.remove_prefix(slash + 1);
                if (std::ranges::any_of(startup, [name](const char* movie)
                {
                    return name.size() == std::strlen(movie) && _strnicmp(name.data(), movie, name.size()) == 0;
                }))
                    regs.eip = resume;
            });
        }
    }
}


export void InstallResolution()
{
    hook::pattern pattern("");
    //Resolution
    pattern = hook::pattern("89 0D ? ? ? ? 89 15 ? ? ? ? 89 0D ? ? ? ? 89 15 ? ? ? ? 74");
    dword_67FBCC.SetAddress(*pattern.get_first<int32_t*>(2));
    dword_67FBD0.SetAddress(*pattern.get_first<int32_t*>(8));
    dword_787A7C.SetAddress(*pattern.get_first<int32_t*>(12 + 2));
    dword_787A80.SetAddress(*pattern.get_first<int32_t*>(12 + 8));
    struct SetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            dword_67FBCC = Screen.Width;
            dword_67FBD0 = Screen.Height;
            dword_787A7C = Screen.Width;
            dword_787A80 = Screen.Height;
        }
    };

    auto width = hook::pattern("89 0D ? ? ? ? 89 15 ? ? ? ? 89 0D ? ? ? ? 89 15 ? ? ? ? 74 0A").get_first<uint8_t>();
    auto widthAccumulator = hook::pattern("A3 ? ? ? ? 89 0D ? ? ? ? E8 ? ? ? ? 83 C4 10").get_first<uint8_t>();
    const char* heights[] = {
        "89 15 ? ? ? ? 89 0D ? ? ? ? 89 15 ? ? ? ? 74 0A",
        "89 0D ? ? ? ? E8 ? ? ? ? 83 C4 10"
    };
    const char* redundant[] = {
        "89 0D ? ? ? ? 8B 08 1B D2",
        "89 0D ? ? ? ? 89 15 ? ? ? ? 74 0A",
        "89 15 ? ? ? ? 8A 54 24 0C",
        "89 15 ? ? ? ? 74 0A"
    };
    std::array<uint8_t*, 2> heightSites{};
    std::array<uint8_t*, 4> redundantSites{};
    for (size_t i = 0; i < heightSites.size(); ++i)
        heightSites[i] = hook::pattern(heights[i]).get_first<uint8_t>();
    for (size_t i = 0; i < redundantSites.size(); ++i)
        redundantSites[i] = hook::pattern(redundant[i]).get_first<uint8_t>();
    injector::MakeInline<SetResHook>(width, width + 6);
    injector::MakeInline<SetResHook>(widthAccumulator);
    for (auto* site : heightSites)
        injector::MakeInline<SetResHook>(site, site + 6);
    for (auto* site : redundantSites)
        injector::MakeNOP(site, 6, true);
}

export void InstallLoadingGradient()
{
    hook::pattern pattern("");
    // The loading gradient writes three DWORDs per entry. Its starting ECX is four bytes past the first
    // destination; size the replacement for the actual loop count instead of overrunning a fixed buffer.
    pattern = hook::pattern("B9 ? ? ? ? 89 ? 24 14 ? ? 24");
    struct LoadingGradientBufferHook
    {
        void operator()(injector::reg_pack& regs)
        {
            static std::vector<uint32_t> gradient;
            gradient.resize(static_cast<size_t>(regs.esi) * 3 + 1);
            regs.ecx = reinterpret_cast<uint32_t>(gradient.data() + 1);
        }
    }; injector::MakeInline<LoadingGradientBufferHook>(pattern.get_first(), pattern.get_first(5));
}

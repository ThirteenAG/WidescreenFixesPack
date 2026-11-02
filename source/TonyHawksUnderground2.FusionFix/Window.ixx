module;

#include <stdafx.h>
#include <atomic>
#include "FixSettings.hpp"
#include "overlay/OverlayD3D9.hpp"
#include "Shared/TonyHawk/Display.hpp"

#pragma comment(lib, "version.lib")

export module Window;

import ComVars;

// The developer overlay on the game's Direct3D 9 device, and the game's window procedure (overlay input, focus)
export namespace THUG2Device
{
    inline asi_overlay::D3D9Overlay Overlay;
    inline SafetyHookInline CreateHook, PresentHook, ResetHook, WindowHook, KeyboardHook;
    inline HWND Window = nullptr;
    inline bool HotkeyDown = false;
    inline std::atomic<int> FrameLimit{ 60 };
    inline void (*MouseFrame)(HWND, bool) = nullptr;
    inline void (*MouseMessage)(HWND, UINT, LPARAM) = nullptr;

    inline void UpdateFrameLimit()
    {
        int limit = FixSettings().Int("MAIN.FPSLimit");
        if (limit == -1)
        {
            DEVMODEW mode{};
            mode.dmSize = sizeof(mode);
            limit = EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &mode) && mode.dmDisplayFrequency > 1
                ? static_cast<int>(mode.dmDisplayFrequency) : 60;
        }
        FrameLimit.store(limit > 0 ? std::clamp(limit, 30, 1000) : 0, std::memory_order_relaxed);
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
            // The hotkey is read here too: the game's DirectInput keyboard can take the key messages
            const bool down = Window && GetForegroundWindow() == Window &&
                (GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(VK_F10) & 0x8000);
            if (down && !HotkeyDown)
                Overlay.SetOpen(!Overlay.IsOpen());
            HotkeyDown = down;
            Overlay.Present(device, ThisModule());
            if (MouseFrame)
                MouseFrame(Window, Overlay.BlocksInput());
            LimitFrameRate();
        }
        return PresentHook.unsafe_stdcall<HRESULT>(device, source, destination, window, dirty);
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
            Window = parameters.hFocusWindow;
        auto table = *reinterpret_cast<void***>(GameDevice.get());
        PresentHook = safetyhook::create_inline(table[17], Present);
        ResetHook = safetyhook::create_inline(table[16], Reset);
        return result;
    }

    inline LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (MouseMessage)
            MouseMessage(window, message, lParam);
        LRESULT result = 0;
        if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN || message == WM_KEYUP || message == WM_SYSKEYUP) &&
            wParam == VK_F10 && ((GetAsyncKeyState(VK_CONTROL) & 0x8000) || HotkeyDown))
            return 0;
        if (Overlay.Message(window, message, wParam, lParam, result))
            return result;
        // The game stops while its window is inactive (and minimizes it); in a window mode it can keep running
        static const int& pauseOnFocusLoss = FixSettings().Int("MAIN.PauseOnFocusLoss");
        if (message == WM_ACTIVATE && LOWORD(wParam) == WA_INACTIVE && TonyDisplay::Mode != 0 && !pauseOnFocusLoss)
            return DefWindowProcA(window, message, wParam, lParam);
        return WindowHook.unsafe_stdcall<LRESULT>(window, message, wParam, lParam);
    }

    // The game reads the keyboard through DirectInput once per frame; the open overlay keeps the keys for itself
    inline void __cdecl ReadKeyboard()
    {
        KeyboardHook.unsafe_ccall<void>();
        if (Overlay.BlocksInput())
            std::memset(KeyBuffer.get().data(), 0, 256);
    }

    inline void Install()
    {
        UpdateFrameLimit();
        FixSettings().OnChange("MAIN.FPSLimit", UpdateFrameLimit);
        auto create = hook::pattern("83 EC 58 A1 ? ? ? ? 89 44 24 54 A0 ? ? ? ? 53 33 DB 84 C0");
        auto window = hook::pattern("8B 44 24 08 3D 00 01 00 00 0F 87 ? ? ? ? 0F 84");
        auto keyboard = hook::pattern("A1 ? ? ? ? 56 57 68 ? ? ? ? B9 40 00 00 00 BE");
        KeyBuffer.SetAddress(*keyboard.get_first<uint8_t*>(8));
        CreateHook = safetyhook::create_inline(create.get_first(), Create);
        WindowHook = safetyhook::create_inline(window.get_first(), WindowProc);
        KeyboardHook = safetyhook::create_inline(keyboard.get_first(), ReadKeyboard);
    }
}

export namespace THUG2Display
{
    void Init(int width, int height)
    {
        auto& settings = FixSettings();
        auto mode = std::clamp(settings.Int("MAIN.WindowMode"), 0, 2);
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
                // The intro plays as intro or, in other languages, introf: one of them at startup
                static std::array<std::pair<std::array<const char*, 2>, bool>, 6> startup{ { { { "Intel_intro" }, false },
                    { { "Dolby_Digital" }, false }, { { "atvi" }, false }, { { "nslogo" }, false }, { { "Beenox_Shift" }, false },
                    { { "intro", "introf" }, false } } };
                std::string_view name(*reinterpret_cast<const char**>(regs.esp));
                if (auto slash = name.find_last_of("/\\"); slash != std::string_view::npos)
                    name.remove_prefix(slash + 1);
                for (auto& [movies, skipped] : startup)
                {
                    for (auto movie : movies)
                    {
                        if (!skipped && movie && name.size() == std::strlen(movie) && _strnicmp(name.data(), movie, name.size()) == 0)
                        {
                            skipped = true;
                            regs.eip = resume;
                            return;
                        }
                    }
                }
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

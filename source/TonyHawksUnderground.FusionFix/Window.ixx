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
export namespace THUGDevice
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

    inline IDirect3D9* __cdecl Create()
    {
        auto result = CreateHook.unsafe_ccall<IDirect3D9*>();
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
        auto create = hook::pattern("D9 E8 81 EC 84 01 00 00 57 D9 15");
        auto device = hook::pattern("A1 ? ? ? ? 8B 08 8B 51 40 68 ? ? ? ? 50 FF D2");
        auto window = hook::pattern("8B 44 24 08 3D 00 01 00 00 0F 87 ? ? ? ? 0F 84");
        auto keyboard = hook::pattern("A1 ? ? ? ? 56 57 68 ? ? ? ? B9 40 00 00 00 BE");
        (void)*device.get_first<IDirect3DDevice9**>(1);
        KeyBuffer.SetAddress(*keyboard.get_first<uint8_t*>(8));
        CreateHook = safetyhook::create_inline(create.get_first(), Create);
        WindowHook = safetyhook::create_inline(window.get_first(), WindowProc);
        KeyboardHook = safetyhook::create_inline(keyboard.get_first(), ReadKeyboard);
    }
}

export namespace THUGDisplay
{
    void Init(int width, int height)
    {
        auto& settings = FixSettings();
        auto mode = std::clamp(settings.Int("MAIN.WindowMode"), 0, 2);
        WindowedModeWrapper::EnableDpiAwareness();
        TonyDisplay::Window(0, width, height);
        if (mode != 0)
        {
            auto pattern = hook::pattern("89 3D ? ? ? ? 89 15 ? ? ? ? 76 05 BD 03 00 00 00 53 6A 04 B8 00 04 00 00");
            NativeWindowed.SetAddress(*pattern.get_first<int*>(2));
            static auto windowedHook = safetyhook::create_mid(pattern.get_first(6), [](SafetyHookContext&)
            {
                NativeWindowed = 1;
            });
            TonyDisplay::Window(mode, width, height);
        }
        if (settings.Bool("MAIN.SkipIntro"))
        {
            // The startup logos and the intro movie play once before the main menu; Options > THUG Movies still plays them
            auto pattern = hook::pattern("51 8B 4C 24 08 6A 01 8D 44 24 04 50 6A 00 E8 ? ? ? ? 84 C0 74 10 8B 0C 24 51 E8 ? ? ? ? 83 C4 04 B0 01 59 C3 32 C0 59 C3");
            static const uintptr_t resume = reinterpret_cast<uintptr_t>(pattern.get_first(32));
            static auto movieHook = safetyhook::create_mid(pattern.get_first(27), [](SafetyHookContext& regs)
            {
                static std::array<std::pair<const char*, bool>, 4> startup{ { { "atvi", false }, { "nslogo", false }, { "beenox", false }, { "intro", false } } };
                std::string_view name(*reinterpret_cast<const char**>(regs.esp));
                if (auto slash = name.find_last_of("/\\"); slash != std::string_view::npos)
                    name.remove_prefix(slash + 1);
                for (auto& [movie, skipped] : startup)
                {
                    if (!skipped && name.size() == std::strlen(movie) && _strnicmp(name.data(), movie, name.size()) == 0)
                    {
                        skipped = true;
                        regs.eip = resume;
                        return;
                    }
                }
            });
        }
    }
}


export void InstallResolutionTable()
{
    hook::pattern pattern("");
    // The native writer starts one DWORD before ECX and emits three per entry.
    static std::vector<uint32_t> resolutionTable;
    pattern = hook::pattern("B9 ? ? ? ? 89 ? 24 14 ? ? 24");
    struct ResolutionTableHook
    {
        void operator()(injector::reg_pack& regs)
        {
            resolutionTable.resize(static_cast<size_t>(regs.esi) * 3 + 1);
            regs.ecx = reinterpret_cast<uint32_t>(resolutionTable.data() + 1);
        }
    }; injector::MakeInline<ResolutionTableHook>(pattern.get_first(), pattern.get_first(5));
}

export void InstallResolution()
{
    hook::pattern pattern("");
    //Resolution
    pattern = hook::pattern("6A 01 6A 01 ? 8B 15");
    dword_6D3608.SetAddress(*pattern.get_first<int32_t*>(7));
    dword_6D360C.SetAddress(*pattern.get_first<int32_t*>(-4));
    struct SetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            dword_6D3608 = Screen.Width;
            dword_6D360C = Screen.Height;
        }
    };
    auto widthAccumulator = hook::pattern("A3 ? ? ? ? A1 ? ? ? ? 8B 10").get_first<uint8_t>();
    auto widthRegister = hook::pattern("89 0D ? ? ? ? 89 15 ? ? ? ? 89 1D ? ? ? ? 89 1D ? ? ? ?").get_first<uint8_t>();
    const char* heightPatterns[] = {
        "89 15 ? ? ? ? 89 1D ? ? ? ? 89 1D ? ? ? ?",
        "89 15 ? ? ? ? 8B 15 ? ? ? ? 52 53 89 0D ? ? ? ? 8B 08 6A 00 50 8B 41 40 FF D0 85 C0 7D 09 03 F3 83 FE 04 72 B7 EB 14 8B 4C B4 14 8B 54 B4 24 89 0D ? ? ? ? 89 15 ? ? ? ? 85 C0 0F 8D ? ? ? ?",
        "89 15 ? ? ? ? 8B 15 ? ? ? ? 52 53 89 0D ? ? ? ? 8B 08 6A 00 50 8B 41 40 FF D0 85 C0 7D 09 03 F3 83 FE 04 72 B7 EB 14 8B 4C B4 14 8B 54 B4 24 89 0D ? ? ? ? 89 15 ? ? ? ? 85 C0 7D 73",
        "89 0D ? ? ? ? 8B 0D ? ? ? ? 51 A3 ? ? ? ?"
    };
    std::array<uint8_t*, 4> heightSites{};
    for (size_t i = 0; i < heightSites.size(); ++i)
        heightSites[i] = hook::pattern(heightPatterns[i]).get_first<uint8_t>();
    injector::MakeInline<SetResHook>(widthAccumulator);
    for (auto* site : heightSites)
        injector::MakeInline<SetResHook>(site, site + 6);
    injector::MakeInline<SetResHook>(widthRegister, widthRegister + 6);
}

module;

#include <stdafx.h>
#include <atomic>
#include "FixSettings.hpp"
#include "overlay/OverlayD3D9.hpp"
#include "Shared/TonyHawk/Display.hpp"

#pragma comment(lib, "version.lib")

export module Window;

import ComVars;
import Hud;
import Cinema;

// The developer overlay on the game's Direct3D 9 device, and the game's window procedure (overlay input, focus)
export namespace THAWDevice
{
    inline asi_overlay::D3D9Overlay Overlay;
    inline SafetyHookInline PresentHook, ResetHook, WindowHook;
    inline HWND Window = nullptr;
    inline bool HotkeyDown = false;
    inline std::atomic<int> FrameLimit{ 60 };

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

    // Called before each presented frame: the device's Present and Reset are hooked once it exists
    inline void Attach()
    {
        if (PresentHook || !GameDevice.is_initialized() || !GameDevice.get())
            return;
        D3DDEVICE_CREATION_PARAMETERS parameters{};
        if (SUCCEEDED((GameDevice.get())->GetCreationParameters(&parameters)))
            Window = parameters.hFocusWindow;
        auto table = *reinterpret_cast<void***>(GameDevice.get());
        PresentHook = safetyhook::create_inline(table[17], Present);
        ResetHook = safetyhook::create_inline(table[16], Reset);
    }

    inline bool BlocksInput()
    {
        return Overlay.BlocksInput();
    }

    inline LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        LRESULT result = 0;
        if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN || message == WM_KEYUP || message == WM_SYSKEYUP) &&
            wParam == VK_F10 && ((GetAsyncKeyState(VK_CONTROL) & 0x8000) || HotkeyDown))
            return 0;
        if (Overlay.Message(window, message, wParam, lParam, result))
            return result;
        // The game stops while its window is inactive; in a window mode it can keep running
        static const int& pauseOnFocusLoss = FixSettings().Int("MAIN.PauseOnFocusLoss");
        if (message == WM_ACTIVATE && LOWORD(wParam) == WA_INACTIVE && TonyDisplay::Mode != 0 && !pauseOnFocusLoss)
            return DefWindowProcA(window, message, wParam, lParam);
        return WindowHook.unsafe_stdcall<LRESULT>(window, message, wParam, lParam);
    }

    inline void Install()
    {
        UpdateFrameLimit();
        FixSettings().OnChange("MAIN.FPSLimit", UpdateFrameLimit);
        auto present = hook::pattern("A1 ? ? ? ? 8B 08 6A 00 6A 00 6A 00 6A 00 50 FF 51 44");
        auto window = hook::pattern("8B 44 24 08 3D 00 01 00 00 0F 87 ? ? ? ? 74");
        GameDevice.SetAddress(*present.get_first<IDirect3DDevice9**>(1));
        WindowHook = safetyhook::create_inline(window.get_first(), WindowProc);
    }
}

export namespace THAWDisplay
{
    inline void (__cdecl* Reset)() = nullptr;
    inline bool Resizable = false;
    inline SafetyHookMid ScaleHook;
    inline SafetyHookInline LetterboxHook;

    // The 2D layer: a 640x480 frame, pillarboxed on wider screens and letterboxed on narrower ones (5:4); the native
    // letterbox screen squeezes it into the middle three quarters of the frame's height
    void UpdateFrame()
    {
        if (!GameScaleY.is_initialized() || !GameYOffset.is_initialized())
            return;
        const float frameHeight = std::min(Screen.fHeight, Screen.fWidth * 0.75f);
        const float scale = frameHeight / 480.0f;
        int32_t offset = static_cast<int32_t>((Screen.fHeight - frameHeight) * 0.5f) + static_cast<int32_t>(scale * 16.0f);
        if (Letterboxed.is_initialized() && Letterboxed.get())
        {
            GameScaleY = scale * 0.75f;
            offset += static_cast<int32_t>(frameHeight) / 4 / 2;
        }
        else
            GameScaleY = scale;
        GameYOffset = offset;
    }

    // The native switch set fixed 640x480 values (0.75/1.0) regardless of the resolution
    void __cdecl SetLetterbox(char on)
    {
        if (Letterboxed.get() == static_cast<uint8_t>(on != 0))
            return;
        Letterboxed = on != 0;
        UpdateFrame();
    }

    void UpdateDimensions(int width, int height)
    {
        Screen.Width = width;
        Screen.Height = height;
        Screen.fWidth = static_cast<float>(width);
        Screen.fHeight = static_cast<float>(height);
        Screen.Width43 = static_cast<int32_t>(Screen.fHeight * (4.0f / 3.0f));
        Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
        Screen.fAspectRatioDiff = 1.0f / (Screen.fAspectRatio / (4.0f / 3.0f));
        Screen.fHUDScaleX = std::min(Screen.fWidth, Screen.fHeight * (4.0f / 3.0f)) / 640.0f / Screen.fWidth;
        Screen.fHudOffset = (480.0f * Screen.fAspectRatio - 640.0f) * 0.5f;
        Screen.fHudOffsetReal = std::max(0.0f, (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) * 0.5f);
        THAWHud::Width = Screen.fWidth;
        THAWHud::Height = Screen.fHeight;
        THAWCinema::Width = width;
        THAWCinema::Height = height;
        TonyDisplay::Width = width;
        TonyDisplay::Height = height;
        WindowedModeWrapper::desiredClientWidth = width;
        WindowedModeWrapper::desiredClientHeight = height;
        if (dword_6D2224.is_initialized()) dword_6D2224 = width;
        if (dword_6D2228.is_initialized()) dword_6D2228 = height;
        if (dword_787D88.is_initialized()) dword_787D88 = static_cast<int32_t>(Screen.fHudOffsetReal);
        if (GameScaleX.is_initialized()) GameScaleX = std::min(Screen.fWidth, Screen.fHeight * (4.0f / 3.0f)) / 640.0f;
        UpdateFrame();
        if (AspectImmediate.is_initialized()) AspectImmediate = Screen.fAspectRatio;
    }

    void UpdateResize()
    {
        if (!Resizable || !WindowedModeWrapper::GameHWND || IsIconic(WindowedModeWrapper::GameHWND))
            return;
        RECT client{};
        if (!GetClientRect(WindowedModeWrapper::GameHWND, &client))
            return;
        const int width = client.right - client.left, height = client.bottom - client.top;
        if (width <= 0 || height <= 0 || width > 16384 || height > 16384)
            return;
        static int pendingWidth = 0, pendingHeight = 0;
        static ULONGLONG changed = 0;
        if (width != pendingWidth || height != pendingHeight)
        {
            pendingWidth = width;
            pendingHeight = height;
            changed = GetTickCount64();
            return;
        }
        if ((width == Screen.Width && height == Screen.Height) || GetTickCount64() - changed < 150)
            return;
        UpdateDimensions(width, height);
        Presentation->BackBufferWidth = width;
        Presentation->BackBufferHeight = height;
        Reset();
    }

    void Init(int width, int height)
    {
        auto& settings = FixSettings();
        auto mode = std::clamp(settings.Int("MAIN.WindowMode"), 0, 2);
        WindowedModeWrapper::EnableDpiAwareness();
        WindowedModeWrapper::bEnableWindowResize = mode == 1;
        TonyDisplay::Window(0, width, height);
        if (mode != 0)
        {
            auto pattern = hook::pattern("A0 ? ? ? ? 8B 3D ? ? ? ? 33 C9 84 C0 0F 94 C1 B8 03 00 00 00 2B C7");
            NativeWindowed.SetAddress(*pattern.get_first<uint8_t*>(1));
            static auto windowedHook = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext&)
            {
                NativeWindowed = 0;
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
                static std::array<std::pair<const char*, bool>, 3> startup{ { { "New_Aspyr_firelogo", false }, { "ATVI", false },
                    { "intro", false } } };
                std::string_view name(*reinterpret_cast<const char**>(regs.esp));
                if (auto slash = name.find_last_of("/\\"); slash != std::string_view::npos)
                    name.remove_prefix(slash + 1);
                if (name.size() > 4 && _strnicmp(name.data() + name.size() - 4, ".bik", 4) == 0)
                    name.remove_suffix(4);
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
    //The native writer starts four bytes before ECX and emits three DWORDs per entry.
    static std::vector<uint32_t> resolutionTable;
    pattern = hook::pattern("B9 ? ? ? ? 89 74 24 14 8B FF 8B C3 99 F7 FE"); //0x50D195
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
    pattern = hook::pattern("89 0D ? ? ? ? 89 0D ? ? ? ? 8A 0D ? ? ? ? 84 C9");
    dword_6D2224.SetAddress(*pattern.get_first<int32_t*>(2));
    dword_6D2228.SetAddress(*hook::get_pattern<int32_t*>("89 3D ? ? ? ? 89 3D ? ? ? ? 89 15 ? ? ? ? 89 15 ? ? ? ? 74", 2));
    dword_7F3A2C.SetAddress(*pattern.get_first<int32_t*>(8));
    dword_7F3A30.SetAddress(*hook::get_pattern<int32_t*>("89 3D ? ? ? ? 89 3D ? ? ? ? 89 15 ? ? ? ? 89 15 ? ? ? ? 74", 8));
    struct SetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            dword_6D2224 = Screen.Width;
            dword_6D2228 = Screen.Height;
            dword_7F3A2C = Screen.Width;
            dword_7F3A30 = Screen.Height;
        }
    };
    auto accumulator = hook::pattern("A3 ? ? ? ? 89 0D ? ? ? ? E8 ? ? ? ? 8B F8").get_first<uint8_t>();
    const char* patterns[] = {
        "89 0D ? ? ? ? 89 0D ? ? ? ? 8A 0D ? ? ? ?",
        "89 3D ? ? ? ? 89 3D ? ? ? ? 89 15 ? ? ? ?",
        "89 0D ? ? ? ? E8 ? ? ? ? 8B F8",
        "89 0D ? ? ? ? 1B C0 83 E0 02",
        "89 0D ? ? ? ? 8A 0D ? ? ? ? 84 C9",
        "89 15 ? ? ? ? 8A 54 24 14",
        "89 3D ? ? ? ? 89 15 ? ? ? ? 89 15 ? ? ? ?"
    };
    std::array<uint8_t*, 7> sites{};
    for (size_t i = 0; i < sites.size(); ++i)
        sites[i] = hook::pattern(patterns[i]).get_first<uint8_t>();
    injector::MakeInline<SetResHook>(accumulator);
    for (auto* site : sites)
        injector::MakeInline<SetResHook>(site, site + 6);
}

export void InstallAspectRatio()
{
    hook::pattern pattern("");
    //Aspect Ratio
    pattern = hook::pattern("68 ? ? ? ? E8 ? ? ? ? 6A 00 68 ? ? ? ? E8 ? ? ? ? D9");
    AspectImmediate.SetAddress(pattern.get_first<float>(1));
    injector::WriteMemory<float>(AspectImmediate.get_ptr(), Screen.fAspectRatio, true);
    pattern = hook::pattern("E8 ? ? ? ? D8 0D ? ? ? ? D8 7C 24");
    injector::WriteMemory<float*>(reinterpret_cast<float**>(injector::GetBranchDestination(pattern.get_first()).as_int() + 2), &Screen.fAspectRatio, true);
}

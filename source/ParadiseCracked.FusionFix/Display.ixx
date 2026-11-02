module;
#include <stdafx.h>
#include "FixSettings.hpp"
#include <ddraw.h>
#include <d3d.h>

export module Display;
import ComVars;


// The game draws everything into one back buffer of its current mode: the 800x600 menus and movies, and the
// chosen resolution in missions. The fix keeps those modes and shows the back buffer in the window (or on the
// display) at its own aspect ratio, centred with black bars, so the menus are a 4:3 picture at the screen's height.
export namespace ParadiseDisplay
{
    inline int Mode = 0;                    // 0 Fullscreen, 1 Windowed, 2 Borderless
    inline int Width = 800;                 // mission resolution
    inline int Height = 600;
    inline bool GameRequest = false;        // the last mode switch was for a mission (not the menus)
    inline bool MenuReady = false;          // the menus have been shown
    inline bool MovieFrame = false;
    inline bool Loading = false;
    inline bool Scaled = false;             // fullscreen: a mode smaller than the display, scaled at each flip
    inline HWND Window = nullptr;
    inline std::atomic<int> MovieMode{ 1 }; // 0 Original, 1 Fill
    inline std::atomic<bool> PauseOnFocusLoss{ true };

    inline IDirectDrawSurface7* Copy = nullptr;
    inline SIZE CopySize{};
    inline uintptr_t SwitchReturn = 0;

    inline SafetyHookMid ModeHook, SwitchHook, ReleaseHook;
    inline SafetyHookInline WindowedPresentHook, FlipHook, FullscreenHook, MovieHook, ShowHook, PauseHook;
    inline HRESULT(__cdecl* Restore)() = nullptr;
    inline void (*OnFrame)() = nullptr;
    inline void (*OnWindow)(HWND) = nullptr;

    struct Layout
    {
        RECT Source;
        RECT Target;
    };

    inline RECT Fit(int width, int height, int areaWidth, int areaHeight)
    {
        if (width <= 0 || height <= 0)
            return { 0, 0, areaWidth, areaHeight };
        const double scale = (std::min)(double(areaWidth) / width, double(areaHeight) / height);
        const int w = int(std::lround(width * scale)), h = int(std::lround(height * scale));
        const int x = (areaWidth - w) / 2, y = (areaHeight - h) / 2;
        return { x, y, x + w, y + h };
    }

    // The part of the back buffer that is shown, and where it goes in an area of the given size.
    // surfaceWidth/Height: the back buffer's real size (bigger than the mode in a scaled fullscreen)
    inline Layout Arrange(int areaWidth, int areaHeight, int surfaceWidth, int surfaceHeight, bool content)
    {
        const int width = Logical.get_ptr()[0], height = Logical.get_ptr()[1];
        RECT source{ 0, 0, width, height };
        if (content && MovieFrame && Movie.get() && *reinterpret_cast<int32_t**>(Movie.get() + 4))
        {
            // Bink frames are copied to the centre of the back buffer surface
            const auto bink = *reinterpret_cast<int32_t**>(Movie.get() + 4);
            const int w = MovieMode.load(std::memory_order_relaxed) ? bink[0] : width;
            const int h = MovieMode.load(std::memory_order_relaxed) ? bink[1] : height;
            const int x = (surfaceWidth - w) / 2, y = (surfaceHeight - h) / 2;
            source = { x, y, x + w, y + h };
        }
        else if (content && Loading && width >= 640 && height >= 480)
        {
            // The loading screen is a 640x480 picture in the middle of the mission resolution
            const int x = (width - 640) / 2, y = (height - 480) / 2;
            source = { x, y, x + 640, y + 480 };
        }
        return { source, Fit(source.right - source.left, source.bottom - source.top, areaWidth, areaHeight) };
    }

    // Window client point <-> game point (the menus' 800x600 picture inside its box)
    inline POINT ToGame(POINT client)
    {
        RECT area{};
        if (!Window || !GetClientRect(Window, &area) || area.right <= 0 || area.bottom <= 0)
            return client;
        const auto box = Arrange(area.right, area.bottom, Logical.get_ptr()[0], Logical.get_ptr()[1], false).Target;
        const double scale = double(box.right - box.left) / (std::max)(1, int(Logical.get_ptr()[0]));
        return { LONG(std::lround((client.x - box.left) / scale)), LONG(std::lround((client.y - box.top) / scale)) };
    }

    inline POINT ToClient(POINT game)
    {
        RECT area{};
        if (!Window || !GetClientRect(Window, &area) || area.right <= 0 || area.bottom <= 0)
            return game;
        const auto box = Arrange(area.right, area.bottom, Logical.get_ptr()[0], Logical.get_ptr()[1], false).Target;
        const double scale = double(box.right - box.left) / (std::max)(1, int(Logical.get_ptr()[0]));
        return { LONG(box.left + std::lround(game.x * scale)), LONG(box.top + std::lround(game.y * scale)) };
    }

    inline void Fill(IDirectDrawSurface7* surface, const RECT& rect)
    {
        if (rect.right <= rect.left || rect.bottom <= rect.top)
            return;
        DDBLTFX effect{ sizeof(effect) };
        effect.dwFillColor = 0;
        RECT target = rect;
        surface->Blt(&target, nullptr, nullptr, DDBLT_COLORFILL | DDBLT_WAIT, &effect);
    }

    inline void Bars(IDirectDrawSurface7* surface, const RECT& area, const RECT& picture)
    {
        Fill(surface, { area.left, area.top, area.right, picture.top });
        Fill(surface, { area.left, picture.bottom, area.right, area.bottom });
        Fill(surface, { area.left, picture.top, picture.left, picture.bottom });
        Fill(surface, { picture.right, picture.top, area.right, picture.bottom });
    }

    inline SIZE SurfaceSize(IDirectDrawSurface7* surface)
    {
        DDSURFACEDESC2 desc{ sizeof(desc) };
        if (!surface || FAILED(surface->GetSurfaceDesc(&desc)))
            return { Logical.get_ptr()[0], Logical.get_ptr()[1] };
        return { LONG(desc.dwWidth), LONG(desc.dwHeight) };
    }

    // Windowed modes, and movies in every mode: the back buffer goes to the window's client area.
    inline HRESULT __cdecl WindowedPresent()
    {
        const auto primary = Primary.get();
        const auto back = Back.get();
        if (!primary || !back || !Window)
            return DD_OK;
        RECT client{};
        POINT origin{};
        if (!GetClientRect(Window, &client) || client.right <= 0 || client.bottom <= 0 || !ClientToScreen(Window, &origin))
            return DD_OK;
        if (OnFrame)
            OnFrame();
        const auto size = SurfaceSize(back);
        auto layout = Arrange(client.right, client.bottom, size.cx, size.cy, true);
        OffsetRect(&client, origin.x, origin.y);
        OffsetRect(&layout.Target, origin.x, origin.y);
        Bars(primary, client, layout.Target);
        const auto result = primary->Blt(&layout.Target, back, &layout.Source, DDBLT_WAIT, nullptr);
        if (result == DDERR_SURFACELOST)
            return Restore();
        return result;
    }

    // Exclusive fullscreen: a mode smaller than the display (menus) or a part of the picture (loading screen) is
    // scaled into the back buffer through a copy, then the game flips.
    inline HRESULT __cdecl Flip()
    {
        if (OnFrame)
            OnFrame();
        const auto back = Back.get();
        if (back)
        {
            const auto size = SurfaceSize(back);
            const auto layout = Arrange(size.cx, size.cy, size.cx, size.cy, true);
            const RECT whole{ 0, 0, size.cx, size.cy };
            if (!EqualRect(&layout.Source, &layout.Target))
            {
                const SIZE needed{ layout.Source.right - layout.Source.left, layout.Source.bottom - layout.Source.top };
                if (Copy && (CopySize.cx < needed.cx || CopySize.cy < needed.cy))
                {
                    Copy->Release();
                    Copy = nullptr;
                }
                if (!Copy && DirectDraw.get())
                {
                    DDSURFACEDESC2 desc{ sizeof(desc) };
                    desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
                    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
                    desc.dwWidth = needed.cx;
                    desc.dwHeight = needed.cy;
                    if (SUCCEEDED((DirectDraw.get())->CreateSurface(&desc, &Copy, nullptr)))
                        CopySize = needed;
                    else
                        Copy = nullptr;
                }
                if (Copy)
                {
                    if (Copy->IsLost() == DDERR_SURFACELOST)
                        Copy->Restore();
                    RECT copied{ 0, 0, needed.cx, needed.cy };
                    RECT source = layout.Source;
                    RECT target = layout.Target;
                    if (SUCCEEDED(Copy->Blt(&copied, back, &source, DDBLT_WAIT, nullptr)))
                    {
                        Bars(back, whole, target);
                        back->Blt(&target, Copy, &copied, DDBLT_WAIT, nullptr);
                    }
                }
            }
        }
        return FlipHook.ccall<HRESULT>();
    }

    inline void ReleaseCopy()
    {
        if (Copy)
            Copy->Release();
        Copy = nullptr;
        CopySize = {};
    }

    // The display stays at the chosen resolution; the menus' 800x600 mode is drawn into its top left corner.
    inline int __cdecl CreateFullscreen(HWND window, GUID* driver, void* device, int width, int height, int depth, int rate)
    {
        Scaled = false;
        const int displayWidth = (std::max)(width, Width), displayHeight = (std::max)(height, Height);
        const auto result = FullscreenHook.ccall<int>(window, driver, device, displayWidth, displayHeight, depth, rate);
        if (result && (width != displayWidth || height != displayHeight))
        {
            Logical.get_ptr()[0] = width;
            Logical.get_ptr()[1] = height;
            Corner.get_ptr()[0] = width;
            Corner.get_ptr()[1] = height;
            D3DVIEWPORT7 viewport{ 0, 0, DWORD(width), DWORD(height), 0.0f, 1.0f };
            (Device.get())->SetViewport(&viewport);
            Scaled = true;
        }
        return result;
    }

    inline void Place(HWND window)
    {
        WindowedModeWrapper::GameHWND = window;
        WindowedModeWrapper::bEnableWindowResize = false;
        if (Mode == 1)
        {
            WindowedModeWrapper::bBorderlessWindowed = false;
            WindowedModeWrapper::desiredClientWidth = Width;
            WindowedModeWrapper::desiredClientHeight = Height;
            WindowedModeWrapper::afterCreateWindow();
        }
        else
        {
            WindowedModeWrapper::bBorderlessWindowed = true;
            WindowedModeWrapper::afterCreateWindow();
            const auto monitor = WindowedModeWrapper::WindowMonitor().rcMonitor;
            SetWindowPos(window, nullptr, monitor.left, monitor.top, monitor.right - monitor.left, monitor.bottom - monitor.top,
                SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }

    inline HWND WINAPI CreateGameWindow(DWORD extended, LPCSTR className, LPCSTR title, DWORD style, int x, int y, int width,
        int height, HWND parent, HMENU menu, HINSTANCE instance, LPVOID parameter)
    {
        if (parent || (style & WS_CHILD))
            return CreateWindowExA(extended, className, title, style, x, y, width, height, parent, menu, instance, parameter);
        style = Mode == 1 ? WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN : WS_POPUP | WS_CLIPCHILDREN;
        const auto window = CreateWindowExA(extended, className, title, style, x, y, width, height, parent, menu, instance, parameter);
        if (window)
        {
            Window = window;
            Place(window);
            if (OnWindow)
                OnWindow(window);
        }
        return window;
    }

    // The game sizes its window to each mode; the window keeps the chosen size instead
    inline BOOL WINAPI MoveGameWindow(HWND window, int x, int y, int width, int height, BOOL repaint)
    {
        if (window && window == Window)
        {
            Place(window);
            return TRUE;
        }
        return MoveWindow(window, x, y, width, height, repaint);
    }

    // The mouse cursor follows DirectInput in game coordinates; these two sync it with Windows' cursor on focus changes
    inline BOOL WINAPI GameScreenToClient(HWND window, LPPOINT point)
    {
        const auto result = ScreenToClient(window, point);
        if (result && point && window == Window && Mode != 0)
            *point = ToGame(*point);
        return result;
    }

    inline BOOL WINAPI GameSetCursorPos(int x, int y)
    {
        POINT origin{};
        if (Window && Mode != 0 && ClientToScreen(Window, &origin))
        {
            const auto client = ToClient({ x - origin.x, y - origin.y });
            x = origin.x + client.x;
            y = origin.y + client.y;
        }
        return SetCursorPos(x, y);
    }

    inline void __cdecl PlayMovieFrame()
    {
        MovieFrame = true;
        MovieHook.ccall<void>();
        MovieFrame = false;
    }

    inline int __fastcall ShowInterface(void* manager, void*, const char* name)
    {
        Loading = name && std::strcmp(name, "LoadingScreen") == 0;
        return ShowHook.thiscall<int>(manager, name);
    }

    inline void __fastcall Pause(void* application, void*)
    {
        // In window modes the game can keep running while another window is active
        if (Mode != 0 && !PauseOnFocusLoss.load(std::memory_order_relaxed))
            return;
        PauseHook.thiscall<void>(application);
    }

    // d3dim700.dll refuses render targets wider or taller than 2048 pixels
    inline void RaiseRenderTargetLimit()
    {
        auto module = LoadLibraryExW(L"d3dim700.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!module)
            return;
        auto pattern = hook::module_pattern(module, "B8 00 08 00 00 39 45 ? 0F 87 ? ? ? ? 39 45 ? 0F 87"); // DIRECT3DDEVICEI::Init
        injector::WriteMemory<uint32_t>(pattern.get_first<uint32_t>(1), 16384, true);
        pattern = hook::module_pattern(module, "B8 00 08 00 00 39 85 ? ? ? ? 0F 87 ? ? ? ? 39 85 ? ? ? ? 0F 87"); // SetRenderTarget
        injector::WriteMemory<uint32_t>(pattern.get_first<uint32_t>(1), 16384, true);
    }

    inline void Init(int mode, int width, int height)
    {
        Mode = std::clamp(mode, 0, 2);
        Width = width;
        Height = height;
        const auto readMovies = []() { MovieMode.store(std::clamp(FixSettings().Int("MAIN.FMVWidescreenMode"), 0, 1)); };
        readMovies();
        FixSettings().OnChange("MAIN.FMVWidescreenMode", readMovies);
        const auto readPause = []() { PauseOnFocusLoss.store(FixSettings().Bool("MAIN.PauseOnFocusLoss")); };
        readPause();
        FixSettings().OnChange("MAIN.PauseOnFocusLoss", readPause);

        RaiseRenderTargetLimit();

        // lpDD, primary, back buffer, Direct3D, device, width, height getters
        auto getters = hook::pattern("A1 ? ? ? ? C3 A1 ? ? ? ? C3 A1 ? ? ? ? C3 A1 ? ? ? ? C3 A1 ? ? ? ? C3 A1 ? ? ? ? C3 A1 ? ? ? ? C3 33 C0");
        DirectDraw.SetAddress(*getters.get_first<IDirectDraw7**>(1));
        Primary.SetAddress(*getters.get_first<IDirectDrawSurface7**>(7));
        Back.SetAddress(*getters.get_first<IDirectDrawSurface7**>(13));
        Device.SetAddress(*getters.get_first<IDirect3DDevice7**>(25));
        Logical.SetAddress(*getters.get_first<int32_t*>(31));

        // Device creation: windowed (offscreen back buffer, Blt to the window) or exclusive (flipping chain)
        auto create = hook::pattern("55 8B EC E8 ? ? ? ? 80 7D 24 00 8B 45 08 A3 ? ? ? ? 74 21");
        const auto windowedPresent = *create.get_first<uintptr_t>(31);
        const auto flip = *create.get_first<uintptr_t>(64);
        const auto createFullscreen = injector::GetBranchDestination(create.get_first(84)).as_int();
        auto present = hook::pattern("A1 ? ? ? ? 8B 08 6A 00 68 00 00 00 01 6A 00 FF 35 ? ? ? ? 68 ? ? ? ? 50 FF 51 14 3D C2 01 76 88 75 05 E9");
        Corner.SetAddress(*present.get_first<int32_t*>(23) + 2);
        Restore = reinterpret_cast<decltype(Restore)>(injector::GetBranchDestination(present.get_first(38)).as_int());
        WindowedPresentHook = safetyhook::create_inline(reinterpret_cast<void*>(windowedPresent), WindowedPresent);
        FlipHook = safetyhook::create_inline(reinterpret_cast<void*>(flip), Flip);
        FullscreenHook = safetyhook::create_inline(reinterpret_cast<void*>(createFullscreen), CreateFullscreen);
        ReleaseHook = safetyhook::create_mid(injector::GetBranchDestination(create.get_first(3)).as_int(), [](SafetyHookContext&)
        {
            ReleaseCopy();
            Scaled = false;
        });

        // Mode switches between the menus (800x600) and missions: always ask for the mode, sized by the fix
        auto switcher = hook::pattern("B8 ? ? ? ? E8 ? ? ? ? 83 EC 20 8B 0D ? ? ? ? 53 56 57 E8 ? ? ? ? 83 C0 0C 50 8D 4D D4 E8 ? ? ? ? 33 DB 39 5D F0 89 5D FC 0F 85");
        SwitchHook = safetyhook::create_mid(switcher.get_first(), [](SafetyHookContext& context)
        {
            GameRequest = *reinterpret_cast<int32_t*>(context.esp + 4) == 1;
            MenuReady = MenuReady || !GameRequest;
        });
        injector::MakeNOP(switcher.get_first(47), 6);
        auto request = hook::pattern("8B 0D ? ? ? ? 8D 45 D4 53 50 E8 ? ? ? ? 8B C8 E8 ? ? ? ? 8B 0D");
        SwitchReturn = reinterpret_cast<uintptr_t>(request.get_first(23));
        const auto setMode = injector::GetBranchDestination(request.get_first(18)).as_int();
        ModeHook = safetyhook::create_mid(setMode, [](SafetyHookContext& context)
        {
            const auto mode = *reinterpret_cast<uint32_t**>(context.esp + 4);
            if (!mode[7])
                return;
            if (*reinterpret_cast<uintptr_t*>(context.esp) == SwitchReturn)
            {
                mode[3] = GameRequest ? Width : 800;
                mode[4] = GameRequest ? Height : 600;
            }
            mode[7] = Mode == 0 ? 2 : 1;
            if (Mode != 0)
                mode[6] = 0;
        });

        // Movies always use the 800x600 mode, also in a window (the window keeps its size); Fill picks the movie
        // out of it at present time
        auto movieMode = hook::pattern("83 F8 01 75 67 E8 ? ? ? ? 8B 40 04 8B 78 04");
        injector::WriteMemory<uint8_t>(movieMode.get_first(3), 0xEB, true);
        auto movieFrame = hook::pattern("E8 ? ? ? ? 8B C8 E8 ? ? ? ? E8 ? ? ? ? 50 E8 ? ? ? ? 8B C8 E8 ? ? ? ? E8 ? ? ? ?");
        Movie.SetAddress(*reinterpret_cast<uint8_t***>(injector::GetBranchDestination(movieFrame.get_first()).as_int() + 0x0C));
        MovieHook = safetyhook::create_inline(movieFrame.get_first(), PlayMovieFrame);

        auto show = hook::pattern("55 8B EC 56 57 8B F1 8B 06 6A 01 FF 50 7C 8B C8 E8");
        ShowHook = safetyhook::create_inline(show.get_first(), ShowInterface);

        auto pause = hook::pattern("C7 41 0C ? ? ? ? E9 ? ? ? ?");
        PauseHook = safetyhook::create_inline(pause.get_first(), Pause);

        IATHook::Replace(GetModuleHandleW(nullptr), "USER32.dll",
            std::forward_as_tuple("CreateWindowExA", CreateGameWindow),
            std::forward_as_tuple("MoveWindow", MoveGameWindow),
            std::forward_as_tuple("ScreenToClient", GameScreenToClient),
            std::forward_as_tuple("SetCursorPos", GameSetCursorPos));
    }
}

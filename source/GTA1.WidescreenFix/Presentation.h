#pragma once
#include <atomic>
#include <ddraw.h>

namespace GTA1Rotation { inline void KeyDown(WPARAM key); }
namespace GTA1Options
{
    inline bool KeyMessage(UINT message, WPARAM key, LPARAM flags);
    inline bool Showing();
    inline void BeforeFrame();
    inline void Frame(int input);
    inline void BeforePresent();
    inline bool OverlayMessage(UINT message, WPARAM key);
    inline bool OverlayActive();
    inline void GameplayPresent(uint8_t* pixels, int width, int height, int pitch, int bits);
    inline bool TakeQueuedEscape();
    inline bool TakeEscape();
}

namespace GTA1Presentation
{
    inline bool RenderingFrontend = false;
    inline bool ScaleFrontend = true;
    inline bool SoftwareBuffer = true;
    inline int* MenuState = nullptr;
    inline bool TextEntry() { return MenuState && *MenuState == 12; }
    inline SafetyHookInline MenuHook;
    inline int** Display = nullptr;
    inline int** Offscreen = nullptr;
    inline int* Width = nullptr;
    inline int* Height = nullptr;
    inline uint8_t** DrawPixels = nullptr;
    inline SafetyHookInline PresentHook;
    inline SafetyHookInline ModeHook;
    inline SafetyHookInline CopyHook;
    inline int (__cdecl* LockSurface)() = nullptr;
    inline int (__cdecl* UnlockSurface)() = nullptr;
    inline DDSURFACEDESC* LockedSurface = nullptr;
    inline int __cdecl CopyFrame(int* dc, int sx, int sy, int right, int bottom,
        int dx, int dy, int operation, uint8_t* source, int pitch, int* sourceDC)
    {
        if (!Display || !Width || !Height || !LockedSurface)   // the frontend signatures were not found
            return CopyHook.call<int>(dc, sx, sy, right, bottom, dx, dy, operation, source, pitch, sourceDC);
        int bytes = (dc[110] + 7) / 8;
        if (dc != *Display || sx || sy || dx || dy || operation || right != *Width || bottom != *Height ||
            !source || (bytes != 2 && bytes != 4) || pitch < right * bytes)
            return CopyHook.call<int>(dc, sx, sy, right, bottom, dx, dy, operation, source, pitch, sourceDC);
        auto status = LockSurface();
        if (status) return status;
        bool valid = LockedSurface->lpSurface && LockedSurface->lPitch >= right * bytes &&
            LockedSurface->dwWidth >= unsigned(right) && LockedSurface->dwHeight >= unsigned(bottom);
        if (valid)
            for (int row = 0; row < bottom; ++row)
                memmove(static_cast<uint8_t*>(LockedSurface->lpSurface) + size_t(row) * LockedSurface->lPitch,
                    source + size_t(row) * pitch, size_t(right) * bytes);
        auto result = UnlockSurface();
        if (!valid && !result)
            return CopyHook.call<int>(dc, sx, sy, right, bottom, dx, dy, operation, source, pitch, sourceDC);
        return result;
    }
    inline int __cdecl CreateMode(int* dc, int width, int height, int pages)
    {
        return ModeHook.call<int>(dc, width, height, SoftwareBuffer && pages > 0 ? 1 : pages);
    }
    inline std::vector<uint8_t> Canvas;
    inline int (__cdecl* OriginalReadKey)() = nullptr;
    inline std::atomic<int> PendingKey = 0;
    inline WNDPROC OriginalWndProc = nullptr;
    inline LRESULT CALLBACK WndProc(HWND window, UINT message, WPARAM key, LPARAM flags)
    {
        if (GTA1Options::KeyMessage(message, key, flags)) return 0;
        if (GTA1Window::KeyDown(message, key, flags)) return 0;
        if (GTA1Options::OverlayMessage(message, key)) return 0;
        InputDevices::KeyboardMessage(message, key);
        if (message == WM_KEYDOWN && !(flags & (1u << 30)) && !InputDevices::Frontend) GTA1Rotation::KeyDown(key);
        if (message == WM_KEYDOWN && !(flags & (1u << 30)) && !InputDevices::Frontend &&
            InputDevices::ModernControls && key == InputDevices::PauseKey) PendingKey = 64;
        if (message == WM_KEYDOWN && InputDevices::Frontend && InputDevices::ModernControls && !TextEntry())
        {
            switch (key)
            {
            case VK_UP: case 'W': PendingKey = 328; break;
            case VK_DOWN: case 'S': PendingKey = 336; break;
            case VK_LEFT: case 'A': PendingKey = 331; break;
            case VK_RIGHT: case 'D': PendingKey = 333; break;
            case VK_RETURN: PendingKey = 28; break;
            case VK_ESCAPE: PendingKey = 1; break;
            case VK_BACK: case VK_DELETE: PendingKey = 14; break;
            }
        }
        if (message == WM_KILLFOCUS) PendingKey = 0;
        // MGL lets the window be destroyed and keeps the game running without it.
        if (message == WM_CLOSE) ExitProcess(0);
        return CallWindowProcW(OriginalWndProc, window, message, key, flags);
    }
    inline void AttachWindow(HWND window)
    {
        OriginalWndProc = reinterpret_cast<WNDPROC>(GetWindowLongPtrW(window, GWLP_WNDPROC));
        SetWindowLongPtrW(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc));
    }

    inline int __cdecl DrawMenu(int action)
    {
        InputDevices::Frontend = true;
        InputDevices::BeginInputFrame();
        RenderingFrontend = true;
        GTA1Options::BeforeFrame();
        if (Offscreen && *Offscreen && DrawPixels)
            *DrawPixels = reinterpret_cast<uint8_t*>((*Offscreen)[104]);
        // The options screen is an extra state: the native handler only finishes the frame.
        // It consumes the input: a key that closes it must not reach the native screen it returns to.
        if (GTA1Options::Showing()) { GTA1Options::Frame(action); action = 0; }
        int result = MenuHook.call<int>(action);
        RenderingFrontend = false;
        if (result && result != 4) InputDevices::Frontend = false;
        return result;
    }

    inline int __cdecl Present()
    {
        int right = *Width, bottom = *Height;
        if (RenderingFrontend) GTA1Options::BeforePresent();
        else if (auto dc = *Offscreen ? *Offscreen : *Display)
            GTA1Options::GameplayPresent(reinterpret_cast<uint8_t*>(dc[104]), right, bottom, dc[114], dc[110]);
        static bool gameplayLogged = false;
        if (Log::Enabled && !InputDevices::Frontend && !gameplayLogged)
        {
            char diagnostic[160]{};
            sprintf_s(diagnostic, "GTA1 gameplay present: size=%dx%d frontendRender=%d pixels=%p", right, bottom,
                RenderingFrontend, *DrawPixels);
            Log::Write(diagnostic); gameplayLogged = true;
        }
        static bool logged = false;
        if (!logged)
        {
            char message[160]{};
            auto dc = *Offscreen ? *Offscreen : *Display;
            sprintf_s(message, "GTA1 frontend: %dx%d input=%d dc=%p bpp=%d pitch=%d", right, bottom,
                int(RenderingFrontend), dc, dc ? dc[110] : 0, dc ? dc[114] : 0);
            Log::Write(message); logged = true;
        }
        if (!ScaleFrontend || !RenderingFrontend || right < 640 || bottom < 480)
            return PresentHook.call<int>();
        auto destination = *Offscreen ? *Offscreen : *Display;
        if (!destination) return PresentHook.call<int>();
        // Frontend art has a fixed 640x480 layout. Scale it independently of the
        // gameplay framebuffer, keeping its original shape and centered position.
        double scale = std::min(right / 640.0, bottom / 480.0);
        int width = int(std::lround(640 * scale)), height = int(std::lround(480 * scale));
        int ox = (right - width) / 2, oy = (bottom - height) / 2;
        auto pixels = *DrawPixels;
        int pitch = destination[114], bytes = (destination[110] + 7) / 8;
        if (pixels && bytes >= 1 && bytes <= 4 && pitch >= right * bytes)
        {
            Canvas.resize(640 * 480 * bytes);
            for (int row = 0; row < 480; ++row)
                memcpy(Canvas.data() + row * 640 * bytes, pixels + row * pitch, 640 * bytes);
            for (int row = 0; row < bottom; ++row)
            {
                auto line = pixels + row * pitch;
                if (row < oy || row >= oy + height) memset(line, 0, right * bytes);
                else
                {
                    memset(line, 0, ox * bytes);
                    memset(line + (ox + width) * bytes, 0, (right - ox - width) * bytes);
                    auto source = Canvas.data() + ((row - oy) * 480 / height) * 640 * bytes;
                    for (int column = 0; column < width; ++column)
                        memcpy(line + (ox + column) * bytes, source + (column * 640 / width) * bytes, bytes);
                }
            }
        }
        return PresentHook.call<int>();
    }

    inline int __cdecl ReadKey()
    {
        int key = OriginalReadKey();
        if (GTA1Options::OverlayActive()) { PendingKey = 0; return 0; }   // the options have the keyboard
        using namespace InputDevices;
        Poll();
        int queued = PendingKey.exchange(0);
        if (Frontend && Focused() && queued) key = queued;
        if (!Frontend && Focused() && queued == 64 && GTA1Input::Available()) key = 64;
        // Esc opens the options menu; Quit Game there passes one to the native quit prompt.
        if (!Frontend && key == 1 && GTA1Options::TakeEscape()) key = 0;
        if (!Frontend && !key && !queued && GTA1Options::TakeQueuedEscape()) key = 1;
        if (Frontend && ModernControls && !TextEntry())
        {
            switch (key)
            {
            case 17: key = 328; break; // W
            case 31: key = 336; break; // S
            case 30: key = 331; break; // A
            case 32: key = 333; break; // D
            }
        }
        else if (!Frontend && ModernControls && GTA1Input::Available() && !GTA1Input::IsPaused())
        {
            int scan = key >= 128 && key < 256 ? key - 128 : key;
            switch (scan)
            {
            case 17: case 30: case 31: case 32: case 33: case 16: case 18: case 57: case 42: case 54:
                key = 0; break;
            }
        }
        static bool held[6]{};
        static DWORD repeat[4]{};
        auto [x, y] = Stick();
        bool down[] = { y < -0.35f || Button(XINPUT_GAMEPAD_DPAD_UP),
            y > 0.35f || Button(XINPUT_GAMEPAD_DPAD_DOWN),
            x < -0.35f || Button(XINPUT_GAMEPAD_DPAD_LEFT),
            x > 0.35f || Button(XINPUT_GAMEPAD_DPAD_RIGHT),
            Button(XINPUT_GAMEPAD_A), Button(XINPUT_GAMEPAD_B) || Button(XINPUT_GAMEPAD_START) };
        constexpr int scans[] = {328, 336, 331, 333, 28, 1};
        DWORD now = GetTickCount();
        for (int i = 0; i < 6; ++i)
        {
            bool pressed = down[i] && !held[i];
            if (i < 4)
            {
                if (pressed) repeat[i] = now + 400;
                else if (down[i] && static_cast<int32_t>(now - repeat[i]) >= 0)
                { pressed = true; repeat[i] = now + 120; }
            }
            if (!key && pressed && Frontend) key = scans[i];
            else if (!key && pressed && i == 5 && Button(XINPUT_GAMEPAD_START) && GTA1Input::Available()) key = 64;
            held[i] = down[i];
        }
        return key;
    }

    inline void Install()
    {
        CIniReader ini("");
        SoftwareBuffer = ini.ReadBoolean("MAIN", "SoftwareBuffer", true);
        auto mode = hook::pattern("8B 44 24 08 81 EC 70 05 00 00 83 F8 FF 90 53 56 57 55 74 34");
        if (mode.size() == 1) ModeHook = safetyhook::create_inline(mode.get_first(), CreateMode);
        auto copy = hook::pattern("55 8B EC E8 ? ? ? ? FF 75 30 FF 75 2C FF 75 28 FF 75 24 FF 75 20 FF 75 1C FF 75 18 FF 75 14 FF 75 10 FF 75 0C FF 75 08 FF 15 ? ? ? ? 83 C4 2C E8");
        if (copy.size() == 1)
        {
            auto entry = copy.get_first<uint8_t>();
            void* lockAddress = injector::GetBranchDestination(entry + 3).get();
            void* unlockAddress = injector::GetBranchDestination(entry + 0x32).get();
            LockSurface = reinterpret_cast<decltype(LockSurface)>(lockAddress);
            UnlockSurface = reinterpret_cast<decltype(UnlockSurface)>(unlockAddress);
            auto lock = reinterpret_cast<uint8_t*>(LockSurface);
            if (lock[0] == 0x53 && lock[10] == 0x68 && lock[20] == 0xFF && lock[22] == 0x64)
            {
                LockedSurface = *reinterpret_cast<DDSURFACEDESC**>(lock + 11);
                CopyHook = safetyhook::create_inline(entry, CopyFrame);
            }
        }
        using GTA1Build::Find, GTA1Build::Off;
        // The frontend state machine, the page presenter and the event reader.
        auto menu = Find("51 A1 ? ? ? ? 53 55 33 ED 56 83 F8 14 57 89 2D",
            "53 A1 ? ? ? ? 56 83 F8 14 C7 05 ? ? ? ? 00 00 00 00 55 0F 87", "A1 ? ? ? ? C7 05 ? ? ? ? 00 00 00 00 83 F8 14 0F 87");
        auto present = Find("A1 ? ? ? ? 85 C0 74 27 8B 0D ? ? ? ? 8B 15 ? ? ? ? 6A 00 6A 00 6A 00 51 52",
            "56 A1 ? ? ? ? 85 C0 74 2C 6A 00 A1 ? ? ? ? 6A 00 8B 0D ? ? ? ? 6A 00 8B 15 ? ? ? ? 50 51 6A 00 A1",
            "A1 ? ? ? ? 85 C0 74 27 8B 0D ? ? ? ? 8B 15 ? ? ? ? 6A 00 6A 00 6A 00 51 52");
        auto key = Find("83 EC 24 8D 44 24 00 68 FF FF 00 00 50 E8",
            "83 EC 24 53 56 57 BE 01 00 00 00 BF 04 00 00 00 33 DB 8D 44 24 0C 68 FF FF 00 00 50 E8",
            "83 EC 24 53 56 8D 44 24 08 68 FF FF 00 00 50 E8");
        if (menu.size() != 1 || present.size() != 1 || key.size() != 1)
        { Log::Write("GTA1 frontend signatures unavailable."); return; }
        MenuState = *menu.get_first<int*>(Off(2, 2, 1));
        MenuHook = safetyhook::create_inline(menu.get_first(), DrawMenu);
        Offscreen = *present.get_first<int**>(Off(1, 2, 1));
        Height = *present.get_first<int*>(Off(11, 13, 11));
        Width = *present.get_first<int*>(Off(17, 21, 17));
        Display = *present.get_first<int**>(Off(35, 38, 35));
        auto drawPixels = Find("8B 96 A0 01 00 00 83 C4 30 89 15 ? ? ? ? 5E C3", "8B 8E A0 01 00 00 89 0D ? ? ? ? 5E C3",
            "8B 96 A0 01 00 00 83 C4 0C 89 15 ? ? ? ? 5E C3");
        if (drawPixels.size() != 1) { Log::Write("GTA1 draw-page signature unavailable."); return; }
        DrawPixels = *drawPixels.get_first<uint8_t**>(Off(11, 8, 11));
        PresentHook = safetyhook::create_inline(present.get_first(), Present);
        Log::Write(PresentHook ? "GTA1 presentation hook installed." : "GTA1 presentation hook FAILED.");
        OriginalReadKey = reinterpret_cast<decltype(OriginalReadKey)>(key.get_first());
        // The event reader loops back into its first few instructions. Patch its
        // callers so those internal jumps continue to target the original code.
        auto calls = hook::pattern("E8 ? ? ? ?");
        for (size_t i = 0; i < calls.size(); ++i)
        {
            auto site = calls.get(i).get<void>();
            void* target = injector::GetBranchDestination(site).get();
            if (target == reinterpret_cast<void*>(OriginalReadKey)) injector::MakeCALL(site, ReadKey);
        }
        // London calls it only through its jump stub.
        if (GTA1Build::Current == GTA1Build::Kind::London)
        {
            auto jumps = hook::pattern("E9 ? ? ? ?");
            for (size_t i = 0; i < jumps.size(); ++i)
            {
                auto site = jumps.get(i).get<void>();
                auto stubs = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)) + 0x5000;   // the stub table follows the headers
                if (reinterpret_cast<uintptr_t>(site) < stubs && injector::GetBranchDestination(site).get() == reinterpret_cast<void*>(OriginalReadKey))
                    injector::MakeJMP(site, ReadKey);
            }
        }
    }
}

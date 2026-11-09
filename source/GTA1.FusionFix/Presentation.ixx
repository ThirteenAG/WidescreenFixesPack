module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/CursorConfinement.hpp"
#include "Shared/GTAClassic/InputDevices.hpp"
#include <atomic>
#include <ddraw.h>

export module Presentation;

import ComVars;
import Window;
import Prompts;
import Options;
import Timing;
import Log;

export namespace GTA1Presentation
{
    bool RenderingFrontend = false;
    bool ScaleFrontend = true;
    SafetyHookInline MenuHook;
    SafetyHookInline PresentHook;
    SafetyHookInline ModeHook;
    SafetyHookInline CopyHook;
    int (__cdecl* LockSurface)() = nullptr;
    int (__cdecl* UnlockSurface)() = nullptr;
    int __cdecl CopyFrame(int* dc, int sx, int sy, int right, int bottom,
        int dx, int dy, int operation, uint8_t* source, int pitch, int* sourceDC)
    {
        int bytes = (dc[110] + 7) / 8;
        if (dc != pDisplayDC || sx || sy || dx || dy || operation || right != nDisplayWidth || bottom != nDisplayHeight ||
            !source || (bytes != 2 && bytes != 4) || pitch < right * bytes)
            return CopyHook.call<int>(dc, sx, sy, right, bottom, dx, dy, operation, source, pitch, sourceDC);
        auto status = LockSurface();
        if (status) return status;
        bool valid = sLockedSurface->lpSurface && sLockedSurface->lPitch >= right * bytes &&
            sLockedSurface->dwWidth >= unsigned(right) && sLockedSurface->dwHeight >= unsigned(bottom);
        if (valid)
            for (int row = 0; row < bottom; ++row)
                memmove(static_cast<uint8_t*>(sLockedSurface->lpSurface) + size_t(row) * sLockedSurface->lPitch,
                    source + size_t(row) * pitch, size_t(right) * bytes);
        auto result = UnlockSurface();
        if (!valid && !result)
            return CopyHook.call<int>(dc, sx, sy, right, bottom, dx, dy, operation, source, pitch, sourceDC);
        return result;
    }
    int __cdecl CreateMode(int* dc, int width, int height, int pages)
    {
        // One page: the game draws into a software buffer that the copy hook presents.
        return ModeHook.call<int>(dc, width, height, pages > 0 ? 1 : pages);
    }
    std::vector<uint8_t> Canvas;
    int (__cdecl* OriginalReadKey)() = nullptr;
    WNDPROC OriginalWndProc = nullptr;
    std::atomic<bool> PauseOnFocusPending = false;
    LRESULT CALLBACK WndProc(HWND window, UINT message, WPARAM key, LPARAM flags)
    {
        if (GTA1Options::KeyMessage(message, key, flags)) return 0;
        if (GTA1Window::Paint(window, message)) return message == WM_ERASEBKGND ? 1 : 0;
        if (GTA1Window::KeyDown(message, key, flags)) return 0;
        GTA1Window::Activate(message, key);
        if (GTA1Options::OverlayMessage(message, key)) return 0;
        InputDevices::KeyboardMessage(message, key);
        if (message == WM_KEYDOWN && !(flags & (1u << 30)) && !InputDevices::Frontend) GTA1Rotation::KeyDown(key);
        if (message == WM_KEYDOWN && !(flags & (1u << 30)) && !InputDevices::Frontend &&
            InputDevices::ModernControls && key == InputDevices::PauseKey) PendingKey = 64;
        if (message == WM_KEYDOWN && QuitPromptActive())
        {
            if (key == VK_RETURN) PendingKey = 28;
            else if (key == VK_ESCAPE) PendingKey = 1;
        }
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
        if (message == WM_KILLFOCUS)
        {
            PendingKey = 0;
            if (!InputDevices::Frontend && !GTA1Options::OverlayActive() && Available() && !QuitPromptActive() && !IsPaused()
                && FixSettings().Bool("MAIN.PauseOnFocusLoss"))
                PauseOnFocusPending = true;
        }
        // MGL lets the window be destroyed and keeps the game running without it.
        if (message == WM_CLOSE) ExitProcess(0);
        return CallWindowProcW(OriginalWndProc, window, message, key, flags);
    }
    void AttachWindow(HWND window)
    {
        OriginalWndProc = reinterpret_cast<WNDPROC>(GetWindowLongPtrW(window, GWLP_WNDPROC));
        SetWindowLongPtrW(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc));
    }

    int __cdecl DrawMenu(int action)
    {
        if (GTA1Options::OverlayActive()) GTA1Options::ResetPause();
        InputDevices::Frontend = true;
        InputDevices::BeginInputFrame();
        InputDevices::UpdateRumble(true);
        CursorConfinement::Frame(InputDevices::Window);
        GTA1Prompts::Frame();
        RenderingFrontend = true;
        GTA1Options::BeforeFrame();
        if (pOffscreenDC != nullptr)
            pDrawPixels = reinterpret_cast<uint8_t*>(pOffscreenDC.get()[104]);
        // The options screen is an extra state: the native handler only finishes the frame.
        // It consumes the input: a key that closes it must not reach the native screen it returns to.
        if (GTA1Options::Showing()) { GTA1Options::Frame(action); action = 0; }
        int result = MenuHook.call<int>(action);
        RenderingFrontend = false;
        if (result && result != 4) InputDevices::Frontend = false;
        return result;
    }

    int __cdecl Present()
    {
        int right = nDisplayWidth, bottom = nDisplayHeight;
        if (RenderingFrontend) GTA1Options::BeforePresent();
        else if (int* dc = pOffscreenDC ? pOffscreenDC : pDisplayDC)
            GTA1Options::GameplayPresent(reinterpret_cast<uint8_t*>(dc[104]), right, bottom, dc[114], dc[110]);
        static bool gameplayLogged = false;
        if (Log::Enabled && !InputDevices::Frontend && !gameplayLogged)
        {
            char diagnostic[160]{};
            sprintf_s(diagnostic, "GTA1 gameplay present: size=%dx%d frontendRender=%d pixels=%p", right, bottom,
                RenderingFrontend, pDrawPixels.get());
            Log::Write(diagnostic); gameplayLogged = true;
        }
        static bool logged = false;
        if (!logged)
        {
            char message[160]{};
            int* dc = pOffscreenDC ? pOffscreenDC : pDisplayDC;
            sprintf_s(message, "GTA1 frontend: %dx%d input=%d dc=%p bpp=%d pitch=%d", right, bottom,
                int(RenderingFrontend), dc, dc ? dc[110] : 0, dc ? dc[114] : 0);
            Log::Write(message); logged = true;
        }
        if (!RenderingFrontend) GTA1Timing::BeforePresent();
        if (!ScaleFrontend || !RenderingFrontend || right < 640 || bottom < 480)
            return PresentHook.call<int>();
        int* destination = pOffscreenDC ? pOffscreenDC : pDisplayDC;
        if (!destination) return PresentHook.call<int>();
        // Frontend art has a fixed 640x480 layout. Scale it independently of the
        // gameplay framebuffer, keeping its original shape and centered position.
        double scale = std::min(right / 640.0, bottom / 480.0);
        int width = int(std::lround(640 * scale)), height = int(std::lround(480 * scale));
        int ox = (right - width) / 2, oy = (bottom - height) / 2;
        uint8_t* pixels = pDrawPixels;
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

    int __cdecl ReadKey()
    {
        int key = OriginalReadKey();
        if (GTA1Options::OverlayActive()) { PendingKey = 0; return 0; }   // the options have the keyboard
        using namespace InputDevices;
        Poll();
        int queued = PendingKey.exchange(0);
        const bool quitPrompt = QuitPromptActive();
        const bool suppressPadPause = GTA1Options::SuppressNativePadPause();
        if (Frontend && Focused() && queued) key = queued;
        if (quitPrompt && Focused() && (queued == 28 || queued == 1)) key = queued;
        if (!Frontend && Focused() && queued == 64 && Available()) key = 64;
        if (PauseOnFocusPending.exchange(false) && !Frontend && Available() && !QuitPromptActive() && !IsPaused()
            && FixSettings().Bool("MAIN.PauseOnFocusLoss"))
            key = 64;
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
        else if (!Frontend && ModernControls && Available() && !IsPaused())
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
            Button(XINPUT_GAMEPAD_A), Button(XINPUT_GAMEPAD_B) || Button(XINPUT_GAMEPAD_START) || (quitPrompt && Button(XINPUT_GAMEPAD_BACK)) };
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
            if (!key && pressed && (Frontend || (quitPrompt && i >= 4))) key = scans[i];
            else if (!key && pressed && i == 5 && Button(XINPUT_GAMEPAD_START) && !suppressPadPause && Available()) key = 64;
            held[i] = down[i];
        }
        return key;
    }

    void Install()
    {
        auto mode = hook::pattern("8B 44 24 08 81 EC 70 05 00 00 83 F8 FF 90 53 56 57 55 74 34");
        ModeHook = safetyhook::create_inline(mode.get_first(), CreateMode);
        auto copy = hook::pattern("55 8B EC E8 ? ? ? ? FF 75 30 FF 75 2C FF 75 28 FF 75 24 FF 75 20 FF 75 1C FF 75 18 FF 75 14 FF 75 10 FF 75 0C FF 75 08 FF 15 ? ? ? ? 83 C4 2C E8");
        {
            auto entry = copy.get_first<uint8_t>();
            void* lockAddress = injector::GetBranchDestination(entry + 3).get();
            void* unlockAddress = injector::GetBranchDestination(entry + 0x32).get();
            LockSurface = reinterpret_cast<decltype(LockSurface)>(lockAddress);
            UnlockSurface = reinterpret_cast<decltype(UnlockSurface)>(unlockAddress);
            auto lock = reinterpret_cast<uint8_t*>(LockSurface);
            const auto lockAt = reinterpret_cast<uintptr_t>(lock);
            hook::range_pattern(lockAt, lockAt + 23, "53 ? ? ? ? ? ? ? ? ? 68 ? ? ? ? ? ? ? ? ? FF ? 64").get_first();
            {
                sLockedSurface.SetAddress(*reinterpret_cast<DDSURFACEDESC**>(lock + 11));
                CopyHook = safetyhook::create_inline(entry, CopyFrame);
            }
        }
        // The event reader.
        auto key = GTA1Build::Find("83 EC 24 8D 44 24 00 68 FF FF 00 00 50 E8",
            "83 EC 24 53 56 57 BE 01 00 00 00 BF 04 00 00 00 33 DB 8D 44 24 0C 68 FF FF 00 00 50 E8",
            "83 EC 24 53 56 8D 44 24 08 68 FF FF 00 00 50 E8");

        MenuHook = safetyhook::create_inline(MenuFrame, DrawMenu);
        PresentHook = safetyhook::create_inline(PresentPage, Present);
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

    // The game's window, once it exists: its messages and the input devices' window
    HWND* PendingWindow = nullptr;
    HHOOK WindowMessages = nullptr;

    void AttachGameWindow(HWND window)
    {
        InputDevices::Window = window;
        AttachWindow(window);
    }

    LRESULT CALLBACK WindowMessage(int code, WPARAM wParam, LPARAM lParam)
    {
        if (code >= 0 && PendingWindow && *PendingWindow)
        {
            const auto message = reinterpret_cast<const MSG*>(lParam);
            if (message->hwnd == *PendingWindow)
            {
                const auto window = *PendingWindow;
                PendingWindow = nullptr;
                UnhookWindowsHookEx(WindowMessages);
                WindowMessages = nullptr;
                AttachGameWindow(window);
            }
        }
        return CallNextHookEx(nullptr, code, wParam, lParam);
    }

    void WatchWindow(HWND* window)
    {
        if (!window)
            return;
        if (*window)
        {
            AttachGameWindow(*window);
            return;
        }
        PendingWindow = window;
        WindowMessages = SetWindowsHookExW(WH_GETMESSAGE, WindowMessage, nullptr, GetCurrentThreadId());
        if (!WindowMessages)
            PendingWindow = nullptr;
    }
}

class Presentation
{
public:
    Presentation()
    {
        WFP::onShutdownEvent() += []()
        {
            if (GTA1Presentation::WindowMessages)
            {
                UnhookWindowsHookEx(GTA1Presentation::WindowMessages);
                GTA1Presentation::WindowMessages = nullptr;
                GTA1Presentation::PendingWindow = nullptr;
            }
        };
    }
} Presentation;

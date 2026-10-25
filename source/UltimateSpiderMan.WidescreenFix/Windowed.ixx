module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include "Display.hpp"

export module Windowed;

import Screen;

namespace
{
    injector::hook_back<HWND(__cdecl*)(LPCSTR, LPCSTR, int, int, LONG, LONG, WNDPROC, HINSTANCE, int, DWORD)> hbCreateWindow;
    SafetyHookMid ResetHook, ReleaseHook, DestroyHook, TextureHook;
    uintptr_t ResourceRestoreEntry = 0;

    __declspec(naked) void RestoreResources()
    {
        __asm
        {
            push ebx
            push esi
            xor ebx, ebx
            mov edx, ResourceRestoreEntry
            xor eax, eax
            jmp edx
        }
    }

    void UpdateResolution()
    {
        auto width = *USMWindow::Dimensions[0];
        auto height = *USMWindow::Dimensions[1];
        if (USMWindow::Mode == 2 || width < 640 || height < 480 || width > 16384 || height > 16384)
        {
            const auto desktop = USMWindow::DesktopSize();
            width = std::max(desktop.cx, 640L);
            height = std::max(desktop.cy, 480L);
            ProtectedGameRef<int32_t>{ USMWindow::Dimensions[0] } = width;
            ProtectedGameRef<int32_t>{ USMWindow::Dimensions[1] } = height;
        }
        Screen.Update(width, height);
    }

    BOOL WINAPI PositionWindow(HWND window, HWND after, int x, int y, int width, int height, UINT flags)
    {
        if (!USMWindow::Mode || window != USMWindow::Window)
            return SetWindowPos(window, after, x, y, width, height, flags);
        USMWindow::Place(*USMWindow::Dimensions[0], *USMWindow::Dimensions[1]);
        return TRUE;
    }

    HWND __cdecl CreateGameWindow(LPCSTR className, LPCSTR title, int x, int y, LONG width, LONG height,
        WNDPROC wndProc, HINSTANCE instance, int icon, DWORD flags)
    {
        const auto window = hbCreateWindow.fun(className, title, USMWindow::Mode ? -1 : x, USMWindow::Mode ? -1 : y,
            width, height, wndProc, instance, icon, USMWindow::Mode ? 0 : flags);
        if (window)
        {
            USMWindow::Attach(window);
            if (USMWindow::Mode)
                USMWindow::Place(width, height);
        }
        return window;
    }
}

class Windowed
{
public:
    Windowed()
    {
        WFP::onInitEvent() += []()
        {
            WindowedModeWrapper::EnableDpiAwareness();
            auto& settings = FixSettings();
            auto width = hook::pattern("A3 ? ? ? ? FF D6 50");
            auto height = hook::pattern("DB 05 ? ? ? ? 83 C4 18 6A 00 D8 0D");
            auto windowed = hook::pattern("C7 05 ? ? ? ? 00 00 00 00 FF 15 ? ? ? ? A1 ? ? ? ? 8B 0D ? ? ? ? 6A 04");
            auto create = hook::pattern("E8 ? ? ? ? 8B 0D ? ? ? ? 83 C4 28 6A 03 51");
            auto position = hook::pattern("FF 15 ? ? ? ? 8B 3D ? ? ? ? 57 E8 ? ? ? ? 83 C4 04 6A 00");
            auto rendererPosition = hook::pattern("FF 15 ? ? ? ? EB 3A 68 ? ? ? ? 50");
            auto dimensions = hook::pattern("A1 ? ? ? ? 8B 0D ? ? ? ? A3 ? ? ? ? A3 ? ? ? ? A1 ? ? ? ? 33 D2 85 C0 6A 29 0F 95 C2 6A 03 6A 00 6A 16 89 0D ? ? ? ? 89 0D ? ? ? ? 8B 0D");
            auto releaseTextures = hook::pattern("56 8B F1 8B 46 0C F6 C4 20 74 ? 5E E9");
            auto destroyBuffer = hook::pattern("56 8B F1 8B 46 08 8B 40 08 8B 08 50 FF 51 08 8B 56 08 C7 42 08 00 00 00 00 5E C3");
            auto releaseBuffers = hook::pattern("56 8B 35 ? ? ? ? 85 F6 74 ? EB ? 8D 49 00 8B 46 08 8B 40 08 8B 08 50 FF 51 08 8B 56 08");
            auto modeReset = hook::pattern("89 35 ? ? ? ? E8 ? ? ? ? 85 F6 8B 1D ? ? ? ? 74");
            if (width.size() != 1 || height.size() != 1 || windowed.size() != 1 || create.size() != 1 || position.size() != 1 ||
                rendererPosition.size() != 1 || dimensions.size() != 1 || modeReset.size() != 1 || releaseBuffers.size() != 1 || releaseTextures.size() != 1 || destroyBuffer.size() != 1)
            {
                OutputDebugStringA("Ultimate Spider-Man: display mode patterns unavailable.\n");
                return;
            }
            const auto resetCall = reinterpret_cast<uintptr_t>(modeReset.get_first(6));
            const auto reset = resetCall + 5 + *reinterpret_cast<const int32_t*>(resetCall + 1);
            auto resetDevice = hook::range_pattern(reset, reset + 0x200, "A1 ? ? ? ? 8B 10 68 ? ? ? ? 50 FF 52 40 A1");
            auto resetReturn = hook::range_pattern(reset, reset + 0x200, "83 C4 08 5E 5B C3");
            auto query = hook::range_pattern(reset, reset + 0x200, "A1 ? ? ? ? 3B C3 74 ? 8B 08 50 FF 51 08 A1");
            const auto texture = reinterpret_cast<uintptr_t>(releaseTextures.get_first());
            auto textureReturn = hook::range_pattern(texture, texture + 0x100, "5F C7 46 40 00 00 00 00 5E C3");
            if (resetDevice.size() != 1 || resetReturn.size() != 1 || query.size() != 1 || textureReturn.empty() ||
                *resetDevice.get_first<D3DPRESENT_PARAMETERS*>(8) != *dimensions.get_first<D3DPRESENT_PARAMETERS*>(17) ||
                *windowed.get_first<uint32_t*>(2) != *dimensions.get_first<uint32_t*>(22))
            {
                OutputDebugStringA("Ultimate Spider-Man: display reset contract unavailable.\n");
                return;
            }
            USMWindow::Dimensions = { *width.get_first<int32_t*>(1), *height.get_first<int32_t*>(2),
                *dimensions.get_first<int32_t*>(1), *dimensions.get_first<int32_t*>(7),
                *dimensions.get_first<int32_t*>(12), *dimensions.get_first<int32_t*>(43) };
            USMWindow::NativeWindowed = *windowed.get_first<uint32_t*>(2);
            USMWindow::Parameters = *resetDevice.get_first<D3DPRESENT_PARAMETERS*>(8);
            USMWindow::Device = *resetDevice.get_first<IDirect3DDevice9**>(1);
            USMWindow::NativeReset = reinterpret_cast<void(__cdecl*)()>(reset);
            USMWindow::FailedResetReturn = reinterpret_cast<uintptr_t>(resetReturn.get_first(3));
            ResourceRestoreEntry = reinterpret_cast<uintptr_t>(resetDevice.get_first(16));
            USMWindow::RestoreResources = RestoreResources;
            USMWindow::Query = *query.get_first<void**>(1);
            USMWindow::TextureReleaseReturn = reinterpret_cast<uintptr_t>(textureReturn.get_first(9));
            USMWindow::ReleaseResume = reinterpret_cast<uintptr_t>(releaseBuffers.get_first(28));
            USMWindow::DestroyResume = reinterpret_cast<uintptr_t>(destroyBuffer.get_first(15));
            ResetHook = safetyhook::create_mid(resetDevice.get_first(16), USMWindow::CaptureReset);
            ReleaseHook = safetyhook::create_mid(releaseBuffers.get_first(22), USMWindow::GuardBufferRelease);
            DestroyHook = safetyhook::create_mid(destroyBuffer.get_first(9), USMWindow::GuardBufferDestroy);
            TextureHook = safetyhook::create_mid(releaseTextures.get_first(), USMWindow::GuardTextureRelease);
            if (!ResetHook || !ReleaseHook || !DestroyHook || !TextureHook)
            {
                ResetHook.reset();
                ReleaseHook.reset();
                DestroyHook.reset();
                TextureHook.reset();
                OutputDebugStringA("Ultimate Spider-Man: display reset hook unavailable.\n");
                return;
            }
            USMWindow::Mode = std::clamp(settings.Int("MAIN.WindowMode"), 0, 2);
            USMWindow::LastWindowMode = USMWindow::Mode ? USMWindow::Mode : 1;
            USMWindow::RememberDesktop();
            USMWindow::ResolutionChanged = [](int width, int height)
            {
                Screen.Update(width, height);
            };
            USMWindow::ModeChanged = [](int mode)
            {
                FixSettings().Set("MAIN.WindowMode", std::to_string(mode));
            };
            settings.OnChange("MAIN.WindowMode", []()
            {
                USMWindow::Pending.store(std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2));
            });
            static auto resolutionHook = safetyhook::create_mid(height.get_first(), [](SafetyHookContext&)
            {
                UpdateResolution();
            });
            if (!resolutionHook)
            {
                ResetHook.reset();
                ReleaseHook.reset();
                DestroyHook.reset();
                TextureHook.reset();
                OutputDebugStringA("Ultimate Spider-Man: display resolution hook unavailable.\n");
                return;
            }
            WriteGameValue<uint32_t>(windowed.get_first(6), USMWindow::Mode ? 1 : 0);
            hbCreateWindow.fun = injector::MakeCALL(create.get_first(), CreateGameWindow, true).get();
            if (USMWindow::Mode)
                WriteGameValue<uint8_t>(create.get_first(15), SW_SHOWNORMAL);
            for (const auto site : { position.get_first(), rendererPosition.get_first() })
            {
                injector::MakeNOP(site, 6, true);
                injector::MakeCALL(site, PositionWindow, true);
            }
            WFP::onGameProcessEvent() += USMWindow::Frame;
        };
        WFP::onShutdownEvent() += []()
        {
            USMWindow::Shutdown();
            ResetHook.reset();
            ReleaseHook.reset();
            DestroyHook.reset();
            TextureHook.reset();
        };
    }
} Windowed;

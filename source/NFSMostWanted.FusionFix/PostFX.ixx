module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <d3d9.h>
#include "overlay/OverlayD3D9.hpp"

#pragma comment(lib, "version.lib")

export module PostFX;

import ComVars;
import PostFXCore;
import AntiAliasing;

namespace
{
    asi_overlay::D3D9Overlay Overlay;

    void ApplySMAA(bool enabled)
    {
        if (CPostFX::bSmaaEnabled != enabled)
        {
            // Recreate the render targets for the new SMAA mode.
            CPostFX::Shutdown();
            CPostFX::bSmaaEnabled = enabled;
        }
    }

    HMODULE ThisModule()
    {
        HMODULE module = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&ThisModule), &module);
        return module;
    }

    std::string BuildTime()
    {
        wchar_t path[MAX_PATH]{};
        GetModuleFileNameW(ThisModule(), path, DWORD(std::size(path)));
        DWORD ignored = 0;
        auto size = GetFileVersionInfoSizeW(path, &ignored);
        std::vector<uint8_t> data(size);
        if (!size || !GetFileVersionInfoW(path, 0, size, data.data()))
            return "Unknown";
        wchar_t* value = nullptr;
        UINT length = 0;
        if (!VerQueryValueW(data.data(), L"\\StringFileInfo\\040004b0\\BuildTimeUtc", reinterpret_cast<void**>(&value), &length) || !value)
            return "Unknown";
        return asi_settings::text::Narrow(value);
    }
}

// The developer overlay: its window messages and whether it takes the game's input
export namespace DeveloperOverlay
{
    bool Message(HWND window, UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result)
    {
        return Overlay.Message(window, message, wParam, lParam, result);
    }

    bool BlocksInput()
    {
        return Overlay.BlocksInput();
    }

    void MouseButton(bool down)
    {
        Overlay.MouseButton(0, down);
    }

    void Gamepad(uint16_t buttons, int8_t x, int8_t y)
    {
        Overlay.Gamepad(buttons, x, y);
    }
}

class PostFX
{
public:
    PostFX()
    {
        WFP::onInitEventAsync() += []()
        {
            asi_overlay::Overlay::AddPage("Debug", "Info", [](asi_overlay::OverlayUI& ui)
            {
                static const auto buildTime = BuildTime();
                ui.Value("Version", rsc_ProductVersion);
                ui.Value("BuildTimeUtc", buildTime.c_str());
                IDirect3DSurface9* backbuffer = nullptr;
                if (auto device = Direct3DDevice.get(); device && SUCCEEDED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer)))
                {
                    D3DSURFACE_DESC desc{};
                    backbuffer->GetDesc(&desc);
                    backbuffer->Release();
                    ui.Value("Resolution", std::to_string(desc.Width) + " x " + std::to_string(desc.Height));
                }
            });

            static const int* pConsoleGamma = &FixSettings().Int("GRAPHICS.ConsoleGamma");
            CPostFX::bConsoleGammaEnabled = *pConsoleGamma != 0;
            CPostFX::bSmaaEnabled = AntiAliasing::IsSMAA();
            CPostFX::bRenderToBackBuffer = true;

            static auto SMAAHook = safetyhook::create_mid(SMAAHookSite, +[](SafetyHookContext& regs)
            {
                ApplySMAA(AntiAliasing::IsSMAA());
                CPostFX::RenderSMAA(Direct3DDevice);
            });

            WFP::onEndScene() += []()
            {
                const auto gamma = *pConsoleGamma;
                const auto smaa = AntiAliasing::IsSMAA();
                CPostFX::bConsoleGammaEnabled = gamma != 0;
                ApplySMAA(smaa);

                // MW's own fx defines the MWGamma technique (X360 preset, cfg mode 2)
                CPostFX::RenderGamma(Direct3DDevice, gamma == 2 ? "MWGamma" : nullptr);
                if constexpr (WFP::DeveloperOverlay)
                    Overlay.Present(Direct3DDevice.get(), ThisModule());
            };

            WFP::onShutdownEvent() += []() { Overlay.ShutdownRenderer(); };

            WFP::onBeforeReset() += []()
            {
                Overlay.BeforeReset();
                CPostFX::Shutdown();
                CPostFX::OnDeviceReset();
            };
        };
    }
} PostFX;

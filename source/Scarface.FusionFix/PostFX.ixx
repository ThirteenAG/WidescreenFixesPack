module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include <d3d9.h>
#include <atomic>

export module PostFX;

import PostFXCore;
import ShaderDump;
import CharacterBlood;
import SkinRendering;
import ControllerPrompts;

IDirect3DDevice9* pDevice = nullptr;
std::atomic<bool> smaaEnabled{ true }, consoleGammaEnabled{ false };

injector::hook_back<void(*)()> hb_sub_654B20;
void sub_654B20()
{
    CPostFX::Shutdown();
    CPostFX::OnDeviceReset();
    return hb_sub_654B20.fun();
}

class PostFX
{
public:
    PostFX()
    {
        WFP::onInitEvent() += []()
        {
            auto& settings = FixSettings();
            const auto update = []()
            {
                consoleGammaEnabled = FixSettings().Bool("GRAPHICS.ConsoleGamma");
                smaaEnabled = std::clamp(FixSettings().Int("GRAPHICS.AntiAliasing"), 0, 1) == 1;
            };
            update();
            settings.OnChange("GRAPHICS.ConsoleGamma", update);
            settings.OnChange("GRAPHICS.AntiAliasing", update);

            auto observe = hook::pattern("88 87 ? ? ? ? 8B 47");
            static SafetyHookMid PresentHook;
            PresentHook = safetyhook::create_mid(observe.get_first(), [](SafetyHookContext& regs)
            {
                pDevice = *reinterpret_cast<IDirect3DDevice9**>(regs.edi + 0x10);
                ControllerPrompts::Update(pDevice);
                SkinRendering::ObserveDevice(pDevice);
                ShaderDump::ObserveDevice(pDevice);
            });
            if (!PresentHook)
            {
                OutputDebugStringA("Scarface: renderer observation hook unavailable.\n");
                return;
            }

            ShaderDump::Initialize();
            CharacterBlood::Initialize();
            ControllerPrompts::Initialize();

            auto draw = hook::pattern("E8 ? ? ? ? 8B 4C 24 ? 8B 54 24 ? ? ? 83 C4");
            auto reset = hook::pattern("8B 46 ? ? ? 50 FF 51 ? 3D");
            auto release = hook::pattern("75 26 E8 ? ? ? ? 8B 86 ? ? ? ?");
            auto resetRelease = hook::pattern("E8 ? ? ? ? 8B 06 8B 08 53");
            static auto DrawHook = safetyhook::create_mid(draw.get_first(), [](SafetyHookContext&)
            {
                if (!pDevice)
                    return;
                CPostFX::bConsoleGammaEnabled = consoleGammaEnabled.load(std::memory_order_relaxed);
                CPostFX::bSmaaEnabled = smaaEnabled.load(std::memory_order_relaxed);
                CPostFX::RenderSMAA(pDevice);
                CPostFX::RenderGamma(pDevice);
            });
            static auto ResetHook = safetyhook::create_mid(reset.get_first(), [](SafetyHookContext&)
            {
                CPostFX::Shutdown();
                CPostFX::OnDeviceReset();
            });
            if (!DrawHook || !ResetHook)
            {
                DrawHook.reset();
                ResetHook.reset();
                CPostFX::bConsoleGammaEnabled = CPostFX::bSmaaEnabled = false;
                OutputDebugStringA("Scarface: post-processing draw/reset hooks unavailable.\n");
                return;
            }
            hb_sub_654B20.fun = injector::MakeCALL(release.get_first(2), sub_654B20, true).get();
            injector::MakeCALL(resetRelease.get_first(), sub_654B20, true);
        };

        WFP::onShutdownEvent() += []()
        {
            SkinRendering::Shutdown();
            ControllerPrompts::Shutdown();
            ShaderDump::Shutdown();
            CharacterBlood::Shutdown();
            CPostFX::Shutdown();
            pDevice = nullptr;
        };
    }
} PostFX;

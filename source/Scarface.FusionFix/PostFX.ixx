module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include <d3d9.h>

export module PostFX;

import PostFXCore;
import ShaderDump;
import CharacterBlood;
import SkinRendering;
import ControllerPrompts;

IDirect3DDevice9* pDevice = nullptr;

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
            CPostFX::bConsoleGammaEnabled = settings.Bool("GRAPHICS.ConsoleGamma");
            CPostFX::bSmaaEnabled = (settings.Int("GRAPHICS.AntiAliasing") == 1);

            if (!CPostFX::bConsoleGammaEnabled && !CPostFX::bSmaaEnabled &&
                ShaderDump::Mode == 0 && !settings.Bool("GRAPHICS.RestoreCharacterBlood") &&
                !settings.Bool("MAIN.ControllerPrompts"))
                return;

            auto observe = hook::pattern("88 87 ? ? ? ? 8B 47");
            if (observe.size() != 1)
            {
                OutputDebugStringA("Scarface: renderer observation pattern unavailable.\n");
                return;
            }
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

            const bool dumpShaders = ShaderDump::Initialize();
            const bool characterBlood = CharacterBlood::Initialize();
            ControllerPrompts::Initialize();
            if (!CPostFX::bConsoleGammaEnabled && !CPostFX::bSmaaEnabled && !dumpShaders && !characterBlood && !ControllerPrompts::IsEnabled())
            {
                PresentHook.reset();
                return;
            }
            if (!CPostFX::bConsoleGammaEnabled && !CPostFX::bSmaaEnabled)
                return;

            auto draw = hook::pattern("E8 ? ? ? ? 8B 4C 24 ? 8B 54 24 ? ? ? 83 C4");
            auto reset = hook::pattern("8B 46 ? ? ? 50 FF 51 ? 3D");
            auto release = hook::pattern("75 26 E8 ? ? ? ? 8B 86 ? ? ? ?");
            auto resetRelease = hook::pattern("E8 ? ? ? ? 8B 06 8B 08 53");
            if (draw.size() != 1 || reset.size() != 1 || release.size() != 1 || resetRelease.size() != 1 ||
                injector::GetBranchDestination(release.get_first(2)).as_int() != injector::GetBranchDestination(resetRelease.get_first()).as_int())
            {
                CPostFX::bConsoleGammaEnabled = CPostFX::bSmaaEnabled = false;
                OutputDebugStringA("Scarface: post-processing draw/reset patterns unavailable.\n");
                return;
            }
            static auto DrawHook = safetyhook::create_mid(draw.get_first(), [](SafetyHookContext&)
            {
                if (!pDevice)
                    return;
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

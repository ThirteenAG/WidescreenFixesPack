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
            hb_sub_654B20.fun = injector::MakeCALL(release.get_first(2), sub_654B20, true).get();
            injector::MakeCALL(resetRelease.get_first(), sub_654B20, true);

            // A windowed device is lost without the focus change on which the game releases its device
            // resources, and the reset in its wait loop then never succeeds. Go through the game's own
            // deactivation and activation instead of that reset.
            auto lostReset = hook::pattern("8B 47 10 8B 08 8D 57 14 52 50 FF 51 40 85 C0 75 05 E8");
            auto setActive = hook::pattern("53 8A 5C 24 08 56 8B F1 38 9E ? ? ? ? 0F 84");
            static auto SetDisplayActive = reinterpret_cast<void(__thiscall*)(void*, char)>(setActive.get_first());
            static auto lostResume = reinterpret_cast<uintptr_t>(lostReset.get_first(22));
            static auto LostResetHook = safetyhook::create_mid(lostReset.get_first(), [](SafetyHookContext& regs)
            {
                auto display = reinterpret_cast<void*>(regs.edi);
                SetDisplayActive(display, 0);
                SetDisplayActive(display, 1);
                regs.eip = lostResume;
            });

            // The game releases its device resources when it is deactivated and recreates them only after
            // a device reset. A device that was never lost needs no reset: recreate them on activation.
            auto restore = hook::pattern("FF 51 40 85 C0 75 ? E8 ? ? ? ? 8B 86");
            static auto RestoreDeviceResources = reinterpret_cast<void(*)()>(injector::GetBranchDestination(restore.get_first(7)).as_int());
            auto activated = hook::pattern("85 C0 75 ? 8B 96 ? ? ? ? 50 6A 01 68 9A 06 00 00");
            static auto ActivatedHook = safetyhook::create_mid(activated.get_first(4), [](SafetyHookContext&)
            {
                RestoreDeviceResources();
            });
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

module;

#include <stdafx.h>
#include "common.h"
#include <d3d9.h>

export module PostFX;

import Skeleton;
import PostFXCore;

injector::hook_back<void(__fastcall*)(void*, void*)> hbRenderMotionBlur;
void __fastcall RenderMotionBlur(void* camera, void* edx);

static IDirect3DDevice9* GetDevice9()
{
    if (!pD3D9Device || !*pD3D9Device) return nullptr;
    return *pD3D9Device;
}

class PostFX
{
public:
    PostFX()
    {
        WFP::onInitEvent() += []()
        {
            CIniReader iniReader("");
            CPostFX::bConsoleGammaEnabled = iniReader.ReadInteger("GRAPHICS", "ConsoleGamma", 0) != 0;
            CPostFX::bSmaaEnabled = iniReader.ReadInteger("GRAPHICS", "SMAA", 0) != 0;

            if (CPostFX::bSmaaEnabled)
            {
                auto pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? EB ? E8");
                hbRenderMotionBlur.fun = injector::MakeCALL(pattern.get_first(), RenderMotionBlur).get();
            }

            if (CPostFX::bConsoleGammaEnabled)
            {
                auto pattern = hook::pattern("E8 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 8B 0D ? ? ? ? 51 E8 ? ? ? ? 83 C4 ? C3");
                static auto ConsoleGammaHook = safetyhook::create_mid(pattern.get_first(), +[](SafetyHookContext& regs)
                {
                    CPostFX::RenderGamma(GetDevice9());
                });
            }

            if (CPostFX::bConsoleGammaEnabled || CPostFX::bSmaaEnabled)
            {
                WFP::onBeforeReset() += []()
                {
                    CPostFX::Shutdown();
                    CPostFX::OnDeviceReset();
                };

                WFP::onShutdownEvent() += []()
                {
                    CPostFX::ShutdownProcess();
                };
            }
        };
    }
} PostFX;

void __fastcall RenderMotionBlur(void* camera, void* edx)
{
    CPostFX::RenderSMAA(GetDevice9());
    hbRenderMotionBlur.fun(camera, edx);
}

module;

#include <stdafx.h>
#include "common.h"
#include <d3d9.h>

export module PostFX;

import Skeleton;
import Draw;
import PostFXCore;

injector::hook_back<void(__fastcall*)(void*, void*)> hbRenderMotionBlur;
void __fastcall RenderMotionBlur(void* camera, void* edx);

static SafetyHookInline SetupBackBuffer;
static void __cdecl SetupBackBufferVertex()
{
    // SilentPatch can rebuild the buffer after our video-mode hook returns,
    // while the frontend still has its constrained canvas active.
    MenuCanvas::Suspend physicalViewport;
    SetupBackBuffer.unsafe_ccall();
}

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

            // Resolve the buffer initializer from CPostEffects::Update; the
            // initial setup call itself can be redirected by SilentPatch.
            auto setup = hook::pattern("A1 ? ? ? ? 85 C0 75 05 E9 ? ? ? ? C3");
            if (setup.size() == 1)
                SetupBackBuffer = safetyhook::create_inline(injector::GetBranchDestination(setup.get_first(9)).as_int(), SetupBackBufferVertex);

            if (iniReader.ReadInteger("GRAPHICS", "DisableSpeedBlur", 0) != 0)
            {
                // Skip only the speed overlay. Keep the caller's state reset
                // and all other post-processing, including SMAA, intact.
                auto speed = hook::pattern("A1 ? ? ? ? 50 E8 ? ? ? ? 83 C4 04 8A 0D ? ? ? ? 32 C0 84 C9");
                if (speed.size() == 1)
                    injector::MakeNOP(speed.get_first(6), 5, true);
            }

            if (CPostFX::bSmaaEnabled)
            {
                auto pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? EB ? E8");
                hbRenderMotionBlur.fun = injector::MakeCALL(pattern.get_first(), RenderMotionBlur).get();
            }

            if (CPostFX::bConsoleGammaEnabled)
            {
                auto pattern = hook::pattern("E8 ? ? ? ? 8B 0D ? ? ? ? 51 E8 ? ? ? ? 8B 15 ? ? ? ? 52 E8 ? ? ? ? 83 C4 ? 83 C4");
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
            }
        };
    }
} PostFX;

void __fastcall RenderMotionBlur(void* camera, void* edx)
{
    CPostFX::RenderSMAA(GetDevice9());
    hbRenderMotionBlur.fun(camera, edx);
}

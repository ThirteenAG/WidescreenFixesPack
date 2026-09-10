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
    if (!pD3D8Device || !*pD3D8Device) return nullptr;

    IDirect3DDevice9* dev9 = nullptr;
    if (FAILED((*pD3D8Device)->QueryInterface(__uuidof(IDirect3DDevice9), (void**)&dev9)))
        return nullptr;

    dev9->Release();
    return dev9;
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
                auto pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? E9 ? ? ? ? E8 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? DF E0");
                hbRenderMotionBlur.fun = injector::MakeCALL(pattern.get_first(), RenderMotionBlur).get();
            }

            if (CPostFX::bConsoleGammaEnabled)
            {
                auto pattern = hook::pattern("83 C4 ? E8 ? ? ? ? E8 ? ? ? ? 83 C4");
                static auto ConsoleGammaHook = safetyhook::create_mid(pattern.get_first(3), +[](SafetyHookContext& regs)
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

module;

#include <stdafx.h>
#include "common.h"

export module Main;

import Draw;
import Skeleton;
import Entity;
import CutsceneMgr;

export GameRef<CScene> Scene;

SafetyHookInline shCameraSize = {};
void __cdecl CameraSize(RwCamera* camera, RwRect* rect, float viewWindow, float aspectRatio)
{
    return shCameraSize.unsafe_ccall(camera, rect, SCREEN_VIEWWINDOW, SCREEN_ASPECT_RATIO);
}

class Main
{
public:
    Main()
    {
        WFP::onInitEvent() += []()
        {
            // Unlock resolutions
            // Advanced Display Options
            injector::MakeNOP(0x745B71, 6); // Skip width check
            injector::MakeNOP(0x745B81, 6); // Skip height check
            injector::WriteMemory<BYTE>(0x745B96, 0xEB, true); // Skip AR check
            injector::MakeNOP(0x745BFC, 2); // Skip VRAM check

            // Resolution selection dialog
            injector::MakeNOP(0x74596C, 6); // Skip width check
            injector::MakeNOP(0x74597A, 6); // Skip height check
            injector::WriteMemory<BYTE>(0x7459D0, 0xEB, true); // Skip AR check

            // default res
            auto [x, y] = GetDesktopRes();
            auto pattern = hook::pattern("BD ? ? ? ? BB ? ? ? ? BE");
            injector::WriteMemory<int32_t>(pattern.get_first(1), x, true);
            injector::WriteMemory<int32_t>(pattern.get_first(6), y, true);

            //CHud::DrawHelpText (help text is lower with widescreen borders, not needed)
            pattern = hook::pattern("74 ? 8A 0D ? ? ? ? 84 C9 75 ? B0");
            injector::WriteMemory<uint8_t>(pattern.get_first(), 0xEB, true);
        };

        WFP::onGameInitEvent() += []()
        {
            auto pattern = hook::pattern("E8 ? ? ? ? 8D 4C 24 ? 51 C7 44 24");
            shCameraSize = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), CameraSize);

            pattern = hook::pattern("8B 15 ? ? ? ? 52 E8 ? ? ? ? E8 ? ? ? ? A1");
            Scene.SetAddress(*pattern.get_first<CScene*>(2));

            // Fix sky multitude
            static float horizSpread = 10.0f;
            pattern = hook::pattern("D8 0D ? ? ? ? 83 C4 40");
            injector::WriteMemory<const void*>(pattern.get_first(2), &horizSpread, true);

            static auto SkyRenderingFix = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                horizSpread = 1.4f * (CDraw::GetAspectRatio() / (4.0f / 3.0f));
            });

            pattern = hook::pattern("D8 0D ? ? ? ? D9 C0 D9 44 24 ? D8 44 24");
            injector::WriteMemory<const void*>(pattern.get_first(2), &horizSpread, true);

            //CCam::Process_WheelCam
            pattern = hook::pattern("C7 86 ? ? ? ? ? ? ? ? 8A 47 ? 24 07");
            injector::MakeNOP(pattern.get_first(), 10, true);
            static auto Process_WheelCamFOV = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                *(float*)(regs.esi + 0xB4) = CDraw::ConvertFOVInverse(70.0f);
            });

            //CCam::Process_Fixed
            pattern = hook::pattern("C7 87 ? ? ? ? ? ? ? ? 80 3D");
            injector::MakeNOP(pattern.get_first(), 10, true);
            static auto Process_FixedFOV = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                *(float*)(regs.edi + 0xB4) = CDraw::ConvertFOVInverse(70.0f);
            });
        };
    }
} Main;
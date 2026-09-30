module;

#include <stdafx.h>

export module LeadD3DRender;

import ComVars;
import Graphics;

// D3DVideoRenderer: quad is left, top, right, bottom in clip space, +0x14 is the render target texture (null for the screen)
SafetyHookInline shVideoRender = {};
void __fastcall VideoRender(void* renderer, void* edx, float* quad)
{
    auto renderTarget = reinterpret_cast<void**>(quad)[5];
    auto ratio = GetAspectRatio() / fDefaultAspectRatio;
    if (renderTarget || std::abs(ratio - 1.0f) < 0.01f)
        return shVideoRender.fastcall<void>(renderer, edx, quad);

    float saved[4] = { quad[0], quad[1], quad[2], quad[3] };
    if (ratio > 1.0f)
    {
        quad[0] /= ratio;
        quad[2] /= ratio;
    }
    else
    {
        quad[1] *= ratio;
        quad[3] *= ratio;
    }
    shVideoRender.fastcall<void>(renderer, edx, quad);
    std::copy(std::begin(saved), std::end(saved), quad);
}

export void InitLeadD3DRender()
{
    CIniReader iniReader("");
    auto bDisableDOF = iniReader.ReadInteger("GRAPHICS", "DisableDOF", 1) != 0;
    auto bDisableBlackAndWhiteFilter = iniReader.ReadInteger("GRAPHICS", "DisableBlackAndWhiteFilter", 0) != 0;
    auto bDisableCharacterLighting = iniReader.ReadInteger("GRAPHICS", "DisableCharacterLighting", 0) != 0;
    auto bEnhancedSonarVision = iniReader.ReadInteger("GRAPHICS", "EnhancedSonarVision", 0) != 0;
    gBlacklistIndicators = iniReader.ReadInteger("GRAPHICS", "BlacklistIndicators", 0);

    if (bDisableDOF)
    {
        auto pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "E8 ? ? ? ? 8B 06 8B 08 83 C4 18 6A 00 68");
        injector::MakeNOP(pattern.get_first(0), 5);
    }

    if (bDisableBlackAndWhiteFilter)
    {
        auto pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "E8 ? ? ? ? 8B 06 8B 08 83 C4 34");
        hb_100177B7.fun = injector::MakeCALL(pattern.get_first(), sub_100177B7, true).get();
    }

    if (bDisableCharacterLighting)
    {
        auto pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "80 78 44 00");
        injector::WriteMemory<uint8_t>(pattern.get_first(3), 1, true);
    }

    if (bEnhancedSonarVision)
    {
        static bool bNightVision = false;
        static auto loc_1002E95F = (uintptr_t)hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "89 B8 ? ? ? ? E8 ? ? ? ? 5F 5E 5B 83 C5 78 C9 C3").get_first(13);

        auto pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "E8 ? ? ? ? 89 5D 78");
        hb_1002581C.fun = injector::MakeCALL(pattern.get_first(), sub_1002581C, true).get();

        pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "8B CE 88 86");
        struct SonarVisionHook
        {
            void operator()(injector::reg_pack& regs)
            {
                bNightVision = regs.eax & 0xff;
                //if (false)
                //    *(uint8_t*)(regs.esi + 0x642C) = regs.eax & 0xff;
                //else
                *(uint8_t*)(regs.esi + 0x642C) = 0;
            }
        }; injector::MakeInline<SonarVisionHook>(pattern.get_first(2), pattern.get_first(8));

        pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "80 B9 ? ? ? ? ? 0F 84 ? ? ? ? 56 57 E8 ? ? ? ? 8B 03 33 FF 47");
        struct NVCheck
        {
            void operator()(injector::reg_pack& regs)
            {
                if (!bNightVision)
                    *(uintptr_t*)(regs.esp - 4) = loc_1002E95F;
            }
        }; injector::MakeInline<NVCheck>(pattern.get_first(0), pattern.get_first(13));

        //DrawVisibleOpaque
        pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "68 ? ? ? ? 57 8D 43 0C");
        injector::MakeNOP(pattern.get_first(0), 17);

        //to do: add bNightVision check
        pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "75 15 8B 80");
        injector::WriteMemory<uint8_t>(pattern.get_first(), 0xEB, true);

        //to do: add bNightVision check
        pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "80 B8 ? ? ? ? ? 74 65");
        injector::MakeNOP(pattern.get_first(0), 7);

        //to do: add bNightVision check
        pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "8B 03 80 B8");
        injector::MakeNOP(pattern.get_first(2), 7);
    }

    if (iniReader.ReadInteger("DISPLAY", "UltraWideSupport", 1) != 0)
    {
        // Flash pass: don't restrict the viewport to the multi-monitor HUD rect (l3d::Options +0x1F8 enable, +0x1FC rect),
        // the Flash renderer places the stage into that rect itself (see InitWidescreenFix)
        auto pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "38 98 F8 01 00 00 74");
        injector::WriteMemory<uint8_t>(pattern.get_first(6), 0xEB, true);

        // Videos are drawn in that pass as a clip space quad covering the whole viewport, keep them in the 16:9 rect
        pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "55 8D 6C 24 8C 81 EC ? ? ? ? 53 56 57 89 4D 28 E8");
        shVideoRender = safetyhook::create_inline(pattern.get_first(), VideoRender);
    }
}

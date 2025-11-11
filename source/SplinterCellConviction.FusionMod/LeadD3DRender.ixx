module;

#include <stdafx.h>

export module LeadD3DRender;

import ComVars;
import Graphics;

export void InitLeadD3DRender()
{
    CIniReader iniReader("");
    auto bDisableDOF = iniReader.ReadInteger("MAIN", "DisableDOF", 1) != 0;
    auto bDisableBlackAndWhiteFilter = iniReader.ReadInteger("MAIN", "DisableBlackAndWhiteFilter", 0) != 0;
    auto bDisableCharacterLighting = iniReader.ReadInteger("MAIN", "DisableCharacterLighting", 0) != 0;
    auto bEnhancedSonarVision = iniReader.ReadInteger("MAIN", "EnhancedSonarVision", 0) != 0;
    gBlacklistIndicators = iniReader.ReadInteger("MAIN", "BlacklistIndicators", 0);
    auto bPartialUltraWideSupport = iniReader.ReadInteger("MAIN", "PartialUltraWideSupport", 1) != 0;

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

    // Viewport
    if (bPartialUltraWideSupport)
    {
        auto pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "F3 0F 11 49 ? F3 0F 11 61");
        static auto ViewportHook1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            if (!OpenedVideosList.empty() && bVideoRender && GetAspectRatio() > fDefaultAspectRatio)
                regs.xmm1.f32[0] /= (GetAspectRatio() / fDefaultAspectRatio);
        });

        pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "F3 0F 11 51 ? F3 0F 11 49 ? F3 0F 59 F5");
        static auto ViewportHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            if (!OpenedVideosList.empty() && bVideoRender && GetAspectRatio() > fDefaultAspectRatio)
                regs.xmm2.f32[0] /= (GetAspectRatio() / fDefaultAspectRatio);
            bVideoRender = false;
        });

        pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "89 4D 28 E8 ? ? ? ? 8B 75 7C 33 FF 8B D8");
        static auto VideoRenderHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            bVideoRender = true;
        });

        pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "8B 07 8B 08 6A 00 50 FF 91 ? ? ? ? 8B 07 8B 08 6A 00");
        static auto d3ddevice = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            pDevice = *(IDirect3DDevice9**)(regs.edi);
        });
    }
}
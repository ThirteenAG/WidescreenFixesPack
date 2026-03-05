module;

#include <stdafx.h>

export module WidescreenFix;

import ComVars;

export void InitWidescreenFix()
{
    if (bPartialUltraWideSupport)
    {
        // override aspect ratio
        auto pattern = hook::pattern("74 0A F3 0F 10 05 ? ? ? ? EB 13");
        injector::MakeNOP(pattern.get_first(), 2, true);

        pattern = hook::pattern("8D 86 ? ? ? ? 8B 08 3B CD");
        static auto IniHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            *(uint32_t*)(regs.esi + 0x104) = 0; //ForceMultiMon
            *(float*)(regs.esi + 0x108) = 99.0f; //MinTripleDispRatio
        });

        static int nCounter = 0;
        static int nCounterPrev = 0;
        static bool bNeedsFix = false;

        pattern = hook::pattern("F3 0F 10 0D ? ? ? ? 53 8B D9 83 4B 10 FF");
        injector::MakeNOP(pattern.get_first(), 8, true);
        static auto FCanvasUtil__SetViewport = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            if (GetAspectRatio() > fDefaultAspectRatio)
                regs.xmm1.f32[0] = 2.0f / (GetAspectRatio() / fDefaultAspectRatio);
            else
                regs.xmm1.f32[0] = 2.0f;
        });

        pattern = hook::pattern("F3 0F 10 0D ? ? ? ? 89 4B 10");
        injector::MakeNOP(pattern.get_first(), 8, true);
        static auto FCanvasUtil__SetViewport2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            if (GetAspectRatio() > fDefaultAspectRatio)
                regs.xmm1.f32[0] = 2.0f / (GetAspectRatio() / fDefaultAspectRatio);
            else
                regs.xmm1.f32[0] = 2.0f;
        });

        //scaling for menus and 3d
        {
            auto pattern = hook::pattern("F3 0F 10 90 ? ? ? ? 83 EC 10 8B C4 F3 0F 11 00");
            injector::MakeNOP(pattern.get_first(0), 8, true);
            static auto UViewportHook1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                regs.xmm2.f32[0] = 1.0f / (GetAspectRatio() / fDefaultAspectRatio);
            });

            pattern = hook::pattern("F3 0F 10 90 ? ? ? ? 83 EC 10 8B C4 F3 0F 11 10");
            injector::MakeNOP(pattern.get_first(0), 8, true);
            static auto UViewportHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                regs.xmm2.f32[0] = 1.0f / (GetAspectRatio() / fDefaultAspectRatio);
            });

            pattern = hook::pattern("8B 3C 88 8B CF E8 ? ? ? ? 85 C0");
            static auto UViewportHook4 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                nCounterPrev = nCounter;
                nCounter = regs.ecx;
            });
        }

        //HUD (all 2D, including what's fixed above)
        {
            auto pattern = hook::pattern("F3 0F 59 6C 24 ? F3 0F 11 44 24");
            static auto FlashMenuRendererHook1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                //if (nCounter == 0 || nCounter == 1 || nCounter == 3 || nCounter == 4)
                if (nCounter != 5 && nCounter != 6 && nCounter != 7 && nCounter != 8 && nCounter != 9 && OpenedVideosList.empty()) // 5 and 6 mess up menu sometimes, 7, 8 and 9 is in-world UI
                {
                    bNeedsFix = true;
                    if (GetAspectRatio() > fDefaultAspectRatio)
                        regs.xmm0.f32[0] /= (GetAspectRatio() / fDefaultAspectRatio);
                }
            });

            pattern = hook::pattern("F3 0F 11 44 24 ? 0F 57 C0 F3 0F 11 5C 24");
            static auto FlashMenuRendererHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (bNeedsFix)
                {
                    if (GetAspectRatio() > fDefaultAspectRatio)
                        regs.xmm3.f32[0] /= (GetAspectRatio() / fDefaultAspectRatio);
                    bNeedsFix = false;
                }
            });
        }

        // blood overlay disable
        {
            auto pattern = hook::pattern("F3 0F 10 05 ? ? ? ? 0F 2F C1 0F 86 ? ? ? ? F3 0F 10 96");
            injector::MakeNOP(pattern.get_first(), 8, true);
            static auto FlashHudHealthFeedback = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (GetAspectRatio() > fDefaultAspectRatio)
                    regs.xmm0.f32[0] = 0.0f;
                else
                    regs.xmm0.f32[0] = 1.0f;
            });
        }

        // sticky camera overlay disable
        {
            pattern = hook::pattern("0F 84 ? ? ? ? F3 0F 10 0D ? ? ? ? 0F B6 C9");
            injector::MakeNOP(pattern.get_first(), 6, true); // make pos overwrite possible

            pattern = hook::pattern("F3 0F 11 96 ? ? ? ? F3 0F 2C C8");
            injector::MakeNOP(pattern.get_first(), 8, true);
            static auto StickyCamBorder1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (GetAspectRatio() > fDefaultAspectRatio)
                    *(float*)(regs.esi + 0x4A8) = -10000.0f;
                else
                    *(float*)(regs.esi + 0x4A8) = regs.xmm2.f32[0];
            });

            pattern = hook::pattern("F3 0F 10 86 ? ? ? ? F3 0F 59 05 ? ? ? ? 33 C9");
            injector::MakeNOP(pattern.get_first(), 8, true);
            static auto StickyCamBorder2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (GetAspectRatio() > fDefaultAspectRatio)
                    regs.xmm0.f32[0] = -10000.0f;
                else
                    regs.xmm0.f32[0] = *(float*)(regs.esi + 0x4A8);
            });
        }
    }
}
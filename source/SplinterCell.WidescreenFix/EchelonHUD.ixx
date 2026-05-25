module;

#include <stdafx.h>

export module EchelonHUD;

import ComVars;

export void InitEchelonHUD()
{
    // Mission Complete
    // Rescale title font size from ClipX / 640 to ClipY / 480
    auto pattern = find_module_pattern(GetModuleHandle(L"EchelonHUD"),
        "D9 5C 24 40 FF 92 98 00 00 00 8B 44 24 10 8B 48 40 89 48 48");

    if (!pattern.empty())
    {
        static auto MissionCompleteScaleHook = safetyhook::create_mid(pattern.get_first(10), [](SafetyHookContext& regs)
        {
            auto canvas = *(uintptr_t*)(regs.esp + 0x10);
            if (!canvas)
                return;

            int clipX = *(int*)(canvas + 0x80);
            int clipY = *(int*)(canvas + 0x84);
            if (clipX <= 0 || clipY <= 0)
                return;

            float ratio = (640.0f * (float)clipY) / (480.0f * (float)clipX);
            *(float*)(regs.esp + 0x20) *= ratio;
            *(float*)(regs.esp + 0x30) *= ratio;
        });
    }

    // Fix title positioning at SetPos
    pattern = find_module_pattern(GetModuleHandle(L"EchelonHUD"),
        "DB 44 24 24 52 51 D9 1C 24 FF 90 90 00 00 00");

    if (!pattern.empty())
    {
        static auto MissionCompleteTitleHook = safetyhook::create_mid(pattern.get_first(9), [](SafetyHookContext& regs)
        {
            auto canvas = regs.ecx;
            if (!canvas)
                return;

            float& X = *(float*)(regs.esp + 0x00);
            float& Y = *(float*)(regs.esp + 0x04);

            int clipX = *(int*)(canvas + 0x80);
            int clipY = *(int*)(canvas + 0x84);
            if (clipX <= 0 || clipY <= 0)
                return;

            float scaleY = (float)clipY / 480.0f;
            float halfX  = (float)(clipX / 2);

            // Fix unscaled X centering
            float elapsed = *(float*)(regs.ebp + 0x474);
            if (elapsed >= 2.5f && elapsed < 5.0f)
            {
                X = halfX - (halfX - X) * scaleY;
                // Small centering correction for glyph advance width
                X += scaleY;
            }

            // Re-anchor Y to the vertically scaled 640x480 layout
            float virtualPenY = Y - (float)(clipY / 2) + 240.0f;
            Y = virtualPenY * scaleY;
        });
    }

    // Mission Failed
    // Rescale title font size from ClipX / 640 to ClipY / 480
    pattern = find_module_pattern(GetModuleHandle(L"EchelonHUD"),
        "DB 44 24 38 89 50 48 8B 4C 24 18 D9 5C 24 30");

    if (!pattern.empty())
    {
        static auto MissionFailedScaleHook = safetyhook::create_mid(pattern.get_first(4), [](SafetyHookContext& regs)
        {
            auto canvas = *(uintptr_t*)(regs.esp + 0x18);
            if (!canvas)
                return;

            int clipX = *(int*)(canvas + 0x80);
            int clipY = *(int*)(canvas + 0x84);
            if (clipX <= 0 || clipY <= 0)
                return;

            *(float*)(regs.esp + 0x2c) *= (640.0f * (float)clipY) / (480.0f * (float)clipX);
        });
    }

    // Fix title positioning at SetPos
    pattern = find_module_pattern(GetModuleHandle(L"EchelonHUD"),
        "D8 6C 24 34 D9 1C 24 FF 90 90 00 00 00");

    if (!pattern.empty())
    {
        static auto MissionFailedTitleHook = safetyhook::create_mid(pattern.get_first(7), [](SafetyHookContext& regs)
        {
            auto canvas = regs.ecx;
            if (!canvas)
                return;

            float& X = *(float*)(regs.esp + 0x00);
            float& Y = *(float*)(regs.esp + 0x04);

            int clipX = *(int*)(canvas + 0x80);
            int clipY = *(int*)(canvas + 0x84);
            if (clipX <= 0 || clipY <= 0)
                return;

            float scaleY = (float)clipY / 480.0f;
            float S = *(float*)(regs.esp + 0x2c);

            // Re-anchor Y to the vertically scaled 640x480 layout
            Y = Y - (float)(clipY / 2) + 4.0f + (240.0f - 4.0f) * scaleY;

            // Small centering correction for glyph advance width
            X += S;
        });
    }
}

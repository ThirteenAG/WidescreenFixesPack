module;

#include <stdafx.h>
#include "common.h"
#include <d3d9.h>

export module Misc;

import Skeleton;
import Frontend;
import Camera;
import Entity;
import Physical;
import Timer;
import Draw;

injector::hook_back<void(__fastcall*)(CCamera*, void*, bool, bool)> hbCalculateDerivedValues;
void __fastcall CalculateDerivedValues(CCamera* camera, void* edx, bool bForMirror, bool bOriented)
{
    CCamera::UpdatePlayerVehicleSpeedBlur(camera);
    return hbCalculateDerivedValues.fun(camera, edx, bForMirror, bOriented);
}

DWORD _EAX;
void __declspec(naked) AllowMouseMovement()
{
    _asm
    {
        mov _EAX, eax
        mov eax, dword ptr ds : [0x8D621C]
        cmp eax, 0
        jne label1
        mov eax, _EAX
        ret

        label1 :
        mov eax, _EAX
            mov _EAX, 0x7453F0
            jmp _EAX
    }
}

int* SelectedMultisamplingLevels = nullptr;

int ReplaceTextShadowWithOutline = 0;

namespace CFont
{
    GameRef<CRGBA> Color([]() -> CRGBA*
    {
        auto pattern = hook::pattern("A2 ? ? ? ? 88 25 ? ? ? ? C1 E9 10");
        if (!pattern.empty())
            return *pattern.get_first<CRGBA*>(1);
        return nullptr;
    });

    GameRef<CRGBA> m_FontDropColor([]() -> CRGBA*
    {
        auto pattern = hook::pattern("A2 ? ? ? ? 88 25 ? ? ? ? C1 E8 10 A2 ? ? ? ? 88 25 ? ? ? ? DF E0");
        if (!pattern.empty())
            return *pattern.get_first<CRGBA*>(1);
        return nullptr;
    });


    GameRef<char> m_FontIsBlip([]() -> char*
    {
        auto pattern = hook::pattern("8A 15 ? ? ? ? 88 51 ? A0 ? ? ? ? 88 41 ? 83 C1 30");
        if (!pattern.empty())
            return *pattern.get_first<char*>(2);
        return nullptr;
    });

    GameRef<uint8_t> m_FontShadow([]() -> uint8_t*
    {
        auto pattern = hook::pattern("C6 05 ? ? ? ? ? 7B ? 0F BF D3 89 54 24 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 8B 4C 24");
        if (!pattern.empty())
            return *pattern.get_first<uint8_t*>(2);
        return nullptr;
    });

    GameRef<uint8_t> m_FontOutlineSize([]() -> uint8_t*
    {
        auto pattern = hook::pattern("C6 05 ? ? ? ? ? 7B ? 0F BF D3 89 54 24 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 8B 7C 24");
        if (!pattern.empty())
            return *pattern.get_first<uint8_t*>(2);
        return nullptr;
    });

    GameRef<uint8_t> m_FontOutlineOrShadow([]() -> uint8_t*
    {
        auto pattern = hook::pattern("A0 ? ? ? ? 88 41 ? 83 C1");
        if (!pattern.empty())
            return *pattern.get_first<uint8_t*>(1);
        return nullptr;
    });

    GameRef<float> Slant([]() -> float*
    {
        auto pattern = hook::pattern("D9 05 ? ? ? ? D8 1D ? ? ? ? DF E0 F6 C4 44 7B ? D9 05 ? ? ? ? D8 64 24 ? D8 0D ? ? ? ? D8 05 ? ? ? ? D8 6C 24");
        if (!pattern.empty())
            return *pattern.get_first<float*>(2);
        return nullptr;
    });

    GameRef<CVector2D> SlantRefPoint([]() -> CVector2D*
    {
        auto pattern = hook::pattern("D8 05 ? ? ? ? D9 1D ? ? ? ? DB 05 ? ? ? ? D8 0D ? ? ? ? D8 C9 D8 05 ? ? ? ? D9 1D ? ? ? ? DD D8 8B 7C 24");
        if (!pattern.empty())
            return *pattern.get_first<CVector2D*>(2);
        return nullptr;
    });

    void __cdecl SetOutlinePosition(char value)
    {
        m_FontShadow = 0;
        m_FontOutlineSize = value;
        m_FontOutlineOrShadow = value;
    }

    void __cdecl SetDropShadowPosition(char value)
    {
        m_FontOutlineSize = 0;
        m_FontOutlineOrShadow = 0;
        m_FontShadow = value;
    }

    void (__cdecl* PrintString)(float x, float y, char* start, char* end, float wrap) = nullptr;
}

injector::hook_back<void(__cdecl*)(float, float, char*)> hbPrintStringFromBottom;
void __cdecl PrintStringFromBottom(float x, float y, char* text)
{
    CFont::SetDropShadowPosition(1);
    CFont::m_FontDropColor = { 0, 0, 0, 0xFF };
    hbPrintStringFromBottom.fun(x, y, text);
    CFont::SetDropShadowPosition(0);
}

injector::hook_back<void(__cdecl*)(float, float, char*)> hbPrintString;
void __cdecl PrintString(float x, float y, char* text)
{
    CFont::SetOutlinePosition(1);
    CFont::m_FontDropColor = { 0, 0, 0, 0xFF };
    hbPrintString.fun(x, y, text);
    CFont::SetOutlinePosition(0);
}

class Misc
{
public:
    Misc()
    {
        WFP::onGameInitEvent() += []()
        {
            CIniReader iniReader("");
            auto bAllowAltTabbingWithoutPausing = iniReader.ReadInteger("MISC", "AllowAltTabbingWithoutPausing", 0) != 0;
            auto bDisableWhiteCrosshairDot = iniReader.ReadInteger("MISC", "DisableWhiteCrosshairDot", 0) != 0;
            static auto nHideAABug = iniReader.ReadInteger("GRAPHICS", "HideAABug", 2);
            ReplaceTextShadowWithOutline = iniReader.ReadInteger("MISC", "ReplaceTextShadowWithOutline", 1);
            auto bCarSpeedDependantFOV = iniReader.ReadInteger("MISC", "CarSpeedDependantFOV", 1) != 0;
            auto bVCSCamShake = iniReader.ReadInteger("MISC", "VCSCamShake", 0) != 0;

            auto pattern = hook::pattern("E8 ? ? ? ? 8A 0D ? ? ? ? 83 C4 14");
            CFont::PrintString = (decltype(CFont::PrintString))injector::GetBranchDestination(pattern.get_first()).as_int();

            pattern = hook::pattern("E8 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? EB ? ? ? ? ? ? EB ? ? ? ? ? ? EB");
            if (!pattern.empty())
                hbPrintStringFromBottom.fun = injector::MakeCALL(pattern.get_first(), PrintStringFromBottom, true).get();

            static auto loc_719F30 = (uintptr_t)hook::pattern("8B 0D ? ? ? ? BA ? ? ? ? 2B D5").get_first();
            pattern = hook::pattern("D9 05 ? ? ? ? A0 ? ? ? ? D8 1D ? ? ? ? 66 0F BE D9 8A 0D ? ? ? ? A2 ? ? ? ? 89 54 24 ? 8A 15 ? ? ? ? DF E0 F6 C4 44 88 0D ? ? ? ? 8A 0D ? ? ? ? C6 05 ? ? ? ? ? 88 15 ? ? ? ? 88 0D ? ? ? ? C6 05 ? ? ? ? ? 7B ? 0F BF D3 89 54 24 ? DB 44 24 ? DB 05 ? ? ? ? D8 0D ? ? ? ? D8 C9 D8 05 ? ? ? ? D9 1D ? ? ? ? DB 05 ? ? ? ? D8 0D ? ? ? ? D8 C9 D8 05 ? ? ? ? D9 1D ? ? ? ? DD D8 8B 7C 24");
            static auto PrintStringHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                static thread_local bool inOutlinePass = false;

                if (inOutlinePass)
                {
                    return_to(loc_719F30);
                }

                const float x = *(float*)(regs.esp + 0x1C);
                const float y = *(float*)(regs.esp + 0x20);
                auto _start = (char*)regs.esi;
                auto _end = (char*)regs.ebp;
                const float wrap = *(float*)(regs.esp + 0x2C);

                const CRGBA savedColor = CFont::Color;
                const uint8_t savedIsBlip = CFont::m_FontIsBlip;
                const uint8_t savedOutlineSize = CFont::m_FontOutlineSize;
                const uint8_t savedShadow = CFont::m_FontShadow;
                const CVector2D savedSlantRef = CFont::SlantRefPoint;

                const float dropShadowPosition = (float)CFont::m_FontOutlineSize;
                const int sampleCount = 16;

                // 16-direction ring (smoother edge)
                static constexpr float dirs16[][2] = {
                    { 1.0000f,  0.0000f}, { 0.9239f,  0.3827f}, { 0.7071f,  0.7071f}, { 0.3827f,  0.9239f},
                    { 0.0000f,  1.0000f}, {-0.3827f,  0.9239f}, {-0.7071f,  0.7071f}, {-0.9239f,  0.3827f},
                    {-1.0000f,  0.0000f}, {-0.9239f, -0.3827f}, {-0.7071f, -0.7071f}, {-0.3827f, -0.9239f},
                    { 0.0000f, -1.0000f}, { 0.3827f, -0.9239f}, { 0.7071f, -0.7071f}, { 0.9239f, -0.3827f}
                };

                const float targetAlpha = (float)savedColor.a / 255.0f;
                const float perPassAlphaF = 1.0f - std::pow(1.0f - targetAlpha, 1.0f / (float)sampleCount);

                constexpr float kOutlineAlphaBoost = 2.0f;
                const uint8_t perPassAlpha = (uint8_t)std::clamp(perPassAlphaF * 255.0f * kOutlineAlphaBoost, (float)std::min<uint8_t>(0, savedColor.a), (float)savedColor.a);

                float outlineStrength = 1.0f;
                switch (ReplaceTextShadowWithOutline)
                {
                    case 0:
                        outlineStrength = 1.0f;
                        break;
                    case 1:
                        outlineStrength = 0.5f;
                        break;
                    case 2:
                        outlineStrength = 1.0f;
                        break;
                    default:
                        break;
                }

                CFont::m_FontIsBlip = 1;
                inOutlinePass = true;

                for (int i = 0; i < sampleCount; ++i)
                {
                    CRGBA outlineColor = CFont::m_FontDropColor;
                    outlineColor.a = perPassAlpha;
                    CFont::Color = outlineColor;

                    const float ox = SCREEN_SCALE_X(dropShadowPosition * dirs16[i][0] * outlineStrength);
                    const float oy = SCREEN_SCALE_Y(dropShadowPosition * dirs16[i][1] * outlineStrength);

                    if (CFont::Slant != 0.0f)
                    {
                        CFont::SlantRefPoint->x += ox;
                        CFont::SlantRefPoint->y += oy;
                        CFont::PrintString(x + ox, y + oy, _start, _end, wrap);
                        CFont::SlantRefPoint->x -= ox;
                        CFont::SlantRefPoint->y -= oy;
                    }
                    else
                    {
                        CFont::PrintString(x + ox, y + oy, _start, _end, wrap);
                    }
                }

                inOutlinePass = false;

                CFont::Color = savedColor;
                CFont::m_FontIsBlip = savedIsBlip;
                CFont::m_FontOutlineSize = savedOutlineSize;
                CFont::m_FontShadow = savedShadow;
                CFont::SlantRefPoint = savedSlantRef;

                return_to(loc_719F30);
            });

            if (bAllowAltTabbingWithoutPausing)
            {
                //Windowed mode fix (from MTA sources)
                if ((GetWindowLong((HWND)RsGlobal->ps, GWL_STYLE) & WS_POPUP) == 0)
                {
                    // Disable MENU AFTER alt + tab
                    //0053BC72   C605 7B67BA00 01 MOV BYTE PTR DS:[BA677B],1
                    injector::WriteMemory<uint8_t>(0x53BC78, 0x00, true);

                    // ALLOW ALT+TABBING WITHOUT PAUSING
                    injector::MakeNOP(0x748A8D, 6, true);
                    injector::MakeJMP(0x6194A0, AllowMouseMovement, true);
                }
            }

            if (bDisableWhiteCrosshairDot)
            {
                injector::MakeNOP(0x58E2DD, 5, true);
            }

            if (nHideAABug)
            {
                auto pattern = hook::pattern("89 3D ? ? ? ? 89 3D ? ? ? ? 89 3D ? ? ? ? 8B F1");
                SelectedMultisamplingLevels = *pattern.get_first<int*>(2);

                WFP::onEndScene() += []()
                {
                    if (*SelectedMultisamplingLevels <= 1 || !nHideAABug)
                        return;

                    auto RwD3DDevice = *(IDirect3DDevice9**)pD3D9Device;

                    if (!RwD3DDevice)
                        return;

                    D3DCOLOR black = D3DCOLOR_ARGB(255, 0, 0, 0);

                    // Top 1px
                    D3DRECT topRect = { 0, -5, RsGlobal->width, 1 };
                    RwD3DDevice->Clear(1, &topRect, D3DCLEAR_TARGET, black, 1.0f, 0);

                    // Left 1px
                    D3DRECT leftRect = { -5, 0, 1, RsGlobal->height };
                    RwD3DDevice->Clear(1, &leftRect, D3DCLEAR_TARGET, black, 1.0f, 0);

                    if (nHideAABug > 1)
                    {
                        // Bottom 1px
                        D3DRECT bottomRect = { 0, RsGlobal->height - 1, RsGlobal->width, RsGlobal->height + 5 };
                        RwD3DDevice->Clear(1, &bottomRect, D3DCLEAR_TARGET, black, 1.0f, 0);

                        // Right 1px
                        D3DRECT rightRect = { RsGlobal->width - 1, 0, RsGlobal->width + 5, RsGlobal->height + 5 };
                        RwD3DDevice->Clear(1, &rightRect, D3DCLEAR_TARGET, black, 1.0f, 0);
                    }
                };
            }

            if (ReplaceTextShadowWithOutline)
            {
                auto pattern = hook::pattern("E8 ? ? ? ? 6A ? E8 ? ? ? ? 0F B6 7B");
                static auto SetDropShadowPosition = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), CFont::SetOutlinePosition);

                pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? 68 ? ? ? ? 8B CF E8 ? ? ? ? ? ? ? ? E8 ? ? ? ? 89 44 24 ? 43");
                hbPrintString.fun = injector::MakeCALL(pattern.get_first(), PrintString, true).get();

                // Controller setup text outlines
                pattern = hook::pattern("6A ? E8 ? ? ? ? 83 C4 ? 6A ? 8D 44 24 ? 50 B9 ? ? ? ? E8 ? ? ? ? ? ? 51 E8 ? ? ? ? 6A ? E8 ? ? ? ? ? ? ? ? ? ? 8A 86");
                if (!pattern.empty())
                    injector::WriteMemory<uint8_t>(pattern.get_first(1), 1, true);

                pattern = hook::pattern("6A ? E8 ? ? ? ? 83 C4 ? 68 ? ? ? ? 6A ? 6A ? 6A ? 8D 4C 24 ? E8 ? ? ? ? ? ? 51 E8 ? ? ? ? ? ? ? ? ? ? 83 C4");
                if (!pattern.empty())
                    injector::WriteMemory<uint8_t>(pattern.get_first(1), 1, true);
            }

            if (bVCSCamShake)
            {
                auto pattern = hook::pattern("E8 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 8B 96 ? ? ? ? 89 8E ? ? ? ? 8B CE 89 86");
                hbCalculateDerivedValues.fun = injector::MakeCALL(pattern.get_first(), CalculateDerivedValues, true).get();
            }
        };
    }
} Misc;
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

    void __cdecl SetOutlinePosition(char value)
    {
        if (ReplaceTextShadowWithOutline > 1)
            value += 1;

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
            }

            if (bVCSCamShake)
            {
                auto pattern = hook::pattern("E8 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 8B 96 ? ? ? ? 89 8E ? ? ? ? 8B CE 89 86");
                hbCalculateDerivedValues.fun = injector::MakeCALL(pattern.get_first(), CalculateDerivedValues, true).get();
            }
        };
    }
} Misc;
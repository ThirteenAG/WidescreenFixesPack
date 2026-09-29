module;

#include <stdafx.h>

export module e2mfc;

import ComVars;

namespace P_Driver
{
    enum ClearingMode
    {
        CLEAR_TARGET = 2,
    };

    void** m_initializedDriver = nullptr;
    void(__fastcall* clearScreen)(void* _this, void* edx, const RECT* rect, int32_t mode, const uint32_t* color) = nullptr;

    // The display size, what P_Driver::getWidth and getHeight return while no render target is set.
    // With one set (mirrors, shadows, post-processing) they return the size of that instead.
    void RefreshScreenResolution()
    {
        if (m_initializedDriver && *m_initializedDriver)
        {
            auto pDriver = static_cast<uint32_t*>(*m_initializedDriver);
            UpdateScreenResolution(pDriver[1], pDriver[2]);
        }
    }
}

export void RefreshScreenResolution()
{
    P_Driver::RefreshScreenResolution();
}

// Fills a rectangle of the back buffer (in pixels) with black, the way the game draws its cutscene bars
export void ClearScreenRect(int32_t left, int32_t top, int32_t right, int32_t bottom)
{
    if (!P_Driver::clearScreen || !P_Driver::m_initializedDriver || !*P_Driver::m_initializedDriver || right <= left || bottom <= top)
        return;

    RECT rect = { left, top, right, bottom };
    constexpr uint32_t black = 0xFF000000;
    P_Driver::clearScreen(*P_Driver::m_initializedDriver, nullptr, &rect, P_Driver::CLEAR_TARGET, &black);
}

// Black bars of the given widths (in pixels) on the left and right screen edges
export void DrawPillarboxBars(int32_t nLeftWidth, int32_t nRightWidth)
{
    ClearScreenRect(0, 0, nLeftWidth, Screen.nHeight);
    ClearScreenRect(Screen.nWidth - nRightWidth, 0, Screen.nWidth, Screen.nHeight);
}

namespace P_Camera
{
    enum
    {
        FLAGS = 0xE0,
        FLAG_PROJECTION_DIRTY = 0x40,
        TAN_HALF_FOV_X = 0x200,
        TAN_HALF_FOV_Y = 0x204,
    };

    // Same place the mobile release applies AspectRatioXMult/AspectRatioYMult. Every consumer of the
    // projection (P_Camera::prepare, clipping planes, getViewPlaneRay, getViewportCoordinate) reads
    // these two tangents, so this is the only change 3D needs. The 2D objects (P_Sprite, P_Text,
    // P_2DLineObject) compute their own tangents from the FOV and are left alone, which keeps them
    // in the centered 4:3 area with correct proportions.
    SafetyHookInline shValidate = {};
    void __fastcall validate(uint8_t* _this, void* edx)
    {
        bool bDirty = (*(uint16_t*)(_this + FLAGS) & FLAG_PROJECTION_DIRTY) != 0;
        shValidate.unsafe_fastcall(_this, edx);
        if (bDirty)
        {
            *(float*)(_this + TAN_HALF_FOV_X) *= Screen.fAspectScaleX;
            *(float*)(_this + TAN_HALF_FOV_Y) *= Screen.fAspectScaleY;
        }
    }

    // Cameras only recompute their projection when dirty, so the ones validated before the
    // resolution was known (or changed) have to be invalidated once.
    std::unordered_map<uint8_t*, uint32_t> CameraGenerations;
    SafetyHookInline shPrepare = {};
    void __fastcall prepare(uint8_t* _this, void* edx)
    {
        P_Driver::RefreshScreenResolution();

        auto& nGeneration = CameraGenerations[_this];
        if (nGeneration != Screen.nGeneration)
        {
            nGeneration = Screen.nGeneration;
            *(uint16_t*)(_this + FLAGS) |= FLAG_PROJECTION_DIRTY;
        }

        shPrepare.unsafe_fastcall(_this, edx);
    }
}

// P_BaseObject::executeHierarchy only draws objects whose executeAlways returns true, this is how
// the graphic novel cursor and controls are hidden
namespace P_Sprite
{
    SafetyHookInline shExecuteAlways = {};
    bool __fastcall executeAlways(uint8_t* _this, void* edx)
    {
        if (MaxPayne_GraphicNovelMode::IsSpriteHidden(_this))
            return false;
        return shExecuteAlways.unsafe_fastcall<bool>(_this, edx);
    }
}

namespace P_Text
{
    SafetyHookInline shExecuteAlways = {};
    bool __fastcall executeAlways(uint8_t* _this, void* edx)
    {
        if (MaxPayne_GraphicNovelMode::IsTextHidden(_this))
            return false;
        return shExecuteAlways.unsafe_fastcall<bool>(_this, edx);
    }
}

export void InitE2MFC()
{
    auto e2mfc = GetModuleHandle(L"e2mfc");

    P_Driver::m_initializedDriver = (void**)GetProcAddress(e2mfc, "?m_initializedDriver@P_Driver@@0PAV1@A");
    P_Driver::clearScreen = (decltype(P_Driver::clearScreen))GetProcAddress(e2mfc, "?clearScreen@P_Driver@@QAEXABUtagRECT@@W4ClearingMode@1@ABVG_Color@@@Z");

    P_Camera::shValidate = safetyhook::create_inline(GetProcAddress(e2mfc, "?validate@P_Camera@@QAEXXZ"), P_Camera::validate);
    P_Camera::shPrepare = safetyhook::create_inline(GetProcAddress(e2mfc, "?prepare@P_Camera@@QAEXXZ"), P_Camera::prepare);

    P_Sprite::shExecuteAlways = safetyhook::create_inline(GetProcAddress(e2mfc, "?executeAlways@P_Sprite@@UAE_NXZ"), P_Sprite::executeAlways);
    P_Text::shExecuteAlways = safetyhook::create_inline(GetProcAddress(e2mfc, "?executeAlways@P_Text@@MAE_NXZ"), P_Text::executeAlways);

    // Hud
    auto pattern = hook::module_pattern(e2mfc, "D9 05 ? ? ? ? D9 E0 D9 45 FC D8 25");
    static float* pHudElementPosX = *pattern.count(2).get(1).get<float*>(2); //0x10065190
    static float* pHudElementPosY = *pattern.count(2).get(1).get<float*>(22); //0x10065194
    struct P_HudPosHook
    {
        void operator()(injector::reg_pack& regs)
        {
            // pivot of the sprite, esi is P_Sprite, eax is the reference point for most of them
            float ElementPosX = *pHudElementPosX;
            float ElementPosY = *pHudElementPosY;
            float ElementNewPosX1 = ElementPosX;
            float ElementNewPosY1 = ElementPosY;
            float ElementNewPosX2 = ElementPosX;
            float ElementNewPosY2 = ElementPosY;

            if (ElementPosX == 7.0f) // bullet time meter
                ElementNewPosX1 = ElementPosX + Screen.fWidescreenHudOffset;
            else if (ElementPosX == 8.0f && regs.eax != 8) // bullet time overlay
                ElementNewPosX1 = ElementPosX + Screen.fWidescreenHudOffset;
            else if (ElementPosX == 12.0f) // painkillers
                ElementNewPosX1 = ElementPosX + Screen.fWidescreenHudOffset;
            else if (ElementPosX == 22.5f) // health bar and overlay
                ElementNewPosX1 = ElementPosX + Screen.fWidescreenHudOffset;
            else if (ElementPosX == 95.0f) // other weapons name
                ElementNewPosX1 = ElementPosX - Screen.fWidescreenHudOffset;
            else if (ElementPosX == 190.0f) // molotovs/grenades name pos
                ElementNewPosX1 = ElementPosX - Screen.fWidescreenHudOffset;

            ElementNewPosX2 = ElementNewPosX1;

            auto fWidth = *(float*)(regs.esi + P_Sprite::WIDTH);
            if (ElementPosX == 0.0f && ElementPosY == 0.0f && regs.eax == P_Sprite::REFERENCE_POINT_TOP_LEFT && fWidth == 640.0f) // fades, flashes and text backgrounds covering the whole 4:3 area
            {
                // at least 640 on each side like before, which also covers overlays that aren't at x = 0
                float fExtension = std::max(640.0f, Screen.fFullscreenExtension);
                ElementNewPosX1 = ElementPosX + fExtension;
                ElementNewPosX2 = ElementPosX - fExtension;
            }
            else if (ElementPosX == 100.0f && (ElementPosY == 0.0f || ElementPosY == 20.0f || ElementPosY == 220.0f)) // sniper scope borders left side
            {
                Screen.bDrawBordersToFillGap = true;
                ElementNewPosX1 += Screen.fFullscreenExtension;
            }
            else if (ElementPosX == 0.0f && (ElementPosY == 0.0f || ElementPosY == 20.0f || ElementPosY == 220.0f) && fWidth == 100.0f) // sniper scope borders right side
            {
                Screen.bDrawBordersToFillGap = true;
                ElementNewPosX2 -= Screen.fFullscreenExtension;
            }

            *(float*)(regs.ebp - 4) -= ElementNewPosX2;
            *(float*)(regs.ebp - 8) -= ElementNewPosY2;

            _asm
            {
                fld     dword ptr[ElementNewPosX1]
                fchs
                fld     dword ptr[ElementNewPosY1]
                fchs
            }
        }
    }; injector::MakeInline<P_HudPosHook>(pattern.count(2).get(1).get<uintptr_t>(0), pattern.count(2).get(1).get<uintptr_t>(40)); //1000856C

    pattern = hook::module_pattern(e2mfc, "D9 05 ? ? ? ? D8 8E 74 01 00 00");
    static auto pTextElementPosX = *pattern.get_first<TextCoords*>(2); //0x100647D0
    struct P_TextPosHook
    {
        void operator()(injector::reg_pack& regs)
        {
            auto TextPosX = pTextElementPosX->a;
            auto TextNewPosX = TextPosX;

            if ((pTextElementPosX->a == 0.0f || pTextElementPosX->a == -8.0f || pTextElementPosX->a == -16.0f || pTextElementPosX->a == -24.0f || pTextElementPosX->a == -32.0f) && pTextElementPosX->b == -10.5f && (pTextElementPosX->c == 8.0f || pTextElementPosX->c == 16.0f || pTextElementPosX->c == 24.0f || pTextElementPosX->c == 32.0f) && pTextElementPosX->d == 21) //ammo numbers(position depends on digits amount)
                TextNewPosX = TextPosX + Screen.fWidescreenHudOffset;

            _asm fld    dword ptr[TextNewPosX]
        }
    }; injector::MakeInline<P_TextPosHook>(pattern.get_first(0), pattern.get_first(6));

    static float TextPosX1, TextPosX2, TextPosY1;
    pattern = hook::module_pattern(e2mfc, "C7 45 D0 00 00 00 00 D9 5D"); //100045FC
    struct P_TextPosHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)(regs.ebp - 0x30) = 0.0f;
            TextPosX1 = *(float*)(regs.ebp - 0x28);
            TextPosY1 = *(float*)(regs.ebp - 0x2C);
        }
    }; injector::MakeInline<P_TextPosHook2>(pattern.get_first(0), pattern.get_first(7));

    pattern = hook::module_pattern(e2mfc, "89 41 08 D9 45 E4 D8 0D"); //0x10004693
    struct P_TextPosHook3
    {
        void operator()(injector::reg_pack& regs)
        {
            TextPosX2 = *(float*)(regs.ebp - 0x1C);

            if (TextPosX1 == (69.0f + Screen.fWidescreenHudOffset) && TextPosY1 == 457.0f) // painkillers amount number
                *(float*)(regs.ebp - 0x1C) += (24.0f * Screen.fWidescreenHudOffset);

            *(uint32_t*)(regs.ecx + 8) = regs.eax;
            auto ebp1C = *(float*)(regs.ebp - 0x1C);
            _asm fld  dword ptr[ebp1C]
        }
    }; injector::MakeInline<P_TextPosHook3>(pattern.get_first(0), pattern.get_first(6));

    //relocate dllmain code of e2_d3d8_driver_mfc
    pattern = hook::module_pattern(e2mfc, "C7 46 ? ? ? ? ? 8B 75 ? 89 07 EB 15 FF 15 ? ? ? ? 50 68 ? ? ? ? 53 E8 ? ? ? ? 83 C4 0C 8D 8D ? ? ? ? 51 56 FF 15 ? ? ? ? 85 C0 75 ? 56 FF 15 ? ? ? ? 8D 95");
    static auto LoadLibraryHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (GetModuleHandle(L"e2_d3d8_driver_mfc") == (HMODULE)regs.eax)
        {
            shDllMainHook.unsafe_stdcall<BOOL>((HMODULE)regs.eax, DLL_PROCESS_ATTACH, nullptr);
        }
    });

    pattern = hook::module_pattern(e2mfc, "51 FF D3 8B 76");
    static auto FreeLibraryHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (GetModuleHandle(L"e2_d3d8_driver_mfc") == (HMODULE)regs.ecx)
        {
            shDllMainHook.unsafe_stdcall<BOOL>((HMODULE)regs.ecx, DLL_PROCESS_DETACH, nullptr);
        }
    });
}

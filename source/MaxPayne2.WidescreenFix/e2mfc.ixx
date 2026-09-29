module;

#include <stdafx.h>
#include <unordered_set>

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

// Covers everything outside the centered 4:3 area, reaching nOverlap pixels into it
export void Draw4by3Borders(int32_t nOverlap = 0)
{
    auto nWidth = static_cast<int32_t>(Screen.fHudOffsetReal);
    if (nWidth > 0)
        DrawPillarboxBars(nWidth + nOverlap, nWidth + nOverlap);

    auto nHeight = static_cast<int32_t>(Screen.fHudOffsetRealY);
    if (nHeight > 0)
    {
        ClearScreenRect(0, 0, Screen.nWidth, nHeight + nOverlap);
        ClearScreenRect(0, Screen.nHeight - nHeight - nOverlap, Screen.nWidth, Screen.nHeight);
    }
}

namespace P_Camera
{
    enum
    {
        FLAGS = 0xEC,
        FLAG_PROJECTION_DIRTY = 0x40,
        TAN_HALF_FOV_X = 0x20C,
        TAN_HALF_FOV_Y = 0x210,
    };

    // Cameras of effects that draw a mesh over their whole viewport, like the post-processing quads
    // and the shadow maps. They need the original projection to keep covering it.
    std::unordered_set<uint8_t*> OriginalProjectionCameras;

    // Everything P_Camera derives its projection from (P_Camera::prepare, clipping planes,
    // getViewPlaneRay, getViewportCoordinate) reads these two tangents. Code outside the camera that
    // builds a frustum of its own (portals, mirrors) is scaled separately. The 2D objects (P_Sprite,
    // P_Text, P_2DLineObject) compute their own tangents from the FOV and are left alone, which keeps
    // them in the centered 4:3 area with correct proportions.
    SafetyHookInline shValidate = {};
    void __fastcall validate(uint8_t* _this, void* edx)
    {
        bool bDirty = (*(_this + FLAGS) & FLAG_PROJECTION_DIRTY) != 0;
        shValidate.unsafe_fastcall(_this, edx);
        if (bDirty && !OriginalProjectionCameras.contains(_this))
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
            *(_this + FLAGS) |= FLAG_PROJECTION_DIRTY;
        }

        shPrepare.unsafe_fastcall(_this, edx);
    }

    SafetyHookInline shDestructor = {};
    void __fastcall destructor(uint8_t* _this, void* edx)
    {
        OriginalProjectionCameras.erase(_this);
        CameraGenerations.erase(_this);
        shDestructor.unsafe_fastcall(_this, edx);
    }
}

export void KeepOriginalProjection(void* pCamera)
{
    auto _this = static_cast<uint8_t*>(pCamera);
    if (_this && P_Camera::OriginalProjectionCameras.insert(_this).second)
        *(_this + P_Camera::FLAGS) |= P_Camera::FLAG_PROJECTION_DIRTY;
}

// P_BaseObject::executeHierarchy only draws objects whose executeAlways returns true, this is how
// the graphic novel cursor and controls are hidden
namespace P_Sprite
{
    SafetyHookInline shExecuteAlways = {};
    bool __fastcall executeAlways(uint8_t* _this, void* edx)
    {
        if (MP_GraphicNovelMode::IsSpriteHidden(_this))
            return false;
        return shExecuteAlways.unsafe_fastcall<bool>(_this, edx);
    }
}

namespace P_Text
{
    SafetyHookInline shExecuteAlways = {};
    bool __fastcall executeAlways(uint8_t* _this, void* edx)
    {
        if (MP_GraphicNovelMode::IsTextHidden(_this))
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
    P_Camera::shDestructor = safetyhook::create_inline(GetProcAddress(e2mfc, "??1P_Camera@@UAE@XZ"), P_Camera::destructor);

    P_Sprite::shExecuteAlways = safetyhook::create_inline(GetProcAddress(e2mfc, "?executeAlways@P_Sprite@@UAE_NXZ"), P_Sprite::executeAlways);
    P_Text::shExecuteAlways = safetyhook::create_inline(GetProcAddress(e2mfc, "?executeAlways@P_Text@@MAE_NXZ"), P_Text::executeAlways);

    // Hud, in P_Sprite::executeAlways after the corners are placed around the pivot:
    // [esp+30h] left, [esp+1Ch] right, [esp+2Ch] top, [esp+20h] bottom
    auto pattern = hook::module_pattern(e2mfc, "D8 25 ? ? ? ? 89 44 24 44 83 EC 08");
    static auto pHudElementPosX = *pattern.get_first<float*>(2); //0x10061134
    pattern = hook::module_pattern(e2mfc, "D8 25 ? ? ? ? D9 5C 24 20 D9 44 24 4C D9 E0 DD 1C 24 E8");
    static auto pHudElementPosY = *pattern.get_first<float*>(2); //0x10061138
    static auto P_SpriteExecuteAlwaysHook = safetyhook::create_mid(pattern.get_first(10), [](SafetyHookContext& regs) //0x1000F2FC
    {
        // pivot of the sprite, esi is P_Sprite
        float ElementPosX = *pHudElementPosX;
        float ElementPosY = *pHudElementPosY;
        float ElementNewPosX1 = ElementPosX;
        float ElementNewPosY1 = ElementPosY;
        float ElementNewPosX2 = ElementPosX;
        float ElementNewPosY2 = ElementPosY;

        if (!MP_GameMode::IsSniperScopeOn())
        {
            if (ElementPosX == 7.0f) // bullet time meter
                ElementNewPosX1 = ElementPosX + Screen.fWidescreenHudOffset;
            else if (ElementPosX == 8.0f && ElementPosY != 8.0f) // bullet time overlay
                ElementNewPosX1 = ElementPosX + Screen.fWidescreenHudOffset;
            else if (ElementPosX == 12.0f) // painkillers
                ElementNewPosX1 = ElementPosX + Screen.fWidescreenHudOffset;
            else if (ElementPosX == 22.5f) // health bar and overlay
                ElementNewPosX1 = ElementPosX + Screen.fWidescreenHudOffset;
            else if (ElementPosX == 96.0f) // other weapons name
                ElementNewPosX1 = ElementPosX - Screen.fWidescreenHudOffset;
            else if (ElementPosX == 192.0f) // molotovs/grenades name pos
                ElementNewPosX1 = ElementPosX - Screen.fWidescreenHudOffset;
        }

        ElementNewPosX2 = ElementNewPosX1;

        if ((uint8_t*)regs.esi == MaxPayne_HUDFadeLayer::pSprite) // fades and flashes, both HUD and mode switch ones
        {
            ElementNewPosX1 = ElementPosX + Screen.fFullscreenExtensionX;
            ElementNewPosX2 = ElementPosX - Screen.fFullscreenExtensionX;
            ElementNewPosY1 = ElementPosY + Screen.fFullscreenExtensionY;
            ElementNewPosY2 = ElementPosY - Screen.fFullscreenExtensionY;
        }

        *(float*)(regs.esp + 0x30) -= ElementNewPosX1 - ElementPosX;
        *(float*)(regs.esp + 0x1C) -= ElementNewPosX2 - ElementPosX;
        *(float*)(regs.esp + 0x2C) -= ElementNewPosY1 - ElementPosY;
        *(float*)(regs.esp + 0x20) -= ElementNewPosY2 - ElementPosY;
    });

    pattern = hook::module_pattern(e2mfc, "D9 05 ? ? ? ? D8 8B 98");
    static auto pTextElementPosX = *pattern.get_first<TextCoords*>(2); //0x10060374
    struct P_TextPosHook
    {
        void operator()(injector::reg_pack& regs)
        {
            float TextUnkVal = *(float*)(*(uintptr_t*)(regs.esp + 0xC) + 0x5C);
            float TextPosX = pTextElementPosX->a;
            float TextNewPosX = TextPosX;
            if (!MP_GameMode::IsSniperScopeOn())
            {
                if ((pTextElementPosX->a == 0.0f || pTextElementPosX->a == -8.0f || pTextElementPosX->a == -16.0f || pTextElementPosX->a == -24.0f || pTextElementPosX->a == -32.0f) && pTextElementPosX->b == -10.5f && (pTextElementPosX->c == 8.0f || pTextElementPosX->c == 16.0f || pTextElementPosX->c == 24.0f || pTextElementPosX->c == 32.0f || pTextElementPosX->c == 57.0f) && pTextElementPosX->d == 21) //ammo numbers(position depends on digits amount)
                {
                    if (TextUnkVal < 0.0f)
                        TextNewPosX = TextPosX - Screen.fWidescreenHudOffset;
                    else
                        TextNewPosX = TextPosX + Screen.fWidescreenHudOffset;
                }
            }
            __asm fld dword ptr[TextNewPosX]
        }
    }; injector::MakeInline<P_TextPosHook>(pattern.get_first(0), pattern.get_first(6)); //0x1000AACC

    //relocate dllmain code of e2_d3d8_driver_mfc
    pattern = hook::module_pattern(e2mfc, "8B 55 ? 40 C7 47 ? ? ? ? ? 89 47 ? 8B 7D ? 89 0A EB 15 FF 15 ? ? ? ? 50 68 ? ? ? ? 56 E8 ? ? ? ? 83 C4 0C 8D 85 ? ? ? ? 50 57 FF 15 ? ? ? ? 85 C0 75 ? 57 FF 15 ? ? ? ? 8D 8D");
    static auto LoadLibraryHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (GetModuleHandle(L"e2_d3d8_driver_mfc") == (HMODULE)regs.ecx)
        {
            shDllMainHook.unsafe_stdcall<BOOL>((HMODULE)regs.ecx, DLL_PROCESS_ATTACH, nullptr);
        }
    });

    pattern = hook::module_pattern(e2mfc, "52 FF D7 8B 76");
    static auto FreeLibraryHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (GetModuleHandle(L"e2_d3d8_driver_mfc") == (HMODULE)regs.edx)
        {
            shDllMainHook.unsafe_stdcall<BOOL>((HMODULE)regs.edx, DLL_PROCESS_DETACH, nullptr);
        }
    });
}

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

// Widescreen HUD. hud.txt places every HUD element in the 640x480 area, which stays centered on
// wider screens. The elements of the blocks below belong to a screen edge and are drawn that much
// closer to it. Their P_Sprite and P_Text objects are told apart by the hud.txt block MP_HUDMode
// creates them in.
namespace WidescreenHud
{
    // Blocks from the top of hud.txt and the screen edge their elements belong to, -1 left, 1 right.
    // The rest stays centered: WeaponInventory, Inventory/PauseSlot, Inventory/SubtitleSlot, LookAt,
    // Weapons/Overlay (sniper scopes) and IntroductionSprites.
    constexpr std::pair<std::string_view, float> Anchors[] =
    {
        { "Health", -1.0f },
        { "SlowMotion", -1.0f },
        { "PrintTip", -1.0f },
        { "Objectives", -1.0f },
        { "Weapons/Painkiller", -1.0f },
        { "Print", 1.0f },
        { "Inventory/Slot0", 1.0f },
        { "Inventory/Slot1", 1.0f },
        { "Inventory/Slot2", 1.0f },
        { "TimedMode", 1.0f },
        { "Weapons/AmmoInClipsText", 1.0f },
        { "Weapons/AmmoInPocketText", 1.0f },
        { "Weapons/SecondaryAmmoText", 1.0f },
        { "Weapons/ActiveWeapons", 1.0f },
    };

    // The R_Script reading hud.txt, while it exists
    uint8_t* pScript = nullptr;

    // P_Sprite and P_Text objects of anchored elements, with their edge
    std::unordered_map<uint8_t*, float> Elements;

    // The blocks the script is in, like "Weapons/Painkiller/AmountText". R_Script keeps the current
    // one at +4, a block has its name at +0 and its parent at +0x14, up to the block of the file itself.
    std::string GetPath()
    {
        std::string Path;
        for (auto pBlock = *(uint8_t**)(pScript + 4); pBlock && *(uint8_t**)(pBlock + 0x14); pBlock = *(uint8_t**)(pBlock + 0x14))
        {
            std::string Name = *(const char**)pBlock;
            Path = Path.empty() ? Name : Name + '/' + Path;
        }
        return Path;
    }

    // Whether the path is the block or inside it, ignoring case like R_Script
    bool IsIn(std::string_view Path, std::string_view Block)
    {
        return Path.size() >= Block.size() && _strnicmp(Path.data(), Block.data(), Block.size()) == 0 && (Path.size() == Block.size() || Path[Block.size()] == '/');
    }

    bool IsSame(std::string_view Path, std::string_view Block)
    {
        return Path.size() == Block.size() && IsIn(Path, Block);
    }

    // Sniper scopes. An [Overlay] is one 640x480 SpriteWindow showing the middle of a scope image,
    // whose dark surround carries on to the edges of the texture. MP_HUDMode is made to create two
    // more SpriteWindows from that block, and those cover the screen beyond the overlay with the
    // outer band of the surround, stretched. The game shows, fades and deletes them with the overlay.
    namespace ScopeFill
    {
        constexpr uint32_t PIECES = 3;   // the overlay's own SpriteWindow and the two around it
        constexpr float fBand = 0.043f;  // width of the stretched band, relative to the overlay's texture window

        uint8_t* pOverlay = nullptr;     // the [Overlay] block its SpriteWindows are created for
        uint32_t nPiece = 0;             // the SpriteWindow being created, 0 is the overlay's own
        bool bEveryOverlay = false;      // whether every overlay gets them

        // P_Sprite objects of the pieces around overlays, 1 left (or top), 2 right (or bottom)
        std::unordered_map<uint8_t*, uint32_t> Pieces;

        // Whether the scope covers the whole screen
        bool IsFilled()
        {
            return bEveryOverlay && !Pieces.empty();
        }

        // P_Sprite::executeAlways places the sprite by this point of it
        std::pair<float, float> GetPivot(uint32_t nReferencePoint, float fWidth, float fHeight)
        {
            switch (nReferencePoint)
            {
            case 0: return { 0.0f, fHeight };                 // down left
            case 1: return { 0.0f, fHeight / 2.0f };          // left
            case 3: return { fWidth / 2.0f, 0.0f };           // up
            case 4: return { fWidth, 0.0f };                  // up right
            case 5: return { fWidth, fHeight / 2.0f };        // right
            case 6: return { fWidth, fHeight };               // down right
            case 7: return { fWidth / 2.0f, fHeight };        // down
            case 8: return { fWidth / 2.0f, fHeight / 2.0f }; // center
            default: return { 0.0f, 0.0f };                   // up left
            }
        }

        // Runs P_Sprite::executeAlways of a piece placed between the overlay and the screen edge.
        // The piece was created from the overlay's block, so it starts out with the overlay's
        // placement and texture window.
        template<typename Fn>
        bool Execute(uint8_t* pSprite, uint32_t nPiece, Fn&& fnExecuteAlways)
        {
            bool bSides = Screen.fFullscreenExtensionX > 0.0f;
            if (!bSides && Screen.fFullscreenExtensionY <= 0.0f)
                return false; // 4:3

            // put back afterwards
            constexpr ptrdiff_t nFirst = P_Sprite::WIDTH;
            constexpr ptrdiff_t nEnd = P_Sprite::SCREEN_POSITION_Y + sizeof(float);
            std::array<uint8_t, nEnd - nFirst> Saved;
            std::memcpy(Saved.data(), pSprite + nFirst, Saved.size());

            auto& fWidth = *(float*)(pSprite + P_Sprite::WIDTH);
            auto& fHeight = *(float*)(pSprite + P_Sprite::HEIGHT);
            auto& nReferencePoint = *(uint32_t*)(pSprite + P_Sprite::REFERENCE_POINT);
            auto UV = (float*)(pSprite + P_Sprite::UV);
            auto& fX = *(float*)(pSprite + P_Sprite::SCREEN_POSITION_X);
            auto& fY = *(float*)(pSprite + P_Sprite::SCREEN_POSITION_Y);

            auto [fPivotX, fPivotY] = GetPivot(nReferencePoint, fWidth, fHeight);
            float fLeft = fX - fPivotX;
            float fTop = fY - fPivotY;
            float fRight = fLeft + fWidth;
            float fBottom = fTop + fHeight;
            float fU0 = UV[0], fV0 = UV[1], fU1 = UV[4], fV1 = UV[5];
            bool bFirst = nPiece == 1;

            // The band lies just outside the overlay's texture window, so the piece starts with the
            // texels the overlay ends with. One unit past the screen edge makes sure its pixels are covered.
            float fBandU0 = fU0, fBandU1 = fU1, fBandV0 = fV0, fBandV1 = fV1;
            if (bSides)
            {
                float fScreenEdge = bFirst ? -Screen.fFullscreenExtensionX - 1.0f : 640.0f + Screen.fFullscreenExtensionX + 1.0f;
                float fBandWidth = std::min(fBand * (fU1 - fU0), bFirst ? fU0 : 1.0f - fU1);
                fX = bFirst ? fScreenEdge : fRight;
                fY = fTop;
                fWidth = bFirst ? fLeft - fScreenEdge : fScreenEdge - fRight;
                fBandU0 = bFirst ? fU0 - fBandWidth : fU1;
                fBandU1 = bFirst ? fU0 : fU1 + fBandWidth;
            }
            else
            {
                float fScreenEdge = bFirst ? -Screen.fFullscreenExtensionY - 1.0f : 480.0f + Screen.fFullscreenExtensionY + 1.0f;
                float fBandHeight = std::min(fBand * (fV1 - fV0), bFirst ? fV0 : 1.0f - fV1);
                fX = fLeft;
                fY = bFirst ? fScreenEdge : fBottom;
                fHeight = bFirst ? fTop - fScreenEdge : fScreenEdge - fBottom;
                fBandV0 = bFirst ? fV0 - fBandHeight : fV1;
                fBandV1 = bFirst ? fV0 : fV1 + fBandHeight;
            }

            nReferencePoint = P_Sprite::REFERENCE_POINT_TOP_LEFT;
            float Corners[] = { fBandU0, fBandV0, fBandU1, fBandV0, fBandU1, fBandV1, fBandU0, fBandV1 };
            std::memcpy(UV, Corners, sizeof(Corners));

            bool bResult = fWidth > 0.0f && fHeight > 0.0f && fnExecuteAlways();
            std::memcpy(pSprite + nFirst, Saved.data(), Saved.size());
            return bResult;
        }
    }

    // Called for every P_Sprite and P_Text constructed
    void Register(uint8_t* pObject)
    {
        if (!pScript)
            return;

        auto Path = GetPath();
        if (ScopeFill::nPiece != 0 && IsSame(Path, "Weapons/Overlay/SpriteWindow"))
        {
            ScopeFill::Pieces[pObject] = ScopeFill::nPiece;
            return;
        }

        for (auto& [Block, fDirection] : Anchors)
        {
            if (IsIn(Path, Block))
            {
                Elements[pObject] = fDirection;
                return;
            }
        }
    }

    // Called for every P_Sprite and P_Text destroyed
    void Unregister(uint8_t* pObject)
    {
        if (!Elements.empty())
            Elements.erase(pObject);
        if (!ScopeFill::Pieces.empty())
            ScopeFill::Pieces.erase(pObject);
    }

    // How far the object is moved while it's drawn, in 640x480 virtual units
    float GetOffset(uint8_t* pObject)
    {
        // a scope that isn't filled is drawn with borders around the 4:3 area, see X_ModeSwitch::render
        if (Elements.empty() || Screen.fWidescreenHudOffset == 0.0f || (MP_GameMode::IsSniperScopeOn() && !ScopeFill::IsFilled()))
            return 0.0f;

        auto it = Elements.find(pObject);
        return it != Elements.end() ? it->second * Screen.fWidescreenHudOffset : 0.0f;
    }

    // Runs executeAlways of a P_Sprite or P_Text with the object moved, nPositionX is where its
    // screen position is
    template<typename Fn>
    bool ExecuteMoved(uint8_t* pObject, ptrdiff_t nPositionX, Fn&& fnExecuteAlways)
    {
        float fOffset = GetOffset(pObject);
        if (fOffset == 0.0f)
            return fnExecuteAlways();

        auto& fPositionX = *(float*)(pObject + nPositionX);
        float fOriginalX = fPositionX;
        fPositionX = fOriginalX + fOffset;
        bool bResult = fnExecuteAlways();
        fPositionX = fOriginalX;
        return bResult;
    }
}

namespace R_Script
{
    SafetyHookInline shConstructor = {};
    uint8_t* __fastcall constructor(uint8_t* _this, void* edx, const char* szFileName)
    {
        auto pResult = shConstructor.unsafe_fastcall<uint8_t*>(_this, edx, szFileName);
        if (szFileName && _stricmp(szFileName, "hud.txt") == 0)
        {
            WidescreenHud::pScript = _this;
            WidescreenHud::ScopeFill::pOverlay = nullptr;
            WidescreenHud::ScopeFill::nPiece = 0;
            WidescreenHud::ScopeFill::bEveryOverlay = true;
        }
        return pResult;
    }

    SafetyHookInline shDestructor = {};
    void __fastcall destructor(uint8_t* _this, void* edx)
    {
        if (_this == WidescreenHud::pScript)
            WidescreenHud::pScript = nullptr;
        shDestructor.unsafe_fastcall(_this, edx);
    }

    // MP_HUDMode creates as many SpriteWindows for an [Overlay] as this counts...
    SafetyHookInline shCount = {};
    uint32_t __fastcall count(uint8_t* _this, void* edx, const char* szBlockName)
    {
        auto nCount = shCount.unsafe_fastcall<uint32_t>(_this, edx, szBlockName);
        if (_this == WidescreenHud::pScript && szBlockName && _stricmp(szBlockName, "SpriteWindow") == 0 && WidescreenHud::IsSame(WidescreenHud::GetPath(), "Weapons/Overlay"))
        {
            if (nCount == 1)
            {
                WidescreenHud::ScopeFill::pOverlay = *(uint8_t**)(_this + 4);
                return WidescreenHud::ScopeFill::PIECES;
            }
            WidescreenHud::ScopeFill::bEveryOverlay = false; // made of several, left as it is
        }
        return nCount;
    }

    // ...and locks each of them, the pieces around the overlay are read from the one there is
    SafetyHookInline shLock = {};
    void __fastcall lock(uint8_t* _this, void* edx, const char* szBlockName, uint32_t nIndex)
    {
        if (_this == WidescreenHud::pScript)
        {
            WidescreenHud::ScopeFill::nPiece = 0;
            if (nIndex > 0 && nIndex < WidescreenHud::ScopeFill::PIECES && *(uint8_t**)(_this + 4) == WidescreenHud::ScopeFill::pOverlay && szBlockName && _stricmp(szBlockName, "SpriteWindow") == 0)
            {
                WidescreenHud::ScopeFill::nPiece = nIndex;
                nIndex = 0;
            }
        }
        shLock.unsafe_fastcall(_this, edx, szBlockName, nIndex);
    }
}

// P_BaseObject::executeHierarchy only draws objects whose executeAlways returns true, this is how
// the graphic novel cursor and controls are hidden. executeAlways also places the object around
// its screen position, the widescreen HUD moves that position for the call.
namespace P_Sprite
{
    SafetyHookInline shConstructor = {};
    uint8_t* __fastcall constructor(uint8_t* _this, void* edx, void* pMaterial, float fWidth, float fHeight, uint32_t nReferencePoint)
    {
        auto pResult = shConstructor.unsafe_fastcall<uint8_t*>(_this, edx, pMaterial, fWidth, fHeight, nReferencePoint);
        WidescreenHud::Register(_this);
        return pResult;
    }

    SafetyHookInline shDestructor = {};
    void __fastcall destructor(uint8_t* _this, void* edx)
    {
        WidescreenHud::Unregister(_this);
        shDestructor.unsafe_fastcall(_this, edx);
    }

    SafetyHookInline shExecuteAlways = {};
    bool __fastcall executeAlways(uint8_t* _this, void* edx)
    {
        if (MP_GraphicNovelMode::IsSpriteHidden(_this))
            return false;

        auto fnExecuteAlways = [&] { return shExecuteAlways.unsafe_fastcall<bool>(_this, edx); };
        if (auto it = WidescreenHud::ScopeFill::Pieces.find(_this); it != WidescreenHud::ScopeFill::Pieces.end())
            return WidescreenHud::ScopeFill::Execute(_this, it->second, fnExecuteAlways);
        return WidescreenHud::ExecuteMoved(_this, SCREEN_POSITION_X, fnExecuteAlways);
    }
}

// Whether the sniper scope covers the whole screen, see WidescreenHud::ScopeFill
export bool IsSniperScopeFilled()
{
    return WidescreenHud::ScopeFill::IsFilled();
}

namespace P_Text
{
    enum
    {
        POSITION_X = 0x190, // of the reference point, in 640x480 virtual units
    };

    SafetyHookInline shConstructor = {};
    uint8_t* __fastcall constructor(uint8_t* _this, void* edx, void* pFont, uint32_t nReferencePoint)
    {
        auto pResult = shConstructor.unsafe_fastcall<uint8_t*>(_this, edx, pFont, nReferencePoint);
        WidescreenHud::Register(_this);
        return pResult;
    }

    SafetyHookInline shDestructor = {};
    void __fastcall destructor(uint8_t* _this, void* edx)
    {
        WidescreenHud::Unregister(_this);
        shDestructor.unsafe_fastcall(_this, edx);
    }

    SafetyHookInline shExecuteAlways = {};
    bool __fastcall executeAlways(uint8_t* _this, void* edx)
    {
        if (MP_GraphicNovelMode::IsTextHidden(_this))
            return false;
        return WidescreenHud::ExecuteMoved(_this, POSITION_X, [&] { return shExecuteAlways.unsafe_fastcall<bool>(_this, edx); });
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
    P_Camera::setFOV = (decltype(P_Camera::setFOV))GetProcAddress(e2mfc, "?setFOV@P_Camera@@QAEXM@Z");
    P_BaseObject::invalidateMatrices = (decltype(P_BaseObject::invalidateMatrices))GetProcAddress(e2mfc, "?invalidateMatrices@P_BaseObject@@IAEXXZ");
    P_BaseObject::calculateObjectToWorldMatrix = (decltype(P_BaseObject::calculateObjectToWorldMatrix))GetProcAddress(e2mfc, "?calculateObjectToWorldMatrix@P_BaseObject@@IAEXXZ");

    P_Sprite::shExecuteAlways = safetyhook::create_inline(GetProcAddress(e2mfc, "?executeAlways@P_Sprite@@UAE_NXZ"), P_Sprite::executeAlways);
    P_Text::shExecuteAlways = safetyhook::create_inline(GetProcAddress(e2mfc, "?executeAlways@P_Text@@MAE_NXZ"), P_Text::executeAlways);

    // Widescreen HUD, see WidescreenHud. rlmfc is loaded before e2mfc, which imports it.
    auto rlmfc = GetModuleHandle(L"rlmfc");
    R_Script::shConstructor = safetyhook::create_inline(GetProcAddress(rlmfc, "??0R_Script@@QAE@PBD@Z"), R_Script::constructor);
    R_Script::shDestructor = safetyhook::create_inline(GetProcAddress(rlmfc, "??1R_Script@@QAE@XZ"), R_Script::destructor);
    R_Script::shCount = safetyhook::create_inline(GetProcAddress(rlmfc, "?count@R_Script@@QBEIPBD@Z"), R_Script::count);
    R_Script::shLock = safetyhook::create_inline(GetProcAddress(rlmfc, "?lock@R_Script@@QAEXPBDI@Z"), R_Script::lock);
    P_Sprite::shConstructor = safetyhook::create_inline(GetProcAddress(e2mfc, "??0P_Sprite@@QAE@PAVP_Material@@MMW4ReferencePoint@P_BitmapInterface@@@Z"), P_Sprite::constructor);
    P_Sprite::shDestructor = safetyhook::create_inline(GetProcAddress(e2mfc, "??1P_Sprite@@UAE@XZ"), P_Sprite::destructor);
    P_Text::shConstructor = safetyhook::create_inline(GetProcAddress(e2mfc, "??0P_Text@@QAE@PAVP_Font@@W4ReferencePoint@0@@Z"), P_Text::constructor);
    P_Text::shDestructor = safetyhook::create_inline(GetProcAddress(e2mfc, "??1P_Text@@UAE@XZ"), P_Text::destructor);

    // Fades and flashes, both HUD and mode switch ones, cover the whole screen instead of the 4:3
    // area. In P_Sprite::executeAlways after the corners are placed around the pivot:
    // [esp+30h] left, [esp+1Ch] right, [esp+2Ch] top, [esp+20h] bottom
    auto pattern = hook::module_pattern(e2mfc, "D8 25 ? ? ? ? D9 5C 24 20 D9 44 24 4C D9 E0 DD 1C 24 E8");
    static auto P_SpriteExecuteAlwaysHook = safetyhook::create_mid(pattern.get_first(10), [](SafetyHookContext& regs) //0x1000F2FC
    {
        if ((uint8_t*)regs.esi == MaxPayne_HUDFadeLayer::pSprite)
        {
            *(float*)(regs.esp + 0x30) -= Screen.fFullscreenExtensionX;
            *(float*)(regs.esp + 0x1C) += Screen.fFullscreenExtensionX;
            *(float*)(regs.esp + 0x2C) -= Screen.fFullscreenExtensionY;
            *(float*)(regs.esp + 0x20) += Screen.fFullscreenExtensionY;
        }
    });

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

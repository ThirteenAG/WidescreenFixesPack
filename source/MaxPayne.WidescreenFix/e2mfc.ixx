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

    // The game asks for vertical sync whenever it opens a fullscreen mode, which P_D3D turns into
    // D3DPRESENT_INTERVAL_ONE, and into D3DPRESENT_INTERVAL_IMMEDIATE without it. Windowed modes
    // never wait for it, like Direct3D 8 always presents there.
    SafetyHookInline shSetFullscreenMode = {};
    void __fastcall setFullscreenMode(void* _this, void* edx, HWND hWnd, uint32_t nWidth, uint32_t nHeight, uint32_t nBitDepth, int32_t nBuffering, uint32_t nRefreshRate, const void* pZBufferFormat, bool bFlag, uint32_t nMultisampleType, bool bVSync, bool bLockableBackBuffer)
    {
        shSetFullscreenMode.unsafe_fastcall(_this, edx, hWnd, nWidth, nHeight, nBitDepth, nBuffering, nRefreshRate, pZBufferFormat, bFlag, nMultisampleType, false, bLockableBackBuffer);
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

// Widescreen HUD. hud.txt places every HUD element in the 640x480 area, which stays centered on
// wider screens. The elements of the blocks below belong to a screen edge and are drawn that much
// closer to it. Their P_Sprite and P_Text objects are told apart by the hud.txt block
// MaxPayne_HUDMode creates them in.
namespace WidescreenHud
{
    // Blocks from the top of hud.txt and the screen edge their elements belong to, -1 left, 1 right.
    // The rest stays centered: WeaponInventory, Inventory/PauseSlot, LookAt and Weapons/Overlay (sniper scope).
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
        { "Weapons/ActiveWeapon", 1.0f },
    };

    // The R_Script reading hud.txt, while it exists
    uint8_t* pScript = nullptr;

    // P_Sprite and P_Text objects of anchored elements, with their edge
    std::unordered_map<uint8_t*, float> Elements;

    // P_Sprite objects the sniper scope is made of
    std::unordered_set<uint8_t*> ScopePieces;

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

    // Called for every P_Sprite and P_Text constructed
    void Register(uint8_t* pObject)
    {
        if (!pScript)
            return;

        auto Path = GetPath();
        if (IsIn(Path, "Weapons/Overlay"))
        {
            ScopePieces.insert(pObject);
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
        if (!ScopePieces.empty())
            ScopePieces.erase(pObject);
    }

    // How far the object is moved while it's drawn, in 640x480 virtual units
    float GetOffset(uint8_t* pObject)
    {
        if (Elements.empty() || Screen.fWidescreenHudOffset == 0.0f)
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
            WidescreenHud::pScript = _this;
        return pResult;
    }

    SafetyHookInline shDestructor = {};
    void __fastcall destructor(uint8_t* _this, void* edx)
    {
        if (_this == WidescreenHud::pScript)
            WidescreenHud::pScript = nullptr;
        shDestructor.unsafe_fastcall(_this, edx);
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
        if (MaxPayne_GraphicNovelMode::IsSpriteHidden(_this))
            return false;
        return WidescreenHud::ExecuteMoved(_this, SCREEN_POSITION_X, [&] { return shExecuteAlways.unsafe_fastcall<bool>(_this, edx); });
    }
}

namespace P_Text
{
    enum
    {
        POSITION_X = 0x16C, // of the reference point, in 640x480 virtual units
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
        if (MaxPayne_GraphicNovelMode::IsTextHidden(_this))
            return false;
        return WidescreenHud::ExecuteMoved(_this, POSITION_X, [&] { return shExecuteAlways.unsafe_fastcall<bool>(_this, edx); });
    }
}

export void InitE2MFC()
{
    auto e2mfc = GetModuleHandle(L"e2mfc");

    P_Driver::m_initializedDriver = (void**)GetProcAddress(e2mfc, "?m_initializedDriver@P_Driver@@0PAV1@A");
    P_Driver::clearScreen = (decltype(P_Driver::clearScreen))GetProcAddress(e2mfc, "?clearScreen@P_Driver@@QAEXABUtagRECT@@W4ClearingMode@1@ABVG_Color@@@Z");

    CIniReader iniReader("");
    if (iniReader.ReadInteger("GRAPHICS", "VSync", 1) == 0)
        P_Driver::shSetFullscreenMode = safetyhook::create_inline(GetProcAddress(e2mfc, "?setFullscreenMode@P_Driver@@QAEXPAUHWND__@@IIIW4Buffering@1@IPBVP_ZBufferFormat@@_NI33@Z"), P_Driver::setFullscreenMode);

    P_Camera::shValidate = safetyhook::create_inline(GetProcAddress(e2mfc, "?validate@P_Camera@@QAEXXZ"), P_Camera::validate);
    P_Camera::shPrepare = safetyhook::create_inline(GetProcAddress(e2mfc, "?prepare@P_Camera@@QAEXXZ"), P_Camera::prepare);
    P_BaseObject::invalidateMatrices = (decltype(P_BaseObject::invalidateMatrices))GetProcAddress(e2mfc, "?invalidateMatrices@P_BaseObject@@IAEXXZ");
    P_BaseObject::calculateObjectToWorldMatrix = (decltype(P_BaseObject::calculateObjectToWorldMatrix))GetProcAddress(e2mfc, "?calculateObjectToWorldMatrix@P_BaseObject@@IAEXXZ");

    P_Sprite::shExecuteAlways = safetyhook::create_inline(GetProcAddress(e2mfc, "?executeAlways@P_Sprite@@UAE_NXZ"), P_Sprite::executeAlways);
    P_Text::shExecuteAlways = safetyhook::create_inline(GetProcAddress(e2mfc, "?executeAlways@P_Text@@MAE_NXZ"), P_Text::executeAlways);

    // Widescreen HUD, see WidescreenHud. rlmfc is loaded before e2mfc, which imports it.
    auto rlmfc = GetModuleHandle(L"rlmfc");
    R_Script::shConstructor = safetyhook::create_inline(GetProcAddress(rlmfc, "??0R_Script@@QAE@PBD@Z"), R_Script::constructor);
    R_Script::shDestructor = safetyhook::create_inline(GetProcAddress(rlmfc, "??1R_Script@@QAE@XZ"), R_Script::destructor);
    P_Sprite::shConstructor = safetyhook::create_inline(GetProcAddress(e2mfc, "??0P_Sprite@@QAE@PAVP_Material@@MMW4ReferencePoint@P_BitmapInterface@@@Z"), P_Sprite::constructor);
    P_Sprite::shDestructor = safetyhook::create_inline(GetProcAddress(e2mfc, "??1P_Sprite@@UAE@XZ"), P_Sprite::destructor);
    P_Text::shConstructor = safetyhook::create_inline(GetProcAddress(e2mfc, "??0P_Text@@QAE@PAVP_Font@@W4ReferencePoint@0@@Z"), P_Text::constructor);
    P_Text::shDestructor = safetyhook::create_inline(GetProcAddress(e2mfc, "??1P_Text@@UAE@XZ"), P_Text::destructor);

    // Fades and the sniper scope, in P_Sprite::executeAlways where the pivot is subtracted
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

            auto fWidth = *(float*)(regs.esi + P_Sprite::WIDTH);
            bool bScopePiece = WidescreenHud::ScopePieces.contains((uint8_t*)regs.esi);
            if (ElementPosX == 0.0f && ElementPosY == 0.0f && regs.eax == P_Sprite::REFERENCE_POINT_TOP_LEFT && fWidth == 640.0f) // fades, flashes and text backgrounds covering the whole 4:3 area
            {
                // at least 640 on each side like before, which also covers overlays that aren't at x = 0
                float fExtension = std::max(640.0f, Screen.fFullscreenExtension);
                ElementNewPosX1 = ElementPosX + fExtension;
                ElementNewPosX2 = ElementPosX - fExtension;
            }
            else if (bScopePiece && ElementPosX == 100.0f && (ElementPosY == 0.0f || ElementPosY == 20.0f || ElementPosY == 220.0f)) // sniper scope borders left side
            {
                Screen.bDrawBordersToFillGap = true;
                ElementNewPosX1 += Screen.fFullscreenExtension;
            }
            else if (bScopePiece && ElementPosX == 0.0f && (ElementPosY == 0.0f || ElementPosY == 20.0f || ElementPosY == 220.0f) && fWidth == 100.0f) // sniper scope borders right side
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

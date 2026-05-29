module;

#include <stdafx.h>
#include <queue>
#include <functional>
#include <unordered_map>
#include <vector>

export module Engine;

import ComVars;
import WidescreenHUD;

// Forward declaration for the softbody fixed timestep update
// Implemented later in the AESoftBodyActor namespace
namespace AESoftBodyActor { void AdvanceFixedTimestep(float deltaTime); }

namespace UGameEngine
{
    SafetyHookInline shPressStartToContinue = {};
    void __fastcall PressStartToContinue(void* UGameEngine, void* edx, int a2, float a3)
    {
        bPressStartToContinue = true;
        shPressStartToContinue.unsafe_fastcall(UGameEngine, edx, a2, a3);
        bPressStartToContinue = false;
    }
}

namespace FCanvasUtil
{
    void(__fastcall* DrawTile)(void* _this, uint32_t EDX, float, float, float, float, float, float, float, float, float, void*, FColor, uint32_t, uint32_t); //0x103D2DA0;
}

void __fastcall FCanvasUtilDrawTileHook(void* _this, uint32_t EDX, float X, float Y, float SizeX, float SizeY, float U, float V, float SizeU, float SizeV, float unk1, void* Texture, FColor Color, uint32_t unk3, uint32_t unk4)
{
    FColor ColBlack; ColBlack.RGBA = 0xFF000000;
    auto n_X = static_cast<uint32_t>(X);
    auto n_Y = static_cast<uint32_t>(Y);
    auto n_SizeX = static_cast<uint32_t>(SizeX);
    auto n_SizeY = static_cast<uint32_t>(SizeY);

    if (n_X == 0 && n_SizeX == 188) //zoom scope 1
    {
        if (Screen.bScopeWidescreenMode)
            return FCanvasUtil::DrawTile(_this, EDX, X, Y, SizeX, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color, unk3, unk4);
        else
        {
            FCanvasUtil::DrawTile(_this, EDX, 0.0f, Y, Screen.fHudOffset, SizeY, 0.0f, 0.0f, 1.0f, 1.0f, unk1, nullptr, ColBlack, unk3, unk4);
            FCanvasUtil::DrawTile(_this, EDX, 640.0f + Screen.fHudOffset, Y, 640.0f + Screen.fHudOffset + Screen.fHudOffset, SizeY, 0.0f, 0.0f, 1.0f, 1.0f, unk1, nullptr, ColBlack, unk3, unk4);
        }
    }

    if (n_X == (640 - 188) && n_SizeX == 640) //zoom scope 2
    {
        if (Screen.bScopeWidescreenMode)
            return FCanvasUtil::DrawTile(_this, EDX, SizeX + Screen.fHudOffset + Screen.fHudOffset - 188.0f, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color, unk3, unk4);
        else
        {
            FCanvasUtil::DrawTile(_this, EDX, 0.0f, Y, Screen.fHudOffset, SizeY, 0.0f, 0.0f, 1.0f, 1.0f, unk1, Texture, ColBlack, unk3, unk4);
            FCanvasUtil::DrawTile(_this, EDX, 640.0f + Screen.fHudOffset, Y, 640.0f + Screen.fHudOffset + Screen.fHudOffset, SizeY, 0.0f, 0.0f, 1.0f, 1.0f, unk1, Texture, ColBlack, unk3, unk4);
        }
    }
    if ((n_X == 0 || n_X == 256 || n_X == 384) && n_Y == 448 && (n_SizeX == 256 || n_SizeX == 384 || n_SizeX == 640) && n_SizeY == 479) //hiding menu background
    {
        // 4:3 menus on widescreen leave stale pixels visible, so fill side areas and the 1px bottom gap with black
        if (!Screen.bOpsatWidescreenMode && Screen.fHudOffset > 0.0f)
        {
            FCanvasUtil::DrawTile(_this, EDX, 0.0f, 0.0f, Screen.fHudOffset, 480.0f, 0.0f, 0.0f, 1.0f, 1.0f, unk1, nullptr, ColBlack, unk3, unk4); //left side bar
            FCanvasUtil::DrawTile(_this, EDX, 640.0f + Screen.fHudOffset, 0.0f, 640.0f + Screen.fHudOffset + Screen.fHudOffset, 480.0f, 0.0f, 0.0f, 1.0f, 1.0f, unk1, nullptr, ColBlack, unk3, unk4); //right side bar
        }
        // The menu texture stops at Y=479, leaving a 1px gap at the very bottom of the 4:3 area
        FCanvasUtil::DrawTile(_this, EDX, 0.0f, 479.0f, 640.0f + Screen.fHudOffset + Screen.fHudOffset, 480.0f, 0.0f, 0.0f, 1.0f, 1.0f, unk1, nullptr, ColBlack, unk3, unk4); //bottom 1px gap

         // Draw menu bottom strip normally
        FCanvasUtil::DrawTile(_this, EDX, X + Screen.fHudOffset, Y, SizeX + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color, unk3, unk4);
        return;
    }
    if (n_X == 0 && n_SizeX == 64) //sony ericsson overlay
    {
        if (Screen.bOpsatWidescreenMode)
        {
            FCanvasUtil::DrawTile(_this, EDX, X, Y, SizeX, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color, unk3, unk4);
            return;
        }
        else
        {
            FCanvasUtil::DrawTile(_this, EDX, 0.0f, Y, Screen.fHudOffset + 64.0f, SizeY, U, V, SizeU, SizeV, unk1, Texture, ColBlack, unk3, unk4);
        }
    }
    if (n_X == 64 && n_SizeX == 320) //sony ericsson overlay
    {
        if (Screen.bOpsatWidescreenMode)
        {
            FCanvasUtil::DrawTile(_this, EDX, X, Y, SizeX + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color, unk3, unk4);
            return;
        }
    }
    if (n_X == 320 && n_SizeX == 576) //sony ericsson overlay
    {
        if (Screen.bOpsatWidescreenMode)
        {
            FCanvasUtil::DrawTile(_this, EDX, X + Screen.fHudOffset, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color, unk3, unk4);
            return;
        }
    }

    if (n_X == 576 && n_SizeX == 640) //sony ericsson overlay
    {
        if (Screen.bOpsatWidescreenMode)
        {
            FCanvasUtil::DrawTile(_this, EDX, X + Screen.fHudOffset + Screen.fHudOffset, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color, unk3, unk4);
            return;
        }
        else
        {
            FCanvasUtil::DrawTile(_this, EDX, 576.0f + Screen.fHudOffset, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, ColBlack, unk3, unk4);
        }
    }

    if (n_X == 0 && (n_SizeX == 640 || n_SizeX == Screen.Width))
    {
        if ((Color.R == 0xFE && Color.G == 0xFE && Color.B == 0xFE) || (Color.R == 0xFF && Color.G == 0xFF && Color.B == 0xFF)) //flashbang grenade flash
        {
            FCanvasUtil::DrawTile(_this, EDX, X, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color, unk3, unk4);
            return;
        }
        else
        {
            FCanvasUtil::DrawTile(_this, EDX, 0.0f, Y, Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, ColBlack, unk3, unk4);
            FCanvasUtil::DrawTile(_this, EDX, SizeX + Screen.fHudOffset, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, ColBlack, unk3, unk4);
        }
    }

    if (Screen.nHudWidescreenMode == 1)
    {
        wchar_t buffer[256];
        curDrawTileManagerTextureName = UObject::GetFullName(Texture, 0, buffer);
        WidescreenHud(X, SizeX, Y, SizeY, Color);
    }
    else if (Screen.nHudWidescreenMode == 2)
    {
        if (n_X == 0 && n_Y == 0 && n_SizeY == 480 && SizeU == 256.0f)
        {
            // fix flashbang drawing
            if (curDrawTileManagerTextureIndex == 163 || curDrawTileManagerTextureIndex == 164)
                X -= Screen.fHudOffset;
        }
    }

    X += Screen.fHudOffset;
    SizeX += Screen.fHudOffset;

    return FCanvasUtil::DrawTile(_this, EDX, X, Y, SizeX, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color, unk3, unk4);
}

#if _DEBUG
SafetyHookInline shFindAxisName = {};
float* __fastcall FindAxisName(void* UInput, void* edx, void* AActor, const wchar_t* a3)
{
    auto ret = shFindAxisName.unsafe_fastcall<float*>(UInput, edx, AActor, a3);

    if (std::wstring_view(a3) == L"aMouseX")
    {
        return ret;
    }
    else if (std::wstring_view(a3) == L"aMouseY")
    {
        return ret;
    }

    return ret;
}
#endif

namespace UInput
{
    uint8_t(__fastcall* GetKey)(void* uInput, void* edx, const wchar_t* a2, int a3) = nullptr;

    void* gUInput = nullptr;
    SafetyHookInline shInit = {};
    void __fastcall Init(void* uInput, void* edx, void* uViewport)
    {
        gUInput = uInput;
        return shInit.unsafe_fastcall(uInput, edx, uViewport);
    }
}

namespace UEngine
{
    SafetyHookInline shInputEvent = {};
    int __fastcall InputEvent(void* _this, void* edx, int a2, int inputID, int a4, int value)
    {
        if (inputID == 202) // A on gamepad
        {
            auto EchelonMainHUDState = UObject::GetState(L"EchelonMainHUD");
            if (EchelonMainHUDState == L"MainHUD" || EchelonMainHUDState == L"s_Slavery")
            {
                auto EPlayerControllerState = UObject::GetState(L"EPlayerController");

                if (EPlayerControllerState == L"s_FirstPersonTargeting" || EPlayerControllerState == L"s_RappellingTargeting" ||
                    EPlayerControllerState == L"s_PlayerBTWTargeting" || EPlayerControllerState == L"s_PlayerSniping" || EPlayerControllerState == L"s_HOHFUTargeting")
                {
                    auto EGameInteractionState = UObject::GetState(L"EGameInteraction");
                    if (EGameInteractionState != L"s_GameInteractionMenu")
                    {
                        shInputEvent.unsafe_fastcall<int>(_this, edx, a2, inputID, a4, value);
                        return shInputEvent.unsafe_fastcall<int>(_this, edx, a2, UInput::GetKey(UInput::gUInput, edx, L"ReloadGun", 0), a4, value);
                    }
                }
            }
        }
        return shInputEvent.unsafe_fastcall<int>(_this, edx, a2, inputID, a4, value);
    }
}

// Restore the in-game console
namespace UInteractionMaster
{
    void(__fastcall* FName_ctor)(int* /*FName out*/, void* /*edx*/, const wchar_t* Name, int FindType) = nullptr;
    void* (__fastcall* FindFunction)(void* /*UObject*/, void* /*edx*/, int /*FName*/, int) = nullptr;
    void(__fastcall* ProcessEvent)(void* /*UObject*/, void* /*edx*/, void* /*UFunction*/, void* /*Parms*/, void* /*Result*/) = nullptr;
    void(__fastcall* SetClip)(void* /*UCanvas*/, void* /*edx*/, float X, float Y) = nullptr;

    constexpr int IK_Tilde = 0xC0; // Tilde key to open console
    constexpr int IK_F2 = 0x71; // F2 key to open console
    constexpr int IST_Press = 1;

    SafetyHookInline shMasterProcessKeyEvent = {};
    int __fastcall MasterProcessKeyEvent(void* self, void* edx, int key, int action, float delta)
    {
        // Input enums are byte-sized
        if (((key & 0xFF) == IK_Tilde || (key & 0xFF) == IK_F2) &&
            (action & 0xFF) == IST_Press)
        {
            // UInteractionMaster Console is at +0x34
            void* console = *(void**)((char*)self + 0x34);

            if (console && FName_ctor && FindFunction && ProcessEvent)
            {
                static int typeName = 0;
                static void* typeFn = nullptr;

                // Cache FName/UFunction on first use
                if (!typeFn)
                {
                    if (typeName == 0)
                        FName_ctor(&typeName, nullptr, L"Type", 1 /* FNAME_Add */);
                    if (typeName != 0)
                        typeFn = FindFunction(console, nullptr, typeName, 0);
                }

                if (typeFn)
                {
                    ProcessEvent(console, nullptr, typeFn, nullptr, nullptr);
                    return 1;
                }
            }
        }
        return shMasterProcessKeyEvent.unsafe_fastcall<int>(self, edx, key, action, delta);
    }

    SafetyHookInline shMasterProcessPostRender = {};
    void __fastcall MasterProcessPostRender(void* self, void* edx, void* canvas)
    {
        if (canvas && SetClip)
            SetClip(canvas, nullptr, 640.0f, 480.0f);

        shMasterProcessPostRender.unsafe_fastcall(self, edx, canvas);
    }
}

namespace UGameEngine
{
    SafetyHookInline shDisplaySplash = {};
    void __fastcall DisplaySplash(void* UGameEngine, void* edx, int a2)
    {
        bDisplayingBackground = true;
        shDisplaySplash.unsafe_fastcall(UGameEngine, edx, a2);
        bDisplayingBackground = false;
    }

    SafetyHookInline shDisplayMenuSplash = {};
    void __fastcall DisplayMenuSplash(void* UGameEngine, void* edx, int a2, int a3)
    {
        bDisplayingBackground = true;
        shDisplayMenuSplash.unsafe_fastcall(UGameEngine, edx, a2, a3);
        bDisplayingBackground = false;
    }

    SafetyHookInline shSplashFadInOut = {};
    void __fastcall SplashFadInOut(void* UGameEngine, void* edx, int a2, int a3)
    {
        bDisplayingBackground = true;
        shSplashFadInOut.unsafe_fastcall(UGameEngine, edx, a2, a3);
        bDisplayingBackground = false;
    }

    SafetyHookInline shTick = {};
    void __fastcall Tick(void* UGameEngine, void* edx, float deltaTime)
    {
        // Update the shared softbody fixed timestep once per frame
        // All cloth actors use the same substep count and interpolation alpha
        AESoftBodyActor::AdvanceFixedTimestep(deltaTime);

        if (Screen.bDeferredInput)
        {
            while (!UWindowsViewport::deferredCauseInputEvent.empty())
            {
                UWindowsViewport::deferredCauseInputEvent.front()();
                UWindowsViewport::deferredCauseInputEvent.pop();
            }
        }

        if (Screen.fRawInputMouse > 0.0f && UWindowsViewport::deferredCauseInputEventForRawInput)
        {
            UWindowsViewport::deferredCauseInputEventForRawInput(228, 4, static_cast<float>(RawInputHandler<int32_t>::RawMouseDeltaX));
            UWindowsViewport::deferredCauseInputEventForRawInput(229, 4, static_cast<float>(RawInputHandler<int32_t>::RawMouseDeltaY));
            RawInputHandler<int32_t>::RawMouseDeltaX = 0;
            RawInputHandler<int32_t>::RawMouseDeltaY = 0;
            UWindowsViewport::deferredCauseInputEventForRawInput = nullptr;
        }

        return shTick.unsafe_fastcall(UGameEngine, edx, deltaTime);
    }
}

namespace UCanvas
{
    SafetyHookInline shSetClip = {};
    void __fastcall SetClip(void* uCanvas, void* edx, float a2, float a3)
    {
        if (Screen.nHudWidescreenMode > 1)
        {
            uint32_t n_offsetX1 = static_cast<uint32_t>(a2);
            uint32_t n_offsetX2 = static_cast<uint32_t>(a3);

            if (n_offsetX1 == 640 && n_offsetX2 == 480)
            {
                a2 = (480.0f * Screen.fAspectRatio);
            }
        }

        return shSetClip.unsafe_fastcall(uCanvas, edx, a2, a3);
    }
}

namespace AEGrassActor
{
    SafetyHookInline shPostLoad = {};
    void __fastcall PostLoad(void* actor, void* edx)
    {
        shPostLoad.unsafe_fastcall(actor, edx);

        constexpr float kGrassFarDistance = 100000.0f;
        *(float*)((char*)actor + 0x3F8) = kGrassFarDistance; // InvisibleDistance
        *(float*)((char*)actor + 0x3FC) = kGrassFarDistance; // LODDistance
        *(float*)((char*)actor + 0x400) = kGrassFarDistance; // LODNear
    }
}

namespace AActor
{
    SafetyHookInline shPostLoad = {};
    void __fastcall PostLoad(void* actor, void* edx)
    {
        shPostLoad.unsafe_fastcall(actor, edx);

        if (fLightDistanceMultiplier != 1.0f)
        {
            float* pTurnOffDistance = (float*)((char*)actor + 0x294); // AActor::TurnOffDistance
            if (*pTurnOffDistance > 0.0f)
                *pTurnOffDistance *= fLightDistanceMultiplier;
        }
    }
}

// Softbody FPS fixes
namespace AESoftBodyActor
{
    constexpr uintptr_t kSoftBodyMember           = 0x36C;
    constexpr uintptr_t kVertCountOff             = 0x9C;
    constexpr uintptr_t kVertArrayOff             = 0x98;
    constexpr uintptr_t kVertStride               = 0x50;
    constexpr uintptr_t kVertPosOff               = 0x04;
    constexpr uintptr_t kSmoothStateOff           = 0x15C;

    constexpr uintptr_t kSoftBodyTickVtblOff          = 0x98;
    constexpr uintptr_t kSoftBodyUpdateBuffersOff     = 0x88;
    constexpr uintptr_t kSoftBodyUpdateAttachmentsOff = 0xCC;
    constexpr uintptr_t kSoftBodyUpdateBoundsOff      = 0xD0;

    constexpr float kFixedDt = 1.0f / 30.0f;
    constexpr float kMinDt   = 1.0f / 1000.0f; // Engine originally clamped dt at 1/200s, widened to 1/1000s

    // Address patched in UESoftBody::Update to force fixed timestep
    float fFrameDelta = kFixedDt;

    using AActorTickFn = int(__fastcall*)(void* self, void* edx, float dt, int tickType);
    AActorTickFn AActorTick = nullptr;

    // Global accumulator (advanced once per engine tick)
    // All cloth actors in a frame share the same substep count and interpolation alpha
    float gAccumulator       = 0.0f;
    int   gSubstepsThisFrame = 0;
    float gAlpha             = 0.0f;

    void AdvanceFixedTimestep(float deltaTime)
    {
        float dt = deltaTime;
        if (!(dt > 0.0f) || dt < kMinDt) dt = kMinDt;

        gAccumulator += dt;
        gSubstepsThisFrame = 0;
        while (gAccumulator >= kFixedDt)
        {
            gAccumulator -= kFixedDt;
            ++gSubstepsThisFrame;
        }
        gAlpha = gAccumulator / kFixedDt;
    }

    struct SbState
    {
        std::vector<float> back;    // t - 1/30
        std::vector<float> front;   // t
        int  nVerts      = 0;
        bool initialized = false;
    };

    // Cleared between levels by UGameEngineLoadGameHook
    std::unordered_map<void*, SbState> states;

    static inline void CaptureAll(void* vBase, int n, std::vector<float>& dst)
    {
        if (static_cast<int>(dst.size()) < n * 3) dst.assign(n * 3, 0.0f);
        const uintptr_t base = reinterpret_cast<uintptr_t>(vBase);
        for (int i = 0; i < n; ++i)
        {
            const float* p = reinterpret_cast<const float*>(base + static_cast<size_t>(i) * kVertStride + kVertPosOff);
            dst[i * 3 + 0] = p[0];
            dst[i * 3 + 1] = p[1];
            dst[i * 3 + 2] = p[2];
        }
    }

    static inline void RestoreAll(void* vBase, int n, const std::vector<float>& src)
    {
        const uintptr_t base = reinterpret_cast<uintptr_t>(vBase);
        for (int i = 0; i < n; ++i)
        {
            float* p = reinterpret_cast<float*>(base + static_cast<size_t>(i) * kVertStride + kVertPosOff);
            p[0] = src[i * 3 + 0];
            p[1] = src[i * 3 + 1];
            p[2] = src[i * 3 + 2];
        }
    }

    static inline void LerpAll(void* vBase, int n,
                               const std::vector<float>& a,
                               const std::vector<float>& b,
                               float t)
    {
        const uintptr_t base = reinterpret_cast<uintptr_t>(vBase);
        for (int i = 0; i < n; ++i)
        {
            float* p = reinterpret_cast<float*>(base + static_cast<size_t>(i) * kVertStride + kVertPosOff);
            p[0] = a[i * 3 + 0] + t * (b[i * 3 + 0] - a[i * 3 + 0]);
            p[1] = a[i * 3 + 1] + t * (b[i * 3 + 1] - a[i * 3 + 1]);
            p[2] = a[i * 3 + 2] + t * (b[i * 3 + 2] - a[i * 3 + 2]);
        }
    }

    static inline void RunOneSubstep(void* softBody)
    {
        *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(softBody) + kSmoothStateOff) = kFixedDt;
        auto vtbl = *reinterpret_cast<uintptr_t**>(softBody);
        using TickFn = void(__fastcall*)(void*);
        TickFn fn = reinterpret_cast<TickFn>(vtbl[kSoftBodyTickVtblOff / sizeof(uintptr_t)]);
        fn(softBody);
    }

    // Run UpdateBuffers after interpolation so the rendered cloth uses interpolated positions
    // UpdateAttachments and UpdateBounds keep attachments and culling bounds in sync
    static inline void SyncRenderToCurrentPositions(void* softBody)
    {
        auto vtbl = *reinterpret_cast<uintptr_t**>(softBody);
        using VoidFn = void(__fastcall*)(void*);
        VoidFn buffers = reinterpret_cast<VoidFn>(vtbl[kSoftBodyUpdateBuffersOff     / sizeof(uintptr_t)]);
        VoidFn attach  = reinterpret_cast<VoidFn>(vtbl[kSoftBodyUpdateAttachmentsOff / sizeof(uintptr_t)]);
        VoidFn bounds  = reinterpret_cast<VoidFn>(vtbl[kSoftBodyUpdateBoundsOff      / sizeof(uintptr_t)]);
        attach (softBody);
        bounds (softBody);
        buffers(softBody);
    }

    SafetyHookInline shTick = {};
    int __fastcall Tick(void* self, void* edx, float deltaTime, int tickType)
    {
        void* softBody = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(self) + kSoftBodyMember);
        if (!softBody)
            return shTick.unsafe_fastcall<int>(self, edx, deltaTime, tickType);

        const int   n     = *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(softBody) + kVertCountOff);
        void* const vBase = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(softBody) + kVertArrayOff);

        if (!vBase || n <= 0)
            return shTick.unsafe_fastcall<int>(self, edx, deltaTime, tickType);

        SbState& st = states[softBody];

        if (!st.initialized || st.nVerts != n)
        {
            st.nVerts = n;
            st.back.assign(n * 3, 0.0f);
            st.front.assign(n * 3, 0.0f);
            CaptureAll(vBase, n, st.back);
            RunOneSubstep(softBody);
            CaptureAll(vBase, n, st.front);
            st.initialized = true;
        }

        RestoreAll(vBase, n, st.front);

        for (int i = 0; i < gSubstepsThisFrame; ++i)
        {
            st.back = st.front;
            RunOneSubstep(softBody);
            CaptureAll(vBase, n, st.front);
        }

        LerpAll(vBase, n, st.back, st.front, gAlpha);

        SyncRenderToCurrentPositions(softBody);

        if (AActorTick)
            return AActorTick(self, edx, deltaTime, tickType);

        // Temporarily use a null SoftBody so AActor::Tick skips SoftBody updates
        *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(self) + kSoftBodyMember) = nullptr;
        int ret = shTick.unsafe_fastcall<int>(self, edx, deltaTime, tickType);
        *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(self) + kSoftBodyMember) = softBody;
        return ret;
    }
}

// Animated texture FPS fixes
namespace UTexture
{
    constexpr uintptr_t kMaxFrameRateOff = 0xA8;
    constexpr uintptr_t kAccumulatorOff  = 0xAC;

    constexpr float kStep = 1.0f / 30.0f;

    SafetyHookInline shTick = {};
    void __fastcall Tick(void* self, void* edx, float deltaSeconds)
    {
        const float maxFrameRate = *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(self) + kMaxFrameRateOff);

        if (maxFrameRate != 0.0f || !(deltaSeconds > 0.0f))
            return shTick.unsafe_fastcall(self, edx, deltaSeconds);

        float& accumulator = *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(self) + kAccumulatorOff);
        accumulator += deltaSeconds;

        if (accumulator >= kStep)
        {
            accumulator -= kStep;
            if (accumulator > kStep)
                accumulator = kStep;
            shTick.unsafe_fastcall(self, edx, deltaSeconds);
        }
    }
}

export void InitEngine()
{
    InitWidescreenHUD();

    auto pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B ? 94 00 00 00 E8");
    FCanvasUtil::DrawTile = (decltype(FCanvasUtil::DrawTile))injector::GetBranchDestination(pattern.count(3).get(0).get<uintptr_t>(6), true).as_int();

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "DC 0D ? ? ? ? DB 43 18 DC 0D ? ? ? ? D9 5D E4 75 12 D9 45 10");
    if (!pattern.empty())
        injector::WriteMemory(pattern.get_first(2), &Screen.dHUDScaleX, true);
    else
    {
        pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "F2 0F 5E 05 ? ? ? ? 66 0F 5A D0");
        injector::WriteMemory(pattern.get_first(4), &Screen.dHUDScaleXInv, true);
    }

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B 4D F4 5F 5E 64 89 0D 00 00 00 00 5B 8B E5 5D C2 4C 00");
    if (!pattern.empty())
        injector::MakeCALL(pattern.count(1).get(0).get<uint32_t>(-5), FCanvasUtilDrawTileHook, true); //pfFUCanvasDrawTile + 0x219
    else
    {
        pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "F3 0F 11 0C 24 E8 ? ? ? ? 5F 5E 5B");
        injector::MakeCALL(pattern.get_first(5), FCanvasUtilDrawTileHook, true);
    }

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B 4C 24 04 8B 51 44 83");
    if (!pattern.empty())
    {
        uint32_t pfsub_103762F0 = (uint32_t)pattern.get_first(0);
        auto rpattern = hook::range_pattern(pfsub_103762F0, pfsub_103762F0 + 0x800, "E8 ? ? ? ? 8B ?");
        injector::MakeCALL(rpattern.get(3).get<uint32_t>(0), FCanvasUtilDrawTileHook, true); //pfsub_103762F0 + 0x36E
        injector::MakeCALL(rpattern.get(5).get<uint32_t>(0), FCanvasUtilDrawTileHook, true); //pfsub_103762F0 + 0x43D
        injector::MakeCALL(rpattern.get(7).get<uint32_t>(0), FCanvasUtilDrawTileHook, true); //pfsub_103762F0 + 0x4DA
        injector::MakeCALL(rpattern.get(9).get<uint32_t>(0), FCanvasUtilDrawTileHook, true); //pfsub_103762F0 + 0x564
    }
    else
    {
        pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "E8 ? ? ? ? 83 7D ? ? 8B 4D");
        injector::MakeCALL(pattern.get_first(0), FCanvasUtilDrawTileHook, true);
    }

    //FOV
    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "E8 ? ? ? ? 8B 4D ? 5F 8B C6 5E 64 89 0D ? ? ? ? 5B 8B E5 5D C2 24 00");
    if (!pattern.empty())
    {
        static auto FCameraSceneNodeCtorHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            if (bRestoreCutsceneFOV && UObject::GetState(L"EchelonMainHUD") == L"s_Cinematic")
            {
                return;
            }

            *(float*)(regs.esi + 0x214) = AdjustFOV(*(float*)(regs.esi + 0x214), Screen.fAspectRatio);
        });
    }
    else
    {
        pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "E8 ? ? ? ? 8B C7 8B 4D ? 64 89 0D ? ? ? ? 59 5F 5E 8B E5 5D C2 24 00");
        static auto FCameraSceneNodeCtorHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            if (bRestoreCutsceneFOV && UObject::GetState(L"EchelonMainHUD") == L"s_Cinematic")
            {
                return;
            }

            *(float*)(regs.edi + 0x214) = AdjustFOV(*(float*)(regs.edi + 0x214), Screen.fAspectRatio);
        });
    }

    if (Screen.nPostProcessFixedScale)
    {
        if (Screen.nPostProcessFixedScale == 1)
            Screen.nPostProcessFixedScale = Screen.Width;

        auto pattern2 = hook::module_pattern(GetModuleHandle(L"Engine"), "68 00 02 00 00 68 00 02 00 00");
        for (size_t i = 0; i < pattern2.size(); i++)
        {
            injector::WriteMemory(pattern2.get(i).get<uint32_t>(1), Screen.nPostProcessFixedScale, true);
            injector::WriteMemory(pattern2.get(i).get<uint32_t>(6), Screen.nPostProcessFixedScale, true);
        }

        // Thermal vision uses a separate render target size so it bypasses the main fix, apply PostProcessFixedScale here as well
        auto thermalRT = find_module_pattern(GetModuleHandle(L"Engine"), "8B 0C 85 ? ? ? ? 8B 75 10");
        if (!thermalRT.empty())
        {
            // Override all thermal RT presets
            uintptr_t thermalSceneSizes = *thermalRT.get_first<uintptr_t>(3);
            for (int i = 0; i < 3; i++)
                injector::WriteMemory<uint32_t>(thermalSceneSizes + i * 4, Screen.nPostProcessFixedScale, true);
        }
    }

    // Force thermal vision to use quality preset 2 (1024px pyramid)
    {
        auto thermalDispatch = find_module_pattern(GetModuleHandle(L"Engine"), "FF B5 FC FB FF FF 56 53 6A 00 E8");
        if (!thermalDispatch.empty())
            injector::WriteMemory<uint8_t>(thermalDispatch.get_first(9), 2, true);
    }

    if (gColor.RGBA)
    {
        pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "B0 7F 88 45 24 88 45 25 88 45 26 C6 45 27 FF 8B 45 24 50 8B CB FF 52 40"); //104070CF
        if (!pattern.empty())
        {
            struct USkeletalMeshInstanceRenderHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    *(uint8_t*)(regs.ebp + 0x24) = gColor.B;
                    *(uint8_t*)(regs.ebp + 0x25) = gColor.G;
                    *(uint8_t*)(regs.ebp + 0x26) = gColor.R;
                    *(uint8_t*)(regs.ebp + 0x27) = 0xFF;
                }
            }; injector::MakeInline<USkeletalMeshInstanceRenderHook>(pattern.get_first(0), pattern.get_first(15));
        }
        else
        {
            pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "C7 85 ? ? ? ? ? ? ? ? FF B5 ? ? ? ? FF 50");
            struct USkeletalMeshInstanceRenderHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    *(uint8_t*)(regs.ebp - 0xA8) = gColor.B;
                    *(uint8_t*)(regs.ebp - 0xA7) = gColor.G;
                    *(uint8_t*)(regs.ebp - 0xA6) = gColor.R;
                    *(uint8_t*)(regs.ebp - 0xA5) = 0xFF;
                }
            }; injector::MakeInline<USkeletalMeshInstanceRenderHook>(pattern.get_first(0), pattern.get_first(10));
        }
    }

    #if _DEBUG
    pattern = find_module_pattern<1>(GetModuleHandle(L"Engine"), "55 8B EC 83 EC ? 53 56 57 6A ? FF 75 ? 8D 4D ? FF 15 ? ? ? ? 83 7D ? ? 0F 84 ? ? ? ? 8B 75");
    if (!pattern.empty())
        shFindAxisName = safetyhook::create_inline(pattern.get_first(), FindAxisName);
    #endif

    // LOD
    pattern = find_module_pattern(GetModuleHandle(L"Engine"), "0F 84 ? ? ? ? FF 15 ? ? ? ? 8B 8C 24", "0F 84 ? ? ? ? 8B 41 ? F3 0F 10 81");
    injector::WriteMemory<uint16_t>(pattern.get_first(0), 0xE990, true); // jz -> jmp

    if (bSkipIntro)
    {
        pattern = find_module_pattern(GetModuleHandle(L"Engine"), "6A ? 6A ? 68 ? ? ? ? 53 E8 ? ? ? ? 83 C4 ? 68", "6A ? FF D6 85 C0");
        auto dest = find_module_pattern(GetModuleHandle(L"Engine"), "8B 83 ? ? ? ? 8B 48 ? 8B 01 83 A0", "8B 87 ? ? ? ? 8D 8D ? ? ? ? 8B 40");
        injector::MakeJMP(pattern.get_first(0), dest.get_first(0), true);
    }

    // Always skip the HealthWarning_<lang>.bik startup video added in the digital release
    pattern = find_module_pattern(GetModuleHandle(L"Engine"), "6A 0A 6A 00 6A 01 8D 85 ? ? ? ? 50 57 E8 ? ? ? ? 8B 35 ? ? ? ? 83 C4 20");
    if (!pattern.empty())
        injector::MakeNOP(pattern.get_first(14), 5, true); // skip PlayMovie("HealthWarning_%s.bik")

    pattern = find_module_pattern(GetModuleHandle(L"Engine"), "E8 ? ? ? ? 83 C4 ? 85 C0 75 ? 8B 4F", "83 7B ? ? BE");
    static auto VideoPlaybackStartHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        bPlayingVideo = true;
    });

    pattern = find_module_pattern(GetModuleHandle(L"Engine"), "8B 0D ? ? ? ? 8B 54 24 ? 89 11", "89 02 8B 13");
    static auto VideoPlaybackEndHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        bPlayingVideo = false;
    });

    pattern = find_module_pattern(GetModuleHandle(L"Engine"), "55 8B EC 6A ? 68 ? ? ? ? 64 A1 ? ? ? ? 50 64 89 25 ? ? ? ? 83 EC ? A1 ? ? ? ? 53 56 8B F1 33 C9", "55 8B EC 83 EC ? 83 3D ? ? ? ? ? 56");
    if (!pattern.empty())
        UGameEngine::shPressStartToContinue = safetyhook::create_inline(pattern.get_first(), UGameEngine::PressStartToContinue);

    if (bSkipPressStartToContinue)
    {
        pattern = find_module_pattern(GetModuleHandle(L"Engine"), "0F 84 ? ? ? ? F3 0F 10 45 ? F3 0F 59 05");
        if (!pattern.empty())
            injector::WriteMemory<uint16_t>(pattern.get_first(0), 0xE990, true); // jz -> jmp
        else
        {
            pattern = find_module_pattern(GetModuleHandle(L"Engine"), "74 ? D9 45 ? D8 0D ? ? ? ? E8");
            if (!pattern.empty())
                injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true); // jz -> jmp
        }
    }

    UInput::GetKey = (decltype(UInput::GetKey))FindProcAddress(GetModuleHandle(L"Engine"), "?GetKey@UInput@@UAEEPB_WH@Z", "?GetKey@UInput@@UAEEPBGH@Z");
    UEngine::shInputEvent = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Engine"), "?InputEvent@UEngine@@UAEHPAVUViewport@@W4EInputKey@@W4EInputAction@@M@Z"), UEngine::InputEvent);
    UInput::shInit = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Engine"), "?Init@UInput@@UAEXPAVUViewport@@@Z"), UInput::Init);

    // Restore the in-game console
    if (bEnableConsole)
    {
        UInteractionMaster::FName_ctor = (decltype(UInteractionMaster::FName_ctor))GetProcAddress(GetModuleHandle(L"Core"), "??0FName@@QAE@PB_WW4EFindName@@@Z");
        UInteractionMaster::FindFunction = (decltype(UInteractionMaster::FindFunction))GetProcAddress(GetModuleHandle(L"Core"), "?FindFunction@UObject@@QAEPAVUFunction@@VFName@@H@Z");
        UInteractionMaster::ProcessEvent = (decltype(UInteractionMaster::ProcessEvent))GetProcAddress(GetModuleHandle(L"Core"), "?ProcessEvent@UObject@@UAEXPAVUFunction@@PAX1@Z");
        UInteractionMaster::SetClip = (decltype(UInteractionMaster::SetClip))GetProcAddress(GetModuleHandle(L"Engine"), "?SetClip@UCanvas@@UAEXMM@Z");
        UInteractionMaster::shMasterProcessKeyEvent = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Engine"), "?MasterProcessKeyEvent@UInteractionMaster@@QAEHW4EInputKey@@W4EInputAction@@M@Z"), UInteractionMaster::MasterProcessKeyEvent);
        UInteractionMaster::shMasterProcessPostRender = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Engine"), "?MasterProcessPostRender@UInteractionMaster@@QAEXPAVUCanvas@@@Z"), UInteractionMaster::MasterProcessPostRender);
    }

    UGameEngine::shDisplaySplash = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Engine"), "?DisplaySplash@UGameEngine@@QAEXH@Z"), UGameEngine::DisplaySplash);
    UGameEngine::shDisplayMenuSplash = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Engine"), "?DisplayMenuSplash@UGameEngine@@QAEXHH@Z"), UGameEngine::DisplayMenuSplash);
    UGameEngine::shSplashFadInOut = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Engine"), "?SplashFadInOut@UGameEngine@@QAEXHH@Z"), UGameEngine::SplashFadInOut);
    UGameEngine::shTick = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Engine"), "?Tick@UGameEngine@@UAEXM@Z"), UGameEngine::Tick);

    UCanvas::shSetClip = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Engine"), "?SetClip@UCanvas@@UAEXMM@Z"), UCanvas::SetClip);

    UTexture::shTick = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Engine"), "?Tick@UTexture@@UAEXM@Z"), UTexture::Tick);

    pattern = find_module_pattern(GetModuleHandle(L"Engine"), "89 3D ? ? ? ? 8B 8B 64 01 00 00", "C7 05 ? ? ? ? ? ? ? ? 8B 87 64 01 00 00");
    static auto UGameEngineLoadGameHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        UObject::objectStates.clear();
        UIntOverrides::ClearCache();
        UFloatOverrides::ClearCache();
        UByteOverrides::ClearCache();
        UNameOverrides::ClearCache();
        UObjectOverrides::ClearCache();
        UArrayOverrides::ClearCache();
        AESoftBodyActor::states.clear();
    });

    // Softbody FPS fixes
    {
        auto sbActorTick = find_module_pattern(GetModuleHandle(L"Engine"),
            "55 8B EC 56 8B F1 8B 8E 6C 03 00 00 85 C9 74 08 8B 01 FF 90 98 00 00 00");

        auto sbUpdate = find_module_pattern(GetModuleHandle(L"Engine"),
            "8B 43 60 8B 80 B8 00 00 00 F3 0F 10 15 ? ? ? ? 5E F3 0F 10 80 64 07 00 00 0F 2F D0 77 0B F3 0F 5D 05");

        if (sbActorTick.size() && sbUpdate.size())
        {
            AESoftBodyActor::AActorTick = reinterpret_cast<AESoftBodyActor::AActorTickFn>(
                GetProcAddress(GetModuleHandle(L"Engine"), "?Tick@AActor@@UAEHMW4ELevelTick@@@Z"));

            AESoftBodyActor::shTick = safetyhook::create_inline(sbActorTick.get_first(), AESoftBodyActor::Tick);

            uintptr_t movss = reinterpret_cast<uintptr_t>(sbUpdate.get_first(18));
            injector::WriteMemory<uint8_t>(movss + 3, 0x05, true);
            injector::WriteMemory<uintptr_t>(movss + 4, reinterpret_cast<uintptr_t>(&AESoftBodyActor::fFrameDelta), true);

            uintptr_t clampLow = *sbUpdate.get_first<uintptr_t>(13);
            injector::WriteMemory<float>(clampLow, AESoftBodyActor::kMinDt, true);
        }
    }

    AEGrassActor::shPostLoad = safetyhook::create_inline(
        GetProcAddress(GetModuleHandle(L"Engine"), "?PostLoad@AEGrassActor@@UAEXXZ"),
        AEGrassActor::PostLoad);

    AActor::shPostLoad = safetyhook::create_inline(
        GetProcAddress(GetModuleHandle(L"Engine"), "?PostLoad@AActor@@UAEXXZ"),
        AActor::PostLoad);

    // Fix thermal vision grain scaling at high resolutions
    pattern = find_module_pattern(GetModuleHandle(L"Engine"), "8B 46 04 8B 80 84 00 00 00 99 F7 F9 66 0F 6E C0");
    if (!pattern.empty())
    {
        static auto ThermalGrainHeightHook = safetyhook::create_mid(pattern.get_first(9), [](SafetyHookContext& regs)
        {
            regs.eax = static_cast<uint32_t>((float)regs.eax + (480.0f - (float)regs.eax) * Screen.fGrainScale);
        });
    }

    pattern = find_module_pattern(GetModuleHandle(L"Engine"), "8B 46 04 8B 80 80 00 00 00 99 F7 F9 8D 8D 74 FE FF FF");
    if (!pattern.empty())
    {
        static auto ThermalGrainWidthHook = safetyhook::create_mid(pattern.get_first(9), [](SafetyHookContext& regs)
        {
            regs.eax = static_cast<uint32_t>((float)regs.eax + (640.0f - (float)regs.eax) * Screen.fGrainScale);
        });
    }
}
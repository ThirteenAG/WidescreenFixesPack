module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <queue>
#include <functional>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <algorithm>
#include <vector>

#include "Shared/CursorConfinement.hpp"

export module Engine;

import ComVars;
import WidescreenHUD;
import Gamepad;
import CutsceneFrame;
import GameplayCamera;

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
        if (Screen.fHudOffset > 0.0f)
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
        FCanvasUtil::DrawTile(_this, EDX, 0.0f, Y, Screen.fHudOffset + 64.0f, SizeY, U, V, SizeU, SizeV, unk1, Texture, ColBlack, unk3, unk4);
    if (n_X == 576 && n_SizeX == 640) //sony ericsson overlay
        FCanvasUtil::DrawTile(_this, EDX, 576.0f + Screen.fHudOffset, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, ColBlack, unk3, unk4);

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

    // Flash afterimage arrives already widened to the full canvas, offsetting it again leaves a bar on the left
    if (n_X == 0 && n_Y == 0 && n_SizeY == 480 && n_SizeX == static_cast<uint32_t>(640.0f + Screen.fHudOffset + Screen.fHudOffset) &&
        ((Color.R == 0xFE && Color.G == 0xFE && Color.B == 0xFE) || (Color.R == 0xFF && Color.G == 0xFF && Color.B == 0xFF)))
        return FCanvasUtil::DrawTile(_this, EDX, X, Y, SizeX, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color, unk3, unk4);

    WidescreenHud(X, SizeX, Y, SizeY, Color);

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
    void(__fastcall* SetKey)(void* uInput, void* edx, const wchar_t* keyAndBinding, const wchar_t* keyName) = nullptr;

    // The pad's look axes after their settings changed
    void RefreshPadBindings()
    {
        if (!gUInput || !SetKey || !Gamepad::TakeBindingsChanged())
            return;
        const std::pair<const wchar_t*, std::string> axes[] = { { L"JoyZ", Gamepad::TurnBinding() }, { L"JoyV", Gamepad::LookUpBinding() } };
        for (const auto& [key, binding] : axes)
        {
            auto command = std::wstring(key) + L" \"" + std::wstring(binding.begin(), binding.end()) + L"\"";
            SetKey(gUInput, nullptr, command.c_str(), key);
        }
    }

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
        RefreshHudConstraint();
        UInput::RefreshPadBindings();
        WFP::onGameProcessEvent().executeAll();
        if (hGameWindow)
            CursorConfinement::Frame(hGameWindow);

        // MAIN.PauseOnFocusLoss: while another application is active (WM_ACTIVATEAPP, or minimized) the engine skips its
        // tick and pauses the sound. Off keeps the windowed game running; a minimized fullscreen game still pauses.
        static const auto& nPauseOnFocusLoss = FixSettings().Int("MAIN.PauseOnFocusLoss");
        auto& bInactive = *reinterpret_cast<int32_t*>(reinterpret_cast<uintptr_t>(UGameEngine) + 0x160);
        if (bInactive && !nPauseOnFocusLoss && hGameWindow && !IsIconic(hGameWindow))
            bInactive = 0;

        // Update the shared softbody fixed timestep once per frame
        // All cloth actors use the same substep count and interpolation alpha
        AESoftBodyActor::AdvanceFixedTimestep(deltaTime);

        while (!UWindowsViewport::deferredCauseInputEvent.empty())
        {
            UWindowsViewport::deferredCauseInputEvent.front()();
            UWindowsViewport::deferredCauseInputEvent.pop();
        }

        if (Screen.bRawInputMouse && UWindowsViewport::deferredCauseInputEventForRawInput)
        {
            UWindowsViewport::deferredCauseInputEventForRawInput(228, 4, static_cast<float>(RawInputHandler<int32_t>::RawMouseDeltaX.exchange(0)));
            UWindowsViewport::deferredCauseInputEventForRawInput(229, 4, static_cast<float>(RawInputHandler<int32_t>::RawMouseDeltaY.exchange(0)));
            UWindowsViewport::deferredCauseInputEventForRawInput = nullptr;
        }

        return shTick.unsafe_fastcall(UGameEngine, edx, deltaTime);
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

namespace FLevelSceneNode
{
    float fPlantFarDistance = 100000.0f;
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

// Sam's suit and goggle lights are EGlow materials in ETexCharacter.Sam. Each one draws part of a face or body
// texture unlit in the scene (Material) and again into the glow buffer (GlowMaterial). Both are pointed at a copy of
// that texture where every texel keeps its brightness and takes the chosen hue, so the light and its glow match.
namespace SuitLights
{
    // Blacklist suit light colours at full brightness, in menu order (setting values 1..10)
    constexpr uint32_t kColors[] =
    {
        0x37FF4F, // Classic Green
        0xFFB137, // Topaz
        0xFF3737, // Voron Red
        0x3766FF, // Navy
        0xFF3795, // Pink
        0xB137FF, // Violet
        0xFFFFFF, // White
        0xF3FF37, // Gold
        0xFFB137, // Amber
        0x37E0FF, // Ghost Blue
    };

    // The original lights peak at about 80% brightness; lift them so the brightest texels reach the full colour
    constexpr float kIntensityGain = 1.25f;

    constexpr uintptr_t kObjectOuterOff      = 0x18;
    constexpr uintptr_t kModifierMaterialOff = 0x4C;
    constexpr uintptr_t kGlowMaterialOff     = 0x50;

    constexpr uintptr_t kTexFormatOff        = 0x4C;
    constexpr uintptr_t kTexUClampModeOff    = 0x4D;
    constexpr uintptr_t kTexVClampModeOff    = 0x4E;
    constexpr uintptr_t kTexUSizeOff         = 0x58;
    constexpr uintptr_t kTexVSizeOff         = 0x5C;
    constexpr uintptr_t kTexFlagsOff         = 0x90;
    constexpr uintptr_t kTexMipsOff          = 0xB0;
    constexpr uintptr_t kMipLazyLoaderOff    = 0x10;
    constexpr uintptr_t kMipLazyArchiveOff   = 0x14;
    constexpr uintptr_t kMipDataOff          = 0x1C;
    constexpr uintptr_t kMipDataNumOff       = 0x20;

    constexpr uint8_t TEXF_DXT1  = 3;
    constexpr uint8_t TEXF_RGBA8 = 5;
    constexpr uint8_t TEXF_DXT3  = 7;
    constexpr uint8_t TEXF_DXT5  = 8;

    // Upload the new data like UGameEngine::LoadTgaTexture does
    constexpr uint32_t kTexRealtimeFlags = 0x60;

    uint32_t gColor = 0;
    void* TextureClass = nullptr;
    void** GError = nullptr;
    int(__fastcall* IsA)(void* object, void* edx, void* cls) = nullptr;
    const wchar_t* (__fastcall* GetName)(void* object, void* edx) = nullptr;
    void* (__cdecl* GetTransientPackage)() = nullptr;
    void* (__cdecl* StaticConstructObject)(void* cls, void* outer, int name, uint32_t flags, void* objectTemplate, void* error, int subobjectRoot) = nullptr;
    void(__fastcall* AddToRoot)(void* object, void* edx) = nullptr;
    void(__fastcall* TextureInit)(void* texture, void* edx, int width, int height) = nullptr;
    void(__fastcall* TexturePostLoad)(void* texture, void* edx) = nullptr;

    std::unordered_map<std::wstring, void*> coloredBySource;
    std::unordered_set<void*> colored;

    // Lazy loader vtable: [0] Load, [1] Unload
    void CallMipLoader(uint8_t* mip, int index)
    {
        auto loader = mip + kMipLazyLoaderOff;
        (*reinterpret_cast<void(__fastcall***)(void*, void*)>(loader))[index](loader, nullptr);
    }

    bool LoadMipData(uint8_t* mip)
    {
        if (*reinterpret_cast<int*>(mip + kMipDataNumOff) != 0 || !*reinterpret_cast<void**>(mip + kMipLazyArchiveOff))
            return false;
        CallMipLoader(mip, 0);
        return true;
    }

    void DecodeColorBlock(const uint8_t* block, bool alwaysFourColors, uint32_t* out)
    {
        const uint32_t c0 = block[0] | (block[1] << 8);
        const uint32_t c1 = block[2] | (block[3] << 8);
        uint32_t r[4], g[4], b[4], a[4] = { 255, 255, 255, 255 };
        r[0] = (c0 >> 11) * 255 / 31; g[0] = ((c0 >> 5) & 63) * 255 / 63; b[0] = (c0 & 31) * 255 / 31;
        r[1] = (c1 >> 11) * 255 / 31; g[1] = ((c1 >> 5) & 63) * 255 / 63; b[1] = (c1 & 31) * 255 / 31;
        if (alwaysFourColors || c0 > c1)
        {
            r[2] = (2 * r[0] + r[1]) / 3; g[2] = (2 * g[0] + g[1]) / 3; b[2] = (2 * b[0] + b[1]) / 3;
            r[3] = (r[0] + 2 * r[1]) / 3; g[3] = (g[0] + 2 * g[1]) / 3; b[3] = (b[0] + 2 * b[1]) / 3;
        }
        else
        {
            r[2] = (r[0] + r[1]) / 2; g[2] = (g[0] + g[1]) / 2; b[2] = (b[0] + b[1]) / 2;
            r[3] = g[3] = b[3] = a[3] = 0;
        }
        const uint32_t bits = block[4] | (block[5] << 8) | (block[6] << 16) | (block[7] << 24);
        for (int i = 0; i < 16; i++)
        {
            const uint32_t index = (bits >> (2 * i)) & 3;
            out[i] = (a[index] << 24) | (r[index] << 16) | (g[index] << 8) | b[index];
        }
    }

    void DecodeAlphaBlock(const uint8_t* block, uint8_t format, uint32_t* out)
    {
        for (int i = 0; i < 16; i++)
        {
            uint32_t alpha = 0;
            if (format == TEXF_DXT3)
                alpha = ((block[i / 2] >> ((i & 1) * 4)) & 15) * 17;
            else
            {
                const uint32_t a0 = block[0], a1 = block[1];
                uint64_t bits = 0;
                for (int k = 0; k < 6; k++)
                    bits |= uint64_t(block[2 + k]) << (8 * k);
                const uint32_t index = (bits >> (3 * i)) & 7;
                if (index == 0)
                    alpha = a0;
                else if (index == 1)
                    alpha = a1;
                else if (a0 > a1)
                    alpha = ((8 - index) * a0 + (index - 1) * a1) / 7;
                else if (index < 6)
                    alpha = ((6 - index) * a0 + (index - 1) * a1) / 5;
                else
                    alpha = index == 6 ? 0 : 255;
            }
            out[i] = (out[i] & 0x00FFFFFF) | (alpha << 24);
        }
    }

    bool Decode(uint8_t format, const uint8_t* data, int dataSize, int width, int height, std::vector<uint32_t>& pixels)
    {
        pixels.assign(width * height, 0);
        if (format == TEXF_RGBA8)
        {
            if (dataSize < width * height * 4)
                return false;
            memcpy(pixels.data(), data, width * height * 4);
            return true;
        }

        if (format != TEXF_DXT1 && format != TEXF_DXT3 && format != TEXF_DXT5)
            return false;
        const int blockSize = format == TEXF_DXT1 ? 8 : 16;
        const int blocksX = (width + 3) / 4, blocksY = (height + 3) / 4;
        if (dataSize < blocksX * blocksY * blockSize)
            return false;

        for (int by = 0; by < blocksY; by++)
        {
            for (int bx = 0; bx < blocksX; bx++)
            {
                const uint8_t* block = data + (by * blocksX + bx) * blockSize;
                uint32_t texels[16];
                DecodeColorBlock(block + blockSize - 8, format != TEXF_DXT1, texels);
                if (format != TEXF_DXT1)
                    DecodeAlphaBlock(block, format, texels);
                for (int i = 0; i < 16; i++)
                {
                    const int x = bx * 4 + (i & 3), y = by * 4 + i / 4;
                    if (x < width && y < height)
                        pixels[y * width + x] = texels[i];
                }
            }
        }
        return true;
    }

    uint32_t Tint(uint32_t texel)
    {
        const uint32_t r = (texel >> 16) & 255, g = (texel >> 8) & 255, b = texel & 255;
        const float intensity = std::min(1.0f, std::max({ r, g, b }) / 255.0f * kIntensityGain);
        auto channel = [&](int shift) { return static_cast<uint32_t>(((gColor >> shift) & 255) * intensity + 0.5f) << shift; };
        return (texel & 0xFF000000) | channel(16) | channel(8) | channel(0);
    }

    void* MakeColoredCopy(uint8_t* source)
    {
        const std::wstring key = GetName(source, nullptr);
        if (auto it = coloredBySource.find(key); it != coloredBySource.end())
            return it->second;

        const uint8_t format = source[kTexFormatOff];
        const int width = *reinterpret_cast<int*>(source + kTexUSizeOff);
        const int height = *reinterpret_cast<int*>(source + kTexVSizeOff);
        auto mip = *reinterpret_cast<uint8_t**>(source + kTexMipsOff);
        if (!mip || width <= 0 || height <= 0)
            return nullptr;

        // The renderer streams the source data in and out on its own; leave it as it was found
        const bool loaded = LoadMipData(mip);
        std::vector<uint32_t> pixels;
        const bool decoded = Decode(format, *reinterpret_cast<uint8_t**>(mip + kMipDataOff), *reinterpret_cast<int*>(mip + kMipDataNumOff), width, height, pixels);
        if (loaded)
            CallMipLoader(mip, 1);
        if (!decoded)
            return nullptr;
        for (auto& texel : pixels)
            texel = Tint(texel);

        auto texture = static_cast<uint8_t*>(StaticConstructObject(TextureClass, GetTransientPackage(), 0, 0, nullptr, *GError, 0));
        texture[kTexFormatOff] = TEXF_RGBA8;
        texture[kTexUClampModeOff] = source[kTexUClampModeOff];
        texture[kTexVClampModeOff] = source[kTexVClampModeOff];
        TextureInit(texture, nullptr, width, height);
        TexturePostLoad(texture, nullptr);
        auto newMip = *reinterpret_cast<uint8_t**>(texture + kTexMipsOff);
        LoadMipData(newMip);
        memcpy(*reinterpret_cast<void**>(newMip + kMipDataOff), pixels.data(), pixels.size() * 4);
        *reinterpret_cast<uint32_t*>(texture + kTexFlagsOff) |= kTexRealtimeFlags;

        // Shared by every level that loads the same light texture
        AddToRoot(texture, nullptr);
        coloredBySource.emplace(key, texture);
        colored.insert(texture);
        return texture;
    }

    bool IsSamGlow(uint8_t* glow)
    {
        auto group = *reinterpret_cast<void**>(glow + kObjectOuterOff);
        auto package = group ? *reinterpret_cast<void**>(static_cast<uint8_t*>(group) + kObjectOuterOff) : nullptr;
        return package && _wcsicmp(GetName(group, nullptr), L"Sam") == 0 && _wcsicmp(GetName(package, nullptr), L"ETexCharacter") == 0;
    }

    void Recolor(uint8_t* glow)
    {
        auto& material = *reinterpret_cast<void**>(glow + kModifierMaterialOff);
        auto& glowMaterial = *reinterpret_cast<void**>(glow + kGlowMaterialOff);
        if ((!material || colored.contains(material)) && (!glowMaterial || colored.contains(glowMaterial)))
            return;
        if (!IsSamGlow(glow))
            return;

        for (void** slot : { &material, &glowMaterial })
        {
            if (*slot && !colored.contains(*slot) && IsA(*slot, nullptr, TextureClass))
            {
                if (auto copy = MakeColoredCopy(static_cast<uint8_t*>(*slot)))
                    *slot = copy;
            }
        }
    }

    // Called for every material D3DDrv sets up; returns the EGlow modifier of the material, if any
    SafetyHookInline shRetreiveGlowModifier = {};
    void* __cdecl RetreiveGlowModifier(void* material)
    {
        auto glow = shRetreiveGlowModifier.ccall<uint8_t*>(material);
        if (glow)
            Recolor(glow);
        return glow;
    }

    void Init()
    {
        if (nSuitLightColor <= 0 || nSuitLightColor > static_cast<int>(std::size(kColors)))
            return;

        // Normalise to full brightness so a dimmer table entry keeps its hue
        const uint32_t color = kColors[nSuitLightColor - 1];
        const uint32_t peak = std::max({ (color >> 16) & 255, (color >> 8) & 255, color & 255 });
        for (int shift : { 16, 8, 0 })
            gColor |= std::min(255u, ((color >> shift) & 255) * 255 / std::max(peak, 1u)) << shift;

        auto core = GetModuleHandle(L"Core");
        auto engine = GetModuleHandle(L"Engine");
        TextureClass = GetProcAddress(engine, "?PrivateStaticClass@UTexture@@0VUClass@@A");
        GError = reinterpret_cast<void**>(GetProcAddress(core, "?GError@@3PAVFOutputDeviceError@@A"));
        IsA = reinterpret_cast<decltype(IsA)>(GetProcAddress(core, "?IsA@UObject@@QBEHPAVUClass@@@Z"));
        GetName = reinterpret_cast<decltype(GetName)>(FindProcAddress(core, "?GetName@UObject@@QBEPBGXZ", "?GetName@UObject@@QBEPB_WXZ"));
        GetTransientPackage = reinterpret_cast<decltype(GetTransientPackage)>(GetProcAddress(core, "?GetTransientPackage@UObject@@SAPAVUPackage@@XZ"));
        StaticConstructObject = reinterpret_cast<decltype(StaticConstructObject)>(GetProcAddress(core, "?StaticConstructObject@UObject@@SAPAV1@PAVUClass@@PAV1@VFName@@K1PAVFOutputDevice@@H@Z"));
        AddToRoot = reinterpret_cast<decltype(AddToRoot)>(GetProcAddress(core, "?AddToRoot@UObject@@QAEXXZ"));
        TextureInit = reinterpret_cast<decltype(TextureInit)>(GetProcAddress(engine, "?Init@UTexture@@UAEXHH@Z"));
        TexturePostLoad = reinterpret_cast<decltype(TexturePostLoad)>(GetProcAddress(engine, "?PostLoad@UTexture@@UAEXXZ"));
        if (!TextureClass || !GError || !IsA || !GetName || !GetTransientPackage || !StaticConstructObject || !AddToRoot || !TextureInit || !TexturePostLoad)
            throw std::runtime_error("Suit light exports not found");

        shRetreiveGlowModifier = safetyhook::create_inline(GetProcAddress(engine, "?RetreiveGlowModifier@UEGlow@@SAPAV1@PAVUMaterial@@@Z"), RetreiveGlowModifier);
    }
}

export void InitEngine()
{
    auto digitalHealth = hook::module_pattern(GetModuleHandle(L"Engine"), "6A 0A 6A 00 6A 01 8D 85 ? ? ? ? 50 57 E8 ? ? ? ? 8B 35 ? ? ? ? 83 C4 20");
    auto retailShadow = hook::module_pattern(GetModuleHandle(L"Engine"), "68 00 00 80 3F 68 6F 12 83 C6");

    auto pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B 8E 94 00 00 00 E8 ? ? ? ? 83 7D F8 00 8B 4D CC");
    if (pattern.empty()) // retail
        pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B 8A 94 00 00 00 E8 ? ? ? ? 8B 4C 24 28 E9 ? ? ? ? 8D 44 24 78");
    FCanvasUtil::DrawTile = (decltype(FCanvasUtil::DrawTile))injector::GetBranchDestination(pattern.get_first(6), true).as_int();

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
        {
            injector::MakeCALL(rpattern.get(3).get<uint32_t>(0), FCanvasUtilDrawTileHook, true); //pfsub_103762F0 + 0x36E
            injector::MakeCALL(rpattern.get(5).get<uint32_t>(0), FCanvasUtilDrawTileHook, true); //pfsub_103762F0 + 0x43D
            injector::MakeCALL(rpattern.get(7).get<uint32_t>(0), FCanvasUtilDrawTileHook, true); //pfsub_103762F0 + 0x4DA
            injector::MakeCALL(rpattern.get(9).get<uint32_t>(0), FCanvasUtilDrawTileHook, true); //pfsub_103762F0 + 0x564
        }
    }
    else
    {
        pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "E8 ? ? ? ? 83 7D ? ? 8B 4D");
        injector::MakeCALL(pattern.get_first(0), FCanvasUtilDrawTileHook, true);
    }

    // Both renderers use the same player camera hook.
    PTGameplayCamera::fullNameSource = &UObject::GetFullName;
    const std::pair<const char*, int> cameraCallers[] =
    {
        { "8B CC 89 11 8B 55 D8 89 41 04 8B 45 C0 50 89 51 08 53 8D 8D 70 F9 FF FF E8 ? ? ? ?", 24 },
        { "83 EC 0C 8B C4 89 08 8B 4D E0 89 50 04 8B 55 08 52 89 48 08 57 8D 8D DC FC FF FF E8 ? ? ? ?", 27 },
        { "83 EC 0C 8B C4 89 08 8B 4D E0 89 50 04 8B 55 08 52 89 48 08 57 8D 8D D0 FC FF FF E8 ? ? ? ?", 27 },
        { "8B C4 FF B5 F8 FA FF FF 8B 8D B0 FA FF FF 56 66 0F D6 00 89 48 08 8D 8D 00 FB FF FF E8 ? ? ? ?", 28 },
        { "8B CC FF B5 20 FD FF FF 8B 85 10 FD FF FF 57 66 0F D6 01 89 41 08 8D 8D 24 FD FF FF E8 ? ? ? ?", 28 },
        { "8B CC FF B5 20 FD FF FF 8B 85 0C FD FF FF 57 66 0F D6 01 89 41 08 8D 8D 24 FD FF FF E8 ? ? ? ?", 28 },
    };
    const auto engine = GetModuleHandle(L"Engine");
    const size_t firstCameraCaller = hook::module_pattern(engine, cameraCallers[0].first).empty() ? 3 : 0; // digital / retail
    for (size_t i = firstCameraCaller; i < firstCameraCaller + 3; ++i)
    {
        auto caller = hook::module_pattern(engine, cameraCallers[i].first);
        auto call = caller.get_first<void>(cameraCallers[i].second);
        PTGameplayCamera::callers[i] = reinterpret_cast<uintptr_t>(call) + 5;
    }

    // The camera's horizontal field of view (for 4:3) at the end of the FCameraSceneNode constructor
    static auto AdjustCameraFOV = [](void* node, uintptr_t returnAddress)
    {
        auto& fov = *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(node) + 0x214);
        if (UObject::GetState(L"EchelonMainHUD") == L"s_Cinematic")
        {
            fov = PTCutscene::FOV(fov, Screen.fAspectRatio);
            return;
        }
        static const auto& fFOVFactor = FixSettings().Float("MAIN.FOVFactor");
        const float factor = PTGameplayCamera::Factor(node, returnAddress, fFOVFactor);
        // The factor widens up to 150 degrees across: on very wide screens anything wider breaks the picture up
        constexpr float kMaxFactorFOV = 150.0f;
        const float base = std::clamp(AdjustFOV(fov, Screen.fAspectRatio), 1.0f, 179.0f);
        fov = std::clamp(base * factor, 1.0f, std::max(base, kMaxFactorFOV));
    };
    // Retail keeps the node in esi, the current build in edi
    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "E8 ? ? ? ? 8B 4D ? 5F 8B C6 5E 64 89 0D ? ? ? ? 5B 8B E5 5D C2 24 00");
    static const bool bNodeInEsi = !pattern.empty();
    if (!bNodeInEsi)
        pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "E8 ? ? ? ? 8B C7 8B 4D ? 64 89 0D ? ? ? ? 59 5F 5E 8B E5 5D C2 24 00");
    {
        static auto FCameraSceneNodeCtorHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            AdjustCameraFOV(reinterpret_cast<void*>(bNodeInEsi ? regs.esi : regs.edi), *reinterpret_cast<uintptr_t*>(regs.ebp + 4));
        });
    }

    if (Screen.nPostProcessFixedScale)
    {
        if (Screen.nPostProcessFixedScale == 1)
            Screen.nPostProcessFixedScale = Screen.Width;

        auto pattern2 = hook::module_pattern(GetModuleHandle(L"Engine"), "68 00 02 00 00 68 00 02 00 00");
        if (pattern2.size() != 6)
            throw std::runtime_error("Post-process signatures not found");
        for (size_t i = 0; i < pattern2.size(); i++)
        {
            injector::WriteMemory(pattern2.get(i).get<uint32_t>(1), Screen.nPostProcessFixedScale, true);
            injector::WriteMemory(pattern2.get(i).get<uint32_t>(6), Screen.nPostProcessFixedScale, true);
        }

        // Thermal vision uses a separate render target size so it bypasses the main fix, apply PostProcessFixedScale here as well
        auto thermalRT = find_module_pattern(GetModuleHandle(L"Engine"), "8B 0C 85 ? ? ? ? 8B 75 10", "8B 34 9D ? ? ? ? 57 75");
        {
            // Override all thermal RT presets
            uintptr_t thermalSceneSizes = *thermalRT.get_first<uintptr_t>(3);
            for (int i = 0; i < 3; i++)
                injector::WriteMemory<uint32_t>(thermalSceneSizes + i * 4, Screen.nPostProcessFixedScale, true);
        }
    }

    // Force thermal vision to use quality preset 2 (1024px pyramid)
    {
        auto thermalDispatch = find_module_pattern(GetModuleHandle(L"Engine"), "FF B5 FC FB FF FF 56 53 6A 00 E8", "0C 8B 55 08 50 51 52 56 6A 00 E8");
        injector::WriteMemory<uint8_t>(thermalDispatch.get_first(9), 2, true);
    }

    SuitLights::Init();

    #if _DEBUG
    auto pFindAxisName = FindProcAddress(GetModuleHandle(L"Engine"), "?FindAxisName@UInput@@MBEPAMPAVAActor@@PB_W@Z", "?FindAxisName@UInput@@MBEPAMPAVAActor@@PBG@Z");
    if (pFindAxisName)
        shFindAxisName = safetyhook::create_inline(pFindAxisName, FindAxisName);
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
    if (!digitalHealth.empty()) // digital
        injector::MakeNOP(digitalHealth.get_first(14), 5, true); // skip PlayMovie("HealthWarning_%s.bik")
    else // retail
        retailShadow.get_first();

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
    UGameEngine::shPressStartToContinue = safetyhook::create_inline(pattern.get_first(), UGameEngine::PressStartToContinue);

    if (bSkipPressStartToContinue)
    {
        pattern = find_module_pattern(GetModuleHandle(L"Engine"), "0F 84 ? ? ? ? F3 0F 10 45 ? F3 0F 59 05");
        if (!pattern.empty())
            injector::WriteMemory<uint16_t>(pattern.get_first(0), 0xE990, true); // jz -> jmp
        else
        {
            pattern = find_module_pattern(GetModuleHandle(L"Engine"), "74 ? D9 45 ? D8 0D ? ? ? ? E8");
            injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true); // jz -> jmp
        }
    }

    UInput::GetKey = (decltype(UInput::GetKey))FindProcAddress(GetModuleHandle(L"Engine"), "?GetKey@UInput@@UAEEPB_WH@Z", "?GetKey@UInput@@UAEEPBGH@Z");
    UInput::SetKey = reinterpret_cast<decltype(UInput::SetKey)>(FindProcAddress(GetModuleHandle(L"Engine"), "?SetKey@UInput@@UAEXPB_W0@Z", "?SetKey@UInput@@UAEXPBG0@Z"));
    UEngine::shInputEvent = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Engine"), "?InputEvent@UEngine@@UAEHPAVUViewport@@W4EInputKey@@W4EInputAction@@M@Z"), UEngine::InputEvent);
    UInput::shInit = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Engine"), "?Init@UInput@@UAEXPAVUViewport@@@Z"), UInput::Init);

    // Restore the in-game console
    if (bEnableConsole)
    {
        UInteractionMaster::FName_ctor = (decltype(UInteractionMaster::FName_ctor))FindProcAddress(GetModuleHandle(L"Core"), "??0FName@@QAE@PB_WW4EFindName@@@Z", "??0FName@@QAE@PBGW4EFindName@@@Z");
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
            "55 8B EC 56 8B F1 8B 8E 6C 03 00 00 85 C9 74 08 8B 01 FF 90 98 00 00 00",
            "56 8B F1 8B 86 6C 03 00 00 85 C0 74 0A 8B C8 8B 01 FF 90 98 00 00 00");

        auto sbUpdate = find_module_pattern(GetModuleHandle(L"Engine"),
            "8B 43 60 8B 80 B8 00 00 00 F3 0F 10 15 ? ? ? ? 5E F3 0F 10 80 64 07 00 00 0F 2F D0 77 0B F3 0F 5D 05");

        auto sbUpdateRetail = find_module_pattern(GetModuleHandle(L"Engine"),
            "8B 57 60 8B 82 B8 00 00 00 D9 80 64 07 00 00 D8 15 ? ? ? ? DF E0 F6 C4 05 7A 0A DD D8 D9 05");

        {
            AESoftBodyActor::AActorTick = reinterpret_cast<AESoftBodyActor::AActorTickFn>(
                GetProcAddress(GetModuleHandle(L"Engine"), "?Tick@AActor@@UAEHMW4ELevelTick@@@Z"));

            AESoftBodyActor::shTick = safetyhook::create_inline(sbActorTick.get_first(), AESoftBodyActor::Tick);

            if (sbUpdate.size())
            {
                uintptr_t movss = reinterpret_cast<uintptr_t>(sbUpdate.get_first(18));
                injector::WriteMemory<uint8_t>(movss + 3, 0x05, true);
                injector::WriteMemory<uintptr_t>(movss + 4, reinterpret_cast<uintptr_t>(&AESoftBodyActor::fFrameDelta), true);

                uintptr_t clampLow = *sbUpdate.get_first<uintptr_t>(13);
                injector::WriteMemory<float>(clampLow, AESoftBodyActor::kMinDt, true);
            }
            else // retail: x87 fld m32
            {
                uintptr_t fld = reinterpret_cast<uintptr_t>(sbUpdateRetail.get_first(9));
                injector::WriteMemory<uint8_t>(fld + 1, 0x05, true);
                injector::WriteMemory<uintptr_t>(fld + 2, reinterpret_cast<uintptr_t>(&AESoftBodyActor::fFrameDelta), true);

                uintptr_t clampLow = *sbUpdateRetail.get_first<uintptr_t>(17);
                injector::WriteMemory<float>(clampLow, AESoftBodyActor::kMinDt, true);
            }
        }
    }

    AEGrassActor::shPostLoad = safetyhook::create_inline(
        GetProcAddress(GetModuleHandle(L"Engine"), "?PostLoad@AEGrassActor@@UAEXXZ"),
        AEGrassActor::PostLoad);

    AActor::shPostLoad = safetyhook::create_inline(
        GetProcAddress(GetModuleHandle(L"Engine"), "?PostLoad@AActor@@UAEXXZ"),
        AActor::PostLoad);

    pattern = find_module_pattern(GetModuleHandle(L"Engine"), "FF 15 ? ? ? ? D9 5D 08 F3 0F 10 45 08 0F 2F 05 ? ? ? ? 0F 86", "A7 94 01 00 00 D9 5C 24 1C FF 15 ? ? ? ? D8 1D ? ? ? ? DF E0");
    {
        injector::WriteMemory<uintptr_t>(pattern.get_first(17), reinterpret_cast<uintptr_t>(&FLevelSceneNode::fPlantFarDistance), true);
    }

    // Buffered shadows (retail): the shadow buffer texture matrix puts the compared depth in 24-bit integer units
    // (z * 16777215, bias -16777.217), which only the hardware of the time understood. Current drivers compare in 0..1,
    // so every receiver ends up behind the stored depth. Use the same matrix in 0..1 units.
    if (!retailShadow.empty()) // retail
    {
        injector::WriteMemory<float>(retailShadow.get_first<float>(6), -0.001f, true);

        pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "6A 00 68 FF FF 7F 4B 6A 00 6A 00");
        injector::WriteMemory<float>(pattern.get_first<float>(3), 1.0f, true);
    }
    else // digital
        digitalHealth.get_first();

    // Fix thermal vision grain scaling at high resolutions
    pattern = find_module_pattern(GetModuleHandle(L"Engine"), "8B 46 04 8B 80 84 00 00 00 99 F7 F9 66 0F 6E C0", "04 8B C8 8B 82 84 00 00 00 99 F7 F9");
    {
        static auto ThermalGrainHeightHook = safetyhook::create_mid(pattern.get_first(9), [](SafetyHookContext& regs)
        {
            regs.eax = static_cast<uint32_t>((float)regs.eax + (480.0f - (float)regs.eax) * Screen.fGrainScale);
        });
    }

    pattern = find_module_pattern(GetModuleHandle(L"Engine"), "8B 46 04 8B 80 80 00 00 00 99 F7 F9 8D 8D 74 FE FF FF", "04 8B C8 8B 82 80 00 00 00 99 F7 F9");
    {
        static auto ThermalGrainWidthHook = safetyhook::create_mid(pattern.get_first(9), [](SafetyHookContext& regs)
        {
            regs.eax = static_cast<uint32_t>((float)regs.eax + (640.0f - (float)regs.eax) * Screen.fGrainScale);
        });
    }
}

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

namespace FCanvasUtil
{
    void(__fastcall* DrawTile)(void* _this, uint32_t EDX, float, float, float, float, float, float, float, float, float, void*, FColor);
}

void __fastcall FCanvasUtilDrawTileHook(void* _this, uint32_t EDX, float X, float Y, float SizeX, float SizeY, float U, float V, float SizeU, float SizeV, float unk1, void* Texture, FColor Color)
{
    bool bIsMainMenu = UObject::GetState(L"EPCConsole") == L"UWindow";

    static FColor ColBlack; ColBlack.RGBA = 0xFF000000;

    if (!bIsMainMenu)
    {
        const float fZoomScopeBorders = (Screen.fHeight * (4.0f / 3.0f)) / (640.0f / 188.0f);
        const float fStickyCamera = (Screen.fHeight * (4.0f / 3.0f)) / (640.0f / 43.0f);
        const float fOpticCable = (Screen.fHeight * (4.0f / 3.0f)) / (640.0f / 30.0f);
        const float fOpticMic = (Screen.fHeight * (4.0f / 3.0f)) / (640.0f / 163.0f);

        if (X == 0.0f && (FloatEqual(SizeX, fZoomScopeBorders) || FloatEqual(SizeX, fStickyCamera) ||
            FloatEqual(SizeX, fOpticCable) || FloatEqual(SizeX, fOpticMic)))
        {
            FCanvasUtil::DrawTile(_this, EDX, 0.0f, Y, Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, nullptr, ColBlack);
            FCanvasUtil::DrawTile(_this, EDX, Screen.fWidth - Screen.fHudOffset - 1.0f, Y, Screen.fWidth, SizeY, U, V, SizeU, SizeV, unk1, nullptr, ColBlack);
        }
    }

    const float fExpectedHeight = Screen.fHeight * (4.0f / 3.0f);

    if (X == 0.0f && FloatEqual(SizeX, fExpectedHeight))
    {
        if ((Color.R == 0x40 && Color.G == 0x00 && Color.B == 0x00) || (Color.R == 0x80 && Color.G == 0x80 && Color.B == 0x80)) //mission failed red screen
        {
            if (!bIsMainMenu)
            {
                FCanvasUtil::DrawTile(_this, EDX, X, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color);
                return;
            }
        }
        else if ((Color.R == 0x00 && Color.G == 0x40 && Color.B == 0x00)) //mission passed green screen
        {
            if (!bIsMainMenu)
            {
                FCanvasUtil::DrawTile(_this, EDX, X, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color);
                return;
            }
        }
        else
        {
            FCanvasUtil::DrawTile(_this, EDX, 0.0f, Y, Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, ColBlack);
            FCanvasUtil::DrawTile(_this, EDX, SizeX + Screen.fHudOffset, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset + 1.0f, SizeY + 1.0f, U, V, SizeU, SizeV, unk1, Texture, ColBlack);
        }
    }

    if (!bIsMainMenu && Screen.nHudWidescreenMode == 1)
    {
        wchar_t buffer[256];
        curDrawTileManagerTextureName = UObject::GetFullName(Texture, 0, buffer);
        WidescreenHud(X, SizeX, Y, SizeY, Color);
    }

    X += Screen.fHudOffset;
    SizeX += Screen.fHudOffset;

    return FCanvasUtil::DrawTile(_this, EDX, X, Y, SizeX, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color);
}

namespace UGameEngine
{
    SafetyHookInline shDisplaySplash = {};
    void __fastcall DisplaySplash(void* UGameEngine, void* edx, int a2)
    {
        bDisplayingSplash = true;
        shDisplaySplash.unsafe_fastcall(UGameEngine, edx, a2);
        bDisplayingSplash = false;
    }

    SafetyHookInline shDisplaySplashProgress = {};
    void __fastcall DisplaySplashProgress(void* UGameEngine, void* edx, float a2)
    {
        bDisplayingSplash = true;
        shDisplaySplashProgress.unsafe_fastcall(UGameEngine, edx, a2);
        bDisplayingSplash = false;
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

// Softbody FPS fixes
namespace AESoftBodyActor
{
    constexpr uintptr_t kSoftBodyMember      = 0x2A8;
    constexpr uintptr_t kVertCountOff        = 0x98;
    constexpr uintptr_t kVertArrayOff        = 0x94;
    constexpr uintptr_t kVertStride          = 0x48;
    constexpr uintptr_t kVertPosOff          = 0x04;
    constexpr uintptr_t kSmoothStateOff      = 0x158;
    constexpr uintptr_t kSoftBodyTickVtblOff          = 0x84;
    constexpr uintptr_t kSoftBodyUpdateBuffersOff     = 0x74;
    constexpr uintptr_t kSoftBodyUpdateAttachmentsOff = 0xB8;
    constexpr uintptr_t kSoftBodyUpdateBoundsOff      = 0xBC;

    constexpr float kFixedDt = 1.0f / 30.0f;
    constexpr float kMinDt = 1.0f / 1000.0f; // Engine originally clamped dt at 1/200s, widened to 1/1000s

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
        int   nVerts      = 0;
        bool  initialized = false;
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

export void InitEngine()
{
    auto pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "68 ? ? ? ? 50 8D 8C 24 ? ? ? ? FF 15 ? ? ? ? 8B C8 FF 15 ? ? ? ? 50 8D 4C 24 ? FF 15 ? ? ? ? 6A 00");
    a_sav = *pattern.get_first<const wchar_t*>(1);

    //HUD
    InitWidescreenHUD();

    auto flt_104E9F78 = *hook::pattern(GetModuleHandle(L"Engine"), "D8 C9 D8 0D").count(9).get(0).get<float*>(4);
    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), pattern_str(0xD8, 0xC9, 0xD8, 0x0D, to_bytes(flt_104E9F78)));
    for (size_t i = 0; i < pattern.count_hint(4).size(); ++i)
    {
        injector::WriteMemory(pattern.get(i).get<void>(4), &Screen.HUDScaleX, true); //(DWORD)Engine + 0x1E9F78
    }

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B ? 94 00 00 00 E8");
    FCanvasUtil::DrawTile = (decltype(FCanvasUtil::DrawTile))injector::GetBranchDestination(pattern.count(1).get(0).get<uintptr_t>(6), true).as_int();

    injector::MakeCALL(pattern.count(1).get(0).get<uintptr_t>(6), FCanvasUtilDrawTileHook, true); //(uint32_t)Engine + 0xC8ECE

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B ? 94 00 00 00 52 E8");
    injector::MakeCALL(pattern.count(2).get(0).get<uint32_t>(7), FCanvasUtilDrawTileHook, true); //(uint32_t)Engine + 0xC9B7C
    injector::MakeCALL(pattern.count(2).get(1).get<uint32_t>(7), FCanvasUtilDrawTileHook, true); //(uint32_t)Engine + 0xC9DE1

    //FOV
    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B CB 89 42 ? 89 72");
    static auto FCameraSceneNodeCtorHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (bRestoreCutsceneFOV && UObject::GetState(L"EchelonMainHUD") == L"s_Cinematic")
        {
            return;
        }

        *(float*)(regs.ebx + 0x200) = AdjustFOV(*(float*)(regs.ebx + 0x200), Screen.fAspectRatio);
    });

    // Resolution overrides
    static auto newSETRES = L"SETRES 000x000"; // Final video resolution switch to 640x480, also crashes with dgvoodoo on some resolutions
    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "68 ? ? ? ? 8B 01 FF 10 68 00 00 80 3F");
    injector::WriteMemory(pattern.get_first(1), newSETRES, true);

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "68 ? ? ? ? 50 FF D5 83 C4 08 85 C0 75 28 84 5E 58 74 23 8B 0D ? ? ? ? 8B 51 48 8B 42 30 8B 00 8D 48 30 A1 ? ? ? ? 8B 00 50 68 ? ? ? ? E9 1F 01 00 00");
    a640x480 = *pattern.get_first<const wchar_t*>(1);

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "68 ? ? ? ? 50 FF D5 83 C4 08 85 C0 75 09");
    a800x600 = *pattern.get_first<const wchar_t*>(1);

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "68 ? ? ? ? 50 FF D5 83 C4 08 85 C0 75 28 84 5E 58 74 23 8B 0D ? ? ? ? 8B 51 48 8B 42 30 8B 00 8D 48 30 A1 ? ? ? ? 8B 00 50 68 ? ? ? ? E9 AE 00 00 00");
    a1024x768 = *pattern.get_first<const wchar_t*>(1);

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "68 ? ? ? ? 50 FF D5 83 C4 08 85 C0 75 25");
    a1280x1024 = *pattern.get_first<const wchar_t*>(1);

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "68 ? ? ? ? 50 FF D5 83 C4 08 85 C0 8A 46 58");
    a1600x1200 = *pattern.get_first<const wchar_t*>(1);

    {
        injector::scoped_unprotect(const_cast<wchar_t*>(a640x480), wcslen(a640x480));
        wcscpy_s(const_cast<wchar_t*>(a640x480), ResList[0].second.size() + 1, ResList[0].second.c_str());
    }

    {
        injector::scoped_unprotect(const_cast<wchar_t*>(a800x600), wcslen(a800x600));
        wcscpy_s(const_cast<wchar_t*>(a800x600), ResList[1].second.size() + 1, ResList[1].second.c_str());
    }

    {
        injector::scoped_unprotect(const_cast<wchar_t*>(a1024x768), wcslen(a1024x768));
        wcscpy_s(const_cast<wchar_t*>(a1024x768), ResList[2].second.size() + 1, ResList[2].second.c_str());
    }

    {
        injector::scoped_unprotect(const_cast<wchar_t*>(a1280x1024), wcslen(a1280x1024));
        wcscpy_s(const_cast<wchar_t*>(a1280x1024), ResList[3].second.size() + 1, ResList[3].second.c_str());
    }

    {
        injector::scoped_unprotect(const_cast<wchar_t*>(a1600x1200), wcslen(a1600x1200));
        wcscpy_s(const_cast<wchar_t*>(a1600x1200), ResList[4].second.size() + 1, ResList[4].second.c_str());
    }

    // LOD
    if (!IsEnhanced())
    {
        pattern = find_module_pattern(GetModuleHandle(L"Engine"), "0F 84 ? ? ? ? 8B 47 24 8D 8C 24 90 00 00 00");
        injector::WriteMemory<uint16_t>(pattern.get_first(0), 0xE990, true); // jz -> jmp
    }

    // Prevent fake backdrops from rendering in thermal vision
    pattern = find_module_pattern(GetModuleHandle(L"Engine"), "8B 44 24 2C 85 C0 0F 8E ? ? ? ? 8B 43 04 8B 48 34 8B 81 04 04 00 00 83 F8 05");
    static auto ThermalFakeBackdropHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        constexpr int32_t  REN_ThermalVision = 10;
        constexpr uint32_t PF_FakeBackdrop   = 0x00000080; // EPolyFlags::PF_FakeBackdrop

        uintptr_t sceneNode = regs.ebx; // FSceneNode*
        uintptr_t surf      = regs.edi; // FBspSurf*
        if (!sceneNode || !surf)
            return;

        uintptr_t viewport = *(uintptr_t*)(sceneNode + 0x04);
        if (!viewport)
            return;

        uintptr_t actor = *(uintptr_t*)(viewport + 0x34);
        if (!actor)
            return;

        int32_t  rendMap   = *(int32_t*)(actor + 0x404); // AActor::RendMap
        uint32_t polyFlags = *(uint32_t*)(surf + 0x04);  // FBspSurf::PolyFlags

        if (rendMap == REN_ThermalVision && (polyFlags & PF_FakeBackdrop))
            *(uint32_t*)(regs.esp + 0x2C) = 0;
    });

    // Fix thermal vision grain scaling at high resolutions
    static auto ThermalGrainScaleHeight = [](SafetyHookContext& regs)
    {
        float blended = (float)regs.eax + (480.0f - (float)regs.eax) * Screen.fGrainScale;
        regs.eax = (uint32_t)blended;
    };

    static auto ThermalGrainScaleWidth = [](SafetyHookContext& regs)
    {
        uint32_t height = *(uint32_t*)(regs.edx + 0x80); // UViewport screen height
        if (height == 0)
            return;

        uint32_t origWidth = regs.eax;
        uint32_t scaledWidth = (uint32_t)((uint64_t)origWidth * 480ull / height); // width * 480 / height
        if (scaledWidth < 1u)
            scaledWidth = 1u;

        float blended = (float)origWidth + ((float)scaledWidth - (float)origWidth) * Screen.fGrainScale;

        uint32_t eff = (uint32_t)blended;
        regs.eax = eff < 1u ? 1u : eff;
    };

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B 82 80 00 00 00 99 F7 F9 8D 4C 24 30");
    size_t nThermalGrain = pattern.count_hint(2).size();
    if (nThermalGrain >= 1)
    {
        static auto ThermalGrainHeightHook0 = safetyhook::create_mid(pattern.get(0).get<void>(0x06), ThermalGrainScaleHeight);
        static auto ThermalGrainWidthHook0  = safetyhook::create_mid(pattern.get(0).get<void>(0x3C), ThermalGrainScaleWidth);
    }
    if (nThermalGrain >= 2)
    {
        static auto ThermalGrainHeightHook1 = safetyhook::create_mid(pattern.get(1).get<void>(0x06), ThermalGrainScaleHeight);
        static auto ThermalGrainWidthHook1  = safetyhook::create_mid(pattern.get(1).get<void>(0x3C), ThermalGrainScaleWidth);
    }

    // Thermal vision noise grain FPS fix
    if (VisionNoiseGrain::Init())
    {
        // Noise grain TexMatrix random rotation (appFrand angle)
        auto patternAngle = hook::module_pattern(GetModuleHandle(L"Engine"), "89 4A 4C FF 15 ? ? ? ? A1");
        // Noise grain TexMatrix random UV offset (appFrand V/U)
        auto patternUV = hook::module_pattern(GetModuleHandle(L"Engine"), "D9 5C 24 28 FF 15 ? ? ? ? D8 4C 24 28 51 D9 1C 24 FF 15 ? ? ? ?");

        static constexpr float(__cdecl* kAngleFns[2])() = { VisionNoiseGrain::HeldFrand<2, 0>, VisionNoiseGrain::HeldFrand<3, 0> };
        static constexpr float(__cdecl* kVFns[2])()     = { VisionNoiseGrain::HeldFrand<2, 1>, VisionNoiseGrain::HeldFrand<3, 1> };
        static constexpr float(__cdecl* kUFns[2])()     = { VisionNoiseGrain::HeldFrand<2, 2>, VisionNoiseGrain::HeldFrand<3, 2> };

        size_t nAngle = patternAngle.count_hint(2).size();
        size_t nUV = patternUV.count_hint(2).size();
        size_t nClusters = nAngle < nUV ? nAngle : nUV;
        for (size_t i = 0; i < nClusters && i < 2; ++i)
        {
            VisionNoiseGrain::RedirectCallSite(reinterpret_cast<uintptr_t>(patternAngle.get(i).get<void>(0x03)), kAngleFns[i]); // rotation angle
            VisionNoiseGrain::RedirectCallSite(reinterpret_cast<uintptr_t>(patternUV.get(i).get<void>(0x04)), kVFns[i]);        // V offset
            VisionNoiseGrain::RedirectCallSite(reinterpret_cast<uintptr_t>(patternUV.get(i).get<void>(0x12)), kUFns[i]);        // U offset
        }
    }

    if (bSkipIntro)
    {
        pattern = find_module_pattern(GetModuleHandle(L"Engine"), "75 ? A1 ? ? ? ? 85 C0 75 ? 68 00 00 80 3F");
        injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true); // jz -> jmp

        pattern = find_module_pattern(GetModuleHandle(L"Engine"), "75 ? 39 3D ? ? ? ? 75");
        injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true); // jz -> jmp
    }

    pattern = find_module_pattern(GetModuleHandle(L"Engine"), "8B 0D ? ? ? ? 56 8B 74 24 10");
    static auto VideoPlaybackStartHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (*(int32_t*)(regs.esp + 4) == 0 && *(int32_t*)(regs.esp + 8) == 0)
            bPlayingVideo = true;
        else
            bGadgetVideoIsPlaying = true;
    });

    pattern = find_module_pattern(GetModuleHandle(L"Engine"), "8B C1 56 8B 74 24 0C");
    static auto VideoPlaybackEndHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        bPlayingVideo = false;
        bGadgetVideoIsPlaying = false;
    });

    pattern = find_module_pattern(GetModuleHandle(L"Engine"), "8B 55 00 6A 01 8B CD FF 52 ? 6A 00");
    static auto PressStartToContinueHook1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        bPressStartToContinue = true;
    });

    pattern = find_module_pattern(GetModuleHandle(L"Engine"), "A1 ? ? ? ? 83 38 00 74 ? C7 85 B4 01 00 00 01 00 00 00");
    static auto PressStartToContinueHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        bPressStartToContinue = false;
    });

    if (bSkipPressStartToContinue)
    {
        pattern = find_module_pattern(GetModuleHandle(L"Engine"), "75 ? ? ? ? ? ? 8B CD FF 52 ? 6A 00");
        injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true); // jz -> jmp
    }

    UInput::GetKey = (decltype(UInput::GetKey))GetProcAddress(GetModuleHandle(L"Engine"), "?GetKey@UInput@@UAEEPBGH@Z");
    UInput::shInit = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Engine"), "?Init@UInput@@UAEXPAVUViewport@@@Z"), UInput::Init);

    UGameEngine::shDisplaySplash = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Engine"), "?DisplaySplash@UGameEngine@@UAEXH@Z"), UGameEngine::DisplaySplash);
    UGameEngine::shDisplaySplashProgress = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Engine"), "?DisplaySplashProgress@UGameEngine@@UAEXM@Z"), UGameEngine::DisplaySplashProgress);
    UGameEngine::shTick = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Engine"), "?Tick@UGameEngine@@UAEXM@Z"), UGameEngine::Tick);

    UCanvas::shSetClip = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Engine"), "?SetClip@UCanvas@@UAEXMM@Z"), UCanvas::SetClip);

    UTexture::shTick = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Engine"), "?Tick@UTexture@@UAEXM@Z"), UTexture::Tick);

    pattern = find_module_pattern(GetModuleHandle(L"Engine"), "8B 85 40 01 00 00 6A 00 8B 48 18");
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
            "56 8B F1 8B 8E A8 02 00 00 85 C9 74 08 8B 01 FF 90 84 00 00 00");

        auto sbUpdate = find_module_pattern(GetModuleHandle(L"Engine"),
            "56 8B F1 57 8B 46 5C 8B 48 6C D9 81 5C 05 00 00 D8 15");

        if (sbActorTick.size() && sbUpdate.size())
        {
            AESoftBodyActor::AActorTick = reinterpret_cast<AESoftBodyActor::AActorTickFn>(
                GetProcAddress(GetModuleHandle(L"Engine"), "?Tick@AActor@@UAEHMW4ELevelTick@@@Z"));

            AESoftBodyActor::shTick = safetyhook::create_inline(sbActorTick.get_first(), AESoftBodyActor::Tick);

            uintptr_t fld = reinterpret_cast<uintptr_t>(sbUpdate.get_first(10));
            injector::WriteMemory<uint8_t>(fld + 1, 0x05, true);
            injector::WriteMemory<uintptr_t>(fld + 2, reinterpret_cast<uintptr_t>(&AESoftBodyActor::fFrameDelta), true);

            uintptr_t clampLow = *sbUpdate.get_first<uintptr_t>(18);
            injector::WriteMemory<float>(clampLow, AESoftBodyActor::kMinDt, true);
        }
    }

    #if _DEBUG
    shFindAxisName = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Engine"), "?FindAxisName@UInput@@MBEPAMPAVAActor@@PBG@Z"), FindAxisName);
    #endif
}
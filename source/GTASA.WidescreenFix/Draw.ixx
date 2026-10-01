module;

#include <stdafx.h>
#include "common.h"

export module Draw;

import Skeleton;
import CutsceneMgr;
import Camera;

template<typename... Args>
class ResChange : public WFP::Event<Args...>
{
public:
    using WFP::Event<Args...>::Event;
};

export __declspec(noinline) ResChange<int, int>& onResChange()
{
    static ResChange<int, int> ResChangeEvent;
    return ResChangeEvent;
}

std::optional<float> fHudAspectRatioConstraint;
std::optional<float> fSCMAspectRatioConstraint;
int lastScreenWidth = 0;
int lastScreenHeight = 0;
export float fWidescreenHudOffset = 0.0f;
export float fWidescreenSCMOffset = 0.0f;
export float fMenuAspectRatio = 0.0f;

// current cutscene camera zoom (tan-space), maintained by the border system
// (Sprite2d writes it each frame so the FOV conversion can follow the border animation)
export float g_cutsceneCameraZoom = 1.0f;
export float fWidescreenHudOffset43 = 0.0f;

static float GetConstraintOffset(const std::optional<float>& constraint, float autoOffset)
{
    if (!constraint.has_value())
        return autoOffset;
    const float value = constraint.value();
    if (value < 0.0f || value > (32.0f / 9.0f))
        return value;
    const float aspect = ClampHudAspectRatio(value, SCREEN_WIDTH / SCREEN_HEIGHT);
    return CalculateWidescreenOffset(SCREEN_HEIGHT * aspect, SCREEN_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0, true);
}

export class CDraw
{
private:
    static inline GameRef<float> ms_fFOV;
    static inline GameRef<float> ms_fNearClipZ;
    static inline GameRef<float> ms_fFarClipZ;
    static inline GameRef<float> ms_fAspectRatio;

    static inline float ms_fScaledFOV;
public:
    static inline bool ms_bProperScaling = true;
    static inline bool ms_bFixRadar = true;
    static inline bool ms_bFixSprites = true;

    static void SetFOV(float fov);
    static void CalculateAspectRatio();
    static float GetFOV() { return ms_fFOV; }
    static float GetScaledFOV() { return ms_fScaledFOV; }

    static float FindAspectRatio();
    static float ConvertFOV(float fov);
    static float ConvertFOVforCutscene(float fov);
    static float ConvertFOVInverse(float fov);
    static float GetAspectRatio() { return ms_fAspectRatio; }
    static void SetAspectRatio(float ratio)
    {
        if (ms_fAspectRatio == ratio && lastScreenWidth == RsGlobal->width && lastScreenHeight == RsGlobal->height)
            return;

        ms_fAspectRatio = ratio;
        lastScreenWidth = RsGlobal->width;
        lastScreenHeight = RsGlobal->height;

        fWidescreenHudOffset43 = CalculateWidescreenOffset(SCREEN_HEIGHT * (4.0f / 3.0f), SCREEN_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0, true);

        fWidescreenHudOffset = GetConstraintOffset(fHudAspectRatioConstraint, 0.0f);
        fWidescreenSCMOffset = GetConstraintOffset(fSCMAspectRatioConstraint, fWidescreenHudOffset);

        onResChange().executeAll(RsGlobal->width, RsGlobal->height);
    }
    static float ScaleY(float y);

    static void SetNearClipZ(float nearclip) { ms_fNearClipZ = nearclip; }
    static float GetNearClipZ(void) { return ms_fNearClipZ; }
    static void SetFarClipZ(float farclip) { ms_fFarClipZ = farclip; }
    static float GetFarClipZ(void) { return ms_fFarClipZ; }

    CDraw()
    {
        WFP::onInitEvent() += []()
        {
            auto pattern = hook::pattern("D9 05 ? ? ? ? 8B 15 ? ? ? ? D8 0D ? ? ? ? 68 AB AA AA 3F");
            ms_fFOV.SetAddress(*pattern.get_first<float*>(2));

            pattern = hook::pattern("D8 35 ? ? ? ? DE C9 D9 E8");
            ms_fAspectRatio.SetAddress(*pattern.get_first<float*>(2));

            pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? 53 53 53 B9");
            static auto SetFOV = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), CDraw::SetFOV);

            pattern = hook::pattern("E8 ? ? ? ? ? ? ? ? ? ? A1 ? ? ? ? ? ? 50 51");
            static auto CalculateAspectRatio = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), CDraw::CalculateAspectRatio);

            pattern = hook::pattern("D8 1D ? ? ? ? DF E0 F6 C4 05 0F 8B ? ? ? ? D9 44 24 ? D8 64 24 ? D8 1D");
            ms_fNearClipZ.SetAddress(*pattern.get_first<float*>(2));

            pattern = hook::pattern("D8 1D ? ? ? ? DF E0 F6 C4 41 0F 84 ? ? ? ? D9 44 24 ? D8 8E");
            ms_fFarClipZ.SetAddress(*pattern.get_first<float*>(2));
        };
    }
} Draw;

export namespace MenuCanvas
{
    inline std::optional<float> Constraint;
    inline bool Enabled = false;
    inline bool Drawing = false;
    inline int PhysicalWidth = 0;
    inline int PhysicalMaximumWidth = 0;
    inline int Depth = 0;
    inline int Suspensions = 0;
    inline float Offset = 0.0f;

    __declspec(noinline) ResChange<int, int>& onLayoutChange()
    {
        static ResChange<int, int> event;
        return event;
    }

    int GetWidth(int width, int height)
    {
        if (!Enabled || !Constraint || height <= 0)
            return width;
        const float value = *Constraint;
        // Match the HUD option's ratio and legacy pixel-offset forms.
        const float result = value < 0.0f || value > 32.0f / 9.0f
            ? width + value * 2.0f
            : height * ClampHudAspectRatio(value, float(width) / height);
        const int minimum = std::min(width, int(std::lround(height * (4.0f / 3.0f))));
        int canvasWidth = int(std::lround(std::clamp(result, float(minimum), float(width))));
        // Native mouse positions are integers. Keep the inset integral too.
        if ((width - canvasWidth) & 1) ++canvasWidth;
        return canvasWidth;
    }

    // Frontend scaling may use a narrower canvas; 3D FOV always uses CDraw.
    float GetCurrentAspectRatio()
    {
        return fMenuAspectRatio != 0.0f ? fMenuAspectRatio : CDraw::GetAspectRatio();
    }

    float GetAspectRatio()
    {
        const int width = Depth && !Suspensions ? PhysicalWidth : RsGlobal->width;
        return float(GetWidth(width, RsGlobal->height)) / RsGlobal->height;
    }

    int GetMouseX(int x)
    {
        // Input remains relative to the menu canvas, including the empty space
        // beside it. Only the physical screen edges limit cursor movement.
        const int inset = int(Offset);
        return std::clamp(x, -inset, PhysicalWidth - inset);
    }

    void Apply()
    {
        PhysicalWidth = RsGlobal->width;
        PhysicalMaximumWidth = RsGlobal->maximumWidth;
        const int width = GetWidth(PhysicalWidth, RsGlobal->height);
        Offset = float(PhysicalWidth - width) * 0.5f;
        RsGlobal->width = width;
        RsGlobal->maximumWidth = width;
        fMenuAspectRatio = float(width) / RsGlobal->height;
        onLayoutChange().executeAll(width, RsGlobal->height);
    }

    void Restore()
    {
        RsGlobal->width = PhysicalWidth;
        RsGlobal->maximumWidth = PhysicalMaximumWidth;
        fMenuAspectRatio = 0.0f;
        onLayoutChange().executeAll(RsGlobal->width, RsGlobal->height);
    }

    class Scope
    {
        bool active;
        bool previousDrawing;
    public:
        explicit Scope(bool drawing = false, bool enabled = true) : active(Enabled && enabled), previousDrawing(Drawing)
        {
            if (!active) return;
            if (!Depth++) Apply();
            Drawing = drawing || Drawing;
        }
        ~Scope()
        {
            if (!active) return;
            Drawing = previousDrawing;
            if (!--Depth) Restore();
        }
        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;
    };

    // Backgrounds, OS cursor warps and renderer resets use the real viewport.
    // Apply again on return so a newly selected video mode is retained.
    class Suspend
    {
        bool active;
    public:
        Suspend() : active(Depth != 0)
        {
            if (active && !Suspensions++) Restore();
        }
        ~Suspend()
        {
            if (active && !--Suspensions) Apply();
        }
        Suspend(const Suspend&) = delete;
        Suspend& operator=(const Suspend&) = delete;
    };
}

float CDraw::FindAspectRatio()
{
    return SCREEN_WIDTH / SCREEN_HEIGHT;
}

float CDraw::ConvertFOV(float hfov)
{
    float ar1 = DEFAULT_ASPECT_RATIO;
    float ar2 = GetAspectRatio();
    hfov = DEGTORAD(hfov);
    float vfov = Atan(tan(hfov / 2) / ar1) * 2;
    hfov = Atan(tan(vfov / 2) * ar2) * 2;
    return RADTODEG(hfov);
}

float CDraw::ConvertFOVforCutscene(float fov)
{
    if (g_cutsceneCameraZoom != 1.0f)
    {
        float hfov = DEGTORAD(fov);
        return RADTODEG(2.0f * Atan(tan(hfov / 2.0f) * g_cutsceneCameraZoom));
    }
    return fov;
}

float CDraw::ConvertFOVInverse(float fov)
{
    const float arFrom = DEFAULT_ASPECT_RATIO;
    const float arTo = GetAspectRatio();

    const float rad = DEGTORAD(fov);
    const float out = 2.0f * Atan(tan(rad * 0.5f) * (arFrom / arTo));
    return RADTODEG(out);
}

export namespace FOVManager
{
    // Internal storage: hash -> multiplier
    inline std::unordered_map<uintptr_t, float> g_FOVMultipliers;
    inline float g_BaseFOVMultiplier = 1.0f; // fallback

    // Recalculate current total multiplier
    float GetCurrentMultiplier()
    {
        float total = g_BaseFOVMultiplier;
        for (const auto& [hash, mult] : g_FOVMultipliers)
            total *= mult;
        return total;
    }

    // API Exports
    void __cdecl GetCurrentFOV(float* out)
    {
        if (out)
            *out = CDraw::GetFOV() * GetCurrentMultiplier();
    }

    void __cdecl SetFOVMultiplier(void* hash, float value)
    {
        if (!hash) return;
        uintptr_t key = reinterpret_cast<uintptr_t>(hash);
        g_FOVMultipliers[key] = std::clamp(value, 0.5f, 2.0f);
    }

    void __cdecl RemoveFOVMultiplier(void* hash)
    {
        if (!hash) return;
        uintptr_t key = reinterpret_cast<uintptr_t>(hash);
        g_FOVMultipliers.erase(key);
    }
}

void CDraw::SetFOV(float fov)
{
    if (!CCutsceneMgr::IsRunning() && !TheCamera->m_bWideScreenOn)
    {
        g_cutsceneCameraZoom = 1.0f;
        ms_fScaledFOV = ConvertFOV(fov) * FOVManager::GetCurrentMultiplier();
    }
    else
    {
        ms_fScaledFOV = ConvertFOVforCutscene(fov);
    }
    ms_fFOV = fov;
}

void CDraw::CalculateAspectRatio()
{
    SetAspectRatio(CDraw::FindAspectRatio());
}

float CDraw::ScaleY(float y)
{
    return ms_bProperScaling ? y : y * ((float)DEFAULT_SCREEN_HEIGHT / SCREEN_HEIGHT_NTSC);
}

void Find3rdPersonCamTargetVectorFMUL(SafetyHookContext& ctx)
{
    float f = CDraw::GetAspectRatio();
    _asm {fmul dword ptr[f]}
}

class Camera
{
public:
    Camera()
    {
        WFP::onGameInitEvent() += []()
        {
            CIniReader iniReader("");
            fHudAspectRatioConstraint = ParseWidescreenHudOffset(iniReader.ReadString("MAIN", "HudAspectRatioConstraint", ""));
            fSCMAspectRatioConstraint = ParseWidescreenHudOffset(iniReader.ReadString("MAIN", "SCMAspectRatioConstraint", "Auto"));
            lastScreenWidth = 0;

            static float fScaledFOV = 0.0f;
            auto pattern = hook::pattern("D9 05 ? ? ? ? 56 D8 0D ? ? ? ? 8B F1");
            injector::WriteMemory(pattern.get_first(2), &fScaledFOV, true);
            injector::MakeNOP(pattern.get_first(7), 6, true);
            static auto CalculateFrustumPlanes = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                fScaledFOV = DEGTORAD(CDraw::GetScaledFOV() / 2.0f);
            });

            pattern = hook::pattern("D9 05 ? ? ? ? D8 25 ? ? ? ? DC C0 D8 C9 D9 5C 24");
            static auto Find3rdPersonCamTargetVector = safetyhook::create_mid(pattern.get_first(), Find3rdPersonCamTargetVectorFMUL);

            static float fCameraWidth = 0.01403292f;
            injector::MakeNOP(0x50AD79, 6, true);
            injector::WriteMemory<const void*>(0x50AD59 + 0x2, &fCameraWidth, true);
            injector::WriteMemory<const void*>(0x51498D + 0x2, &fCameraWidth, true);
        };
    }
} Camera;

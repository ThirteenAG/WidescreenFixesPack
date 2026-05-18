module;

#include <stdafx.h>
#include <cmath>
#include <chrono>

export module Camera;

import ComVars;

void* off_702640 = nullptr;

export GameRef<bool> byte_7103C2([]() -> bool*
{
    auto pattern = hook::pattern("38 1D ? ? ? ? 74 ? 83 7E");
    if (!pattern.empty())
        return *pattern.get_first<bool*>(2);
    return nullptr;
});

export GameRef<int*> pProfileSettings([]() -> int**
{
    auto pattern = hook::pattern("8B 3D ? ? ? ? E8 ? ? ? ? 8B CF");
    if (!pattern.empty())
        return *pattern.get_first<int**>(2);
    return nullptr;
});

export GameRef<float> MouseDeltaX([]() -> float*
{
    auto pattern = hook::pattern("F3 0F 11 05 ? ? ? ? 89 1D");
    if (!pattern.empty())
        return *pattern.get_first<float*>(4);
    return nullptr;
});

export GameRef<float> MouseDeltaY([]() -> float*
{
    auto pattern = hook::pattern("F3 0F 11 05 ? ? ? ? F3 0F 11 05 ? ? ? ? 89 1D");
    if (!pattern.empty())
        return *pattern.get_first<float*>(4);
    return nullptr;
});

int (__fastcall* IsActionAssigned)(void* _this, void* edx, const char* a1) = nullptr;
float* (__fastcall* MakeBetweenPiAndMinusPi)(float* a1, float* a2, float* a3) = nullptr;
void (__fastcall* MakeBetweenPiAndMinusPiX3)(float* _this, void* edx) = nullptr;

static constexpr float kTurretPitchMax = 1.2217306f;
static constexpr float kTurretPitchMin = -0.34906587f;
static constexpr float kTurretInitialYaw = 3.0f;
static constexpr float kTurretInputScale = 0.0049999999f;
static constexpr float kLookAroundAngle = 2.3561945f;
static constexpr float kCameraReturnSpeed = 0.052000001f;

static float sIdleTimer = 0.0f;
static float sIdleTimeoutSeconds = 2.0f;
static float sReturnSpeed = 2.0f;
static constexpr float kSnapThreshold = 0.01f;
static bool sIsReturning = false;

static std::chrono::steady_clock::time_point sLastFrameTime = std::chrono::steady_clock::now();

struct CameraState
{
    float& yaw() { return data[0]; }
    float& targetYaw() { return data[1]; }
    float& returnSpeed() { return data[2]; }
    float& prevYaw() { return data[3]; }
    float& turretYaw() { return data[5]; }
    DWORD& inputCounter() { return reinterpret_cast<DWORD*>(data)[6]; }
    float& turretPitch() { return data[8]; }
    float& rawInputYaw() { return data[9]; }
    float& rawInputPitch() { return data[10]; }
    bool& isLookAround() { return *reinterpret_cast<bool*>(reinterpret_cast<BYTE*>(data) + 16); }
    bool& isTurretActive() { return *reinterpret_cast<bool*>(reinterpret_cast<BYTE*>(data) + 17); }
    bool& isAiming() { return *reinterpret_cast<bool*>(reinterpret_cast<BYTE*>(data) + 28); }

    explicit CameraState(float* p) : data(p) {}
    float* data;
};

namespace CLookAround
{
    int (__fastcall* GetAngle)(int _this, void* edx, float* a2, float a3) = nullptr;

    SafetyHookInline shStep = {};
    void __fastcall Step(float* _this, void* edx, float vehicleYaw, int isTankTurret)
    {
        if (ReadActionMapsID())
            return shStep.unsafe_fastcall(_this, edx, vehicleYaw, isTankTurret);

        CameraState cam(_this);

        const auto now = std::chrono::steady_clock::now();
        const float dt = std::clamp(std::chrono::duration<float>(now - sLastFrameTime).count(), 0.0f, 0.1f);
        sLastFrameTime = now;

        if (isTankTurret)
        {
            isTurret = IsActionAssigned(off_702640, edx, "Vehicle_Tank_RotateTurret") == 0;
            if (isTurret)
            {
                cam.isTurretActive() = true;
                byte_7103C2 = true;
                cam.rawInputYaw() = kTurretInitialYaw;
            }
        }
        else
        {
            isTurret = false;
        }

        const float sensitivity = *(float*)((uintptr_t)*pProfileSettings + 76620);

        cam.rawInputYaw() += MouseDeltaX;
        cam.rawInputPitch() += MouseDeltaY;

        const bool hasInput = byte_7103C2
            || fabsf(MouseDeltaX) > 0.001f
            || fabsf(MouseDeltaY) > 0.001f;

        const bool wasReturning = sIsReturning;

        if (hasInput)
        {
            sIdleTimer = 0.0f;
            sIsReturning = false;

            ++cam.inputCounter();
            cam.turretYaw() -= MouseDeltaX * sensitivity * kTurretInputScale;

            const float newPitch = cam.turretPitch() + (MouseDeltaY * sensitivity * kTurretInputScale);
            cam.turretPitch() = std::clamp(newPitch, kTurretPitchMin, kTurretPitchMax);

            // If return was just interrupted by input, snap all yaw state to vehicleYaw
            // so GetAngle's stale interpolation target doesn't cause a jump
            if (wasReturning)
            {
                cam.yaw() = vehicleYaw;
                cam.targetYaw() = vehicleYaw;
                cam.returnSpeed() = kCameraReturnSpeed;
                cam.prevYaw() = vehicleYaw;
            }
        }
        else
        {
            sIdleTimer += dt;
            if (sIdleTimer >= sIdleTimeoutSeconds)
                sIsReturning = true;
        }

        // Apply return to cam.yaw() BEFORE GetAngle so it interpolates from our value
        if (sIsReturning)
        {
            const float blend = 1.0f - expf(-sReturnSpeed * dt);

            // Pitch return
            cam.turretPitch() *= (1.0f - blend);
            if (fabsf(cam.turretPitch()) < kSnapThreshold)
                cam.turretPitch() = 0.0f;

            // Rebase unbounded cam.yaw() to nearest equivalent to vehicleYaw, then lerp
            float diff = vehicleYaw - cam.yaw();
            while (diff > 3.14159265f) diff -= 6.28318530f;
            while (diff < -3.14159265f) diff += 6.28318530f;
            cam.yaw() = vehicleYaw - diff;

            if (fabsf(diff) > kSnapThreshold)
            {
                cam.yaw() += diff * blend;
            }
            else
            {
                sIsReturning = false;
                sIdleTimer = 0.0f;
                cam.yaw() = vehicleYaw;
            }

            // Keep turretYaw (float[5]) in sync — GetAngle reads this and writes it
            // back to cam.yaw(), so it must track the lerp or it will jump on resume
            cam.turretYaw() = cam.yaw();
        }

        if (wasReturning && !sIsReturning)
        {
            // Return just completed — ensure turretYaw is fully at vehicleYaw
            cam.turretYaw() = vehicleYaw;
            cam.yaw() = vehicleYaw;
        }

        // Look-around / aiming
        cam.prevYaw() = cam.yaw();
        float newYaw = 0.0f;
        GetAngle((int)_this, edx, &newYaw, vehicleYaw);

        // Override GetAngle's write during and on the frame return ends
        if (sIsReturning || (wasReturning && !sIsReturning))
        {
            const float forcedYaw = cam.yaw();
            newYaw = forcedYaw;
            cam.targetYaw() = forcedYaw;
            cam.returnSpeed() = kCameraReturnSpeed;
            cam.prevYaw() = forcedYaw;
        }

        const float* lookAngle = MakeBetweenPiAndMinusPi(&vehicleYaw, _this + 3, &newYaw);
        const bool lookAround = fabs(*lookAngle) > kLookAroundAngle;

        cam.isLookAround() = lookAround;

        if (cam.isAiming())
            cam.isLookAround() = true;

        if (!sIsReturning && !(wasReturning && !sIsReturning))
        {
            if (cam.isLookAround())
            {
                cam.yaw() = newYaw;
                cam.targetYaw() = newYaw;
                cam.returnSpeed() = kCameraReturnSpeed;
                cam.prevYaw() = newYaw;
            }
            else
            {
                cam.targetYaw() = newYaw;
                MakeBetweenPiAndMinusPiX3(_this, edx);
            }
        }

        cam.isTurretActive() = byte_7103C2;
    }
}

class Camera
{
public:
    Camera()
    {
        WFP::onInitEventAsync() += []()
        {
            CIniReader iniReader("");
            auto bEnable = iniReader.ReadFloat("CAMERA", "Enable", 1) != 0;
            sIdleTimeoutSeconds = iniReader.ReadFloat("CAMERA", "CameraReturnTimeout", 3.0f);
            sReturnSpeed = iniReader.ReadFloat("CAMERA", "CameraReturnSpeed", 2.0f);

            if (!bEnable)
                return;

            auto pattern = hook::pattern("BE ? ? ? ? 6A ? 8B CE A2");
            off_702640 = *pattern.get_first<void**>(1);

            // for some reason disables horizontal camera movement, something is not right
            static auto Camera_LookLeft = "_Camera_LookLeft";
            pattern = hook::pattern("68 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? F7 D8 1A C0 FE C0 88 46");
            injector::WriteMemory(pattern.get_first(1), &Camera_LookLeft, true);

            pattern = hook::pattern("E8 ? ? ? ? 33 F6 3B C6 74 ? FF B3");
            IsActionAssigned = (decltype(IsActionAssigned))injector::GetBranchDestination(pattern.get_first()).as_int();

            pattern = hook::pattern("E8 ? ? ? ? ? ? F3 0F 10 05 ? ? ? ? ? ? ? ? 89 46");
            CLookAround::GetAngle = (decltype(CLookAround::GetAngle))injector::GetBranchDestination(pattern.get_first()).as_int();

            pattern = hook::pattern("E8 ? ? ? ? ? ? 51 ? ? ? E8 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 76");
            MakeBetweenPiAndMinusPi = (decltype(MakeBetweenPiAndMinusPi))injector::GetBranchDestination(pattern.get_first()).as_int();

            pattern = hook::pattern("E8 ? ? ? ? A0 ? ? ? ? 5E");
            MakeBetweenPiAndMinusPiX3 = (decltype(MakeBetweenPiAndMinusPiX3))injector::GetBranchDestination(pattern.get_first()).as_int();

            pattern = hook::pattern("E8 ? ? ? ? 8B CB E8 ? ? ? ? 8B 43");
            CLookAround::shStep = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), CLookAround::Step);
        };
    }
} Camera;
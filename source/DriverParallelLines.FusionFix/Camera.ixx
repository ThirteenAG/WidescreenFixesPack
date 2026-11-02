module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <cmath>
#include <chrono>

export module Camera;

import ComVars;






export void InitCameraRequiredRefs()
{
    byte_7103C2.SetAddress(*hook::pattern("38 1D ? ? ? ? 74 ? 83 7E").get_first<bool*>(2));
    pProfileSettings.SetAddress(*hook::pattern("8B 3D ? ? ? ? E8 ? ? ? ? 8B CF").get_first<int**>(2));
    MouseDeltaX.SetAddress(*hook::pattern("F3 0F 11 05 ? ? ? ? 89 1D").get_first<float*>(4));
    MouseDeltaY.SetAddress(*hook::pattern("F3 0F 11 05 ? ? ? ? F3 0F 11 05 ? ? ? ? 89 1D").get_first<float*>(4));
}


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
    int (__fastcall* GetAngle)(int _this, void* edx, float* a2, void* controller) = nullptr;

    int __fastcall CameraLookAssigned(void* self, void* edx, const char* action)
    {
        return ReadActionMapsID() ? IsActionAssigned(self, edx, action) : 0;
    }

    SafetyHookInline shStep = {};
    void __fastcall Step(float* _this, void* edx, void* controller, int isTankTurret)
    {
        if (ReadActionMapsID())
        {
            sIdleTimer = 0.0f;
            sIsReturning = false;
            sLastFrameTime = std::chrono::steady_clock::now();
            return shStep.unsafe_fastcall(_this, edx, controller, isTankTurret);
        }

        CameraState cam(_this);
        constexpr float returnYaw = 0.0f;

        const auto now = std::chrono::steady_clock::now();
        const float dt = std::clamp(std::chrono::duration<float>(now - sLastFrameTime).count(), 0.0f, 0.1f);
        sLastFrameTime = now;

        if (isTankTurret)
        {
            isTurret = IsActionAssigned(off_702640.get_ptr(), edx, "Vehicle_Tank_RotateTurret") == 0;
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

        }
        else
        {
            sIdleTimer += dt;
            if (sIdleTimer >= FixSettings().Float("CAMERA.CameraReturnTimeout"))
                sIsReturning = true;
        }

        // Apply return to cam.yaw() BEFORE GetAngle so it interpolates from our value
        if (sIsReturning)
        {
            const float blend = 1.0f - expf(-FixSettings().Float("CAMERA.CameraReturnSpeed") * dt);

            // Pitch return
            cam.turretPitch() *= (1.0f - blend);
            if (fabsf(cam.turretPitch()) < kSnapThreshold)
                cam.turretPitch() = 0.0f;

            // Rebase unbounded cam.yaw() to nearest equivalent to returnYaw, then lerp
            const float diff = std::remainder(returnYaw - cam.yaw(), 6.28318530f);
            cam.yaw() = returnYaw - diff;

            if (fabsf(diff) > kSnapThreshold)
            {
                cam.yaw() += diff * blend;
            }
            else
            {
                cam.yaw() = returnYaw;
                if (cam.turretPitch() == 0.0f)
                {
                    sIsReturning = false;
                    sIdleTimer = 0.0f;
                }
            }

            // Keep turretYaw (float[5]) in sync — GetAngle reads this and writes it
            // back to cam.yaw(), so it must track the lerp or it will jump on resume
            cam.turretYaw() = cam.yaw();
        }

        const bool returnFinished = wasReturning && !sIsReturning && !hasInput;
        if (returnFinished)
        {
            // Return just completed — ensure turretYaw is fully at returnYaw
            cam.turretYaw() = returnYaw;
            cam.yaw() = returnYaw;
        }

        // Look-around / aiming
        cam.prevYaw() = cam.yaw();
        float newYaw = 0.0f;
        GetAngle((int)_this, edx, &newYaw, controller);

        // Override GetAngle's write during and on the frame return ends
        if (sIsReturning || returnFinished)
        {
            const float forcedYaw = cam.yaw();
            newYaw = forcedYaw;
            cam.targetYaw() = forcedYaw;
            cam.returnSpeed() = kCameraReturnSpeed;
            cam.prevYaw() = forcedYaw;
        }

        float angleDifference = 0.0f;
        const float* lookAngle = MakeBetweenPiAndMinusPi(&angleDifference, _this + 3, &newYaw);
        const bool lookAround = fabs(*lookAngle) > kLookAroundAngle;

        cam.isLookAround() = lookAround;

        if (cam.isAiming())
            cam.isLookAround() = true;

        if (!sIsReturning && !returnFinished)
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
            auto& settings = FixSettings();
            auto bEnable = settings.Int("CAMERA.Enable") != 0;

            if (!bEnable)
                return;

            auto pattern = hook::pattern("BE ? ? ? ? 6A ? 8B CE A2");
            off_702640.SetAddress(*pattern.get_first<void**>(1));

            pattern = hook::pattern("E8 ? ? ? ? 33 F6 3B C6 74 ? FF B3");
            IsActionAssigned = (decltype(IsActionAssigned))injector::GetBranchDestination(pattern.get_first()).as_int();

            // Keep the game's controller look action while the mouse camera uses its own path.
            pattern = hook::pattern("68 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? F7 D8 1A C0 FE C0 88 46");
            injector::MakeCALL(pattern.get_first(10), CLookAround::CameraLookAssigned, true);

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
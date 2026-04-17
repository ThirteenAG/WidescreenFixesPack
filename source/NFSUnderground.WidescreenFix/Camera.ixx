module;

#include <stdafx.h>
#include <cmath>
#include <usercall.hpp>

export module Camera;

import ComVars;

using namespace usercall;

// Offset from the game's original camera (zero = original camera)
static float YawOffset = 0.0f;
static float PitchOffset = 0.0f;

bool InvertLook = false;
float StickLookSensitivity = 1.0f;
float MouseLookSensitivity = 1.0f;

// Auto-return state
float IdleTimer = 0.0f;
float IdleTimeoutSeconds = 3.0f;
float ReturnSpeed = 2.0f;

// Car movement tracking
bVector3 PrevCarPos = { 0.0f, 0.0f, 0.0f };
bool  PrevCarPosValid = false;
float CarSpeed2D = 0.0f;
float CarMoveThreshold = 0.5f;

constexpr float DEG2RAD = 0.0174533f;
constexpr float RAD2DEG = 57.2957795f;

float LerpAngleDeg(float from, float to, float t)
{
    float diff = to - from;
    while (diff > 180.0f) diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;
    return from + diff * t;
}

using SteadyClock = std::chrono::steady_clock;
using TimePoint = SteadyClock::time_point;
static constexpr auto HideCursorDuration = std::chrono::milliseconds(100);
TimePoint LastCameraInputTime = {};
uint8_t bLookBehind = 0;
uint32_t nCameraMode = 0;
injector::hook_back<void(__cdecl*)(bVector3*, bVector3*, bMatrix4*, bVector3*)> hb_eCreateLookAtMatrix;
void __cdecl eCreateLookAtMatrix(bVector3* to, bVector3* from, bMatrix4* out, bVector3* up)
{
    if (!GameFlowManager::IsPaused())
    {
        bHideCursorForMouseLook = true;
        LastCameraInputTime = SteadyClock::now();
    }

    if (bLookBehind || (nCameraMode != 0 && nCameraMode != 1 && nCameraMode != 4))
    {
        YawOffset = 0.0f;
        PitchOffset = 0.0f;
        IdleTimer = 0.0f;
        PrevCarPosValid = false;
        return usercall::Call<reg::eax, reg::edi, reg::esi, reg::stack>(hb_eCreateLookAtMatrix.fun, to, from, out, up);
    }

    float dt = actualDeltaTime;

    // Derive car speed from target position delta
    if (PrevCarPosValid && dt > 0.0f)
    {
        float cdx = to->x - PrevCarPos.x;
        float cdy = to->y - PrevCarPos.y;
        CarSpeed2D = sqrtf(cdx * cdx + cdy * cdy) / dt;
    }
    PrevCarPos = *to;
    PrevCarPosValid = true;

    // Consume mouse delta from DirectInput (read and zero to avoid reuse)
    float mouseDeltaX = (float)g_Mouse.DeltaX;
    float mouseDeltaY = (float)g_Mouse.DeltaY;
    g_Mouse.DeltaX = 0;
    g_Mouse.DeltaY = 0;

    // Combine stick + mouse input
    float deltaX = -g_RightStick.X + (-mouseDeltaX * MouseLookSensitivity);
    float deltaY = g_RightStick.Y + (mouseDeltaY * MouseLookSensitivity);

    bool hasInput = (fabsf(deltaX) > 0.01f || fabsf(deltaY) > 0.01f);
    bool carIsMoving = (CarSpeed2D > CarMoveThreshold);

    // Update offsets based on input state
    if (!GameFlowManager::IsPaused() && hasInput)
    {
        YawOffset += deltaX * StickLookSensitivity;
        PitchOffset += deltaY * StickLookSensitivity * (InvertLook ? -1.0f : 1.0f);
        IdleTimer = 0.0f;
    }
    else
    {
        IdleTimer += dt;

        if (IdleTimer >= IdleTimeoutSeconds && carIsMoving)
        {
            // Blend offsets back toward zero
            float blendFactor = 1.0f - expf(-ReturnSpeed * dt);
            YawOffset = LerpAngleDeg(YawOffset, 0.0f, blendFactor);
            PitchOffset = PitchOffset + (0.0f - PitchOffset) * blendFactor;

            // Snap to zero when close enough
            if (fabsf(YawOffset) < 0.3f && fabsf(PitchOffset) < 0.3f)
            {
                YawOffset = 0.0f;
                PitchOffset = 0.0f;
            }
        }
    }

    // If offsets are zero, pass through to the original camera untouched
    if (YawOffset == 0.0f && PitchOffset == 0.0f)
    {
        return usercall::Call<reg::eax, reg::edi, reg::esi, reg::stack>(hb_eCreateLookAtMatrix.fun, to, from, out, up);
    }

    // Clamp pitch offset so total pitch stays reasonable
    if (PitchOffset > 60.0f) PitchOffset = 60.0f;
    if (PitchOffset < -20.0f) PitchOffset = -20.0f;

    // Decompose the game's original camera into spherical coords
    float dx = from->x - to->x;
    float dy = from->y - to->y;
    float dz = from->z - to->z;
    float dist = sqrtf(dx * dx + dy * dy + dz * dz);
    float dist2D = sqrtf(dx * dx + dy * dy);

    float origYawRad = atan2f(dy, dx);
    float origPitchRad = atan2f(dz, dist2D);

    // Apply offsets
    float finalYawRad = origYawRad + YawOffset * DEG2RAD;
    float finalPitchRad = origPitchRad + PitchOffset * DEG2RAD;

    // Clamp total pitch to avoid gimbal lock
    if (finalPitchRad > 1.39626f) finalPitchRad = 1.39626f;   // ~80 deg
    if (finalPitchRad < -1.39626f) finalPitchRad = -1.39626f;

    // Reconstruct camera position at original distance
    from->x = to->x + dist * cosf(finalPitchRad) * cosf(finalYawRad);
    from->y = to->y + dist * cosf(finalPitchRad) * sinf(finalYawRad);
    from->z = to->z + dist * sinf(finalPitchRad);

    return usercall::Call<reg::eax, reg::edi, reg::esi, reg::stack>(hb_eCreateLookAtMatrix.fun, to, from, out, up);
}

class Camera
{
public:
    Camera()
    {
        WFP::onInitEventAsync() += []()
        {
            CIniReader iniReader("");
            IdleTimeoutSeconds = iniReader.ReadFloat("CAMERA", "CameraReturnTimeout", 3.0f);
            ReturnSpeed = iniReader.ReadFloat("CAMERA", "CameraReturnSpeed", 2.0f);
            StickLookSensitivity = iniReader.ReadFloat("CAMERA", "StickLookSensitivity", 1.0f);
            MouseLookSensitivity = iniReader.ReadFloat("CAMERA", "MouseLookSensitivity", 1.0f) * 0.15f;
            InvertLook = iniReader.ReadInteger("CAMERA", "InvertLook", 0) != 0;

            //CubicCameraMover::Update
            auto pattern = hook::pattern("E8 ? ? ? ? 8B 43 ? 50 8D BC 24");
            hb_eCreateLookAtMatrix.fun = usercall::MakeUsercall(pattern.get_first(), [](SafetyHookContext& regs)
            {
                auto to = (bVector3*)regs.eax;
                auto from = (bVector3*)regs.edi;
                auto out = (bMatrix4*)regs.esi;
                auto up = *(bVector3**)(regs.esp + 0x0);
                eCreateLookAtMatrix(to, from, out, up);
            });

            pattern = hook::pattern("C6 43 ? ? 74 ? 8B 43 ? 85 C0 0F 84");
            static auto GetCameraData = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                bLookBehind = *(uint8_t*)(regs.ebx + 0x45);
                nCameraMode = *(uint32_t*)(regs.ebx + 0x40);
            });

            WFP::onGameProcessEvent() += []()
            {
                bHideCursorForMouseLook = (SteadyClock::now() - LastCameraInputTime) < HideCursorDuration;
            };
        };
    }
} Camera;
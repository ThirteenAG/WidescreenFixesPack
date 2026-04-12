module;

#include <stdafx.h>
#include <cmath>
#include <chrono>

export module Camera;

import ComVars;

float YawOffset = 0.0f;
float PitchOffset = 0.0f;

bool InvertLook = false;
float StickLookSensitivity = 1.0f;
float MouseLookSensitivity = 1.0f;

float IdleTimer = 0.0f;
float IdleTimeoutSeconds = 3.0f;
float ReturnSpeed = 2.0f;
float CollisionOffset = 0.3f;
float MinCamDistance = 1.5f;

constexpr float DEG2RAD = 0.0174533f;

float LerpAngleDeg(float from, float to, float t)
{
    float diff = to - from;
    while (diff > 180.0f)  diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;
    return from + diff * t;
}

using SteadyClock = std::chrono::steady_clock;
using TimePoint = SteadyClock::time_point;
constexpr auto HideCursorDuration = std::chrono::milliseconds(100);
TimePoint LastCameraInputTime = {};

uint8_t  bLookBehind = 0;
uint8_t  bIsDriving = 0;
uint8_t  bFreeLook = 0;
uint32_t nCameraMode = 0;
std::vector<std::string> vCameraStack;

struct alignas(16) RayCastHit
{
    __m128   m_position;     // +0
    __m128   m_normal;       // +16
    void* m_rigidBody;       // +32  (non-null = hit)
    void* m_material;        // +36
    uint32_t m_part;         // +40
    uint32_t m_bone;         // +44
    float    m_lambda;       // +48  (hit fraction 0..1 along ray)
    uint32_t _pad[3];        // +52  (pad to 64 bytes)

    void init()
    {
        m_position = _mm_setzero_ps();
        m_normal = _mm_setzero_ps();
        m_rigidBody = nullptr;
        m_material = (void*)(uintptr_t)-1;
        m_part = 0;
        m_bone = 0;
        m_lambda = 1.0f;
        _pad[0] = _pad[1] = _pad[2] = 0;
    }
};

namespace fb
{
    namespace CinebotRay
    {
        bool (__cdecl* test)(__m128* from, __m128* to, RayCastHit* hit, void* ignore) = nullptr;
    }

    namespace CinebotCamera
    {
        injector::hook_back<void(__fastcall*)(float*, void*, void*)> hbCommitShot;
        void __fastcall commitShot(float* CinebotCamera, void* edx, void* CinebotCameraShot)
        {
            bHideCursorForMouseLook = true;
            LastCameraInputTime = SteadyClock::now();

            bool bIsChaseCamera = false;
            for (auto& it : vCameraStack)
            {
                if (it.ends_with("chaseMode") || it.ends_with("lookBackMode"))
                {
                    bIsChaseCamera = true;
                    break;
                }
            }

            if (!bIsDriving || bFreeLook || bLookBehind || !bIsChaseCamera)
            {
                YawOffset = 0.0f;
                PitchOffset = 0.0f;
                IdleTimer = 0.0f;
                return hbCommitShot.fun(CinebotCamera, edx, CinebotCameraShot);
            }

            float dt = fb::GameTimer::actualDeltaTime;

            float mouseDeltaX = (float)g_Mouse.DeltaX;
            float mouseDeltaY = (float)g_Mouse.DeltaY;
            g_Mouse.DeltaX = 0;
            g_Mouse.DeltaY = 0;

            float deltaX = -g_RightStick.X + (-mouseDeltaX * MouseLookSensitivity);
            float deltaY = g_RightStick.Y + (mouseDeltaY * MouseLookSensitivity);

            bool hasInput = (fabsf(deltaX) > 0.01f || fabsf(deltaY) > 0.01f);

            if (hasInput)
            {
                YawOffset += deltaX * StickLookSensitivity;
                PitchOffset += deltaY * StickLookSensitivity * (InvertLook ? -1.0f : 1.0f);
                IdleTimer = 0.0f;
            }
            else
            {
                IdleTimer += dt;
                if (IdleTimer >= IdleTimeoutSeconds)
                {
                    float blendFactor = 1.0f - expf(-ReturnSpeed * dt);
                    YawOffset = LerpAngleDeg(YawOffset, 0.0f, blendFactor);
                    PitchOffset = PitchOffset * (1.0f - blendFactor);
                    if (fabsf(YawOffset) < 0.3f && fabsf(PitchOffset) < 0.3f)
                    {
                        YawOffset = 0.0f;
                        PitchOffset = 0.0f;
                    }
                }
            }

            if (PitchOffset > 60.0f) PitchOffset = 60.0f;
            if (PitchOffset < -30.0f) PitchOffset = -30.0f;

            if (YawOffset == 0.0f && PitchOffset == 0.0f)
                return hbCommitShot.fun(CinebotCamera, edx, CinebotCameraShot);

            float* shotEye = (float*)((uintptr_t)CinebotCameraShot);
            float* shotTgt = (float*)((uintptr_t)CinebotCameraShot + 16);

            float origEyeX = shotEye[0], origEyeY = shotEye[1], origEyeZ = shotEye[2];
            float tgtX = shotTgt[0], tgtY = shotTgt[1], tgtZ = shotTgt[2];

            float dx = origEyeX - tgtX;
            float dy = origEyeY - tgtY;
            float dz = origEyeZ - tgtZ;
            float dist = sqrtf(dx * dx + dy * dy + dz * dz);
            float distXZ = sqrtf(dx * dx + dz * dz);

            float origYaw = atan2f(dx, dz);
            float origPitch = atan2f(dy, distXZ);

            float finalYaw = origYaw + YawOffset * DEG2RAD;
            float finalPitch = origPitch + PitchOffset * DEG2RAD;
            if (finalPitch > 1.39626f) finalPitch = 1.39626f;
            if (finalPitch < -1.39626f) finalPitch = -1.39626f;

            float cp = cosf(finalPitch);
            shotEye[0] = tgtX + dist * cp * sinf(finalYaw);
            shotEye[1] = tgtY + dist * sinf(finalPitch);
            shotEye[2] = tgtZ + dist * cp * cosf(finalYaw);

            if (dist > MinCamDistance)
            {
                RayCastHit hit;
                hit.init();

                //__m128 rayFrom = _mm_setr_ps(tgtX, tgtY, tgtZ, 0.0f);
                float skip = 1.4f;
                if (skip >= dist) skip = dist * 0.3f;
                float invD = 1.0f / dist;
                __m128 rayFrom = _mm_setr_ps(
                    tgtX + (shotEye[0] - tgtX) * invD * skip,
                    tgtY + (shotEye[1] - tgtY) * invD * skip,
                    tgtZ + (shotEye[2] - tgtZ) * invD * skip, 0.0f);

                __m128 rayTo = _mm_setr_ps(shotEye[0], shotEye[1], shotEye[2], 0.0f);

                if (fb::CinebotRay::test(&rayFrom, &rayTo, &hit, nullptr))
                {
                    // m_lambda = fraction along ray (0 = at target, 1 = at eye)
                    float safeDist = skip + (dist - skip) * hit.m_lambda - CollisionOffset;

                    if (safeDist < MinCamDistance)
                        safeDist = MinCamDistance;

                    if (safeDist < dist)
                    {
                        float t = safeDist / dist;
                        shotEye[0] = tgtX + (shotEye[0] - tgtX) * t;
                        shotEye[1] = tgtY + (shotEye[1] - tgtY) * t;
                        shotEye[2] = tgtZ + (shotEye[2] - tgtZ) * t;
                    }
                }
            }

            hbCommitShot.fun(CinebotCamera, edx, CinebotCameraShot);

            shotEye[0] = origEyeX;
            shotEye[1] = origEyeY;
            shotEye[2] = origEyeZ;
        }
    }

    namespace CinebotStateLogicEntity
    {
        SafetyHookInline shevent = {};
        void __fastcall event(void* _this, void* edx, int a2)
        {
            DWORD eventType = *(DWORD*)((uintptr_t)a2 + 4);

            std::string cameraName = "Unknown";

            if (eventType == 210993314) // BEGIN / PUSH
            {
                cameraName = std::string_view(*(const char**)(*(uintptr_t*)((*((DWORD*)_this + 5)) + 0x10) + 0xC));

                vCameraStack.push_back(cameraName);

                while (vCameraStack.size() > 16)
                    vCameraStack.erase(vCameraStack.begin());
            }
            else if (eventType == 193438506) // END / POP
            {
                cameraName = std::string_view(*(const char**)(*(uintptr_t*)((*((DWORD*)_this + 5)) + 0x10) + 0xC));

                for (auto it = vCameraStack.rbegin(); it != vCameraStack.rend(); ++it)
                {
                    if (*it == cameraName)
                    {
                        vCameraStack.erase(std::next(it).base());
                        break;
                    }
                }
            }

            return shevent.unsafe_fastcall(_this, edx, a2);
        }
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
            IdleTimeoutSeconds = iniReader.ReadFloat("CAMERA", "CameraReturnTimeout", 3.0f);
            ReturnSpeed = iniReader.ReadFloat("CAMERA", "CameraReturnSpeed", 2.0f);
            StickLookSensitivity = iniReader.ReadFloat("CAMERA", "StickLookSensitivity", 1.0f) * 5.0f;
            MouseLookSensitivity = iniReader.ReadFloat("CAMERA", "MouseLookSensitivity", 1.0f) * 0.15f;
            InvertLook = iniReader.ReadInteger("CAMERA", "InvertLook", 0) != 0;
            CollisionOffset = iniReader.ReadFloat("CAMERA", "CollisionOffset", 0.3f);
            MinCamDistance = iniReader.ReadFloat("CAMERA", "MinCamDistance", 1.5f);

            auto pattern = find_pattern("E8 ? ? ? ? 8B 44 24 ? 0F 57 D2", "E8 ? ? ? ? 0F 57 D2 F3 0F 11 93");
            fb::CinebotCamera::hbCommitShot.fun = injector::MakeCALL(pattern.get_first(), fb::CinebotCamera::commitShot, true).get();

            pattern = hook::pattern("83 EC ? 8D 44 24 ? 8B C8");
            fb::CinebotRay::test = (decltype(fb::CinebotRay::test))pattern.get_first();

            pattern = hook::pattern("8B 44 24 ? 8B 40 ? 83 EC ? 56 8B F1 3D ? ? ? ? 0F 84");
            fb::CinebotStateLogicEntity::shevent = safetyhook::create_inline(pattern.get_first(), fb::CinebotStateLogicEntity::event);

            pattern = hook::pattern("8B 01 8B 90 ? ? ? ? 5F 5E FF E2");
            static auto LookBehindHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                bLookBehind = *(uint8_t*)(regs.ecx + 0xE9);
            });

            pattern = hook::pattern("80 7E ? ? 57 74 ? 33 C0");
            static auto DrivingHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                bIsDriving = *(uint8_t*)(regs.esi + 0x4);
            });

            static double g_minStickDeviationForFreeLook = 2.0;
            pattern = hook::pattern("F2 0F 59 C8 F2 0F 58 D1 66 0F 2F 15");
            injector::WriteMemory(pattern.get_first(12), &g_minStickDeviationForFreeLook, true);

            static auto FreeLookHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                bFreeLook = *(uint8_t*)(regs.esi + 0xEA);

                static bool bWasPressed = false;
                if (bBothSticksClicked)
                {
                    if (!bWasPressed)
                    {
                        g_minStickDeviationForFreeLook = (g_minStickDeviationForFreeLook > 1.0) ? 0.25 : 2.0;
                        bWasPressed = true;
                    }
                }
                else
                {
                    bWasPressed = false;
                }
            });

            WFP::onGameProcessEvent() += []()
            {
                bHideCursorForMouseLook = (SteadyClock::now() - LastCameraInputTime) < HideCursorDuration;
            };
        };
    }
} Camera;
module;

#include <stdafx.h>
#include <cmath>
#include <chrono>
#include <numbers>

export module Camera;

import ComVars;

// CLookAround, turns the driving look controls into a yaw relative to the vehicle
struct LookAround
{
    float yaw;
    float targetYaw;
    float returnSpeed;
    float prevYaw;
    bool snapped;
};

constexpr float Pi = std::numbers::pi_v<float>;
constexpr float MouseRadiansPerCount = 0.15f * Pi / 180.0f;
constexpr float StickRadiansPerSecond = 2.5f;
constexpr float StickDeadzone = 0.2f;
constexpr float MinElevation = -0.15f; // chase camera, relative to the point it looks at
constexpr float MaxElevation = 1.3f;
constexpr float LookAroundReturnSpeed = 0.052f;

float IdleTimeoutSeconds = 3.0f;
float ReturnSpeed = 2.0f;
float MouseLookSensitivity = 1.0f;
float StickLookSensitivity = 1.0f;
bool InvertLook = false;

namespace OnFoot
{
    int32_t PendingMouseX = 0; // since the player controller last turned the player
    int32_t PendingMouseY = 0;
    int32_t MouseY = 0; // for its aim, after its turn
    std::chrono::steady_clock::time_point LastUpdate = {};
}

namespace Orbit
{
    bool Enabled = false;

    float Yaw = 0.0f;   // replaces the look around yaw, 0 = behind the vehicle
    float Pitch = 0.0f; // added to the chase camera elevation

    float IdleTimer = 0.0f;
    bool Returning = false;
    bool Suspended = false; // the game's look controls are in use
    bool HasInput = false;

    int32_t PendingMouseX = 0;
    int32_t PendingMouseY = 0;

    uint32_t Frame = 0;
    uint32_t UpdatedFrame = UINT32_MAX;
    LookAround* Owner = nullptr;
    uintptr_t ChaseCameraPlacement = 0; // the orbiting chase camera, while it positions itself this frame
    std::chrono::steady_clock::time_point LastUpdate = {};

    void Reset()
    {
        Yaw = 0.0f;
        Pitch = 0.0f;
        IdleTimer = 0.0f;
        Returning = false;
    }
}

float GetJoystickAxis(uint32_t axis)
{
    uintptr_t manager = InputManager;
    if (!manager || axis > Keymap::AxisSlider2)
        return 0.0f;

    return *reinterpret_cast<float*>(manager + 0x6D8 + axis * sizeof(float));
}

// Joystick part of a two-way action (-1..1), so the orbit follows the stick bound to on foot look in the controls menu
float GetStickAxis(Keymap::Action action)
{
    auto entry = Keymap::Find(Keymap::Live(), Keymap::EntryCount, action);
    if (!entry || !entry->twoWay)
        return 0.0f;

    float value = 0.0f;
    for (int i = 0; i < 2; ++i)
    {
        const auto& binding = entry->binding[i];
        if (!binding.isAxis)
            continue;

        const float axis = GetJoystickAxis(binding.code);
        const float amount = std::max(0.0f, binding.positive ? axis : -axis);
        value += (i == 0) ? -amount : amount;
    }

    const float magnitude = std::min(std::abs(value), 1.0f);
    if (magnitude < StickDeadzone)
        return 0.0f;

    return std::copysign((magnitude - StickDeadzone) / (1.0f - StickDeadzone), value);
}

float GetActionValue(void* controller, Keymap::Action action)
{
    using GetValueFn = float(__thiscall*)(void*, uint32_t);
    return (*reinterpret_cast<GetValueFn**>(controller))[3](controller, action);
}

void UpdateOrbit(LookAround* owner)
{
    if (Orbit::UpdatedFrame == Orbit::Frame)
        return;
    Orbit::UpdatedFrame = Orbit::Frame;

    const auto now = std::chrono::steady_clock::now();
    const float elapsed = std::chrono::duration<float>(now - Orbit::LastUpdate).count();
    Orbit::LastUpdate = now;

    int32_t mouseX = std::exchange(Orbit::PendingMouseX, 0);
    int32_t mouseY = std::exchange(Orbit::PendingMouseY, 0);

    if (owner != Orbit::Owner)
    {
        Orbit::Owner = owner;
        Orbit::Reset();
        Orbit::Suspended = false;
    }

    // mouse movement from before the camera was active belongs to something else
    if (elapsed > 0.25f)
    {
        mouseX = 0;
        mouseY = 0;
    }

    const float dt = std::clamp(elapsed, 0.0f, 0.1f);
    const float stickX = GetStickAxis(Keymap::FootLookLeftRight);
    const float stickY = GetStickAxis(Keymap::FootLookUpDown);
    const float invert = InvertLook ? -1.0f : 1.0f;

    Orbit::HasInput = mouseX != 0 || mouseY != 0 || stickX != 0.0f || stickY != 0.0f;
    if (Orbit::HasInput)
    {
        Orbit::IdleTimer = 0.0f;
        Orbit::Returning = false;

        const float yawDelta = mouseX * MouseLookSensitivity * MouseRadiansPerCount + stickX * StickLookSensitivity * StickRadiansPerSecond * dt;
        const float pitchDelta = mouseY * MouseLookSensitivity * MouseRadiansPerCount + stickY * StickLookSensitivity * StickRadiansPerSecond * dt;
        Orbit::Yaw = std::remainder(Orbit::Yaw - yawDelta, 2.0f * Pi);
        Orbit::Pitch = std::clamp(Orbit::Pitch + pitchDelta * invert, -Pi / 2.0f, Pi / 2.0f);
    }
    else
    {
        Orbit::IdleTimer += dt;
        if (Orbit::IdleTimer >= IdleTimeoutSeconds)
            Orbit::Returning = true;
    }

    if (Orbit::Returning)
    {
        // yaw stays within [-pi, pi], so this takes the short way back
        const float blend = 1.0f - std::exp(-ReturnSpeed * dt);
        Orbit::Yaw -= Orbit::Yaw * blend;
        Orbit::Pitch -= Orbit::Pitch * blend;

        if (std::abs(Orbit::Yaw) < 0.005f && std::abs(Orbit::Pitch) < 0.005f)
            Orbit::Reset();
    }
}

namespace CLookAround
{
    SafetyHookInline shStep = {};
    void __fastcall Step(LookAround* _this, void* edx, void* controller)
    {
        if (!controller)
            return shStep.unsafe_thiscall(_this, controller);

        // The driving look controls keep the game's behavior, e.g. to look back
        if (GetActionValue(controller, Keymap::DriveLookLeftRight) != 0.0f || GetActionValue(controller, Keymap::DriveLookBackForward) != 0.0f)
        {
            // start from the default view, so that looking back snaps like it does without the orbit
            if (!Orbit::Suspended)
                _this->yaw = 0.0f;

            shStep.unsafe_thiscall(_this, controller);

            Orbit::Reset();
            Orbit::Yaw = _this->yaw;
            Orbit::Suspended = true;
            Orbit::Owner = _this;
            Orbit::UpdatedFrame = Orbit::Frame;
            Orbit::LastUpdate = std::chrono::steady_clock::now();
            Orbit::PendingMouseX = 0;
            Orbit::PendingMouseY = 0;
            return;
        }

        UpdateOrbit(_this);

        if (Orbit::Suspended)
        {
            // let the game bring the camera back after a look, unless the player takes over
            if (!Orbit::HasInput && std::abs(_this->yaw) > 0.01f)
            {
                shStep.unsafe_thiscall(_this, controller);
                Orbit::Yaw = _this->yaw;
                return;
            }
            Orbit::Suspended = false;
        }

        _this->yaw = Orbit::Yaw;
        _this->targetYaw = Orbit::Yaw;
        _this->prevYaw = Orbit::Yaw;
        _this->returnSpeed = LookAroundReturnSpeed;
        _this->snapped = false;
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
            Orbit::Enabled = iniReader.ReadInteger("CAMERA", "Enable", 1) != 0;
            MouseLookSensitivity = iniReader.ReadFloat("CAMERA", "MouseLookSensitivity", 1.0f);

            //mouse movement, right after the input devices are read (see Mouse.ixx)
            auto pattern = hook::pattern("E8 ? ? ? ? 57 8D 46 14 E8");
            static auto MousePollHook = safetyhook::create_mid(pattern.get_first(5), [](SafetyHookContext& regs)
            {
                OnFoot::PendingMouseX += MouseRead::X;
                OnFoot::PendingMouseY += MouseRead::Y;

                if (Orbit::Enabled)
                {
                    Orbit::PendingMouseX += MouseRead::X;
                    Orbit::PendingMouseY += MouseRead::Y;
                }
            });

            //on foot, the player controller turns the player by look * -0.0025 rad and aims by look * 0.8 * 0.001953125 rad on every world update,
            //but the mouse look is read for it only every 33-50 ms, so the same movement turned the player several times;
            //give it the movement since its previous update instead, turning as far per mouse count as the car camera
            pattern = hook::pattern("8B 4E 14 DD D8 D9 46 2C 8B 11 D8 0D ? ? ? ? 51 D9 1C 24 FF 92 60 01 00 00");
            static const float TurnPerLook = std::abs(**pattern.get_first<float*>(12));
            static auto OnFootTurnHook = safetyhook::create_mid(pattern.get_first(5), [](SafetyHookContext& regs)
            {
                // movement from before the player was on foot belongs to something else
                const auto now = std::chrono::steady_clock::now();
                const bool stale = now - OnFoot::LastUpdate > std::chrono::milliseconds(250);
                OnFoot::LastUpdate = now;

                const int32_t mouseX = std::exchange(OnFoot::PendingMouseX, 0);
                const int32_t mouseY = std::exchange(OnFoot::PendingMouseY, 0);
                OnFoot::MouseY = stale ? 0 : mouseY;

                *reinterpret_cast<float*>(regs.esi + 0x2C) = stale ? 0.0f : mouseX * MouseLookSensitivity * MouseRadiansPerCount / TurnPerLook;
            });

            pattern = hook::pattern("8B 4E 14 DD D8 D9 46 30 8B 11 D8 0D ? ? ? ? 51 D9 1C 24 FF 92 64 01 00 00");
            static const float AimPerLook = std::abs(**pattern.get_first<float*>(12)) * 0.001953125f; // the player adds its aim input * 0.001953125
            static auto OnFootAimHook = safetyhook::create_mid(pattern.get_first(5), [](SafetyHookContext& regs)
            {
                *reinterpret_cast<float*>(regs.esi + 0x30) = OnFoot::MouseY * MouseLookSensitivity * MouseRadiansPerCount / AimPerLook;
            });

            if (!Orbit::Enabled)
                return;

            IdleTimeoutSeconds = iniReader.ReadFloat("CAMERA", "CameraReturnTimeout", 3.0f);
            ReturnSpeed = iniReader.ReadFloat("CAMERA", "CameraReturnSpeed", 2.0f);
            StickLookSensitivity = iniReader.ReadFloat("CAMERA", "StickLookSensitivity", 1.0f);
            InvertLook = iniReader.ReadInteger("CAMERA", "InvertLook", 0) != 0;

            Keymap::Live();

            //yaw, for every camera that looks around: chase, in-car and crane
            pattern = hook::pattern("83 EC 10 53 56 8B D9 8B 03 57 8B 7C 24 20");
            CLookAround::shStep = safetyhook::create_inline(pattern.get_first(), CLookAround::Step);

            //pitch, the chase camera rotates the vehicle direction by the yaw, then places itself behind it
            pattern = hook::pattern("8D 43 04 8B 08 8B 50 04 89 4C 24 ? 8B 48 08 89 54 24 ? 8B 50 0C 83 C4 0C 8D 44 24 ? 89 4C 24 ? 50 8D 4C 24 ? 89 54 24 ? 51 8B D1 52 E8");
            static auto ChaseCameraUpdateHook = safetyhook::create_mid(pattern.get_first(0x2E), [](SafetyHookContext& regs)
            {
                auto controller = *reinterpret_cast<void**>(regs.ebx + 0x240);
                auto lookAround = reinterpret_cast<LookAround*>(regs.ebx + 0x28);
                Orbit::ChaseCameraPlacement = (controller && lookAround == Orbit::Owner && !Orbit::Suspended) ? regs.ebx + 0xB0 : 0;
            });

            //it places itself at a fixed height and distance from the point it looks at, rotate that offset up or down
            pattern = hook::pattern("E8 ? ? ? ? 8B 87 80 00 00 00 50 8D 44 24 44 E8 ? ? ? ? 8B 8F 88 00 00 00");
            static auto ChaseCameraPlacementHook = safetyhook::create_mid(pattern.get_first(0x15), [](SafetyHookContext& regs)
            {
                if (regs.edi != Orbit::ChaseCameraPlacement)
                    return;

                auto offset = reinterpret_cast<float*>(regs.esp + 0x40);
                const float horizontal = std::sqrt(offset[0] * offset[0] + offset[2] * offset[2]);
                if (horizontal < 0.001f)
                    return;

                // no pitch beyond the limits, so the camera follows as soon as the mouse turns around
                const float distance = std::sqrt(horizontal * horizontal + offset[1] * offset[1]);
                const float elevation = std::atan2(offset[1], horizontal);
                Orbit::Pitch = std::clamp(Orbit::Pitch, std::min(0.0f, MinElevation - elevation), std::max(0.0f, MaxElevation - elevation));
                if (Orbit::Pitch == 0.0f)
                    return;

                const float scale = distance * std::cos(elevation + Orbit::Pitch) / horizontal;
                offset[0] *= scale;
                offset[1] = distance * std::sin(elevation + Orbit::Pitch);
                offset[2] *= scale;
            });

            WFP::onGameProcessEvent() += []()
            {
                if (Orbit::UpdatedFrame != Orbit::Frame)
                {
                    Orbit::PendingMouseX = 0;
                    Orbit::PendingMouseY = 0;
                }
                Orbit::ChaseCameraPlacement = 0;
                ++Orbit::Frame;
            };
        };
    }
} Camera;

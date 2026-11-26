module;

#include <stdafx.h>
#include <xmmintrin.h>
#include <cmath>
#include <dinput.h>
#include <wrl/client.h>
#include "FixSettings.hpp"

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

export module RawInput;

import ComVars;
import Overlay;

// Raw mouse counts to the game's camera units
namespace TCRawMouse
{
    std::atomic<float> sensitivity{ 1.0f };
    std::atomic<bool> invertY{ false };

    void SetSensitivity(float value)
    {
        sensitivity = std::isfinite(value) ? std::clamp(value, 0.1f, 5.0f) : 1.0f;
    }

    float Scale(float nativeSensitivity)
    {
        return std::isfinite(nativeSensitivity) ? nativeSensitivity * sensitivity.load() / 1000.0f : 0.0f;
    }

    struct Delta
    {
        float x, y;
    };
    Delta FromCounts(int32_t x, int32_t y, float nativeSensitivity)
    {
        auto scale = Scale(nativeSensitivity);
        return { static_cast<float>(x) * scale, -static_cast<float>(y) * scale };
    }
}

bool bRawInputMouse = true;
float MouseDeltaX = 0.0f, MouseDeltaY = 0.0f;

bool bIsAimingModeSwitch = false;
bool bNonMouseCameraMovement = false;

SafetyHookInline shsub_41D3C0 = {};
int __fastcall sub_41D3C0(int8_t* _this, void* edx, int a2, void* a3)
{
    static int8_t PreviousCameraMode = -1;
    PreviousCameraMode = CurrentCameraMode;
    CurrentCameraMode = _this[145];

    if (PreviousCameraMode != CurrentCameraMode)
    {
        bIsAimingModeSwitch = ((PreviousCameraMode == CameraPrecisionAim && (CurrentCameraMode == CameraCrosshair || CurrentCameraMode == CameraOnFoot)) ||
                                 ((PreviousCameraMode == CameraCrosshair || PreviousCameraMode == CameraOnFoot) && CurrentCameraMode == CameraPrecisionAim));
    }

    return shsub_41D3C0.unsafe_fastcall<int>(_this, edx, a2, a3);
}

HWND hGameWindow = nullptr;

namespace TCDirectInputMouse
{
    Microsoft::WRL::ComPtr<IDirectInput8A> input;
    Microsoft::WRL::ComPtr<IDirectInputDevice8A> mouse;
    bool acceptMovement = false;
    uint32_t overlayGeneration = 0;

    void Open(HWND window)
    {
        static const auto module = LoadLibraryExW(L"dinput8.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        using Create = HRESULT(WINAPI*)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
        static const auto create = reinterpret_cast<Create>(module ? GetProcAddress(module, "DirectInput8Create") : nullptr);
        Microsoft::WRL::ComPtr<IDirectInput8A> directInput;
        Microsoft::WRL::ComPtr<IDirectInputDevice8A> device;
        if (!create || FAILED(create(GetModuleHandleW(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8A,
            reinterpret_cast<void**>(directInput.GetAddressOf()), nullptr)) ||
            FAILED(directInput->CreateDevice(GUID_SysMouse, device.GetAddressOf(), nullptr)) ||
            FAILED(device->SetDataFormat(&c_dfDIMouse2)) ||
            FAILED(device->SetCooperativeLevel(window, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND)))
        {
            return;
        }
        input = std::move(directInput);
        mouse = std::move(device);
    }

    void Reset()
    {
        if (mouse)
            mouse->Unacquire();
        mouse.Reset();
        input.Reset();
        acceptMovement = false;
    }

    std::optional<DIMOUSESTATE2> Sample(bool enabled, uint32_t generation)
    {
        // The camera owns this immediate-mode device. The game keeps its button bindings and action maps.
        const bool keep = enabled && acceptMovement && generation == overlayGeneration;
        overlayGeneration = generation;
        acceptMovement = enabled;
        if (!mouse && enabled && !keep)
            Open(hGameWindow);
        if (!mouse)
            return std::nullopt;

        // Consume the device's pending movement even when the overlay owns the mouse.
        DIMOUSESTATE2 state{};
        auto result = mouse->GetDeviceState(sizeof(state), &state);
        if (enabled && (result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED))
        {
            if (SUCCEEDED(mouse->Acquire()))
                mouse->GetDeviceState(sizeof(state), &state);
            return std::nullopt;
        }
        return keep && SUCCEEDED(result) ? std::optional{ state } : std::nullopt;
    }
}

export void SetRawInputWindow(HWND window)
{
    if (bRawInputMouse && hGameWindow != window)
    {
        TCDirectInputMouse::Reset();
        hGameWindow = window;
    }
}
constexpr ULONGLONG MouseCameraTimeoutMs = 5000;
static ULONGLONG mouseCameraUntil = 0;
static bool mouseCameraActive = false;

// Mouse movement since the last read
bool ReadMouse()
{
    const bool enabled = hGameWindow && hGameWindow == GetForegroundWindow() && !TCOverlay::BlocksInput();
    const auto sample = TCDirectInputMouse::Sample(enabled, TCOverlay::InputGeneration());
    if (!sample)
    {
        MouseDeltaX = MouseDeltaY = 0.0f;
        mouseCameraUntil = 0;
        mouseCameraActive = false;
        return false;
    }
    if (sample->lX || sample->lY)
    {
        const auto delta = TCRawMouse::FromCounts(sample->lX, sample->lY, fMouseSens.get());
        MouseDeltaX += delta.x;
        MouseDeltaY += delta.y;
    }
    return true;
}

// Look Behind in a vehicle (the game's action flags)
GameRef<uint32_t> nActionFlags;
constexpr uint32_t LookBehind = 0x800;

SafetyHookInline shsub_40D990 = {};
void __stdcall sub_40D990(float a1)
{
    // Keep draining while another window or the overlay owns the mouse.
    ReadMouse();
    if (TCOverlay::BlocksInput())
        return;
    static float accumulated_yaw = 0.0f;
    static float accumulated_pitch = 0.0f;
    static int8_t prev_camera_mode_local = -1;

    float delta_x = MouseDeltaX;
    float delta_y = MouseDeltaY;

    if (dword_79430C[4111])
        delta_x = -delta_x;

    if (dword_79430C[4110])
        delta_y = -delta_y;
    if (TCRawMouse::invertY.load(std::memory_order_relaxed))
        delta_y = -delta_y;

    // Reset deltas for next frame
    MouseDeltaX = 0.0f;
    MouseDeltaY = 0.0f;

    bool mouse_moved = (std::fabs(delta_x) > 1e-6f || std::fabs(delta_y) > 1e-6f);

    if (nLoading != 0 || bCutscene || bPause || bNonMouseCameraMovement || (nActionFlags.get() & LookBehind))
    {
        mouseCameraUntil = 0;
        mouseCameraActive = false;
        prev_camera_mode_local = CurrentCameraMode;
        shsub_40D990.unsafe_stdcall(a1);
        return;
    }

    // Keep the current deadline through the aiming transition.
    if (CurrentCameraMode != prev_camera_mode_local)
    {
        if (!bIsAimingModeSwitch)
        {
            mouseCameraUntil = 0;
            mouseCameraActive = false;
        }
        prev_camera_mode_local = CurrentCameraMode;
    }

    const auto now = GetTickCount64();
    if (mouse_moved)
        mouseCameraUntil = now + MouseCameraTimeoutMs;

    if (now >= mouseCameraUntil)
    {
        mouseCameraActive = false;
        shsub_40D990.unsafe_stdcall(a1);
        return;
    }

    constexpr float RadiansPerDegree = 3.1415926536f / 180.0f;
    if (!mouseCameraActive)
    {
        // Recover yaw and pitch from the current view, including the camera's fixed roll.
        const __m128 orientation = xmmword_778BF0;
        const float sinYaw = -orientation.m128_f32[1];
        const float cosYaw = -orientation.m128_f32[3];
        const float cosPitch = std::hypot(sinYaw, cosYaw);
        const float sinPitch = cosPitch > 0.0f
            ? (orientation.m128_f32[0] * cosYaw + orientation.m128_f32[2] * sinYaw) / cosPitch
            : 0.0f;
        accumulated_yaw = 2.0f * std::atan2(sinYaw, cosYaw);
        accumulated_pitch = 2.0f * std::atan2(sinPitch, cosPitch);
        mouseCameraActive = true;
    }

    const float maxPitch = (CurrentCameraMode == CameraInVehicle ? 10.0f : 60.0f) * RadiansPerDegree;
    accumulated_yaw = std::remainder(accumulated_yaw - delta_x, 360.0f * RadiansPerDegree);
    accumulated_pitch = std::clamp(accumulated_pitch + delta_y, -89.0f * RadiansPerDegree, maxPitch);

    const float sinYaw = std::sin(accumulated_yaw * 0.5f);
    const float cosYaw = std::cos(accumulated_yaw * 0.5f);
    const float sinPitch = std::sin(accumulated_pitch * 0.5f);
    const float cosPitch = std::cos(accumulated_pitch * 0.5f);
    const __m128 orientation = _mm_set_ps(-cosPitch * cosYaw, sinPitch * sinYaw,
        -cosPitch * sinYaw, sinPitch * cosYaw);

    // Apply the same orbit offset as the native camera update.
    const __m128 offset = _mm_set_ps(0.0f, float_70F390, 0.0f, 0.0f);
    const __m128 rotated = _mm_shuffle_ps(orientation, orientation, 201);
    const __m128 cross = _mm_sub_ps(_mm_mul_ps(orientation, _mm_shuffle_ps(offset, offset, 201)),
        _mm_mul_ps(rotated, offset));
    const __m128 first = _mm_sub_ps(_mm_shuffle_ps(cross, cross, 201),
        _mm_mul_ps(offset, _mm_shuffle_ps(orientation, orientation, 255)));
    const __m128 second = _mm_sub_ps(_mm_mul_ps(first, rotated),
        _mm_mul_ps(_mm_shuffle_ps(first, first, 201), orientation));

    xmmword_778BF0 = orientation;
    xmmword_778BD0 = _mm_sub_ps(xmmword_778BA0,
        _mm_sub_ps(offset, _mm_mul_ps(_mm_shuffle_ps(second, second, 201), _mm_set1_ps(2.0f))));
}

void InitRawInput()
{
    nActionFlags.SetAddress(*hook::pattern("80 0D ? ? ? ? 08 EB ? 80 25 ? ? ? ? F7").get_first<uintptr_t>(2) - 1);

    auto pattern = hook::pattern("D8 0D ? ? ? ? 6A 00 68 3F 8F F7 79");
    fMouseSens.SetAddress(*pattern.get_first<float*>(2));

    pattern = hook::pattern("38 1D ? ? ? ? 0F 84 ? ? ? ? 8B 86");
    byte_850D8A.SetAddress(*pattern.get_first<uint8_t*>(2));

    pattern = hook::pattern("0F 29 05 ? ? ? ? 0F 28 46 ? 6A 00");
    xmmword_778BF0.SetAddress(*pattern.get_first<__m128*>(3));

    pattern = hook::pattern("0F 29 05 ? ? ? ? 0F 28 46 ? 68 8F C2 F5 3C");
    xmmword_778BD0.SetAddress(*pattern.get_first<__m128*>(3));

    pattern = hook::pattern("0F 28 05 ? ? ? ? 0F 5C C1 0F 28 D0");
    xmmword_778BA0.SetAddress(*pattern.get_first<__m128*>(3));

    pattern = hook::pattern("F3 0F 11 05 ? ? ? ? F3 0F 10 05 ? ? ? ? F3 0F 59 05 ? ? ? ? 8D 4C 24");
    float_70F390.SetAddress(*pattern.get_first<float*>(4));

    pattern = hook::pattern("A1 ? ? ? ? 80 BC 28 ? ? ? ? ? 8D B4 28 ? ? ? ? C7 03 00 00 00 00 0F 8C ? ? ? ? 8B CE E8 ? ? ? ? 84 C0 0F 84");
    dword_79430C.SetAddress(*pattern.get_first<uint8_t**>(1));

    pattern = hook::pattern("55 8B EC 83 E4 F0 83 EC 20 F3 0F 10 05 ? ? ? ? F3 0F 59 45");
    shsub_40D990 = safetyhook::create_inline(pattern.get_first(0), sub_40D990);

    static float menuDeltaX = 0.0f;
    static float menuDeltaY = 0.0f;

    pattern = hook::pattern("C6 44 24 ? ? E8 ? ? ? ? 8B 4C 24 ? 3B CD");
    static auto UpdateInputHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (!ReadMouse())
        {
            menuDeltaX = menuDeltaY = 0.0f;
            return;
        }

        if (byte_850D8A)
            bNonMouseCameraMovement = true;
        else if (std::fabs(MouseDeltaX) > 1e-6f || std::fabs(MouseDeltaY) > 1e-6f)
            bNonMouseCameraMovement = false;

        if (nLoading != 0 || bCutscene || bPause || bNonMouseCameraMovement)
        {
            menuDeltaX = MouseDeltaX;
            menuDeltaY = MouseDeltaY;
            mouseCameraUntil = 0;
            MouseDeltaX = 0.0f;
            MouseDeltaY = 0.0f;
        }
    });

    // menu map cursor
    pattern = hook::pattern("76 ? F3 0F 10 05 ? ? ? ? F3 0F 10 4C 24 ? F3 0F 59 C8 F3 0F 58 4E");
    injector::MakeNOP(pattern.get_first(), 2, true);

    pattern = hook::pattern("F3 0F 11 4E ? F3 0F 10 4C 24 ? F3 0F 59 C8");
    injector::MakeNOP(pattern.get_first(), 5, true);
    static auto MouseX = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        bool mouse_moved = (std::fabs(menuDeltaX) > 1e-6f);
        if (mouse_moved)
        {
            *(float*)(regs.esi + 0x54) += menuDeltaX * 27.5f;
        }
        else
        {
            *(float*)(regs.esi + 0x54) = regs.xmm1.f32[0];
        }
        menuDeltaX = 0.0f;
    });

    pattern = hook::pattern("F3 0F 11 4E ? 8B 46");
    injector::MakeNOP(pattern.get_first(), 5, true);
    static auto MouseY = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        bool mouse_moved = (std::fabs(menuDeltaY) > 1e-6f);
        if (mouse_moved)
        {
            *(float*)(regs.esi + 0x58) += -menuDeltaY * 27.5f;
        }
        else
        {
            *(float*)(regs.esi + 0x58) = regs.xmm1.f32[0];
        }
        menuDeltaY = 0.0f;
    });

    // Fix crazy deadzone to prevent mouse camera from locking up
    pattern = hook::pattern("0F 87 ? ? ? ? 0F 28 44 24");
    injector::WriteMemory<uint16_t>(pattern.get_first(), 0xE990, true);

    //static float deadzone_threshold = 0.0f;
    //pattern = hook::pattern("D9 05 ? ? ? ? D9 C9 83 C4 08 DF F1 DD D8 0F 87 ? ? ? ? 6A 20");
    //injector::WriteMemory(pattern.get_first(2), &deadzone_threshold, true);
    //
    //pattern = hook::pattern("D9 05 ? ? ? ? D9 C9 83 C4 08 DF F1 DD D8 0F 87 ? ? ? ? 0F 28 44 24");
    //injector::WriteMemory(pattern.get_first(2), &deadzone_threshold, true);
}

class RawInput
{
public:
    RawInput()
    {
        WFP::onInitEvent() += []()
        {
            auto& settings = FixSettings();
            MessagePumpGuard::Install(GetModuleHandleW(nullptr));
            auto cameraMode = hook::pattern("64 A1 ? ? ? ? 6A FF 68 ? ? ? ? 50 8B 44 24 ? 83 F8 51 64 89 25 ? ? ? ? 56 8B F1 77");
            shsub_41D3C0 = safetyhook::create_inline(cameraMode.get_first(), sub_41D3C0);
            bRawInputMouse = settings.Bool("MAIN.RawInputMouse");
            static float nativeSensitivity[2]{}, scaledSensitivity[2]{};
            if (!bRawInputMouse)
            {
                // Without the fix's camera the setting scales the game's own mouse sensitivity
                const char* sites[] = { "D8 0D ? ? ? ? DD 1C 24", "F3 0F 59 05 ? ? ? ? 83 C2 FF" };
                const int offsets[] = { 2, 4 };
                for (size_t i = 0; i < std::size(sites); ++i)
                {
                    auto operand = hook::pattern(sites[i]).get_first<float*>(offsets[i]);
                    nativeSensitivity[i] = **operand;
                    injector::WriteMemory(operand, &scaledSensitivity[i], true);
                }
            }

            static std::atomic<bool> mouseSettingsPending{ false };
            const auto update = []()
            {
                const float value = ReadNumericSetting("MAIN.MouseSensitivity", 1.0f);
                TCRawMouse::SetSensitivity(value);
                TCRawMouse::invertY.store(FixSettings().Get("MAIN.InvertMouseY") == "1", std::memory_order_relaxed);
                if (!bRawInputMouse)
                {
                    const float factor = std::isfinite(value) ? std::clamp(value, 0.1f, 5.0f) : 1.0f;
                    for (size_t i = 0; i < std::size(scaledSensitivity); ++i)
                        scaledSensitivity[i] = nativeSensitivity[i] * factor;
                }
            };
            for (const auto setting : { "MAIN.MouseSensitivity", "MAIN.InvertMouseY" })
                settings.OnChange(setting, []() { mouseSettingsPending.store(true); });
            update();
            WFP::onGameProcessEvent() += [update]()
            {
                if (mouseSettingsPending.exchange(false))
                    update();
            };

            if (bRawInputMouse)
                InitRawInput();
        };
    }
} RawInput;

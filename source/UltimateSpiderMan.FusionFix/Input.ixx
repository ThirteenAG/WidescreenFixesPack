module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include <atomic>
#ifdef _DEBUG
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#endif

export module Input;

import ComVars;

import Screen;

GameRef<HWND>* pWindow = nullptr;
GameRef<uintptr_t>* pInput = nullptr;

int(__thiscall* pSetCursorRect)(void*, int, int, int, int) = nullptr;
injector::hook_back<double(__cdecl*)(int32_t)> hbNormalizeAxis;
void(__thiscall* pUpdateLookaroundAxis)(uintptr_t, float) = nullptr;

int __fastcall SetMouseCursorRect(void* cursor, void*, int x, int y, int width, int height)
{
    // The window UI's cursor uses pretransformed D3D vertices, bypassing NGL.
    // Its 48x48 artwork must use one pixel scale; leave the hotspot untouched.
    auto size = std::min(width, height);
    return pSetCursorRect(cursor, x, y, size, size);
}

POINT CursorToHUD(POINT point, int width, int height)
{
    // Mouse positions cover the client area; HUD coordinates can extend outside
    // 0..640 / 0..480 when the artwork is fitted to the center of that area.
    return {
        static_cast<LONG>(std::lround(Screen.UnscaleX(point.x * 640.0f / width))),
        static_cast<LONG>(std::lround(Screen.UnscaleY(point.y * 480.0f / height)))
    };
}

BOOL __fastcall UpdateMouseCursor(uintptr_t cursor, void*)
{
    if (!*reinterpret_cast<uint8_t*>(cursor + 0x120))
        *reinterpret_cast<uint8_t*>(cursor + 0x114) = 1;

    auto& screenPoint = *reinterpret_cast<POINT*>(cursor + 0xFC);
    if (!GetCursorPos(&screenPoint))
        return FALSE;
    auto point = screenPoint;
    auto window = pWindow->get();
    // Convert to client pixels before scaling, including windows with an offset.
    if (!ScreenToClient(window, &point))
        return FALSE;
    RECT client{};
    if (!GetClientRect(window, &client))
        return FALSE;
    // Positions are client pixels; normalize before mapping into the HUD canvas.
    auto width = client.right - client.left;
    auto height = client.bottom - client.top;
    if (width <= 0 || height <= 0)
        return FALSE;
    *reinterpret_cast<POINT*>(cursor + 0x104) = CursorToHUD(point, width, height);
    return TRUE;
}

struct InputBinding
{
    int32_t type;
    int32_t control;
    float value;
};

struct InputBindings
{
    uint32_t count;
    InputBinding actions[50][6];
};
static_assert(sizeof(InputBindings) == 0xE14);

static_assert(sizeof(DefaultControl) == 72);

SafetyHookInline shBuildControlBindings;
GameRef<uintptr_t>* MenuControls = nullptr;
void* (__thiscall* RegisterControl)(uintptr_t, int, int, int, int) = nullptr;

uintptr_t __cdecl BuildControlBindings()
{
    const auto result = shBuildControlBindings.ccall<uintptr_t>();
    if (MenuControls && MenuControls->get())
    {
        // Preserve the two keyboard slots and native analog-stick navigation.
        for (const auto [action, control] : { std::pair{ 16, 20 }, { 17, 19 }, { 18, 18 }, { 19, 17 } })
            RegisterControl(MenuControls->get() + 24, action, 2, 3, control);
        if (USMPadMenuShortcutsAvailable())
        {
            RegisterControl(MenuControls->get() + 24, 48, 2, 3, 24);
            RegisterControl(MenuControls->get() + 24, 49, 2, 3, 23);
        }
    }
    return result;
}

void SetDefaultPadBindings()
{
    auto site = hook::pattern("C7 44 24 14 ? ? ? ? C7 44 24 24 ? ? ? ? 8B 74 24 24 8D 54 24 30");
    auto menu = hook::pattern("8B 0D ? ? ? ? 6A 18 56 56 6A 07 83 C1 18 E8");
    auto build = hook::pattern("55 8B EC 83 E4 F8 81 EC 70 01 00 00 A1 ? ? ? ? 53 55 56 57");
    defaultControls.SetAddress(*site.get_first<DefaultControl*>(12));
    // Native ThrowWeb is swing; BlackButton is the web-attack action.
    constexpr std::pair<const char*, uint32_t> bindings[] =
    {
        { "ThrowWeb", 11 },
        { "BlackButton", 5 },
        { "CameraUp", 10 },
        { "CameraDown", 9 },
        { "CameraLeft", 8 },
        { "CameraRight", 7 },
        { "CameraCenter", 30 },
        { "Pause", 28 },
        { "BackButton", 27 },
    };
    std::array<uint32_t*, std::size(bindings)> destinations{};
    for (size_t index = 0; index < std::size(bindings); ++index)
    {
        for (size_t row = 0; row < 18; ++row)
        {
            if (std::strncmp(defaultControls.get_ptr()[row].name, bindings[index].first, sizeof(defaultControls.get_ptr()[row].name)) == 0 &&
                defaultControls.get_ptr()[row].pad == 0xFFFF)
                destinations[index] = &defaultControls.get_ptr()[row].pad;
        }
        if (!destinations[index])
        {
            OutputDebugStringA("Ultimate Spider-Man: default controller layout contract unavailable.\n");
            return;
        }
    }
    MenuControls = BindGameRef(menuControls, *menu.get_first<uintptr_t*>(2));
    RegisterControl = reinterpret_cast<decltype(RegisterControl)>(injector::GetBranchDestination(menu.get_first(15)).as_int());
    shBuildControlBindings = safetyhook::create_inline(build.get_first(), BuildControlBindings);
    // The engine uses this column only when a saved controller binding is absent.
    for (size_t index = 0; index < destinations.size(); ++index)
        WriteGameValue(destinations[index], 0x30000u | bindings[index].second);
    WriteGameValue<uint8_t>(menu.get_first(7), 22);
}

thread_local float* pLookaroundDeadzone = nullptr;
thread_local bool* pLookaroundMouse = nullptr;
thread_local std::array<bool, 2> LookaroundMouse{};
std::atomic<float> GamepadCameraSpeed{ 1.0f };
std::atomic<float> MouseSensitivity{ 1.0f };
std::atomic<bool> InvertMouseY{ false };

float AdjustCameraInput(float value, bool mouse, size_t axis, float gamepadSpeed, bool invertMouseY)
{
    if (!std::isfinite(value))
        return value;
    if (mouse)
        return axis == 1 && invertMouseY ? -value : value;
    if (!std::isfinite(gamepadSpeed))
        gamepadSpeed = 1.0f;
    return value * std::clamp(gamepadSpeed, 0.1f, 5.0f);
}

InputBinding GetStrongestBinding(const InputBindings& bindings, uint32_t action)
{
    InputBinding result = {};
    if (action < bindings.count && action < std::size(bindings.actions))
    {
        for (const auto& binding : bindings.actions[action])
        {
            if (binding.type && std::abs(binding.value) > std::abs(result.value))
                result = binding;
        }
    }
    return result;
}

void PreserveMouseAxis(int32_t& axis, const InputBindings& bindings, uint32_t negative, uint32_t positive)
{
    auto left = GetStrongestBinding(bindings, negative);
    auto right = GetStrongestBinding(bindings, positive);
    if (std::abs(left.value) == std::abs(right.value))
        return;
    bool useLeft = std::abs(left.value) > std::abs(right.value);
    auto binding = useLeft ? left : right;
    if (binding.type != 2 || binding.control < 3 || binding.control > 8 || !std::isfinite(binding.value))
        return;

    auto value = useLeft ? -binding.value : binding.value;
    auto sensitivity = MouseSensitivity.load(std::memory_order_relaxed);
    if (!std::isfinite(sensitivity))
        sensitivity = 1.0f;
    value *= std::clamp(sensitivity, 0.1f, 5.0f);
    // These two state fields are copied into old/current snapshots, then consumed
    // only by the camera-axis normalizers below. Normal float bit patterns lie
    // outside the signed 16-bit stick range. Preserve mouse precision and history
    // without clipping fast mouse movement to an emulated stick's maximum speed.
    if (std::isnormal(value))
        std::memcpy(&axis, &value, sizeof(axis));
}

SafetyHookInline shReadInput;
int __cdecl ReadInput(uint32_t device, void* state)
{
    auto result = shReadInput.ccall<int>(device, state);
    if (result == 0 && state && device >= 1 && device <= 4 && USMOverlayBlocksInput())
    {
        // Native output is a four-byte header followed by 28 button/axis bytes.
        std::memset(static_cast<uint8_t*>(state) + 4, 0, 28);
        return result;
    }
    auto input = pInput->get();
    if (result == 0 && state && input && device >= 1 && device <= 4)
    {
        auto settings = *reinterpret_cast<uintptr_t*>(input + 0x129D4 + device * 4);
        if (settings)
        {
            auto& bindings = *reinterpret_cast<InputBindings*>(settings + 0x18);
            auto axes = static_cast<int32_t*>(state);
            PreserveMouseAxis(axes[6], bindings, 22, 23);
            PreserveMouseAxis(axes[7], bindings, 21, 20);
        }
    }
    return result;
}

double __cdecl NormalizeCameraAxis(int32_t value)
{
    // Only the current state's first normalization decides whether to bypass
    // the look-around cutoff. Its previous-state read must not change that.
    auto deadzone = std::exchange(pLookaroundDeadzone, nullptr);
    auto mouseSource = std::exchange(pLookaroundMouse, nullptr);
    if (value < -32768 || value > 32767)
    {
        float mouse;
        std::memcpy(&mouse, &value, sizeof(mouse));
        if (std::isnormal(mouse))
        {
            if (deadzone)
                *deadzone = 0.0f;
            if (mouseSource)
                *mouseSource = true;
            return mouse;
        }
    }
    return hbNormalizeAxis.fun(value);
}

void __fastcall UpdateLookaroundAxis(uintptr_t axis, void*, float dt)
{
    auto& deadzone = *reinterpret_cast<float*>(axis + 8);
    auto savedDeadzone = deadzone;
    auto previous = pLookaroundDeadzone;
    auto previousMouse = pLookaroundMouse;
    const auto action = *reinterpret_cast<const uint32_t*>(axis + 4);
    pLookaroundMouse = action == 108 || action == 109 ? &LookaroundMouse[109 - action] : nullptr;
    if (pLookaroundMouse) *pLookaroundMouse = false;
    pLookaroundDeadzone = &deadzone;
    pUpdateLookaroundAxis(axis, dt);
    pLookaroundDeadzone = previous;
    pLookaroundMouse = previousMouse;
    deadzone = savedDeadzone;
}

struct CameraRecenterState
{
    uintptr_t camera = 0;
    int mode = -1;
    double idle = 0.0;
    double blend = 1.0;
    double blendDuration = 0.4;
    bool looked = false;
    bool held = false;
    bool beginBlend = false;

    bool Update(uintptr_t currentCamera, bool input, float dt, float speed, int setting, float delay, bool nativeLooking = false)
    {
        setting = std::clamp(setting, 0, 2);
        if (camera != currentCamera || mode != setting)
        {
            camera = currentCamera;
            mode = setting;
            idle = 0.0;
            blend = 1.0;
            looked = held = nativeLooking;
            beginBlend = false;
        }
        beginBlend = false;
        if (setting == 0)
            return false;
        const auto seconds = std::isfinite(dt) && dt > 0 ? std::min(double(dt), 0.1) : 0.0;
        if (input)
        {
            idle = 0.0;
            looked = true;
            held = true;
            blend = 1.0;
            return true;
        }
        if (!looked)
            return false;
        if (setting == 2)
            return true;
        idle = std::min(10.0, idle + seconds);
        const auto velocity = std::isfinite(speed) ? std::clamp(double(speed), 0.0, 100.0) : 0.0;
        const auto motion = std::clamp(velocity / 20.0, 0.0, 1.0);
        const auto base = std::isfinite(delay) ? std::clamp(double(delay), 0.5, 5.0) : 1.0;
        const auto wait = base * (1.0 - 0.55 * motion);
        if (idle < wait)
            return true;
        if (held)
        {
            beginBlend = true;
            blend = 0.0;
            blendDuration = 0.4 - 0.18 * motion;
            held = false;
        }
        else
            blend = std::min(1.0, blend + seconds / blendDuration);
        return false;
    }

    double Blend() const
    {
        return blend * blend * (3.0 - 2.0 * blend);
    }
} RecenterDelay;

std::atomic<int> CameraAutoCenter{ 1 };
std::atomic<float> CameraRecenterSeconds{ 1.0f };
SafetyHookInline shCameraRecenter;
bool RecenterUpdated = false;
std::array<float, 9> RecenterStart{};
std::array<float, 3> RecenterOrigin{};

uintptr_t __fastcall UpdateRecenterCamera(uintptr_t camera, void*, float dt, float* pose, uintptr_t target)
{
    if (!pose || CameraAutoCenter.load() == 0)
        return shCameraRecenter.thiscall<uintptr_t>(camera, dt, pose, target);
    std::array<float, 9> before;
    std::copy_n(pose, before.size(), before.begin());
    const auto owner = *reinterpret_cast<const uintptr_t*>(camera + 4);
    const auto wasLooking = *reinterpret_cast<const uint8_t*>(owner + 460) != 0;
    RecenterUpdated = false;
    const auto result = shCameraRecenter.thiscall<uintptr_t>(camera, dt, pose, target);
    if (!RecenterUpdated)
    {
        RecenterDelay = {};
        return result;
    }
    if (!target || RecenterDelay.mode != 1 || RecenterDelay.held)
        return result;
    // A stationary native camera may retain look-around after the idle delay.
    // Start the blend only when the game actually resumes its following camera.
    if (*reinterpret_cast<const uint8_t*>(owner + 460))
    {
        RecenterDelay.blend = 0.0;
        return result;
    }
    if (wasLooking)
    {
        RecenterDelay.beginBlend = true;
        RecenterDelay.blend = 0.0;
    }
    if (RecenterDelay.blend >= 1.0)
        return result;
    const auto center = reinterpret_cast<const float*>(target);
    if (!std::all_of(before.begin(), before.end(), [](float x) { return std::isfinite(x); }) ||
        !std::all_of(pose, pose + before.size(), [](float x) { return std::isfinite(x); }) ||
        !std::all_of(center, center + 3, [](float x) { return std::isfinite(x); }))
    {
        RecenterDelay.blend = 1.0;
        return result;
    }
    if (RecenterDelay.beginBlend)
    {
        RecenterStart = before;
        std::copy_n(center, RecenterOrigin.size(), RecenterOrigin.begin());
    }
    std::array<float, 9> adjusted;
    const auto amount = RecenterDelay.Blend();
    double distance = 0;
    for (size_t i = 0; i < 3; ++i)
    {
        const auto movement = double(center[i]) - RecenterOrigin[i];
        distance += movement * movement;
    }
    if (distance > 10000.0)
    {
        RecenterDelay.blend = 1.0;
        return result;
    }
    for (size_t i = 0; i < 3; ++i)
    {
        const auto movement = double(center[i]) - RecenterOrigin[i];
        const auto origin = double(RecenterStart[i]) + movement;
        adjusted[i] = static_cast<float>(origin + (double(pose[i]) - origin) * amount);
    }
    // Collision remains in the outer native chain. Blend unit directions along
    // an arc, including a half turn, and keep the up vector perpendicular.
    std::array<double, 3> start, end;
    double startLength = 0, endLength = 0;
    for (size_t i = 0; i < 3; ++i)
    {
        start[i] = RecenterStart[i + 3];
        end[i] = pose[i + 3];
        startLength += start[i] * start[i];
        endLength += end[i] * end[i];
    }
    if (startLength < 1.0e-10 || endLength < 1.0e-10)
    {
        RecenterDelay.blend = 1.0;
        return result;
    }
    double dot = 0;
    for (size_t i = 0; i < 3; ++i)
    {
        start[i] /= std::sqrt(startLength);
        end[i] /= std::sqrt(endLength);
        dot += start[i] * end[i];
    }
    dot = std::clamp(dot, -1.0, 1.0);
    const auto angle = std::acos(dot);
    const auto sine = std::sin(angle);
    for (size_t i = 0; i < 3; ++i)
    {
        double direction;
        if (dot < -0.9995)
        {
            const auto j = (i + 1) % 3, k = (i + 2) % 3;
            const auto right = double(RecenterStart[j + 6]) * start[k] -
                double(RecenterStart[k + 6]) * start[j];
            direction = start[i] * std::cos(3.14159265358979323846 * amount) +
                right * std::sin(3.14159265358979323846 * amount);
        }
        else if (sine > 1.0e-6)
            direction = (start[i] * std::sin((1.0 - amount) * angle) +
                end[i] * std::sin(amount * angle)) / sine;
        else
            direction = start[i] + (end[i] - start[i]) * amount;
        adjusted[i + 3] = static_cast<float>(direction);
        adjusted[i + 6] = static_cast<float>(double(RecenterStart[i + 6]) +
            (double(pose[i + 6]) - RecenterStart[i + 6]) * amount);
    }
    double forwardLength = 0;
    for (size_t i = 3; i < 6; ++i)
        forwardLength += double(adjusted[i]) * adjusted[i];
    if (forwardLength < 1.0e-10)
    {
        RecenterDelay.blend = 1.0;
        return result;
    }
    double projection = 0;
    for (size_t i = 0; i < 3; ++i)
    {
        adjusted[i + 3] = static_cast<float>(adjusted[i + 3] / std::sqrt(forwardLength));
        projection += double(adjusted[i + 3]) * adjusted[i + 6];
    }
    double upLength = 0;
    for (size_t i = 0; i < 3; ++i)
    {
        adjusted[i + 6] = static_cast<float>(adjusted[i + 6] - projection * adjusted[i + 3]);
        upLength += double(adjusted[i + 6]) * adjusted[i + 6];
    }
    if (upLength < 1.0e-10)
    {
        RecenterDelay.blend = 1.0;
        return result;
    }
    for (size_t i = 6; i < 9; ++i)
        adjusted[i] = static_cast<float>(adjusted[i] / std::sqrt(upLength));
    WriteGameValue(pose, adjusted[0]);
    for (size_t i = 1; i < adjusted.size(); ++i)
        WriteGameValue(pose + i, adjusted[i]);
    return result;
}

class Input
{
public:
    Input()
    {
        WFP::onInitEvent() += []
        {

            auto pattern = hook::pattern("8B 15 ? ? ? ? 89 44 24 ? 8B 4F"); //0x473B75 + 2
            pInput = BindGameRef(inputManager, *pattern.get_first<uintptr_t*>(2));
            SetDefaultPadBindings();

            #ifdef _DEBUG
            // Allow the Windows key while debugging; keep foreground, nonexclusive input.
            pattern = hook::pattern("6A 16 51 50 FF 52 34 8B 44 24 ? 8B 10 50 FF 52 1C"); // 0x82150C + 1
            WriteGameValue<uint8_t>(pattern.get_first(1), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
            #endif

            pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? 85 C0 74 ? 50 68"); //0x81D240
            shReadInput = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), ReadInput);

            pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? C2 ? ? 8B 54 24"); //0x58E973
            hbNormalizeAxis.fun = injector::MakeCALL(pattern.get_first(), NormalizeCameraAxis, true).get();

            pattern = hook::pattern("E8 ? ? ? ? ? ? 83 C4 ? C2 ? ? 8A 44 24"); //0x58E985
            injector::MakeCALL(pattern.get_first(), NormalizeCameraAxis, true);

            pattern = hook::pattern("E8 ? ? ? ? 8B 54 24 ? 52 8D 4D"); //0x4B54E6
            pUpdateLookaroundAxis = reinterpret_cast<decltype(pUpdateLookaroundAxis)>(injector::MakeCALL(pattern.get_first(), UpdateLookaroundAxis, true).get<void>());

            pattern = hook::pattern("E8 ? ? ? ? 8A 45 ? 84 C0 0F 84 ? ? ? ? 8A 45"); //0x4B54F3
            injector::MakeCALL(pattern.get_first(), UpdateLookaroundAxis, true);

            auto& settings = FixSettings();
            MouseSensitivity.store(settings.Float("MAIN.MouseSensitivity"), std::memory_order_relaxed);
            settings.OnChange("MAIN.MouseSensitivity", []()
            {
                MouseSensitivity.store(FixSettings().Float("MAIN.MouseSensitivity"), std::memory_order_relaxed);
            });
            InvertMouseY.store(settings.Bool("MAIN.InvertMouseY"), std::memory_order_relaxed);
            settings.OnChange("MAIN.InvertMouseY", []()
            {
                InvertMouseY.store(FixSettings().Bool("MAIN.InvertMouseY"), std::memory_order_relaxed);
            });
            GamepadCameraSpeed.store(settings.Float("MAIN.GamepadCameraSpeed"));
            settings.OnChange("MAIN.GamepadCameraSpeed", []()
            {
                GamepadCameraSpeed.store(FixSettings().Float("MAIN.GamepadCameraSpeed"));
            });
            auto cameraSpeed = hook::pattern("D9 44 24 30 D8 4C 24 60 D9 5C 24 18 D9 44 24 34 D8 4C 24 60 D9 5C 24 24");
            static auto cameraSpeedHook = safetyhook::create_mid(cameraSpeed.get_first(), [](SafetyHookContext& regs)
            {
                auto speed = GamepadCameraSpeed.load();
                const auto invert = InvertMouseY.load(std::memory_order_relaxed);
                // Adjust after the native response curve, before frame-time multiplication.
                for (size_t axis = 0; axis < LookaroundMouse.size(); ++axis)
                {
                    auto value = reinterpret_cast<float*>(regs.esp + 0x30 + axis * sizeof(float));
                    const auto adjusted = AdjustCameraInput(*value, LookaroundMouse[axis], axis, speed, invert);
                    if (std::isfinite(adjusted) && adjusted != *value)
                        WriteGameValue(value, adjusted);
                }
            });
            CameraAutoCenter.store(settings.Int("MAIN.CameraAutoCenter"));
            CameraRecenterSeconds.store(settings.Float("MAIN.CameraRecenterDelay"));
            settings.OnChange("MAIN.CameraAutoCenter", []()
            {
                CameraAutoCenter.store(FixSettings().Int("MAIN.CameraAutoCenter"));
            });
            settings.OnChange("MAIN.CameraRecenterDelay", []()
            {
                CameraRecenterSeconds.store(FixSettings().Float("MAIN.CameraRecenterDelay"));
            });
            auto recenter = hook::pattern("83 EC 4C 53 8B 5C 24 5C 55 8B E9 8B 45 04 8A 88 BC 01 00 00");
            shCameraRecenter = safetyhook::create_inline(recenter.get_first(), UpdateRecenterCamera);
            // Both camera axes are current here; preserve native translation and collision.
            pattern = hook::pattern("8A 45 6D 84 C0 0F 84 ? ? ? ? 8A 45 3D 84 C0");
            static auto activeLookaround = injector::ReadRelativeOffset(pattern.get_first(7), 4).as_int();
            static auto CameraRecenterHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                const auto horizontalIdle = *reinterpret_cast<uint8_t*>(regs.ebp + 0x3D) != 0;
                const auto verticalIdle = *reinterpret_cast<uint8_t*>(regs.ebp + 0x6D) != 0;
                const auto active = !horizontalIdle || !verticalIdle;
                const auto dt = *reinterpret_cast<float*>(regs.esp + 0x60);
                const auto velocity = reinterpret_cast<const float*>(regs.ebx + 36);
                double squared = 0;
                for (size_t i = 0; i < 3; ++i)
                    if (std::isfinite(velocity[i])) squared += double(velocity[i]) * velocity[i];
                RecenterUpdated = true;
                const auto hold = RecenterDelay.Update(regs.ebp, active, dt,
                    static_cast<float>(std::sqrt(std::min(squared, 10000.0))),
                    shCameraRecenter ? CameraAutoCenter.load() : 1,
                    shCameraRecenter ? CameraRecenterSeconds.load() : 3.0f,
                    *reinterpret_cast<const uint8_t*>(*reinterpret_cast<const uintptr_t*>(regs.ebp + 4) + 460) != 0);
                if (hold && !active)
                {
                    for (const auto offset : { 0x20, 0x50, 0x70, 0x74 })
                        WriteGameValue<float>(regs.ebp + offset, 0.0f);
                    regs.eip = activeLookaround;
                }
            });

            pattern = hook::pattern("A1 ? ? ? ? 53 50 FF 15"); //0x581CBB + 1
            pWindow = BindGameRef(inputWindow, *pattern.get_first<HWND*>(1));

            pattern = hook::pattern("8B 4E ? 52 E8 ? ? ? ? 5E"); //0x5855A5
            pSetCursorRect = reinterpret_cast<decltype(pSetCursorRect)>(injector::MakeCALL(pattern.get_first(4), SetMouseCursorRect, true).get<void>());

            pattern = hook::pattern("E8 ? ? ? ? 8B 7C 24 ? 8B 74 24 ? 8B 43"); //0x585830
            injector::MakeCALL(pattern.get_first(), SetMouseCursorRect, true);

            pattern = hook::pattern("E8 ? ? ? ? 8B 86 ? ? ? ? 8B 8E ? ? ? ? 89 44 24"); //0x581C60
            static auto MouseCursorHook = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), UpdateMouseCursor);
        };
    }
} Input;

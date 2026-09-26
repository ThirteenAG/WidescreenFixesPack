module;

#include "stdafx.h"
#ifdef _DEBUG
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#endif

export module Input;

import Screen;

HWND* pWindow = nullptr;
uintptr_t* pInput = nullptr;

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
    auto window = *pWindow;
    // Convert to client pixels before scaling, including windows with an offset.
    if (!ScreenToClient(window, &point))
        return FALSE;
    // The cursor is drawn in backbuffer pixels, even if the client size differs.
    auto width = Screen.Width;
    auto height = Screen.Height;
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

thread_local float* pLookaroundDeadzone = nullptr;

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
    auto input = *pInput;
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
    if (value < -32768 || value > 32767)
    {
        float mouse;
        std::memcpy(&mouse, &value, sizeof(mouse));
        if (std::isnormal(mouse))
        {
            if (deadzone)
                *deadzone = 0.0f;
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
    pLookaroundDeadzone = &deadzone;
    pUpdateLookaroundAxis(axis, dt);
    pLookaroundDeadzone = previous;
    deadzone = savedDeadzone;
}

struct CameraRecenterDelay
{
    uintptr_t camera = 0;
    double idleSeconds = 3.0;

    bool Update(uintptr_t currentCamera, bool cameraInput, float dt)
    {
        if (camera != currentCamera)
        {
            camera = currentCamera;
            idleSeconds = 3.0;
        }
        if (cameraInput)
            idleSeconds = 0.0;
        else if (std::isfinite(dt) && dt > 0.0f)
            idleSeconds = std::min(3.0, idleSeconds + dt);
        return idleSeconds < 3.0;
    }
} RecenterDelay;

class Input
{
public:
    Input()
    {
        WFP::onInitEvent() += []
        {
            auto pattern = hook::pattern("8B 15 ? ? ? ? 89 44 24 ? 8B 4F"); //0x473B75 + 2
            pInput = *pattern.get_first<uintptr_t*>(2);

            #ifdef _DEBUG
            // Allow the Windows key while debugging; keep foreground, nonexclusive input.
            pattern = hook::pattern("6A 16 51 50 FF 52 34 8B 44 24 ? 8B 10 50 FF 52 1C"); // 0x82150C + 1
            injector::WriteMemory<uint8_t>(pattern.get_first(1), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE, true);
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

            // Both axes have been updated here. The idle path immediately
            // releases look-around when moving; retain it with zero rotation
            // for three seconds, still running translation/collision handling.
            pattern = hook::pattern("8A 45 6D 84 C0 0F 84 ? ? ? ? 8A 45 3D 84 C0"); //0x4B54F8
            static auto activeLookaround = injector::ReadRelativeOffset(pattern.get_first(7), 4).as_int(); //0x4B54F8 + 7
            static auto CameraRecenterHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                auto horizontalIdle = *reinterpret_cast<uint8_t*>(regs.ebp + 0x3D) != 0;
                auto verticalIdle = *reinterpret_cast<uint8_t*>(regs.ebp + 0x6D) != 0;
                auto active = !horizontalIdle || !verticalIdle;
                auto dt = *reinterpret_cast<float*>(regs.esp + 0x60);
                if (RecenterDelay.Update(regs.ebp, active, dt) && !active)
                {
                    *reinterpret_cast<float*>(regs.ebp + 0x20) = 0.0f;
                    *reinterpret_cast<float*>(regs.ebp + 0x50) = 0.0f;
                    *reinterpret_cast<float*>(regs.ebp + 0x70) = 0.0f;
                    *reinterpret_cast<float*>(regs.ebp + 0x74) = 0.0f;
                    regs.eip = activeLookaround;
                }
            });

            pattern = hook::pattern("A1 ? ? ? ? 53 50 FF 15"); //0x581CBB + 1
            pWindow = *pattern.get_first<HWND*>(1);

            pattern = hook::pattern("8B 4E ? 52 E8 ? ? ? ? 5E"); //0x5855A5
            pSetCursorRect = reinterpret_cast<decltype(pSetCursorRect)>(injector::MakeCALL(pattern.get_first(4), SetMouseCursorRect, true).get<void>());

            pattern = hook::pattern("E8 ? ? ? ? 8B 7C 24 ? 8B 74 24 ? 8B 43"); //0x585830
            injector::MakeCALL(pattern.get_first(), SetMouseCursorRect, true);

            pattern = hook::pattern("E8 ? ? ? ? 8B 86 ? ? ? ? 8B 8E ? ? ? ? 89 44 24"); //0x581C60
            static auto MouseCursorHook = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), UpdateMouseCursor);
        };
    }
} Input;

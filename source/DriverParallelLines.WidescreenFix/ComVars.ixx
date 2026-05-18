module;

#include "stdafx.h"
#include <d3d9.h>

export module ComVars;

export float fTimeStep = 0.0f;

export GameRef<HWND> hWnd;
export GameRef<uint32_t> dwWindowedMode;
export GameRef<int> BackbufferWidth;
export GameRef<int> BackbufferHeight;
export GameRef<IDirect3DDevice9*> Direct3DDevice;
export GameRef<int32_t> pCurrentMovieStatus;

export GameRef<bool> isTurret([]() -> bool*
{
    auto pattern = hook::pattern("A2 ? ? ? ? 74 ? F3 0F 10 05");
    if (!pattern.empty())
        return *pattern.get_first<bool*>(1);
    return nullptr;
});

export GameRef<uintptr_t> dword_70C59C([]() -> uintptr_t*
{
    auto pattern = hook::pattern("A3 ? ? ? ? E8 ? ? ? ? 6A ? 59 E8 ? ? ? ? A3 ? ? ? ? ? ? 6A");
    if (!pattern.empty())
        return *pattern.get_first<uintptr_t*>(1);
    return nullptr;
});

export auto ReadActionMapsID = []() -> uint16_t
{
    uintptr_t ptr = *dword_70C59C;
    if (!ptr)
        return 0;

    auto index = *(uint16_t*)(ptr + 0x596);
    return *(uint16_t*)(ptr + 0x584 + 4 * index);
};

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

export bool IsKeyboardKeyPressed(int vkeycode)
{
    return (GetAsyncKeyState(vkeycode) & 0x8000) != 0;
}

export struct DoubleTapDetector
{
    bool  prev_pressed = false;
    bool  waiting_for_release = false;
    int   tap_count = 0;
    float tap_timer = 0.0f;

    static constexpr float DOUBLE_TAP_WINDOW = 500.0f;

    bool Update(float cur_value, float dt_ms)
    {
        bool cur_pressed = (cur_value != 0.0f);
        bool fired = false;

        bool just_pressed = cur_pressed && !prev_pressed;
        bool just_released = !cur_pressed && prev_pressed;

        if (just_released)
            waiting_for_release = false;

        if (just_pressed && !waiting_for_release)
        {
            if (tap_count == 1 && tap_timer > 0.0f)
            {
                fired = true;
                tap_count = 0;
                tap_timer = 0.0f;
            }
            else
            {
                tap_count = 1;
                tap_timer = DOUBLE_TAP_WINDOW;
            }
            waiting_for_release = true;
        }

        if (tap_count == 1 && tap_timer > 0.0f)
        {
            tap_timer -= dt_ms;
            if (tap_timer <= 0.0f)
            {
                tap_count = 0;
                tap_timer = 0.0f;
            }
        }

        prev_pressed = cur_pressed;
        return fired;
    }
};
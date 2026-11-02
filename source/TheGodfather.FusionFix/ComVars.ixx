module;

#include <stdafx.h>

export module ComVars;

export GameRef<uint32_t> dword_1062BA4;
export GameRef<uint32_t> dword_1062BA8;
export GameRef<float> dword_1061AF0;
export GameRef<uint32_t> Timer_ticks, Timer_milliseconds, Timer_elapsedMilliseconds, Timer_gameMilliseconds, Timer_maximumTicks;
export GameRef<uint64_t> Timer_gameMicroseconds;
export GameRef<float> Timer_tickError, Timer_seconds, Timer_gameSeconds, Timer_unscaled, Timer_scaled, Timer_scale, Timer_quantum, Timer_frequency;
export GameRef<void*> CinematicRoot;
export GameRef<int> GodfatherWantedMode;
export GameRef<uint32_t> GodfatherPauseReasons;
export GameRef<uint8_t> GodfatherIdle;
export GameRef<void*> GodfatherInputSlot;
export GameRef<void*> GodfatherTextSlot;
export GameRef<void*> GodfatherRenderSlot;

export namespace GodfatherWindowModes
{
    struct Mode
    {
        int width{}, height{};
        unsigned refresh{}, format{}, flags{};
    };
    static_assert(sizeof(Mode) == 20);
    GameRef<Mode*> modes;
    GameRef<int> count, active, selected;
    GameRef<void*> engine, display, frontend;
}
export GameRef<const char*> GodfatherWindowClass;
export GameRef<int> GodfatherInterpreter;
export GameRef<void*> GodfatherMenuScreens;

export __declspec(noinline) WFP::Event<int, int>& onResChange()
{
    static WFP::Event<int, int> event;
    return event;
}

export struct Screen
{
    int32_t nWidth = 640;
    int32_t nHeight = 480;
    float fWidth = 640.0f;
    float fHeight = 480.0f;
    float fAspectRatio = 4.0f / 3.0f;
    int32_t nWidth43;
    float fWidth43;
    float fHudScale = 1.0f;
    float fHudOffset;
    float fHudOffsetReal;
    float fMainMenuPlayerFOV;
    bool bHudFlash;
} Screen;

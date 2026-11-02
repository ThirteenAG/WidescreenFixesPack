module;

#include "stdafx.h"
#include <d3d9.h>

export module ComVars;

export struct CVehicle;
export struct CCharacter
{
    CVehicle* GetVehicle()
    {
        return *(CVehicle**)((uintptr_t)this + 0x688);
    }
};

export struct CPlayerDriverBehaviour
{
    CCharacter* GetCharacter()
    {
        return *(CCharacter**)((uintptr_t)this + 0x2C4);
    }
};

export struct CLifePlayer
{
    int field0;           // +0x00
    int field4;           // +0x04
    CPlayerDriverBehaviour* pDriverBehaviour; // +0x08

    CPlayerDriverBehaviour* GetDriverBehaviour()
    {
        return pDriverBehaviour;
    }
};

export struct CLifeSystem
{
    int field0;   // +0x00
    int field4;   // +0x04
    CLifePlayer* pPlayer;  // +0x08

    CLifePlayer* GetPlayer()
    {
        return pPlayer;
    }
};

export GameRef<CLifeSystem*> pLifeSystem;

export struct CGameTime
{
    float f[2];
};

export GameRef<CGameTime*> pGameTime;

export class FrontendInputManager;
export GameRef<FrontendInputManager*> pFrontendInputManager;


export float fTimeStep = 1.0f / 60.0f;
export bool BlockGameInput = false;

export GameRef<HWND> hWnd;
export GameRef<void*> GuiService;
export GameRef<void*> FontService;
export GameRef<void*> LanguageService;
export GameRef<uint8_t> TextureManager;
export GameRef<uint8_t> off_702640;
export GameRef<uint8_t> bLegalScreenShown;
export GameRef<uint32_t> DiffuseHash;
export GameRef<bool> byte_7103C2;
export GameRef<int*> pProfileSettings;
export GameRef<float> MouseDeltaX;
export GameRef<float> MouseDeltaY;
export GameRef<float> fFieldOfView;

export struct MouseAccumulation
{
    uint32_t totalX, totalY;
    float x, y, velocityX, velocityY;
    uint8_t buttons[3];
};
export GameRef<MouseAccumulation> accumulation;
export GameRef<float> coefficient;

export GameRef<uint8_t> dwWindowedMode;
export GameRef<int> BackbufferWidth;
export GameRef<int> BackbufferHeight;
export GameRef<IDirect3DDevice9*> Direct3DDevice;
export GameRef<int32_t> pCurrentMovieStatus;

export GameRef<bool> isTurret;

export GameRef<uintptr_t> dword_70C59C;

export void InitComVarsRequiredRefs()
{
    isTurret.SetAddress(*hook::pattern("A2 ? ? ? ? 74 ? F3 0F 10 05").get_first<bool*>(1));
    dword_70C59C.SetAddress(*hook::pattern("8B 0D ? ? ? ? E8 ? ? ? ? 8B 0D ? ? ? ? E8 ? ? ? ? 8B 0D ? ? ? ? E8 ? ? ? ? 8B 0D ? ? ? ? 83 C1").get_first<uintptr_t*>(2));
}


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
    return !BlockGameInput && hWnd.get() && GetForegroundWindow() == hWnd.get() && (GetAsyncKeyState(vkeycode) & 0x8000) != 0;
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
module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include "Frame.hpp"
#include "GamepadProfile.hpp"
#include "RawMouse.hpp"
#include "OverlayInput.hpp"
#include "VibrationInput.hpp"
#include <stdexcept>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

export module Input;
import ComVars;
import ControllerPrompts;

void InitXidi();
void InitScarfaceHook();

class OptionManager
{
public:
    static inline bool bGamepadUsed = false;
    static inline const float* GamepadCameraSpeed = nullptr;
    static inline const float* VehicleCameraRecenterDelay = nullptr;
    static inline uint32_t nVehicleCameraAimHoldOffset = 0;
    static inline bool (*pScarfaceHook_GetMenuActive)() = nullptr;
    static inline bool (*NativeMenuActive)() = nullptr;

    static inline float GetVehicleCameraRecenterDelay()
    {
        return VehicleCameraRecenterDelay && std::isfinite(*VehicleCameraRecenterDelay)
            ? std::clamp(*VehicleCameraRecenterDelay, 0.0f, 60.0f) : 3.0f;
    }

    static inline void RecordGamepadMotion(float x, float y)
    {
        bGamepadUsed = (std::isfinite(x) && x != 0.0f) || (std::isfinite(y) && y != 0.0f);
    }

    __declspec(noinline) static void ScaleGamepadMotion(volatile float* x, volatile float* y)
    {
        alignas(16) unsigned char state[512];
        _fxsave(state);
        const float horizontal = *x, vertical = *y;
        RecordGamepadMotion(horizontal, vertical);
        const float speed = GamepadCameraSpeed && std::isfinite(*GamepadCameraSpeed)
            ? std::clamp(*GamepadCameraSpeed, 0.1f, 5.0f) : 1.0f;
        if (std::isfinite(horizontal))
            *x = horizontal * speed;
        if (std::isfinite(vertical))
            *y = vertical * speed;
        _fxrstor(state);
    }

    static inline bool IsInvertX()
    {
        return bIsInvertX.get();
    }
    static inline bool IsInvertY()
    {
        return bIsInvertY.get();
    }
    static inline float GetMouseLookSensitivity()
    {
        return nMouseLookSensitivity.get() / 10.0f;
    }
    static inline bool ScarfaceHook_GetMenuActive()
    {
        return (pScarfaceHook_GetMenuActive && pScarfaceHook_GetMenuActive()) ||
            (NativeMenuActive && NativeMenuActive());
    }
};

SafetyHookInline shWndProc = {};
LRESULT WINAPI WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if ((Msg == WM_KILLFOCUS || (Msg == WM_ACTIVATEAPP && !wParam)) && ScarfaceVibrationInput::stop)
        ScarfaceVibrationInput::stop();
    LRESULT result = 0;
    if (ScarfaceOverlayInput::Message(hWnd, Msg, wParam, lParam, result))
        return result;
    ControllerPrompts::OnMessage(hWnd, Msg, wParam, lParam);
    if (Msg == WM_KILLFOCUS || (Msg == WM_ACTIVATEAPP && !wParam))
        ScarfaceRawMouse::Clear();
    static bool bOnce = false;
    if (!bOnce)
    {
        RawCursorHandler<float>::Initialize(hWnd, 1.0f);
        bOnce = true;
    }

    return shWndProc.unsafe_stdcall<LRESULT>(hWnd, Msg, wParam, lParam);
}

bool SetMouseInput(float* dest_x, float* dest_y, std::optional<float> baseline_x = {}, std::optional<float> baseline_y = {}, unsigned axes = 3)
{
    if (ScarfaceOverlayInput::BlocksInput() || !hWndInput.is_initialized() || !hWndInput.get() || hWndInput.get() != GetForegroundWindow() || OptionManager::ScarfaceHook_GetMenuActive())
    {
        ScarfaceRawMouse::Clear();
        return false;
    }
    if (OptionManager::bGamepadUsed && (!ScarfaceRawMouse::Active() || !ScarfaceRawMouse::HasMotion(axes)))
        return false;

    const auto [dx, dy] = ScarfaceRawMouse::Consume(axes, OptionManager::GetMouseLookSensitivity() / 100.0f);
    const float sign_x = OptionManager::IsInvertX() ? -1.0f : 1.0f;
    const float sign_y = OptionManager::IsInvertY() ? -1.0f : 1.0f;
    if (axes & 1)
        *dest_x = baseline_x.value_or(*dest_x) + dx * sign_x;
    if (axes & 2)
        *dest_y = baseline_y.value_or(*dest_y) + dy * sign_y;
    return dx != 0.0f || dy != 0.0f;
}

injector::hook_back<char(__fastcall*)(float* mInputs, void* edx, float* angleH, float* angleV, float deltaTime)> hb_GenericVehicleCamera_ApplyRightStickMotion;
char __fastcall GenericVehicleCamera_ApplyRightStickMotion(float* mInputs, void* edx, float* angleH, float* angleV, float deltaTime)
{
    auto H = *angleH;
    auto V = *angleV;

    auto ret = hb_GenericVehicleCamera_ApplyRightStickMotion.fun(mInputs, edx, angleH, angleV, deltaTime);

    const bool mouseMoved = SetMouseInput(angleH, angleV, H, V);

    static const float* lastCamera = nullptr;
    static float timeSinceLook = 60.0f;
    static bool temporaryAimHold = false;
    auto& aimHold = *reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(mInputs) + OptionManager::nVehicleCameraAimHoldOffset);
    if (lastCamera != mInputs)
    {
        lastCamera = mInputs;
        timeSinceLook = 60.0f;
        temporaryAimHold = false;
    }

    if (GetAsyncKeyState(VK_RBUTTON))
    {
        // Preserve the existing native aim-hold mode; do not expire or clear it.
        timeSinceLook = 60.0f;
        temporaryAimHold = false;
        return 1;
    }

    if (ret || mouseMoved)
        timeSinceLook = 0.0f;
    else if (std::isfinite(deltaTime) && deltaTime > 0.0f)
        timeSinceLook = std::min(60.0f, timeSinceLook + deltaTime);

    // A genuine stick input owns the game's native hold. Otherwise remember when
    // our timeout creates a hold, so ordinary mouse look cannot become permanent
    // if the vehicle stops before the delay expires.
    if (ret)
        temporaryAimHold = false;
    const bool active = ret || mouseMoved || timeSinceLook < OptionManager::GetVehicleCameraRecenterDelay();
    if (active && !ret && !aimHold)
        temporaryAimHold = true;
    if (!active && temporaryAimHold)
    {
        aimHold = false;
        temporaryAimHold = false;
    }
    return active;
}

float __cdecl math__ClampX(float* value, float* min, float* max)
{
    float dummyY = 0.0f;
    SetMouseInput(value, &dummyY, {}, {}, 1);
    return *value;
}

float __cdecl math__ClampY(float* value, float* min, float* max)
{
    float dummyX = 0.0f;
    SetMouseInput(&dummyX, value, {}, {}, 2);
    return *value;
}

void Init()
{
    auto& settings = FixSettings();
    auto bScrollWeaponsWithMouseWheel = settings.Bool("MAIN.ScrollWeaponsWithMouseWheel");
    OptionManager::GamepadCameraSpeed = &settings.Float("MAIN.GamepadCameraSpeed");
    OptionManager::VehicleCameraRecenterDelay = &settings.Float("MAIN.VehicleCameraRecenterDelay");
    auto menu = hook::pattern("E8 ? ? ? ? 84 C0 75 A4");
    OptionManager::NativeMenuActive = reinterpret_cast<decltype(OptionManager::NativeMenuActive)>(injector::GetBranchDestination(menu.get_first()).as_int());

    auto pattern = hook::pattern("A2 ? ? ? ? A2 ? ? ? ? A2 ? ? ? ? A2 ? ? ? ? A2 ? ? ? ? A2 ? ? ? ? A3");
    bIsInvertX.SetAddress(*pattern.get_first<bool*>(1));
    bIsInvertY.SetAddress(*pattern.get_first<bool*>(6));
    pattern = hook::pattern("8B 44 24 04 A3 ? ? ? ? C3 CC CC CC CC CC CC A1 ? ? ? ? C3 CC CC CC CC CC CC CC CC CC CC 8A 44 24 04 A2 ? ? ? ? C3 CC CC CC CC CC CC A0 ? ? ? ? C3 CC CC CC CC CC CC CC CC CC CC 8A 44 24 04 A2 ? ? ? ? C3");
    nMouseLookSensitivity.SetAddress(*pattern.get_first<int*>(5));

    pattern = hook::pattern("A3 ? ? ? ? 5B 74 0F");
    hWndInput.SetAddress(*pattern.get_first<HWND*>(1));

    pattern = hook::pattern("8B 0D ? ? ? ? 53 8B 5C 24 ? 55");
    shWndProc = safetyhook::create_inline(pattern.get_first(), WndProc);

    pattern = hook::pattern("D9 58 10 83 C4 5C C2 0C 00");
    struct SetOutput_Update_FreeLook
    {
        void operator()(injector::reg_pack& regs)
        {
            SetMouseInput((float*)(regs.eax + 0x0C), (float*)(regs.eax + 0x10));
        }
    }; injector::MakeInline<SetOutput_Update_FreeLook>(pattern.get_first(6));
    injector::MakeRET(pattern.get_first(6 + 5), 0xC);

    //camera blend disable
    pattern = hook::pattern("83 EC 10 57 8B F9 80 BF");
    injector::MakeRET(pattern.get_first(0), 8);

    //vehicle camera
    pattern = hook::pattern("E8 ? ? ? ? 84 C0 74 0C C6 85");
    OptionManager::nVehicleCameraAimHoldOffset = *pattern.get_first<uint32_t>(11);
    hb_GenericVehicleCamera_ApplyRightStickMotion.fun = injector::MakeCALL(pattern.get_first(0), GenericVehicleCamera_ApplyRightStickMotion, true).get();

    //melee camera
    pattern = hook::pattern("D9 59 10 88 41 19");
    struct SetOutput_Update_MeleeTarget
    {
        void operator()(injector::reg_pack& regs)
        {
            *reinterpret_cast<uint8_t*>(regs.ecx + 0x19) = static_cast<uint8_t>(regs.eax);
            *reinterpret_cast<uint8_t*>(regs.ecx + 0x18) = 0;
            SetMouseInput((float*)(regs.ecx + 0x0C), (float*)(regs.ecx + 0x10));
        }
    }; injector::MakeInline<SetOutput_Update_MeleeTarget>(pattern.get_first(3), pattern.get_first(10));

    //map camera
    pattern = hook::pattern("E8 ? ? ? ? D9 5C 24 2C 83 C4 0C D9 05");
    injector::MakeCALL(pattern.get_first(0), math__ClampX, true); //mousePanXID
    pattern = hook::pattern("E8 ? ? ? ? D9 5C 24 24 D9 05 ? ? ? ? 83");
    injector::MakeCALL(pattern.get_first(0), math__ClampY, true); //mousePanYID
    pattern = hook::pattern("7A 0E D9 44 24 1C B1 01");
    injector::MakeNOP(pattern.get_first(0), 2);
    pattern = hook::pattern("75 2B 8B 44 24 20 89 44 24 10");
    injector::MakeNOP(pattern.get_first(0), 2);
    pattern = hook::pattern("75 2B 8B 44 24 18 89 44 24 10 21 54 24 10");
    injector::MakeNOP(pattern.get_first(0), 2);
    pattern = hook::pattern("7A 0E D9 44 24 1C D8 4C 24 18");
    injector::MakeNOP(pattern.get_first(0), 2);
    pattern = hook::pattern("7A 08 DD D8 D9 05 ? ? ? ? D9 44 24 20 8D BE ? ? ? ? D8 C9");
    injector::MakeNOP(pattern.get_first(0), 2);

    pattern = hook::pattern("B9 ? ? ? ? 21 4C 24 10 D9 44 24 10 89 54 24 14");
    struct GamepadCheck
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx = 0x7FFFFFFF;
            OptionManager::ScaleGamepadMotion(reinterpret_cast<float*>(regs.esp + 0x1C), reinterpret_cast<float*>(regs.esp + 0x18));
        }
    }; injector::MakeInline<GamepadCheck>(pattern.get_first(0));

    if (bScrollWeaponsWithMouseWheel)
    {
        static int nMouseWheelValue = 0;
        pattern = hook::pattern("8B B7 ? ? ? ? 85 F6 74 21 90");
        struct GetDeviceDataHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.esi = *(uint32_t*)(regs.edi + 0x264);
                auto controller = std::string_view((char*)regs.edi + 0x40);

                if (controller.starts_with("Mouse"))
                {
                    auto rgdod = (LPDIDEVICEOBJECTDATA)(regs.esp + 0x10);
                    if (rgdod)
                    {
                        switch (rgdod->dwOfs)
                        {
                            case offsetof(DIMOUSESTATE, lZ):
                                nMouseWheelValue = int(std::clamp<int64_t>(int64_t(nMouseWheelValue) +
                                    int32_t(rgdod->dwData), -120 * WHEEL_DELTA, 120 * WHEEL_DELTA));
                                break;
                            default:
                                break;
                        }
                    }
                }
            }
        }; injector::MakeInline<GetDeviceDataHook>(pattern.get_first(0), pattern.get_first(6));

        pattern = hook::pattern("0F BF 54 24 08 ? ? ? ? 03");
        if (pattern.empty()) pattern = hook::pattern("0F BF 54 24 ? E9");
        struct MouseScroll
        {
            void operator()(injector::reg_pack& regs)
            {
                *(int32_t*)&regs.edx = *(int16_t*)(regs.esp + 0x8);

                if (ScarfaceOverlayInput::BlocksInput() || !hWndInput.is_initialized() || !hWndInput.get() ||
                    GetForegroundWindow() != hWndInput.get() || OptionManager::bGamepadUsed || OptionManager::ScarfaceHook_GetMenuActive())
                {
                    nMouseWheelValue = 0;
                    return;
                }

                enum
                {
                    weaponleft = 0x9,
                    weaponright = 0xA
                };

                // Consume each physical detent once. Mouse motion events must not erase queued wheel input.
                if (nMouseWheelValue >= WHEEL_DELTA)
                {
                    *reinterpret_cast<int16_t*>(regs.ecx + weaponleft * 2) = 1;
                    nMouseWheelValue -= WHEEL_DELTA;
                }
                else if (nMouseWheelValue <= -WHEEL_DELTA)
                {
                    *reinterpret_cast<int16_t*>(regs.ecx + weaponright * 2) = 1;
                    nMouseWheelValue += WHEEL_DELTA;
                }
            }
        }; injector::MakeInline<MouseScroll>(pattern.get_first(0));
    }

    ScarfaceFrame::Handlers.push_back([](void* display)
    {
        if (hWndInput.is_initialized() && hWndInput.get() &&
            *reinterpret_cast<HWND*>(static_cast<uint8_t*>(display) + 0x1C8) == hWndInput.get())
        {
            ScarfaceRawMouse::EndFrame();
            ScarfaceProfile::Update();
        }
    });
    ScarfaceRawMouse::Install(settings.Int("MAIN.RawInputMouse"), settings.Float("MAIN.MouseSensitivity"), &hWndInput);
    WFP::onShutdownEvent() += []()
    {
        ScarfaceProfile::Shutdown();
        ScarfaceRawMouse::Shutdown();
    };

    CallbackHandler::RegisterCallback(L"Xidi.32.dll", InitXidi);
    CallbackHandler::RegisterCallback(L"ScarfaceHook.asi", InitScarfaceHook);
}

void InitXidi()
{
    auto& settings = FixSettings();
    auto bModernControlScheme = settings.Bool("MAIN.ModernControlScheme");

    if (bModernControlScheme)
    {
        typedef bool (*XidiRegisterProfileCallbackFunc)(const wchar_t* (*callback)());
        auto xidiModule = GetModuleHandleW(L"Xidi.32.dll");

        if (xidiModule)
        {
            auto XidiRegisterProfileCallback = (XidiRegisterProfileCallbackFunc)GetProcAddress(xidiModule, "XidiRegisterProfileCallback");
            if (XidiRegisterProfileCallback)
            {
                auto menu = hook::pattern("A1 ? ? ? ? 80 B8 77 01 00 00 00 75 12 B9 80 00 00 00 39 48 44");
                auto player = hook::pattern("A1 ? ? ? ? 85 C0 74 50");
                static const ScarfaceProfile::Context context = [&]()
                {
                    auto isPaused = reinterpret_cast<bool(__cdecl*)()>(menu.get_first());
                    const auto flow = *menu.get_first<uintptr_t*>(1);
                    if (flow)
                        profileFlowManager.SetAddress(const_cast<uintptr_t*>(flow));
                    const auto character = *player.get_first<void**>(1);
                    if (character)
                        profileCharacter.SetAddress(character);
                    return ScarfaceProfile::Context{ isPaused, &profileFlowManager, &profileCharacter, 0x2E8 };
                }();
                ScarfaceProfile::Bindings.store(&context, std::memory_order_release);
                if (!XidiRegisterProfileCallback(ScarfaceProfile::Read))
                {
                    ScarfaceProfile::Shutdown();
                    throw std::runtime_error("Controller profile callback rejected");
                }
            }
            else
                throw std::runtime_error("Controller profile callback unavailable");
        }
        else
            throw std::runtime_error("Controller proxy unavailable");
    }
}

void InitScarfaceHook()
{
    OptionManager::pScarfaceHook_GetMenuActive = (bool (*)())GetProcAddress(GetModuleHandleW(L"ScarfaceHook.asi"), "ScarfaceHook_GetMenuActive");
}

class Input
{
public:
    Input() { WFP::onInitEvent() += Init; }
} Input;

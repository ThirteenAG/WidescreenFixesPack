module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include "Frame.hpp"
#include "GamepadProfile.hpp"
#include "RawMouse.hpp"
#include "OverlayInput.hpp"
#include "VibrationInput.hpp"
#include <stdexcept>

export module Input;
import ComVars;
import ControllerPrompts;

void InitXidi();
void InitScarfaceHook();

class OptionManager
{
public:
    static inline bool bGamepadUsed = false;
    static inline bool ScrollWeapons = false;
    static inline std::atomic<int> WeaponWheel{ 0 };
    static inline std::atomic<float> GamepadCameraSpeed{ 1.0f };
    static inline std::atomic<float> VehicleCameraRecenterDelay{ 3.0f };
    static inline std::atomic<float> OnFootCameraRecenterDelay{ 0.0f };
    static inline std::atomic<int> WalkKey{ 0 };
    static inline uint32_t nVehicleCameraAimHoldOffset = 0;
    static inline bool (*pScarfaceHook_GetMenuActive)() = nullptr;
    static inline bool (*NativeMenuActive)() = nullptr;

    static void QueueWeaponWheel(int delta)
    {
        auto pending = WeaponWheel.load(std::memory_order_relaxed);
        while (!WeaponWheel.compare_exchange_weak(pending, std::clamp(pending + delta, -120 * WHEEL_DELTA, 120 * WHEEL_DELTA),
            std::memory_order_relaxed))
        {
        }
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
        const float speed = GamepadCameraSpeed.load();
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
    {
        OptionManager::WeaponWheel.store(0, std::memory_order_relaxed);
        ScarfaceRawMouse::Clear();
    }
    if (Msg == WM_MOUSEWHEEL && OptionManager::ScrollWeapons && hWnd == GetForegroundWindow() &&
        !ScarfaceOverlayInput::BlocksInput() && !OptionManager::ScarfaceHook_GetMenuActive())
    {
        OptionManager::QueueWeaponWheel(GET_WHEEL_DELTA_WPARAM(wParam));
    }
    if (Msg == WM_NCDESTROY || Msg == WM_KILLFOCUS || (Msg == WM_ACTIVATEAPP && !wParam))
        ScarfaceRawMouse::UnbindCursor(hWnd);

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

    // Holding the secondary mouse button keeps the aimed view. GetAsyncKeyState reads the physical buttons, so
    // follow the system's left-handed setting; only while the game has the input.
    const int secondaryButton = GetSystemMetrics(SM_SWAPBUTTON) ? VK_LBUTTON : VK_RBUTTON;
    const bool hasInput = hWndInput.is_initialized() && hWndInput.get() && hWndInput.get() == GetForegroundWindow() &&
        !ScarfaceOverlayInput::BlocksInput() && !OptionManager::ScarfaceHook_GetMenuActive();
    if (hasInput && !OptionManager::bGamepadUsed && (GetAsyncKeyState(secondaryButton) & 0x8000))
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
    const bool active = ret || mouseMoved || timeSinceLook < OptionManager::VehicleCameraRecenterDelay.load();
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

// On-foot camera: while walking without look input the game switches to its follow mode, which swings the
// camera behind Tony and levels the pitch. The delay keeps the last aimed view for a while after looking.
namespace OnFootCamera
{
    constexpr uint32_t InputOffset = 0xAC;      // GenericCharacterCamera::mControllerInput
    constexpr uint32_t RightStickX = 0x0C;      // ControllerInput::mRightStickX, mRightStickY follows
    constexpr uint32_t LastAngleXZ = 0x1E8;     // mRotationAngleXZ_last, mRotationAngleY_last follows

    const uint8_t* Camera = nullptr;
    float TimeSinceLook = 60.0f;
    bool MouseLooked = false;

    injector::hook_back<void(__fastcall*)(uint8_t*, void*, float, float, float, float)> hbUpdateInput;
    void __fastcall UpdateInput(uint8_t* input, void* edx, float accelThreshold, float accelTime, float decelTime, float deltaTime)
    {
        hbUpdateInput.fun(input, edx, accelThreshold, accelTime, decelTime, deltaTime);

        const auto camera = input - InputOffset;
        if (camera != Camera)
        {
            Camera = camera;
            TimeSinceLook = 60.0f;
            MouseLooked = false;
        }
        const auto stick = reinterpret_cast<const float*>(input + RightStickX);
        if (MouseLooked || stick[0] != 0.0f || stick[1] != 0.0f)
            TimeSinceLook = 0.0f;
        else if (std::isfinite(deltaTime) && deltaTime > 0.0f)
            TimeSinceLook = std::min(60.0f, TimeSinceLook + deltaTime);
        MouseLooked = false;
    }

    injector::hook_back<uint8_t*(__fastcall*)(uint8_t*, void*, uint8_t*, float)> hbFollow;
    uint8_t* __fastcall Follow(uint8_t* camera, void* edx, uint8_t* output, float deltaTime)
    {
        auto result = hbFollow.fun(camera, edx, output, deltaTime);
        if (camera == Camera && TimeSinceLook < OptionManager::OnFootCameraRecenterDelay.load())
        {
            auto angles = reinterpret_cast<float*>(output + 0x0C);
            const auto last = reinterpret_cast<const float*>(camera + LastAngleXZ);
            angles[0] = last[0];
            angles[1] = last[1];
            if (SetMouseInput(&angles[0], &angles[1]))
                TimeSinceLook = 0.0f;
        }
        return result;
    }
}

// Walk key: limits the on-foot movement input to the walking range of the game's speed rings.
namespace WalkInput
{
    constexpr float WalkSpeed = 0.5f;

    injector::hook_back<void(__fastcall*)(float*, void*, void*)> hbUpdate;
    void __fastcall Update(float* input, void* edx, void* actionMap)
    {
        hbUpdate.fun(input, edx, actionMap);

        const int key = OptionManager::WalkKey.load();
        if (key <= 0 || key > 0xFE || !(GetAsyncKeyState(key) & 0x8000))
            return;
        if (ScarfaceOverlayInput::BlocksInput() || !hWndInput.is_initialized() || !hWndInput.get() ||
            hWndInput.get() != GetForegroundWindow() || OptionManager::ScarfaceHook_GetMenuActive())
            return;
        const float x = input[0], y = input[1];
        const float length = std::sqrt(x * x + y * y);
        if (!std::isfinite(length) || length <= WalkSpeed)
            return;
        input[0] = x * WalkSpeed / length;
        input[1] = y * WalkSpeed / length;
    }
}

void Init()
{
    auto& settings = FixSettings();
    OptionManager::ScrollWeapons = settings.Bool("MAIN.ScrollWeaponsWithMouseWheel");
    auto publish = []()
    {
        static std::mutex mutex;
        std::lock_guard lock(mutex);
        auto& settings = FixSettings();
        auto readFloat = [&](const char* key, float fallback, float minimum, float maximum)
        {
            const auto text = settings.Get(key);
            float value = fallback;
            std::from_chars(text.data(), text.data() + text.size(), value);
            return std::isfinite(value) ? std::clamp(value, minimum, maximum) : fallback;
        };
        OptionManager::GamepadCameraSpeed.store(readFloat("MAIN.GamepadCameraSpeed", 1.0f, 0.1f, 5.0f));
        OptionManager::VehicleCameraRecenterDelay.store(readFloat("MAIN.VehicleCameraRecenterDelay", 3.0f, 0.0f, 60.0f));
        OptionManager::OnFootCameraRecenterDelay.store(readFloat("MAIN.OnFootCameraRecenterDelay", 0.0f, 0.0f, 60.0f));
        OptionManager::WalkKey.store(std::atoi(settings.Get("MAIN.WalkKey").c_str()));
    };
    for (const char* key : { "MAIN.GamepadCameraSpeed", "MAIN.VehicleCameraRecenterDelay", "MAIN.OnFootCameraRecenterDelay", "MAIN.WalkKey" })
        settings.OnChange(key, publish);
    publish();

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
            if (SetMouseInput((float*)(regs.eax + 0x0C), (float*)(regs.eax + 0x10)))
                OnFootCamera::MouseLooked = true;
        }
    }; injector::MakeInline<SetOutput_Update_FreeLook>(pattern.get_first(6));
    injector::MakeRET(pattern.get_first(6 + 5), 0xC);

    //on-foot camera recenter delay
    pattern = hook::pattern("8D 8E AC 00 00 00 E8 ? ? ? ? 8B 86 F0 00 00 00 83 F8 06");
    OnFootCamera::hbUpdateInput.fun = injector::MakeCALL(pattern.get_first(6), OnFootCamera::UpdateInput, true).get();
    pattern = hook::pattern("8B 47 0C 50 8D 4C 24 38 51 8B CE E8 ? ? ? ? EB 5B");
    OnFootCamera::hbFollow.fun = injector::MakeCALL(pattern.get_first(11), OnFootCamera::Follow, true).get();

    //walk key
    pattern = hook::pattern("50 B9 ? ? ? ? E8 ? ? ? ? 57 8B CE E8 ? ? ? ? 8B CE E8 ? ? ? ? 57 8B CE E8");
    WalkInput::hbUpdate.fun = injector::MakeCALL(pattern.get_first(6), WalkInput::Update, true).get();

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

    if (OptionManager::ScrollWeapons)
    {
        pattern = hook::pattern("0F BF 54 24 08 ? ? ? ? 03");
        if (pattern.empty()) pattern = hook::pattern("0F BF 54 24 ? E9");
        struct MouseScroll
        {
            void operator()(injector::reg_pack& regs)
            {
                *(int32_t*)&regs.edx = *(int16_t*)(regs.esp + 0x8);

                auto& wheel = OptionManager::WeaponWheel;
                auto pending = wheel.load(std::memory_order_relaxed);
                if (pending > -WHEEL_DELTA && pending < WHEEL_DELTA)
                    return;
                constexpr int previousWeapon = 9;
                constexpr int nextWeapon = 10;
                const int action = *reinterpret_cast<const int*>(regs.esp + 4);
                if (action != (pending > 0 ? previousWeapon : nextWeapon))
                    return;
                if (ScarfaceOverlayInput::BlocksInput() || !hWndInput.is_initialized() || !hWndInput.get() ||
                    GetForegroundWindow() != hWndInput.get() || OptionManager::ScarfaceHook_GetMenuActive())
                {
                    wheel.store(0, std::memory_order_relaxed);
                    return;
                }

                // The native mapper writes this action later in the update. Give it a pressed input for just this
                // action and let the next update release it before sending another detent in the same direction.
                if (reinterpret_cast<const int16_t*>(regs.ecx)[action] == 1)
                    return;
                const int remaining = pending + (pending > 0 ? -WHEEL_DELTA : WHEEL_DELTA);
                if (wheel.compare_exchange_strong(pending, remaining, std::memory_order_relaxed))
                    regs.edx = 2; // Native input state 2 maps to action state 1 (pressed).
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
    ScarfaceRawMouse::Install(&hWndInput);
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

module;

#include <stdafx.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <dbt.h>
#include "TexturePack.hpp"
#include "FixSettings.hpp"
#include "../Shared/NFS/StickDeadzone.hpp"

export module Input;

import ComVars;

// Gamepads are handled by Xidi (Xidi.32.dll, loaded by the ASI loader, with the SDL3 plugin for PlayStation,
// Switch and other non-XInput pads). Xidi presents each pad as a "Xidi virtual controller". The bundled
// Xidi.ini uses the XInputNative layout with each trigger on its own half axis: X/Y = left stick,
// RotX/RotY = right stick, Z+ = LT, RotZ+ = RT, POV = d-pad, buttons 1-10 = A B X Y LB RB Back Start
// LS RS. This module gives such pads modern default bindings, proper names in the controls menu,
// controller icons in the front end, front end actions for the PC-only hotkeys and the right stick
// for the camera. Other devices (wheels and so on) keep the game's own behaviour.

namespace
{
    // The game's binding table: every action has two keyboard and two joystick bindings
    enum BindingType : int32_t
    {
        BindNone = 0,
        BindKey = 1,
        BindButton = 2,         // index = button
        BindPov = 3,            // index = 0 up, 1 right, 2 down, 3 left
        BindAxisLow = 4,        // X, Y, Z (index 0-2), the half below the centre
        BindAxisHigh = 5,       // the half above the centre
        BindRotationLow = 6,    // RotX, RotY, RotZ (index 0-2)
        BindRotationHigh = 7,
    };

    struct Binding
    {
        int32_t type;
        int32_t device;     // the key for keyboard bindings
        int32_t index;
    };

    struct ActionEntry
    {
        const char* name;
        Binding keyboard[2];
        Binding joystick[2];
    };
    static_assert(sizeof(ActionEntry) == 52);

    constexpr int ActionCount = 76;
    ActionEntry* Actions = nullptr;

    enum Action
    {
        ActionGas = 0,
        ActionBrake = 1,
        ActionSteerLeft = 2,
        ActionSteerRight = 3,
        ActionHandbrake = 4,
        ActionSpeedbreaker = 5,
        ActionNitrous = 6,
        ActionShiftDown = 7,
        ActionShiftUp = 8,
        ActionReset = 9,
        ActionEngageEvent = 10,
        ActionHudDown = 11,
        ActionHudLeft = 12,
        ActionHudRight = 13,
        ActionCameraChange = 15,
        ActionLookBack = 16,
        ActionPullback = 17,
        ActionPause = 18,
        ActionFELeft = 19,
        ActionFERight = 20,
        ActionFEUp = 21,
        ActionFEDown = 22,
        ActionSkipNIS = 23,
        ActionFERightTrigger = 29,
        ActionFELeftTrigger = 30,
        ActionFEAccept = 32,
        ActionFECancel = 33,
        ActionFEStart = 34,
        ActionFEPrevious = 35,      // LB
        ActionFENext = 36,          // RB
        ActionFEKey3 = 37,          // the "3" key
        ActionFEKey4 = 39,          // "4"
        ActionFEKey2 = 40,          // "2"
        ActionFEKey1 = 41,          // "1"
    };

    // Joystick state of the game's input library: lX, lY, lZ, then buttons as 0/128
    constexpr int MaxButtons = 128;

    enum PadButton { PadA, PadB, PadX, PadY, PadLB, PadRB, PadBack, PadStart, PadLS, PadRS, PadButtonCount };

    // Joystick devices of the game; the first one is used. The DirectInput device is at +0x218.
    uintptr_t* Joysticks = nullptr;

    enum class IconStyle { Xbox, PlayStation };
    const int* pGamepadIcons = nullptr;   // 0 = auto, 1 = Xbox, 2 = PlayStation; also changed in the options menu
    std::atomic<IconStyle> CurrentIconStyle = IconStyle::Xbox;
    std::atomic<bool> bIconStyleOutdated = false;

    bool IsXidiDevice(IDirectInputDevice8A* device)
    {
        // Xidi virtual controller instance GUIDs end with "XIDIVC"
        static std::map<IDirectInputDevice8A*, bool> cache;
        if (!device)
            return false;

        if (auto it = cache.find(device); it != cache.end())
            return it->second;

        DIDEVICEINSTANCEA info = { sizeof(DIDEVICEINSTANCEA) };
        bool isXidi = SUCCEEDED(device->GetDeviceInfo(&info)) && memcmp(info.guidInstance.Data4, "XIDIVC", 6) == 0;
        cache[device] = isXidi;
        return isXidi;
    }

    IDirectInputDevice8A* GetJoystick()
    {
        auto joystick = Joysticks ? Joysticks[0] : 0;
        return joystick ? *reinterpret_cast<IDirectInputDevice8A**>(joystick + 0x218) : nullptr;
    }

    bool IsXidiJoystick()
    {
        return IsXidiDevice(GetJoystick());
    }
}

// Icon style detection
namespace
{
    IconStyle DetectIconStyle()
    {
        if (*pGamepadIcons == 1)
            return IconStyle::Xbox;
        if (*pGamepadIcons == 2)
            return IconStyle::PlayStation;

        // XInput pads are Xbox pads; the SDL3 plugin handles everything else, so look for Sony pads
        static auto pXInputGetState = []() -> DWORD(WINAPI*)(DWORD, void*)
        {
            for (auto name : { "xinput1_4.dll", "xinput1_3.dll", "xinput9_1_0.dll" })
            {
                if (auto module = LoadLibraryA(name))
                    return reinterpret_cast<DWORD(WINAPI*)(DWORD, void*)>(GetProcAddress(module, "XInputGetState"));
            }
            return nullptr;
        }();

        if (pXInputGetState)
        {
            uint8_t state[16];
            for (DWORD i = 0; i < 4; ++i)
            {
                if (pXInputGetState(i, state) == ERROR_SUCCESS)
                    return IconStyle::Xbox;
            }
        }

        UINT count = 0;
        if (GetRawInputDeviceList(nullptr, &count, sizeof(RAWINPUTDEVICELIST)) == 0 && count > 0)
        {
            std::vector<RAWINPUTDEVICELIST> devices(count);
            if (GetRawInputDeviceList(devices.data(), &count, sizeof(RAWINPUTDEVICELIST)) != UINT(-1))
            {
                for (const auto& device : devices)
                {
                    if (device.dwType != RIM_TYPEHID)
                        continue;

                    RID_DEVICE_INFO info = { sizeof(RID_DEVICE_INFO) };
                    UINT size = sizeof(info);
                    if (GetRawInputDeviceInfoA(device.hDevice, RIDI_DEVICEINFO, &info, &size) != UINT(-1) &&
                        info.hid.dwVendorId == 0x054C && info.hid.usUsagePage == 0x01 && (info.hid.usUsage == 0x04 || info.hid.usUsage == 0x05))
                        return IconStyle::PlayStation;
                }
            }
        }

        return IconStyle::Xbox;
    }
}

// Which device was used last, for the front end icons
namespace
{
    using SteadyClock = std::chrono::steady_clock;
    SteadyClock::time_point LastPadInput = {};
    SteadyClock::time_point LastKeyboardMouseInput = {};

    bool LastInputWasPad()
    {
        return LastPadInput > LastKeyboardMouseInput;
    }
}

export namespace Input
{
    // The icon style setting changed
    void RefreshIconStyle()
    {
        bIconStyleOutdated = true;
    }

    struct LookInput
    {
        float stickX = 0.0f;    // -1..1, right is positive
        float stickY = 0.0f;    // -1..1, down is positive
        int mouseX = 0;
        int mouseY = 0;
    };

    LookInput Look;

    // Called once per camera update; mouse deltas accumulate between calls
    LookInput ConsumeLook()
    {
        LookInput result = Look;
        Look.mouseX = 0;
        Look.mouseY = 0;
        return result;
    }

    bool UsingGamepad()
    {
        return LastInputWasPad();
    }
}

// Modern default bindings for Xidi pads
namespace
{
    void Bind(int action, Binding primary, Binding secondary = {})
    {
        Actions[action].joystick[0] = primary;
        Actions[action].joystick[1] = secondary;
    }

    constexpr Binding Button(int button) { return { BindButton, 0, button }; }
    constexpr Binding Pov(int direction) { return { BindPov, 0, direction }; }
    constexpr Binding AxisLow(int axis) { return { BindAxisLow, 0, axis }; }
    constexpr Binding AxisHigh(int axis) { return { BindAxisHigh, 0, axis }; }
    constexpr Binding RotationHigh(int axis) { return { BindRotationHigh, 0, axis }; }

    constexpr int PovUp = 0, PovRight = 1, PovDown = 2, PovLeft = 3;
    constexpr int AxisX = 0, AxisY = 1, AxisZ = 2;

    // A member function (this in ecx, no stack arguments): ecx has to reach the game's function
    SafetyHookInline shSetJoystickDefaults = {};
    void __fastcall SetJoystickDefaults(void* self, void*)
    {
        shSetJoystickDefaults.unsafe_thiscall(self);
        if (!Actions || !IsXidiJoystick())
            return;

        // Race and free roam
        Bind(ActionGas, RotationHigh(AxisZ));                   // RT
        Bind(ActionBrake, AxisHigh(AxisZ));                     // LT
        Bind(ActionSteerLeft, AxisLow(AxisX), Pov(PovLeft));
        Bind(ActionSteerRight, AxisHigh(AxisX), Pov(PovRight));
        Bind(ActionHandbrake, Button(PadA));
        Bind(ActionNitrous, Button(PadB));
        Bind(ActionSpeedbreaker, Button(PadX));
        Bind(ActionCameraChange, Button(PadY));
        Bind(ActionShiftDown, Button(PadLB));
        Bind(ActionShiftUp, Button(PadRB));
        Bind(ActionReset, Button(PadBack));
        Bind(ActionPause, Button(PadStart));
        Bind(ActionLookBack, Button(PadLS));
        Bind(ActionPullback, Button(PadRS));
        Bind(ActionEngageEvent, Pov(PovUp));
        Bind(ActionHudDown, Pov(PovDown));
        Bind(ActionHudLeft, {});
        Bind(ActionHudRight, {});
        Bind(ActionSkipNIS, Button(PadA), Button(PadStart));

        // Front end
        Bind(ActionFELeft, Pov(PovLeft), AxisLow(AxisX));
        Bind(ActionFERight, Pov(PovRight), AxisHigh(AxisX));
        Bind(ActionFEUp, Pov(PovUp), AxisLow(AxisY));
        Bind(ActionFEDown, Pov(PovDown), AxisHigh(AxisY));
        Bind(ActionFEAccept, Button(PadA));
        Bind(ActionFECancel, Button(PadB));
        Bind(ActionFEStart, Button(PadStart));
        Bind(ActionFEPrevious, Button(PadLB));
        Bind(ActionFENext, Button(PadRB));
        Bind(ActionFELeftTrigger, AxisHigh(AxisZ));
        Bind(ActionFERightTrigger, RotationHigh(AxisZ));
        Bind(ActionFEKey1, Button(PadY));
        Bind(ActionFEKey2, Button(PadX));
        Bind(ActionFEKey3, Button(PadBack));
        Bind(ActionFEKey4, Button(PadStart));
    }
}

// Controls menu labels for Xidi pads
namespace
{
    struct PadNames
    {
        const char* buttons[PadButtonCount];
        const char* leftTrigger;
        const char* rightTrigger;
    };

    constexpr PadNames XboxNames = { { "A", "B", "X", "Y", "LB", "RB", "View", "Menu", "LS", "RS" }, "LT", "RT" };
    constexpr PadNames PlayStationNames = { { "Cross", "Circle", "Square", "Triangle", "L1", "R1", "Share", "Options", "L3", "R3" }, "L2", "R2" };

    const PadNames& CurrentNames()
    {
        return CurrentIconStyle == IconStyle::PlayStation ? PlayStationNames : XboxNames;
    }

    const char* BindingName(const Binding& binding)
    {
        const auto& names = CurrentNames();
        static constexpr const char* PovNames[] = { "D-pad Up", "D-pad Right", "D-pad Down", "D-pad Left" };
        static constexpr const char* LeftStick[2][2] = { { "Left Stick Left", "Left Stick Right" }, { "Left Stick Up", "Left Stick Down" } };
        static constexpr const char* RightStick[2][2] = { { "Right Stick Left", "Right Stick Right" }, { "Right Stick Up", "Right Stick Down" } };

        bool high = binding.type == BindAxisHigh || binding.type == BindRotationHigh;
        switch (binding.type)
        {
            case BindButton:
                return (binding.index >= 0 && binding.index < PadButtonCount) ? names.buttons[binding.index] : nullptr;
            case BindPov:
                return (binding.index >= 0 && binding.index < 4) ? PovNames[binding.index] : nullptr;
            case BindAxisLow:
            case BindAxisHigh:
                if (binding.index == AxisZ)
                    return high ? names.leftTrigger : nullptr;
                return (binding.index == AxisX || binding.index == AxisY) ? LeftStick[binding.index][high] : nullptr;
            case BindRotationLow:
            case BindRotationHigh:
                if (binding.index == AxisZ)
                    return high ? names.rightTrigger : nullptr;
                return (binding.index == AxisX || binding.index == AxisY) ? RightStick[binding.index][high] : nullptr;
        }
        return nullptr;
    }

    SafetyHookInline shGetBindingName = {};
    void* __cdecl GetBindingName(char* text, size_t size, int deviceType, Binding* binding)
    {
        constexpr int DeviceJoystick = 3;
        if (deviceType == DeviceJoystick && binding && IsXidiJoystick())
        {
            if (auto name = BindingName(*binding))
            {
                strncpy_s(text, size, name, _TRUNCATE);
                return text;
            }
        }

        return shGetBindingName.unsafe_ccall<void*>(text, size, deviceType, binding);
    }
}

// Front end icons: the PC key images are swapped for controller buttons while a pad is used
namespace
{
    struct HotkeyTextures
    {
        const char* pc;
        const char* xbox;
        const char* playStation;
    };

    constexpr HotkeyTextures HotkeyImages[] =
    {
        { "L1",                      "XBOX_L1",                   "PS_L1"                   },
        { "R1",                      "XBOX_R1",                   "PS_R1"                   },
        { "PC_KEY_Q",                "XBOX_KEY_Q",                "PS_KEY_Q"                },
        { "PC_GREATERTHAN_LESSTHAN", "XBOX_GREATERTHAN_LESSTHAN", "PS_GREATERTHAN_LESSTHAN" },
        { "PC_KEY_ENTER",            "XBOX_KEY_ENTER",            "PS_KEY_ENTER"            },
        { "PC_KEY_ESC_FRENCH",       "XBOX_KEY_ESC_FRENCH",       "PS_KEY_ESC_FRENCH"       },
        { "PC_KEY_ESC",              "XBOX_KEY_ESC",              "PS_KEY_ESC"              },
        { "PC_KEY_1",                "XBOX_KEY_1",                "PS_KEY_1"                },
        { "PC_KEY_2",                "XBOX_KEY_2",                "PS_KEY_2"                },
        { "PC_KEY_3",                "XBOX_KEY_3",                "PS_KEY_3"                },
        { "PC_KEY_4",                "XBOX_KEY_4",                "PS_KEY_4"                },
        { "PC_ARROW_KEYS_UP_DOWN",   "XBOX_ARROW_KEYS_UP_DOWN",   "PS_ARROW_KEYS_UP_DOWN"   },
        { "PC_LOAD",                 "XBOX_LOAD",                 "PS_LOAD"                 },
        { "PC_ARROW_KEYS",           "XBOX_ARROW_KEYS",           "PS_ARROW_KEYS"           },
    };

    struct HotkeyHashes
    {
        uint32_t pc, xbox, playStation;
    };

    std::vector<HotkeyHashes> HotkeyImageHashes;

    bool (__cdecl* LoadChunks)(void* data, int size) = nullptr;

    void LoadButtonTextures()
    {
        static bool bLoaded = false;
        if (bLoaded || !LoadChunks)
            return;
        bLoaded = true;

        auto textures = TexturePack::LoadResourceTextures();
        if (textures.empty())
            return;

        // The texture pack keeps pointing into this memory, so it's never freed
        auto pack = TexturePack::Build(textures, "WFP_NFSMW_BUTTONS");
        auto buffer = static_cast<uint8_t*>(_aligned_malloc(pack.size(), 0x80));
        if (!buffer)
            return;
        memcpy(buffer, pack.data(), pack.size());
        LoadChunks(buffer, int(pack.size()));
    }

    char __stdcall UpdateHotkeyImage(FEObject* object)
    {
        if (object->Type != FE_Image)
            return 1;

        for (const auto& hashes : HotkeyImageHashes)
        {
            if (object->Handle != hashes.pc && object->Handle != hashes.xbox && object->Handle != hashes.playStation)
                continue;

            uint32_t texture = !LastInputWasPad() ? hashes.pc : (CurrentIconStyle == IconStyle::PlayStation ? hashes.playStation : hashes.xbox);
            if (object->Handle != texture)
            {
                object->Handle = texture;
                object->Flags |= 0x2400000;
            }
            break;
        }

        return 1;
    }

    ProtectedGameRef<uint32_t> TitlePrompt;
    uint32_t TitlePromptOriginal = 0;

    SafetyHookInline shFEngService = {};
    void __fastcall FEngService(void* cFEng, void* edx, unsigned int deltaTicks, unsigned int lock)
    {
        LoadButtonTextures();

        if (bIconStyleOutdated.exchange(false))
            CurrentIconStyle = DetectIconStyle();

        if (TitlePromptOriginal)
        {
            constexpr uint32_t PressStartXbox = 0xD18D4C4C;
            constexpr uint32_t PressStartPlayStation = 0xDC64C04C;
            uint32_t prompt = !LastInputWasPad() ? TitlePromptOriginal : (CurrentIconStyle == IconStyle::PlayStation ? PressStartPlayStation : PressStartXbox);
            if (TitlePrompt != prompt)
                TitlePrompt = prompt;
        }

        struct FEObjectCallback
        {
            void* destructor = nullptr;
            void* function = nullptr;
        };

        FEObjectCallback callback = { nullptr, reinterpret_cast<void*>(&UpdateHotkeyImage) };
        void* callbackPointer = &callback;

        uintptr_t package = *reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(cFEng) + 0xE0);
        while (package)
        {
            FEPackage::ForAllObjects(reinterpret_cast<void*>(package), nullptr, &callbackPointer);
            uintptr_t next = *reinterpret_cast<uintptr_t*>(package + 4);
            if (next == package)
                break;
            package = next;
        }

        shFEngService.unsafe_fastcall(cFEng, edx, deltaTicks, lock);
    }
}

// Per-frame processing after the game read its actions
namespace
{
    // FE message the game sends when Q is pressed in the main menu
    void* QueueMessage = nullptr;
    void** FEngInstance = nullptr;
    uint32_t QuitMessage = 0;

    struct PadState
    {
        bool quitCombo = false;
    } Pad;

    SafetyHookInline shPollActions = {};
    void __fastcall PollActions(uintptr_t poll, void* edx)
    {
        shPollActions.unsafe_fastcall(poll, edx);

        // LS + RS quits the game from the main menu, like Q
        if (Pad.quitCombo && IsFrontEnd() && QueueMessage && FEngInstance && *FEngInstance)
        {
            Pad.quitCombo = false;
            reinterpret_cast<void(__thiscall*)(void*, uint32_t, const char*, uint32_t)>(QueueMessage)(*FEngInstance, QuitMessage, nullptr, 0xFF);
        }
    }
}

class InputModule
{
public:
    InputModule()
    {
        WFP::onInitEvent() += []()
        {
            pGamepadIcons = &FixSettings().Int("MAIN.GamepadIcons");
            FixSettings().OnChange("MAIN.GamepadIcons", [] { Input::RefreshIconStyle(); });
            CurrentIconStyle = DetectIconStyle();

            for (const auto& images : HotkeyImages)
                HotkeyImageHashes.push_back({ bStringHash(images.pc), bStringHash(images.xbox), bStringHash(images.playStation) });

            // Binding table, read by the action polling function
            auto pattern = hook::pattern("51 8B 44 24 08 D9 05 ? ? ? ? 8B 4C 24 0C 6B C0 34 D9 14 24 05");
            if (!pattern.empty())
                Actions = *pattern.get_first<ActionEntry*>(22);

            // Joystick defaults, applied for a new device and by "Reset to defaults"
            pattern = hook::pattern("A1 ? ? ? ? 81 EC 40 02 00 00 53 55 56 33 F6 3B C6 57 8B F9");
            if (!pattern.empty() && Actions)
            {
                Joysticks = *pattern.get_first<uintptr_t*>(1);
                shSetJoystickDefaults = safetyhook::create_inline(pattern.get_first(), SetJoystickDefaults);
            }

            pattern = hook::pattern("55 8B EC 83 E4 F8 81 EC 4C 03 00 00 8B 45 0C 8B 4D 08 53 56 57 50 68");
            if (!pattern.empty() && Joysticks)
                shGetBindingName = safetyhook::create_inline(pattern.get_first(), GetBindingName);

            // Q in the main menu
            pattern = hook::pattern("83 FE 71 74 05 83 FE 51 75 17 8B 0D ? ? ? ? 68 FF 00 00 00 6A 00 68 ? ? ? ? E8");
            if (!pattern.empty())
            {
                FEngInstance = *pattern.get_first<void**>(12);
                QuitMessage = *pattern.get_first<uint32_t>(24);
                QueueMessage = injector::GetBranchDestination(pattern.get_first(28)).get();
            }

            pattern = hook::pattern("83 EC 0C 55 56 8B F1 8B 46 24 33 ED 3B C5 89 74 24 0C 0F 85");
            if (!pattern.empty())
                shPollActions = safetyhook::create_inline(pattern.get_first(), PollActions);

            // Joystick state, read by the game's input library: esi = device, DIJOYSTATE2 at ebp-0x13C
            pattern = hook::pattern("8B 85 ? ? ? ? 3B 86");
            static auto JoyStateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                auto device = *reinterpret_cast<IDirectInputDevice8A**>(regs.esi + 0x468);
                auto state = reinterpret_cast<DIJOYSTATE2*>(regs.ebp - 0x13C);
                if (!IsXidiDevice(device))
                    return;

                static const float& leftDeadzone = FixSettings().Float("GAMEPAD.LeftStickDeadzone");
                static const float& rightDeadzone = FixSettings().Float("GAMEPAD.RightStickDeadzone");
                WFPStickDeadzone::Apply(state->lX, state->lY, leftDeadzone);
                WFPStickDeadzone::Apply(state->lRx, state->lRy, rightDeadzone);

                auto axis = [](LONG value) -> float
                {
                    return std::clamp((float(value) - 32767.5f) / 32767.5f, -1.0f, 1.0f);
                };

                Input::Look.stickX = axis(state->lRx);
                Input::Look.stickY = axis(state->lRy);

                bool active = std::fabs(axis(state->lX)) > 0.5f || std::fabs(axis(state->lY)) > 0.5f ||
                    std::fabs(Input::Look.stickX) > 0.5f || std::fabs(Input::Look.stickY) > 0.5f ||
                    axis(state->lZ) > 0.5f || axis(state->lRz) > 0.5f || state->rgdwPOV[0] != 0xFFFFFFFF;
                for (int i = 0; i < PadButtonCount && !active; ++i)
                    active = (state->rgbButtons[i] & 0x80) != 0;
                if (active)
                    LastPadInput = SteadyClock::now();

                static bool bWasCombo = false;
                bool combo = (state->rgbButtons[PadLS] & 0x80) && (state->rgbButtons[PadRS] & 0x80);
                if (combo && !bWasCombo)
                    Pad.quitCombo = true;
                bWasCombo = combo;
            });

            // Mouse: deltas at ebp-0x14
            pattern = hook::pattern("8B 45 ? 3B C7 75 ? 3B D7");
            static auto MouseStateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (hWnd != GetForegroundWindow())
                    return;

                struct MouseState { int dx, dy, wheel; };
                auto state = reinterpret_cast<MouseState*>(regs.ebp - 0x14);
                Input::Look.mouseX += state->dx;
                Input::Look.mouseY += state->dy;
                if (state->dx || state->dy || state->wheel)
                    LastKeyboardMouseInput = SteadyClock::now();
            });

            // Keyboard: key states at ebp-0x130
            pattern = hook::pattern("89 45 ? 0F 84 ? ? ? ? 8D 4D ? E8");
            static auto KeyboardStateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                auto keys = reinterpret_cast<const uint8_t*>(regs.ebp - 0x130);
                if (std::any_of(keys, keys + 256, [](uint8_t key) { return (key & 0x80) != 0; }))
                    LastKeyboardMouseInput = SteadyClock::now();
            });

            onWindowMessage() += [](HWND, UINT msg, WPARAM wParam, LPARAM)
            {
                if (msg == WM_DEVICECHANGE && (wParam == DBT_DEVNODES_CHANGED || wParam == DBT_DEVICEARRIVAL || wParam == DBT_DEVICEREMOVECOMPLETE))
                    bIconStyleOutdated = true;
            };

            // Button textures and the front end icons
            pattern = hook::pattern("8B 44 24 08 56 8B 74 24 08 57 8D 3C 06 3B F7 73 ? E8");
            if (!pattern.empty())
                LoadChunks = reinterpret_cast<decltype(LoadChunks)>(pattern.get_first());

            pattern = hook::pattern("E8 ? ? ? ? A1 ? ? ? ? ? ? 85 C0 74");
            shFEngService = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first(0)).as_int(), FEngService);

            // "Press START to begin" on the title screen while a pad is used: the prompt's text hash is
            // pushed for the title screen setup
            pattern = hook::pattern("68 ? ? ? ? 68 ? ? ? ? 51 E8 ? ? ? ? 83 C4 ? 3B FB");
            if (!pattern.empty())
            {
                TitlePrompt.SetAddress(pattern.get_first<uint32_t>(1));
                TitlePromptOriginal = TitlePrompt;
            }
        };
    }
} InputModule;

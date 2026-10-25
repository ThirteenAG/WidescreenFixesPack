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

    // Every action has a list of slots: two keyboard slots, then two for each joystick
    struct ActionSlots
    {
        Binding* slots;
        int32_t count;
    };

    ActionSlots* Actions = nullptr;
    constexpr int DefaultedActions = 47;    // the joystick defaults cover the actions up to FE_ACTION_B5

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
        ActionReset = 18,
        ActionCrew = 19,
        ActionEngageEvent = 20,
        ActionHudLeft = 21,
        ActionHudRight = 22,
        ActionCameraChange = 24,
        ActionLookBack = 25,
        ActionPullback = 26,
        ActionPause = 28,
        ActionFELeft = 29,
        ActionFERight = 30,
        ActionFEUp = 31,
        ActionFEDown = 32,
        ActionFERotateLeft = 33,
        ActionFERotateRight = 34,
        ActionFERotateUp = 35,
        ActionFERotateDown = 36,
        ActionFEAccept = 37,
        ActionFECancel = 38,
        ActionFEStart = 39,
        ActionFEPrevious = 40,      // LB, the "9" key
        ActionFENext = 41,          // RB, the "0" key
        ActionFEKey3 = 43,          // "3"
        ActionFEKey2 = 45,          // "2"
        ActionFEKey1 = 46,          // "1"
        ActionSkipNIS = 47,
        ActionFERightTrigger = 53,
        ActionFELeftTrigger = 54,
    };

    constexpr int ActionCount = 72;

    // Joystick state of the game's input library: lX, lY, lZ, then buttons as 0/128
    constexpr int MaxButtons = 128;

    enum PadButton { PadA, PadB, PadX, PadY, PadLB, PadRB, PadBack, PadStart, PadLS, PadRS, PadButtonCount };

    // Joystick devices of the game. The DirectInput device is at +0x218.
    constexpr int MaxJoysticks = 4;
    uintptr_t* Joysticks = nullptr;

    enum class IconStyle { Xbox, PlayStation };
    const int* pGamepadIcons = nullptr;   // 0 = auto, 1 = Xbox, 2 = PlayStation; also changed in the options menu
    std::atomic<IconStyle> CurrentIconStyle = IconStyle::Xbox;
    std::atomic<bool> bIconStyleOutdated = false;

    // The device states are read by the game's input library, which may run on a thread of its own, while the
    // bindings and the camera are handled on the game's thread: what's shared is guarded or atomic.
    bool IsXidiDevice(IDirectInputDevice8A* device)
    {
        // Xidi virtual controller instance GUIDs end with "XIDIVC"
        static std::map<IDirectInputDevice8A*, bool> cache;
        static std::mutex mutex;
        if (!device)
            return false;

        std::lock_guard lock(mutex);
        if (auto it = cache.find(device); it != cache.end())
            return it->second;

        DIDEVICEINSTANCEA info = { sizeof(DIDEVICEINSTANCEA) };
        bool isXidi = SUCCEEDED(device->GetDeviceInfo(&info)) && memcmp(info.guidInstance.Data4, "XIDIVC", 6) == 0;
        cache[device] = isXidi;
        return isXidi;
    }

    IDirectInputDevice8A* GetJoystick(int index)
    {
        if (!Joysticks || index < 0 || index >= MaxJoysticks)
            return nullptr;
        auto joystick = Joysticks[index];
        return (joystick && joystick != uintptr_t(-4)) ? *reinterpret_cast<IDirectInputDevice8A**>(joystick + 0x218) : nullptr;
    }

    bool IsXidiJoystick(int index)
    {
        return IsXidiDevice(GetJoystick(index));
    }

    bool AnyXidiJoystick()
    {
        for (int i = 0; i < MaxJoysticks; ++i)
        {
            if (IsXidiJoystick(i))
                return true;
        }
        return false;
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
    std::atomic<SteadyClock::rep> LastPadInput = 0;
    std::atomic<SteadyClock::rep> LastKeyboardMouseInput = 0;

    void MarkInput(std::atomic<SteadyClock::rep>& time)
    {
        time = SteadyClock::now().time_since_epoch().count();
    }

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

    struct SharedLookInput
    {
        std::atomic<float> stickX = 0.0f;
        std::atomic<float> stickY = 0.0f;
        std::atomic<int> mouseX = 0;
        std::atomic<int> mouseY = 0;
    } Look;

    // Called once per camera update; mouse deltas accumulate between calls
    LookInput ConsumeLook()
    {
        LookInput result;
        result.stickX = Look.stickX;
        result.stickY = Look.stickY;
        result.mouseX = Look.mouseX.exchange(0);
        result.mouseY = Look.mouseY.exchange(0);
        return result;
    }

    bool UsingGamepad()
    {
        return LastInputWasPad();
    }

    // While the title screen is shown, "accept" is pressed twice a second, so that it continues by itself.
    // It only accepts once its prompt has appeared.
    std::atomic<bool> bSkipTitleScreen = false;
    std::atomic<bool> AutoAccept = false;

    void SetSkipTitleScreen(bool enable)
    {
        bSkipTitleScreen = enable;
    }

    void OnSplashScreen(bool shown)
    {
        AutoAccept = shown && bSkipTitleScreen;
    }
}

// Modern default bindings for Xidi pads
namespace
{
    int DefaultsDevice = 0;

    void Bind(int action, Binding primary, Binding secondary = {})
    {
        auto& entry = Actions[action];
        int slot = 2 + 2 * DefaultsDevice;
        if (!entry.slots || slot + 1 >= entry.count)
            return;

        primary.device = primary.type ? DefaultsDevice : 0;
        secondary.device = secondary.type ? DefaultsDevice : 0;
        entry.slots[slot] = primary;
        entry.slots[slot + 1] = secondary;
    }

    constexpr Binding Button(int button) { return { BindButton, 0, button }; }
    constexpr Binding Pov(int direction) { return { BindPov, 0, direction }; }
    constexpr Binding AxisLow(int axis) { return { BindAxisLow, 0, axis }; }
    constexpr Binding AxisHigh(int axis) { return { BindAxisHigh, 0, axis }; }
    constexpr Binding RotationLow(int axis) { return { BindRotationLow, 0, axis }; }
    constexpr Binding RotationHigh(int axis) { return { BindRotationHigh, 0, axis }; }

    constexpr int PovUp = 0, PovRight = 1, PovDown = 2, PovLeft = 3;
    constexpr int AxisX = 0, AxisY = 1, AxisZ = 2;

    SafetyHookInline shSetJoystickDefaults = {};
    void __stdcall SetJoystickDefaults(int device)
    {
        shSetJoystickDefaults.unsafe_stdcall(device);
        if (!Actions || !IsXidiJoystick(device))
            return;

        DefaultsDevice = device;

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
        Bind(ActionCrew, Button(PadRS));
        Bind(ActionPullback, {});
        Bind(ActionEngageEvent, Pov(PovUp));
        Bind(ActionHudLeft, {});
        Bind(ActionHudRight, {});
        Bind(ActionSkipNIS, Button(PadA), Button(PadStart));

        // Front end
        Bind(ActionFELeft, Pov(PovLeft), AxisLow(AxisX));
        Bind(ActionFERight, Pov(PovRight), AxisHigh(AxisX));
        Bind(ActionFEUp, Pov(PovUp), AxisLow(AxisY));
        Bind(ActionFEDown, Pov(PovDown), AxisHigh(AxisY));
        Bind(ActionFERotateLeft, RotationLow(AxisX));
        Bind(ActionFERotateRight, RotationHigh(AxisX));
        Bind(ActionFERotateUp, RotationLow(AxisY));
        Bind(ActionFERotateDown, RotationHigh(AxisY));
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
    void __cdecl GetBindingName(char* text, size_t size, int deviceType, Binding* binding)
    {
        constexpr int DeviceKeyboard = 1;
        if (deviceType != DeviceKeyboard && binding && binding->type != BindKey && IsXidiJoystick(binding->device))
        {
            if (auto name = BindingName(*binding))
            {
                strncpy_s(text, size, name, _TRUNCATE);
                return;
            }
        }

        shGetBindingName.unsafe_ccall(text, size, deviceType, binding);
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
        { "PC_KEY_ENTER",       "XBOX_KEY_ENTER",       "PS_KEY_ENTER"       },
        { "PC_KEY_ESC",         "XBOX_KEY_ESC",         "PS_KEY_ESC"         },
        { "PC_KEY_ESC_FRENCH",  "XBOX_KEY_ESC_FRENCH",  "PS_KEY_ESC_FRENCH"  },
        { "PC_KEY_Q",           "XBOX_KEY_Q",           "PS_KEY_Q"           },
        { "L1",                 "XBOX_L1",              "PS_L1"              },
        { "R1",                 "XBOX_R1",              "PS_R1"              },
        { "PC_KEY_9",           "XBOX_KEY_9",           "PS_KEY_9"           },
        { "PC_KEY_0",           "XBOX_KEY_0",           "PS_KEY_0"           },
        { "PC_KEY_1",           "XBOX_KEY_1",           "PS_KEY_1"           },
        { "PC_KEY_2",           "XBOX_KEY_2",           "PS_KEY_2"           },
        { "PC_KEY_3",           "XBOX_KEY_3",           "PS_KEY_3"           },
        { "PC_KEY_SPC",         "XBOX_KEY_SPC",         "PS_KEY_SPC"         },
        { "PC_KEY_SPC_CZECH",   "XBOX_KEY_SPC_CZECH",   "PS_KEY_SPC_CZECH"   },
        { "PC_KEY_SPC_DANISH",  "XBOX_KEY_SPC_DANISH",  "PS_KEY_SPC_DANISH"  },
        { "PC_KEY_SPC_DUTCH",   "XBOX_KEY_SPC_DUTCH",   "PS_KEY_SPC_DUTCH"   },
        { "PC_KEY_SPC_FRENCH",  "XBOX_KEY_SPC_FRENCH",  "PS_KEY_SPC_FRENCH"  },
        { "PC_KEY_SPC_GERMAN",  "XBOX_KEY_SPC_GERMAN",  "PS_KEY_SPC_GERMAN"  },
        { "PC_KEY_SPC_ITALIAN", "XBOX_KEY_SPC_ITALIAN", "PS_KEY_SPC_ITALIAN" },
        { "PC_KEY_SPC_SPANISH", "XBOX_KEY_SPC_SPANISH", "PS_KEY_SPC_SPANISH" },
        { "PC_X",               "XBOX_X",               "PS_X"               },
        { "PC_CLICKDRAG_ICON",  "XBOX_CLICKDRAG_ICON",  "PS_CLICKDRAG_ICON"  },
        { "PC_LOAD",            "XBOX_LOAD",            "PS_LOAD"            },
    };

    struct HotkeyHashes
    {
        uint32_t pc, xbox, playStation;
    };

    std::vector<HotkeyHashes> HotkeyImageHashes;

    int (__cdecl* LoadChunks)(void* data, int size, bool stopAtUnknown) = nullptr;

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
        auto pack = TexturePack::Build(textures, "WFP_NFSC_BUTTONS");
        auto buffer = static_cast<uint8_t*>(_aligned_malloc(pack.size(), 0x80));
        if (!buffer)
            return;
        memcpy(buffer, pack.data(), pack.size());
        LoadChunks(buffer, int(pack.size()), false);
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
            constexpr uint32_t PressStartXbox = 0x186AAECC;
            constexpr uint32_t PressStartPlayStation = 0x703A92CC;
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
    // FE message the main menu quits on, like Q
    void* QueueMessage = nullptr;
    void** FEngInstance = nullptr;
    constexpr uint32_t QuitMessage = 0x62799A4C;

    struct PadState
    {
        std::atomic<bool> quitCombo = false;
    } Pad;

    SafetyHookInline shPollActions = {};
    void __fastcall PollActions(uintptr_t poll, void* edx)
    {
        shPollActions.unsafe_fastcall(poll, edx);

        // Pulses of a few polls, twice a second
        static SteadyClock::time_point lastAccept = {};
        static int acceptPolls = 0;
        auto now = SteadyClock::now();
        if (!Input::AutoAccept)
            acceptPolls = 0;
        else if (acceptPolls == 0 && now - lastAccept > std::chrono::milliseconds(500))
        {
            lastAccept = now;
            acceptPolls = 4;
        }
        if (acceptPolls > 0)
        {
            --acceptPolls;
            if (auto values = *reinterpret_cast<float**>(poll + 0x20))
                values[ActionFEAccept] = 1.0f;
        }


        // LS + RS quits the game from the main menu, like Q. A press outside the front end is dropped, so it
        // doesn't quit later when the front end comes back.
        if (Pad.quitCombo.exchange(false) && IsFrontEnd() && QueueMessage && FEngInstance && *FEngInstance)
            reinterpret_cast<void(__thiscall*)(void*, uint32_t, const char*, uint32_t)>(QueueMessage)(*FEngInstance, QuitMessage, nullptr, 0xFF);
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

            // Binding table, read by the action value function
            auto pattern = hook::pattern("51 8B 4C 24 0C D9 05 ? ? ? ? 53 D9 54 24 04 56 57 8B 7C 24 14 8B C1 99 8D 34 49 8B 0C FD");
            if (!pattern.empty())
                Actions = *pattern.get_first<ActionSlots*>(31);

            // Joystick defaults for a device, applied for a new device and by "Reset to defaults"
            pattern = hook::pattern("81 EC 68 04 00 00 53 55 56 57 8B BC 24 7C 04 00 00 8B 04 BD");
            if (!pattern.empty() && Actions)
            {
                Joysticks = *pattern.get_first<uintptr_t*>(20);
                shSetJoystickDefaults = safetyhook::create_inline(pattern.get_first(), SetJoystickDefaults);
            }

            pattern = hook::pattern("55 8B EC 83 E4 F8 81 EC 0C 02 00 00 53 8B 5D 0C 56 8B 75 08 57 53 68");
            if (!pattern.empty() && Joysticks)
                shGetBindingName = safetyhook::create_inline(pattern.get_first(), GetBindingName);

            // cFEng::QueueGameMessage
            pattern = hook::pattern("8B 44 24 0C 8B 54 24 08 50 8B 44 24 08 6A FF 52 50 E8 ? ? ? ? C2 0C 00");
            auto instance = hook::pattern("8B 0D ? ? ? ? E8 ? ? ? ? 8B 0D ? ? ? ? 3B CF 5F");
            if (!pattern.empty() && !instance.empty())
            {
                QueueMessage = pattern.get_first();
                FEngInstance = *instance.get_first<void**>(2);
            }

            pattern = hook::pattern("83 EC 0C 55 57 8B F9 8B 47 24 33 ED 3B C5 89 7C 24 0C 0F 85");
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

                float lookX = axis(state->lRx);
                float lookY = axis(state->lRy);
                Input::Look.stickX = lookX;
                Input::Look.stickY = lookY;

                bool active = std::fabs(axis(state->lX)) > 0.5f || std::fabs(axis(state->lY)) > 0.5f ||
                    std::fabs(lookX) > 0.5f || std::fabs(lookY) > 0.5f ||
                    axis(state->lZ) > 0.5f || axis(state->lRz) > 0.5f || LOWORD(state->rgdwPOV[0]) != 0xFFFF;
                for (int i = 0; i < PadButtonCount && !active; ++i)
                    active = (state->rgbButtons[i] & 0x80) != 0;
                if (active)
                    MarkInput(LastPadInput);

                static bool bWasCombo = false;
                bool combo = (state->rgbButtons[PadLS] & 0x80) && (state->rgbButtons[PadRS] & 0x80);
                if (combo && !bWasCombo)
                    Pad.quitCombo = true;
                bWasCombo = combo;
            });

            // Mouse: deltas at ebp-0x14
            pattern = hook::pattern("8B 45 ? 3B C7 75");
            static auto MouseStateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (hWnd != GetForegroundWindow())
                    return;

                struct MouseState { int dx, dy, wheel; };
                auto state = reinterpret_cast<MouseState*>(regs.ebp - 0x14);
                Input::Look.mouseX += state->dx;
                Input::Look.mouseY += state->dy;
                if (state->dx || state->dy || state->wheel)
                    MarkInput(LastKeyboardMouseInput);
            });

            // Keyboard: key states at ebp-0x130
            pattern = hook::pattern("8B 83 ? ? ? ? 8B 80");
            static auto KeyboardStateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                auto keys = reinterpret_cast<const uint8_t*>(regs.ebp - 0x130);
                if (std::any_of(keys, keys + 256, [](uint8_t key) { return (key & 0x80) != 0; }))
                    MarkInput(LastKeyboardMouseInput);
            });

            onWindowMessage() += [](HWND, UINT msg, WPARAM wParam, LPARAM)
            {
                if (msg == WM_DEVICECHANGE && (wParam == DBT_DEVNODES_CHANGED || wParam == DBT_DEVICEARRIVAL || wParam == DBT_DEVICEREMOVECOMPLETE))
                    bIconStyleOutdated = true;
            };

            // Button textures and the front end icons
            pattern = hook::pattern("8B 44 24 08 53 55 56 8B 74 24 10 57 8D 3C 06 3B F7 73 ? 8A 5C 24 1C");
            if (!pattern.empty())
                LoadChunks = reinterpret_cast<decltype(LoadChunks)>(pattern.get_first());

            pattern = hook::pattern("E8 ? ? ? ? 57 8B CE E8 ? ? ? ? 8B F0");
            shFEngService = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first(0)).as_int(), FEngService);

            // "Press START to begin" on the title screen while a pad is used: the prompt's text hash is
            // pushed for the title screen setup
            pattern = hook::pattern("68 ? ? ? ? 68 ? ? ? ? 51 E8 ? ? ? ? 83 C4 14 E8");
            if (!pattern.empty())
            {
                TitlePrompt.SetAddress(pattern.get_first<uint32_t>(1));
                TitlePromptOriginal = TitlePrompt;
            }
        };
    }
} InputModule;

module;

#include <stdafx.h>
#include "usercall.hpp"
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <dbt.h>
#include "TexturePack.hpp"
#include "FixSettings.hpp"
#include "../Shared/NFS/StickDeadzone.hpp"

export module Input;

import ComVars;

using namespace usercall;

// Gamepads are handled by Xidi (Xidi.32.dll, loaded by the ASI loader, with the SDL3 plugin for PlayStation,
// Switch and other non-XInput pads). Xidi presents each pad as a "Xidi virtual controller" using its
// XInputNative layout: X/Y = left stick, RotX/RotY = right stick, Z = LT, RotZ = RT, POV = d-pad,
// buttons 1-10 = A B X Y LB RB Back Start LS RS. This module gives such pads modern default bindings,
// proper names in the controls menu, button icons, front end actions for the PC-only hotkeys and
// the right stick for the camera. Other devices (wheels and so on) keep the game's own behaviour.

namespace
{
    // Game input action IDs. A channel's 64-bit action mask has a bit cleared for every pressed action.
    enum Action : uint32_t
    {
        ActionNone = 0x00,
        ActionThrottle = 0x01,
        ActionBrake = 0x02,
        ActionSteerLeft = 0x03,
        ActionSteerRight = 0x04,
        ActionHandbrake = 0x05,
        ActionNitrous = 0x06,
        ActionShiftUp = 0x07,
        ActionShiftDown = 0x08,
        ActionLookBack = 0x09,
        ActionCamera = 0x0A,
        ActionHUD = 0x0B,
        ActionPause = 0x0C,
        ActionRespot = 0x0E,
        ActionMenuUp = 0x10,
        ActionMenuDown = 0x11,
        ActionMenuLeft = 0x12,
        ActionMenuRight = 0x13,
        ActionMenuSelect = 0x14,
        ActionMenuSelectAlt = 0x15,
        ActionMenuBack = 0x17,
        ActionMenuCustomise = 0x18,     // C
        ActionMenuButton2 = 0x19,       // ,
        ActionMenuButton3 = 0x1A,       // .
        ActionMenuRotateRight = 0x20,   // D, also "delete" in the profile screens
        ActionMenuPreset = 0x23,        // P
        ActionMenuQuit = 0x24,          // Q
        ActionMenuHotkeyZ = 0x2C,       // Z, "default/stock" in the PC front end
        ActionMenuHotkeyT = 0x2F,       // T, tutorials
    };

    // Per-channel input mapping (624 bytes per channel)
    namespace Channel
    {
        constexpr size_t ActionMaskLow = 0x08;
        constexpr size_t ActionMaskHigh = 0x0C;
        constexpr size_t SteerByte = 0x12;
        constexpr size_t PedalsByte = 0x13;
        constexpr size_t ButtonGameAction = 0x18;   // [16]
        constexpr size_t PovGameAction = 0x58;      // [4][4]: down, up, left, right
        constexpr size_t ButtonMenuAction = 0x98;   // [16]
        constexpr size_t Index = 0x130;
        constexpr size_t DeviceType = 0x134;
        constexpr size_t ConfigMode = 0x23C;
        constexpr size_t AxisOffset = 0x244;        // [4]: steer, throttle, brake, combined pedals
        constexpr size_t AxisSign = 0x254;          // [4]
        constexpr size_t AxisEnabled = 0x264;       // [4] bytes
        constexpr size_t CombinedPedals = 0x268;

        constexpr int32_t KeyboardType = 0x13;

        template<typename T> T& At(uintptr_t channel, size_t offset) { return *reinterpret_cast<T*>(channel + offset); }
    }

    constexpr size_t JoyStateSize = 0x110;
    constexpr int MaxJoysticks = 2;

    // DIJOYSTATE2 offsets of the XInputNative mapper
    constexpr DWORD OfsLeftX = 0x00, OfsLeftY = 0x04, OfsLT = 0x08;
    constexpr DWORD OfsRightX = 0x0C, OfsRightY = 0x10, OfsRT = 0x14;
    constexpr DWORD OfsButton0 = 0x30;

    enum PadButton { PadA, PadB, PadX, PadY, PadLB, PadRB, PadBack, PadStart, PadLS, PadRS, PadButtonCount };

    IDirectInputDevice8A** Joysticks = nullptr;
    uint8_t** JoyStateCopy = nullptr;

    enum class IconStyle { Xbox, PlayStation };
    const int* pGamepadIcons = nullptr;   // 0 = auto, 1 = Xbox, 2 = PlayStation; also changed in the options menu
    std::atomic<IconStyle> CurrentIconStyle = IconStyle::Xbox;
    std::atomic<bool> bDevicesChanged = false;     // rescan for new game controllers
    std::atomic<bool> bIconStyleOutdated = false;  // detect the icon style again

    // Devices known to be Xidi's or not. The game releases and creates its devices again when controllers change,
    // and a new device can get the address of an old one, so this is forgotten then.
    std::map<IDirectInputDevice8A*, bool> XidiDeviceCache;

    bool IsXidiDevice(IDirectInputDevice8A* device)
    {
        // Xidi virtual controller instance GUIDs end with "XIDIVC"
        auto& cache = XidiDeviceCache;
        if (!device)
            return false;

        if (auto it = cache.find(device); it != cache.end())
            return it->second;

        DIDEVICEINSTANCEA info = { sizeof(DIDEVICEINSTANCEA) };
        bool isXidi = SUCCEEDED(device->GetDeviceInfo(&info)) && memcmp(info.guidInstance.Data4, "XIDIVC", 6) == 0;
        cache[device] = isXidi;
        return isXidi;
    }

    IDirectInputDevice8A* GetJoystick(int index)
    {
        return (index >= 0 && index < MaxJoysticks) ? Joysticks[index] : nullptr;
    }

    // The stick dead zone from the options, applied by Xidi to an axis of the left or right stick
    void SetStickDeadzone(IDirectInputDevice8A* device, DWORD object, DWORD how, DWORD offset)
    {
        DIPROPDWORD deadzone = {};
        deadzone.diph.dwSize = sizeof(DIPROPDWORD);
        deadzone.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        deadzone.diph.dwObj = object;
        deadzone.diph.dwHow = how;
        deadzone.dwData = WFPStickDeadzone::Property(FixSettings().Float(offset == OfsLeftX || offset == OfsLeftY ? "GAMEPAD.LeftStickDeadzone" : "GAMEPAD.RightStickDeadzone"));
        device->SetProperty(DIPROP_DEADZONE, &deadzone.diph);
    }

    DIJOYSTATE2* GetJoyState(int index)
    {
        return (JoyStateCopy && *JoyStateCopy) ? reinterpret_cast<DIJOYSTATE2*>(*JoyStateCopy + JoyStateSize * index) : nullptr;
    }

    void SetActionPressed(uintptr_t channel, uint32_t action, bool pressed)
    {
        auto& mask = Channel::At<uint32_t>(channel, action < 32 ? Channel::ActionMaskLow : Channel::ActionMaskHigh);
        uint32_t bit = 1u << (action % 32);
        mask = pressed ? (mask & ~bit) : (mask | bit);
    }

    bool IsActionPressed(uintptr_t channel, uint32_t action)
    {
        auto mask = Channel::At<uint32_t>(channel, action < 32 ? Channel::ActionMaskLow : Channel::ActionMaskHigh);
        return (mask & (1u << (action % 32))) == 0;
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

        // An XInput pad wins, otherwise a connected Sony gamepad selects PlayStation icons
        using XInputGetStateFn = DWORD(WINAPI*)(DWORD, void*);
        static auto XInputGetStateFunc = []() -> XInputGetStateFn
        {
            for (auto dll : { "xinput1_4.dll", "xinput1_3.dll", "xinput9_1_0.dll" })
            {
                if (auto module = LoadLibraryA(dll))
                    return reinterpret_cast<XInputGetStateFn>(GetProcAddress(module, "XInputGetState"));
            }
            return nullptr;
        }();

        if (XInputGetStateFunc)
        {
            uint8_t state[16] = {};
            for (DWORD i = 0; i < 4; ++i)
            {
                if (XInputGetStateFunc(i, state) == ERROR_SUCCESS)
                    return IconStyle::Xbox;
            }
        }

        UINT count = 0;
        if (GetRawInputDeviceList(nullptr, &count, sizeof(RAWINPUTDEVICELIST)) == 0 && count)
        {
            std::vector<RAWINPUTDEVICELIST> devices(count);
            if (GetRawInputDeviceList(devices.data(), &count, sizeof(RAWINPUTDEVICELIST)) != (UINT)-1)
            {
                for (UINT i = 0; i < count; ++i)
                {
                    if (devices[i].dwType != RIM_TYPEHID)
                        continue;

                    RID_DEVICE_INFO info = { sizeof(RID_DEVICE_INFO) };
                    UINT size = sizeof(info);
                    if (GetRawInputDeviceInfoA(devices[i].hDevice, RIDI_DEVICEINFO, &info, &size) == (UINT)-1)
                        continue;

                    constexpr DWORD SonyVendorId = 0x054C;
                    constexpr USHORT GenericDesktopPage = 0x01, Joystick = 0x04, Gamepad = 0x05;
                    if (info.hid.dwVendorId == SonyVendorId && info.hid.usUsagePage == GenericDesktopPage &&
                        (info.hid.usUsage == Gamepad || info.hid.usUsage == Joystick))
                        return IconStyle::PlayStation;
                }
            }
        }

        return IconStyle::Xbox;
    }
}

// Input device tracking: icons follow the last device used
namespace
{
    using SteadyClock = std::chrono::steady_clock;
    SteadyClock::time_point LastPadInput = {};
    SteadyClock::time_point LastKeyboardMouseInput = {};

    bool LastInputWasPad()
    {
        return LastPadInput > LastKeyboardMouseInput;
    }

    struct PadSnapshot
    {
        LONG axes[6] = {};
        DWORD pov = 0xFFFFFFFF;
        BYTE buttons[PadButtonCount] = {};
        bool valid = false;
    } PreviousPad[MaxJoysticks];

    void TrackPadActivity(int index, const DIJOYSTATE2& state)
    {
        constexpr LONG AxisActivity = 2500;
        PadSnapshot now;
        now.valid = true;
        memcpy(now.axes, &state.lX, sizeof(now.axes));
        now.pov = state.rgdwPOV[0];
        memcpy(now.buttons, state.rgbButtons, sizeof(now.buttons));

        auto& before = PreviousPad[index];
        if (!before.valid)
        {
            // The first state is the reference, not input
            before = now;
            before.valid = true;
            return;
        }

        bool active = now.pov != before.pov || memcmp(now.buttons, before.buttons, sizeof(now.buttons)) != 0;
        for (int i = 0; i < 6 && !active; ++i)
            active = std::abs(now.axes[i] - before.axes[i]) > AxisActivity;

        if (active)
        {
            before = now;
            LastPadInput = SteadyClock::now();
        }
    }
}

// Right stick and mouse, used by the camera
export namespace Input
{
    // The stick dead zones changed in the options menu
    void ApplyStickDeadzone()
    {
        if (!Joysticks)
            return;
        for (int i = 0; i < MaxJoysticks; ++i)
        {
            auto device = GetJoystick(i);
            if (!IsXidiDevice(device))
                continue;
            for (DWORD offset : { OfsLeftX, OfsLeftY, OfsRightX, OfsRightY })
                SetStickDeadzone(device, offset, DIPH_BYOFFSET, offset);
        }
    }

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
    std::chrono::steady_clock::time_point LastStickUpdate = {};

    // Called once per camera update; mouse deltas accumulate between calls. The stick only counts while its pad
    // is read: a pad that is unplugged or switched to the controls menu's setup doesn't leave the camera turning.
    LookInput ConsumeLook()
    {
        LookInput result = Look;
        if (std::chrono::steady_clock::now() - LastStickUpdate > std::chrono::milliseconds(100))
            result.stickX = result.stickY = 0.0f;
        Look.mouseX = 0;
        Look.mouseY = 0;
        return result;
    }

    bool UsingGamepad()
    {
        return LastInputWasPad();
    }

    // While enabled, presses "select" in the front end twice a second, like Enter. Screens such as the
    // title screen only accept it after their prompt has appeared. With a limit, it stops after that many
    // presses: a screen that starts loading the profile must not get a second press while it loads, or
    // the front end waits for the load forever and only takes the quit key.
    std::atomic<bool> AutoAccept = false;
    std::atomic<int> AutoAcceptPresses = -1;    // -1 = no limit
    std::chrono::steady_clock::time_point AutoAcceptStart = {};

    void SetAutoAccept(bool enable, int presses = -1)
    {
        if (enable && AutoAccept && AutoAcceptPresses == 0)
            return;     // its presses are used up
        if (enable && !AutoAccept)
            AutoAcceptStart = std::chrono::steady_clock::now();
        AutoAcceptPresses = enable ? presses : -1;
        AutoAccept = enable;
    }
}

// Controller button names and icons
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

    const char* ObjectName(DWORD offset)
    {
        const auto& names = CurrentNames();
        switch (offset)
        {
            case OfsLeftX: return "Left Stick";
            case OfsLeftY: return "Left Stick Y";
            case OfsRightX: return "Right Stick";
            case OfsRightY: return "Right Stick Y";
            case OfsLT: return names.leftTrigger;
            case OfsRT: return names.rightTrigger;
        }

        if (offset >= OfsButton0 && offset < OfsButton0 + PadButtonCount)
            return names.buttons[offset - OfsButton0];

        return nullptr;
    }

    // Front end hotkey images and the icons shown for them
    struct HotkeyIcon
    {
        const char* objectName;
        const char* keyboardTexture;
        const char* xboxTexture;
        const char* playStationTexture;
    };

    constexpr HotkeyIcon HotkeyIcons[] =
    {
        { "DELETEPROFILE_HOTKEY", "PC_DELETE",      "BUTTON_RS",   "BUTTON_R3"       },
        { "DELETEPERSONA_HOTKEY", "PC_DELETE",      "BUTTON_RS",   "BUTTON_R3"       },
        { "DEFAULT_HOTKEY",       "PC_CREATEROOM",  "BUTTON_Y",    "BUTTON_TRIANGLE" },
        { "STOCK_HOTKEY",         "PC_CREATEROOM",  "BUTTON_Y",    "BUTTON_TRIANGLE" },
        { "PERFORMANCE_HOTKEY",   "PC_PERFORMANCE", "BUTTON_X",    "BUTTON_SQUARE"   },
        { "CHANGECOLOR_HOTKEY",   "PC_X",           "BUTTON_A",    "BUTTON_CROSS"    },
        { "DECALCOLOR_HOTKEY",    "PC_CREATEROOM",  "BUTTON_Y",    "BUTTON_TRIANGLE" },
        { "QUIT_HOTKEY",          "PC_QUIT",        "BUTTON_QUIT", "BUTTON_QUIT"     },
        { "CUSTOMIZE_HOTKEY",     "PC_CUSTOM",      "BUTTON_VIEW", "BUTTON_SHARE"    },
        { "NEXT_HOTKEY",          "PC_X",           "BUTTON_A",    "BUTTON_CROSS"    },
        { "BACK_HOTKEY",          "PC_BACK",        "BUTTON_B",    "BUTTON_CIRCLE"   },
        { "PC_TUTORIAL_01",       "PC_TUTORIAL",    "BUTTON_LS",   "BUTTON_L3"       },
        { "PC_TUTORIAL_02",       "PC_TUTORIAL",    "BUTTON_LS",   "BUTTON_L3"       },
    };

    FEImage* FindImage(const char* package, uint32_t nameHash)
    {
        auto pkg = CallAndRet<uintptr_t, reg::eax>(FEPkgMgr_FindPackage, package);
        if (!pkg)
            return nullptr;

        auto object = reinterpret_cast<FEObject*>(CallAndRet<uintptr_t, reg::edx, reg::ecx>(FEngFindObject, pkg, nameHash));
        return (object && object->Type == FE_Image) ? static_cast<FEImage*>(object) : nullptr;
    }

    void SetImageTexture(FEImage* image, uint32_t textureHash)
    {
        if (image->Handle != textureHash)
        {
            image->Handle = textureHash;
            image->Flags |= 0x2400000;
        }
    }

    void UpdateHotkeyIcons(const char* package)
    {
        bool pad = LastInputWasPad();
        bool playStation = CurrentIconStyle == IconStyle::PlayStation;
        for (const auto& icon : HotkeyIcons)
        {
            if (auto image = FindImage(package, bStringHash(icon.objectName)))
                SetImageTexture(image, bStringHash(!pad ? icon.keyboardTexture : (playStation ? icon.playStationTexture : icon.xboxTexture)));
        }
    }
}

// Button textures: the DDS files in the "TPKDDS" resources are packed into a texture pack in memory
// and handed to the game's chunk loader, the same way it loads a TPK file.
namespace ButtonTextures
{
    char* (__cdecl* bMalloc)(int size, int debugName, int debugLine, int flags) = nullptr;
    bool (__cdecl* LoadChunksFromMemory)(char** buffer, int* size, int allocFlags, const char* debugName) = nullptr;

    void Load()
    {
        auto textures = TexturePack::LoadResourceTextures();
        if (textures.empty() || !bMalloc || !LoadChunksFromMemory)
            return;

        auto pack = TexturePack::Build(textures, "WFP_NFSU_BUTTONS");
        int size = int(pack.size());
        char* buffer = bMalloc(size, 0, 0, 0);
        if (!buffer)
            return;

        // The loader takes ownership of the buffer and frees it
        memcpy(buffer, pack.data(), pack.size());
        LoadChunksFromMemory(&buffer, &size, 0, "WFP_NFSU_BUTTONS");
    }
}

// Controls menu labels for Xidi pads
namespace
{
    SafetyHookInline shGetJoystickBindingName = {};
    const char* __stdcall GetJoystickBindingName(uintptr_t channel, int action, int secondary)
    {
        auto index = Channel::At<int32_t>(channel, Channel::Index);
        if (!IsXidiDevice(GetJoystick(index)))
            return shGetJoystickBindingName.unsafe_stdcall<const char*>(channel, action, secondary);

        // Same search as the game: axis bindings first, then buttons, then d-pad directions.
        // A secondary binding is the second match.
        bool skipFirst = secondary != 0;
        auto match = [&]() -> bool
        {
            if (skipFirst)
            {
                skipFirst = false;
                return false;
            }
            return true;
        };

        auto enabled = [&](int i) { return Channel::At<uint8_t>(channel, Channel::AxisEnabled + i) != 0; };
        bool combined = Channel::At<uint8_t>(channel, Channel::CombinedPedals) != 0;
        int axis = -1;
        if (enabled(0) && (action == ActionSteerLeft || action == ActionSteerRight)) axis = 0;
        if (enabled(1) && !combined && action == ActionThrottle) axis = 1;
        if (enabled(2) && !combined && action == ActionBrake) axis = 2;
        if (enabled(3) && combined && (action == ActionThrottle || action == ActionBrake)) axis = 3;

        if (axis >= 0 && match())
        {
            if (auto name = ObjectName(Channel::At<DWORD>(channel, Channel::AxisOffset + axis * 4)))
                return name;
        }

        for (int i = 0; i < 16; ++i)
        {
            if (Channel::At<int32_t>(channel, Channel::ButtonGameAction + i * 4) == action && match())
            {
                if (auto name = ObjectName(OfsButton0 + i))
                    return name;
            }
        }

        static constexpr const char* DpadNames[] = { "D-pad Down", "D-pad Up", "D-pad Left", "D-pad Right" };
        for (int pov = 0; pov < 4; ++pov)
        {
            for (int direction = 0; direction < 4; ++direction)
            {
                if (Channel::At<int32_t>(channel, Channel::PovGameAction + (pov * 4 + direction) * 4) == action && match())
                    return DpadNames[direction];
            }
        }

        return "";
    }
}

// Default bindings for Xidi pads, used when there's no saved configuration and by "Default Settings"
namespace
{
    SafetyHookInline shSetDefaultJoystickBindings = {};
    void __stdcall SetDefaultJoystickBindings(uintptr_t channel)
    {
        shSetDefaultJoystickBindings.unsafe_stdcall(channel);

        auto index = Channel::At<int32_t>(channel, Channel::Index);
        if (!IsXidiDevice(GetJoystick(index)))
            return;

        // Left stick steers, RT accelerates, LT brakes and reverses. Triggers rest at the axis minimum.
        Channel::At<DWORD>(channel, Channel::AxisOffset + 0) = OfsLeftX;
        Channel::At<DWORD>(channel, Channel::AxisOffset + 4) = OfsRT;
        Channel::At<DWORD>(channel, Channel::AxisOffset + 8) = OfsLT;
        Channel::At<DWORD>(channel, Channel::AxisOffset + 12) = OfsLeftY;
        for (int i = 0; i < 4; ++i)
            Channel::At<int32_t>(channel, Channel::AxisSign + i * 4) = 1;
        Channel::At<uint8_t>(channel, Channel::AxisEnabled + 0) = 1;
        Channel::At<uint8_t>(channel, Channel::AxisEnabled + 1) = 1;
        Channel::At<uint8_t>(channel, Channel::AxisEnabled + 2) = 1;
        Channel::At<uint8_t>(channel, Channel::AxisEnabled + 3) = 0;
        Channel::At<uint8_t>(channel, Channel::CombinedPedals) = 0;

        constexpr uint32_t GameActions[PadButtonCount] =
        {
            ActionHandbrake,    // A
            ActionNitrous,      // B
            ActionLookBack,     // X
            ActionCamera,       // Y
            ActionShiftDown,    // LB
            ActionShiftUp,      // RB
            ActionRespot,       // Back
            ActionPause,        // Start
            ActionHUD,          // LS
            ActionNone,         // RS
        };

        constexpr uint32_t MenuActions[PadButtonCount] =
        {
            ActionMenuSelect,       // A
            ActionMenuBack,         // B
            ActionMenuPreset,       // X: performance
            ActionMenuHotkeyZ,      // Y: default/stock
            ActionMenuButton3,      // LB
            ActionMenuButton2,      // RB
            ActionMenuCustomise,    // Back
            ActionMenuSelectAlt,    // Start
            ActionMenuHotkeyT,      // LS: tutorial
            ActionMenuRotateRight,  // RS: delete
        };

        for (int i = 0; i < 16; ++i)
        {
            Channel::At<uint32_t>(channel, Channel::ButtonGameAction + i * 4) = i < PadButtonCount ? GameActions[i] : ActionNone;
            Channel::At<uint32_t>(channel, Channel::ButtonMenuAction + i * 4) = i < PadButtonCount ? MenuActions[i] : ActionNone;
        }

        // D-pad left/right steer; menu navigation on the d-pad is set up by the game
        for (int pov = 0; pov < 4; ++pov)
        {
            uint32_t* directions = &Channel::At<uint32_t>(channel, Channel::PovGameAction + pov * 16);
            directions[0] = ActionNone;
            directions[1] = ActionNone;
            directions[2] = ActionSteerLeft;
            directions[3] = ActionSteerRight;
        }
    }
}

// Stick dead zones for Xidi pads, set when the game enumerates the device's axes
namespace
{
    SafetyHookInline shEnumJoystickObject = {};
    BOOL __stdcall EnumJoystickObject(LPCDIDEVICEOBJECTINSTANCEA object, LPVOID ref)
    {
        BOOL result = shEnumJoystickObject.unsafe_stdcall<BOOL>(object, ref);

        auto device = GetJoystick(int(reinterpret_cast<intptr_t>(ref)));
        if (object && (object->dwType & DIDFT_AXIS) && IsXidiDevice(device))
        {
            auto ofs = object->dwOfs;
            if (ofs == OfsLeftX || ofs == OfsLeftY || ofs == OfsRightX || ofs == OfsRightY)
                SetStickDeadzone(device, object->dwType, DIPH_BYID, ofs);
        }

        return result;
    }
}

// Per-frame processing of a joystick channel, after the game built its action mask and analog bytes
namespace
{
    bool bSeenAtRest[MaxJoysticks] = {};
    IDirectInputDevice8A* LastDevice[MaxJoysticks] = {};

    uint8_t AxisToByte(LONG value)
    {
        // -10000..10000 -> 0..254, matching the game's own conversion
        return uint8_t(std::clamp(127 + int(value * 0.0127), 0, 254));
    }

    SteadyClock::time_point SyntheticKeysUntil = {};   // the fix's own key presses aren't the player's keyboard input

    // The last letter typed into the game's window, read by the PC menu bar for its hotkeys
    char* TypedChar = nullptr;

    void TypeMenuKey(char key)
    {
        if (TypedChar)
            *TypedChar = key;
    }

    void ProcessKeyboardChannel(uintptr_t channel)
    {
        static SteadyClock::time_point lastPress = {};
        static bool bPressed = false;
        if (bPressed)
        {
            SetActionPressed(channel, ActionMenuSelect, false);
            bPressed = false;
        }

        auto now = SteadyClock::now();
        // The first press waits a second: a press while the title screen is still coming up stops the game
        // from reading the keyboard, and only the quit key, which comes from window messages, still works
        if (Input::AutoAccept && Input::AutoAcceptPresses != 0 && now - Input::AutoAcceptStart > std::chrono::seconds(1) && now - lastPress > std::chrono::milliseconds(500))
        {
            lastPress = now;
            if (Input::AutoAcceptPresses > 0)
                --Input::AutoAcceptPresses;
            SetActionPressed(channel, ActionMenuSelect, true);
            bPressed = true;
            SyntheticKeysUntil = now + std::chrono::milliseconds(200);
        }
    }

    void ProcessJoystickChannel(uintptr_t channel)
    {
        if (Channel::At<int32_t>(channel, Channel::DeviceType) == Channel::KeyboardType)
            return ProcessKeyboardChannel(channel);

        if (Channel::At<int32_t>(channel, Channel::ConfigMode) != 0)
            return;

        int index = Channel::At<int32_t>(channel, Channel::Index);
        auto device = GetJoystick(index);
        auto state = GetJoyState(index);
        if (!device || !state)
            return;

        if (LastDevice[index] != device)
        {
            LastDevice[index] = device;
            bSeenAtRest[index] = false;
            PreviousPad[index].valid = false;
        }

        if (!IsXidiDevice(device))
        {
            // Some devices report a stick or pedal that never returns to the centre. In the front end that
            // reads as the left stick being held up and the menus keep scrolling. Ignore the analog
            // navigation of such a device until it has been seen at rest.
            if (IsFrontEnd() && !bSeenAtRest[index])
            {
                auto& steer = Channel::At<uint8_t>(channel, Channel::SteerByte);
                auto& pedals = Channel::At<uint8_t>(channel, Channel::PedalsByte);
                if (std::abs(int(steer) - 0x80) < 16 && std::abs(int(pedals) - 0x80) < 16)
                    bSeenAtRest[index] = true;
                else
                {
                    steer = 0x80;
                    pedals = 0x80;
                }
            }
            return;
        }

        TrackPadActivity(index, *state);

        // The right stick of the pad in use drives the camera
        Input::Look.stickX = std::clamp(state->lRx / 10000.0f, -1.0f, 1.0f);
        Input::Look.stickY = std::clamp(state->lRy / 10000.0f, -1.0f, 1.0f);
        Input::LastStickUpdate = SteadyClock::now();

        if (IsFrontEnd())
        {
            // Left stick up/down navigates menus the same way combined pedals do
            Channel::At<uint8_t>(channel, Channel::PedalsByte) = AxisToByte(state->lY);

            // The menu bar's hotkeys (the PC front end's sub_4E6F08: Q quit, D delete, Z default/stock, T tutorial,
            // P performance, C customize) only read the last typed letter, not the menu actions, so the pad types
            // them like a keyboard does: Y is Z, X is P, Back is C, a stick click on its own is T or D when it's let
            // go, both together are Q
            bool ls = (state->rgbButtons[PadLS] & 0x80) != 0;
            bool rs = (state->rgbButtons[PadRS] & 0x80) != 0;
            bool y = (state->rgbButtons[PadY] & 0x80) != 0;
            bool x = (state->rgbButtons[PadX] & 0x80) != 0;
            bool back = (state->rgbButtons[PadBack] & 0x80) != 0;
            static bool bWasLS = false, bWasRS = false, bWasY = false, bWasX = false, bWasBack = false, bBothSticks = false;
            if (ls && rs)
            {
                if (!bBothSticks)
                    TypeMenuKey('q');
                bBothSticks = true;
                SetActionPressed(channel, ActionMenuHotkeyT, false);
                SetActionPressed(channel, ActionMenuRotateRight, false);
            }
            if (!bBothSticks)
            {
                if (bWasLS && !ls)
                    TypeMenuKey('t');
                if (bWasRS && !rs)
                    TypeMenuKey('d');
            }
            if (!ls && !rs)
                bBothSticks = false;
            if (y && !bWasY)
                TypeMenuKey('z');
            if (x && !bWasX)
                TypeMenuKey('p');
            if (back && !bWasBack)
                TypeMenuKey('c');
            bWasLS = ls;
            bWasRS = rs;
            bWasY = y;
            bWasX = x;
            bWasBack = back;
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
            CurrentIconStyle = DetectIconStyle();
            // Changes from the options menu or an editor
            FixSettings().OnChange("MAIN.GamepadIcons", [] { Input::RefreshIconStyle(); });
            FixSettings().OnChange("GAMEPAD.LeftStickDeadzone", [] { Input::ApplyStickDeadzone(); });
            FixSettings().OnChange("GAMEPAD.RightStickDeadzone", [] { Input::ApplyStickDeadzone(); });

            auto pattern = hook::pattern("83 EC 18 56 8B 74 24 24 8B 04 B5 ? ? ? ? 85 C0 57 74");
            auto joyState = hook::pattern("A3 ? ? ? ? B0 80 89 0D");
            if (pattern.empty() || joyState.empty())
                return;

            Joysticks = *pattern.get_first<IDirectInputDevice8A**>(11);
            JoyStateCopy = *joyState.get_first<uint8_t**>(1);
            shEnumJoystickObject = safetyhook::create_inline(pattern.get_first(), EnumJoystickObject);

            pattern = hook::pattern("81 EC AC 03 00 00 55 8B AC 24 B4 03 00 00 8B 85 30 01 00 00");
            if (!pattern.empty())
                shSetDefaultJoystickBindings = safetyhook::create_inline(pattern.get_first(), SetDefaultJoystickBindings);

            pattern = hook::pattern("51 53 8B 5C 24 0C 55 8B 6C 24 14 57 33 C0 B9 4F 00 00 00 BF");
            if (!pattern.empty())
                shGetJoystickBindingName = safetyhook::create_inline(pattern.get_first(), GetJoystickBindingName);

            pattern = hook::pattern("E8 ? ? ? ? 8B 46 08 8B 3D");
            if (!pattern.empty())
            {
                static auto JoystickChannelHook = safetyhook::create_mid(pattern.get_first(5), [](SafetyHookContext& regs)
                {
                    ProcessJoystickChannel(regs.esi);
                });
            }

            // While the controls menu is open the game enumerates all game controllers every 32 frames to
            // find new ones. That's slow with many HID devices and makes the menu stutter. Only do it after
            // Windows reports a device change.
            pattern = hook::pattern("8B C8 83 E1 1F 40 84 C9");
            if (!pattern.empty())
            {
                static auto RescanDevicesHook = safetyhook::create_mid(pattern.get_first(6), [](SafetyHookContext& regs)
                {
                    regs.ecx = bDevicesChanged.exchange(false) ? 0 : 1;
                });
            }

            onWindowMessage() += [](HWND, UINT msg, WPARAM wParam, LPARAM)
            {
                if (msg == WM_DEVICECHANGE && (wParam == DBT_DEVNODES_CHANGED || wParam == DBT_DEVICEARRIVAL || wParam == DBT_DEVICEREMOVECOMPLETE))
                {
                    bDevicesChanged = true;
                    bIconStyleOutdated = true;
                    XidiDeviceCache.clear();
                }
            };

            // Keyboard and mouse activity switches the icons back
            pattern = hook::pattern("8B 44 24 ? 8B 4C 24 ? 8B 53");
            static auto MouseStateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (hWnd != GetForegroundWindow())
                    return;

                struct MouseState { int dx, dy, wheel; };
                auto state = reinterpret_cast<MouseState*>(regs.esp + 0x4);
                Input::Look.mouseX += state->dx;
                Input::Look.mouseY += state->dy;
                if (state->dx || state->dy || state->wheel)
                {
                    LastKeyboardMouseInput = SteadyClock::now();
                }
            });

            pattern = hook::pattern("85 C0 7D ? 3D ? ? ? ? 74 ? 3D ? ? ? ? 0F 85 ? ? ? ? A1 ? ? ? ? ? ? 50 FF 52 ? 5F 5B");
            static auto KeyboardStateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (Channel::At<int32_t>(regs.esi, Channel::DeviceType) != Channel::KeyboardType)
                    return;

                // Only the keyboard's own actions are keyboard input, not the fix's presses
                if (SteadyClock::now() < SyntheticKeysUntil)
                    return;
                if (*reinterpret_cast<int32_t*>(regs.esi + Channel::ActionMaskLow) != -1 || *reinterpret_cast<int32_t*>(regs.esi + Channel::ActionMaskHigh) != -1)
                {
                    LastKeyboardMouseInput = SteadyClock::now();
                }
            });

            pattern = hook::pattern("8A 44 24 1C A2 ? ? ? ? E8");
            if (!pattern.empty())
                TypedChar = *pattern.get_first<char*>(5);

            pattern = hook::pattern("57 57 50 E8 ? ? ? ? 8B 4C 24 ? 68 00 20 00 00");
            ButtonTextures::bMalloc = reinterpret_cast<decltype(ButtonTextures::bMalloc)>(injector::GetBranchDestination(pattern.get_first(3)).as_int());

            pattern = hook::pattern("57 E8 ? ? ? ? 8A 46 0C 83 C4 10 A8 04");
            ButtonTextures::LoadChunksFromMemory = reinterpret_cast<decltype(ButtonTextures::LoadChunksFromMemory)>(injector::GetBranchDestination(pattern.get_first(1)).as_int());

            onMenuScreenBaseNotify() += [](MenuScreen* menu)
            {
                static bool bTexturesLoaded = false;
                if (!bTexturesLoaded)
                {
                    bTexturesLoaded = true;
                    ButtonTextures::Load();
                }

                if (bIconStyleOutdated.exchange(false))
                    CurrentIconStyle = DetectIconStyle();

                UpdateHotkeyIcons(menu->PackageFilename);
            };
        };
    }
} InputModule;

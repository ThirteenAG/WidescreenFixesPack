module;

#include <stdafx.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <dbt.h>
#include "FixSettings.hpp"
#include "RawMouse.hpp"
#include "Shared/NFS/StickDeadzone.hpp"

export module Input;

import ComVars;
import TexturePack;

import PostFX;

// Gamepads are handled by Xidi (Xidi.32.dll, loaded by the ASI loader). PlayStation and Switch pads use
// Steam Input or DS4Windows. Xidi presents each pad as a "Xidi Virtual Controller". The bundled
// Xidi.ini uses the XInputNative layout with each trigger on its own half axis: X/Y = left stick,
// RotX/RotY = right stick, Z+ = LT, RotZ+ = RT, POV = d-pad, buttons 1-10 = A B X Y LB RB Back Start
// LS RS. This module gives such pads the Xbox 360 version's default bindings, proper names in the
// controls menu, controller icons in the front end and the right stick for the camera. Other devices
// (wheels and so on) keep the game's own behaviour.

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

    enum Action
    {
        ActionGas = 0,
        ActionBrake = 1,
        ActionSteerLeft = 2,
        ActionSteerRight = 3,
        ActionHandbrake = 4,
        ActionNitrous = 5,
        ActionShiftDown = 6,
        ActionShiftUp = 7,
        ActionClutch = 8,
        ActionReset = 17,
        ActionToggleLeaderboard = 18,
        ActionToggleMinimap = 19,
        ActionToggleBestLine = 20,
        ActionToggleArrows = 21,
        ActionCameraChange = 22,
        ActionLookBack = 23,
        ActionPause = 25,
        ActionFELeft = 26,
        ActionFERight = 27,
        ActionFEUp = 28,
        ActionFEDown = 29,
        ActionFERotateLeft = 30,
        ActionFERotateRight = 31,
        ActionFERotateUp = 32,
        ActionFERotateDown = 33,
        ActionFEAccept = 34,
        ActionFECancel = 35,
        ActionFEStart = 36,
        ActionFELeftTrigger = 37,   // "9"
        ActionFERightTrigger = 38,  // "0"
        ActionFEButton0 = 39,       // "3"
        ActionFEButton1 = 40,       // "4"
        ActionFEButton3 = 41,       // "M"
        ActionFEButton4 = 42,       // "2"
        ActionFEButton5 = 43,       // "1"
        ActionSkipNIS = 44,
        ActionNextSong = 45,
        ActionFERightTrigger2 = 50,
        ActionFELeftTrigger2 = 51,
    };

    constexpr int ActionCount = 69;

    enum PadButton { PadA, PadB, PadX, PadY, PadLB, PadRB, PadBack, PadStart, PadLS, PadRS, PadButtonCount };

    // Joystick devices of the game; the product name is at +0x114
    constexpr int MaxJoysticks = 8;

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

    bool IsXidiJoystick(int index)
    {
        if (!Joysticks.is_initialized() || index < 0 || index >= MaxJoysticks)
            return false;
        auto joystick = Joysticks.get_ptr()[index];
        if (!joystick || joystick == uintptr_t(-4))
            return false;
        return strncmp(reinterpret_cast<const char*>(joystick + 0x114), "Xidi Virtual Controller", 23) == 0;
    }
}

// Icon style detection
namespace
{
    IconStyle DetectIconStyle()
    {
        return std::clamp(*pGamepadIcons, 0, 2) == 2 ? IconStyle::PlayStation : IconStyle::Xbox;
    }
}

// Which device was used last, for the front end icons and the mouse cursor
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
        bool rawMouse = false;  // the mouse movement is raw input
    };

    LookInput Look;

    // Called once per camera update; mouse deltas accumulate between calls
    LookInput ConsumeLook()
    {
        LookInput result = Look;
        Look.mouseX = 0;
        Look.mouseY = 0;
        result.rawMouse = RawMouse::Consume(result.mouseX, result.mouseY);
        if (result.rawMouse && (result.mouseX || result.mouseY))
            LastKeyboardMouseInput = SteadyClock::now();
        if (DeveloperOverlay::BlocksInput())
            return {};
        return result;
    }

    bool UsingGamepad()
    {
        return LastInputWasPad();
    }

    // Whether a driving action is bound to the vertical axis of a pad's right stick, such as shifting
    // in the Xbox 360 layout. The camera then only turns sideways with the stick.
    bool RightStickVerticalBound()
    {
        if (!Actions.is_initialized() || !NumJoysticks.is_initialized())
            return false;

        int slotCount = 2 + 2 * NumJoysticks.get();
        for (int action = ActionGas; action <= ActionLookBack; ++action)
        {
            auto slots = Actions.get_ptr()[action].slots;
            for (int slot = 2; slots && slot < slotCount; ++slot)
            {
                const auto& binding = slots[slot];
                if ((binding.type == BindRotationLow || binding.type == BindRotationHigh) && binding.index == 1 && IsXidiJoystick(binding.device))
                    return true;
            }
        }
        return false;
    }
}

// The Xbox 360 version's default bindings for Xidi pads
namespace
{
    int DefaultsDevice = 0;

    void Bind(int action, Binding primary, Binding secondary = {})
    {
        if (!Actions.is_initialized() || !NumJoysticks.is_initialized() || action >= ActionCount)
            return;

        auto slots = Actions.get_ptr()[action].slots;
        int slot = 2 + 2 * DefaultsDevice;
        if (!slots || slot + 1 >= 2 + 2 * NumJoysticks.get())
            return;

        primary.device = primary.type ? DefaultsDevice : 0;
        secondary.device = secondary.type ? DefaultsDevice : 0;
        slots[slot] = primary;
        slots[slot + 1] = secondary;
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
    void __fastcall SetJoystickDefaults(void* self, void* edx, int device)
    {
        shSetJoystickDefaults.unsafe_thiscall(self, device);
        if (!IsXidiJoystick(device))
            return;

        DefaultsDevice = device;

        // Race
        Bind(ActionGas, RotationHigh(AxisZ));                   // RT
        Bind(ActionBrake, AxisHigh(AxisZ));                     // LT
        Bind(ActionSteerLeft, AxisLow(AxisX));
        Bind(ActionSteerRight, AxisHigh(AxisX));
        Bind(ActionHandbrake, Button(PadA));
        Bind(ActionNitrous, Button(PadB));
        Bind(ActionClutch, Button(PadLB));
        Bind(ActionShiftUp, RotationLow(AxisY));                // right stick up
        Bind(ActionShiftDown, RotationHigh(AxisY));             // right stick down
        Bind(ActionReset, Button(PadBack));
        Bind(ActionPause, Button(PadStart));
        Bind(ActionCameraChange, Button(PadX));
        Bind(ActionLookBack, Button(PadY));
        Bind(ActionToggleArrows, Pov(PovUp));
        Bind(ActionToggleLeaderboard, Pov(PovLeft));
        Bind(ActionToggleBestLine, Pov(PovDown));
        Bind(ActionToggleMinimap, Pov(PovRight));
        Bind(ActionSkipNIS, Button(PadA), Button(PadStart));
        Bind(ActionNextSong, Button(PadRS));

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
        Bind(ActionFEButton0, Button(PadLB));
        Bind(ActionFEButton1, Button(PadRB));
        Bind(ActionFEButton3, Button(PadRS));
        Bind(ActionFEButton4, Button(PadX));
        Bind(ActionFEButton5, Button(PadY));
        Bind(ActionFELeftTrigger, AxisHigh(AxisZ));
        Bind(ActionFERightTrigger, RotationHigh(AxisZ));
        Bind(ActionFELeftTrigger2, AxisHigh(AxisZ));
        Bind(ActionFERightTrigger2, RotationHigh(AxisZ));
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
    constexpr const char* HotkeyImages[] =
    {
        "PC_KEY_ENTER", "PC_KEY_ESC", "PC_KEY_ESC_FRENCH",
        "PC_KEY_SPC", "PC_KEY_SPC_CZECH", "PC_KEY_SPC_DANISH", "PC_KEY_SPC_DUTCH", "PC_KEY_SPC_FRENCH",
        "PC_KEY_SPC_GERMAN", "PC_KEY_SPC_ITALIAN", "PC_KEY_SPC_SPANISH",
        "PC_KEY_0", "PC_KEY_1", "PC_KEY_2", "PC_KEY_3", "PC_KEY_4", "PC_KEY_9", "PC_KEY_M", "PC_KEY_M_FRENCH",
        "PC_GREATERTHAN_LESSTHAN_KEYS", "PC_ARROW_KEYS_UP_DOWN", "L1", "R1", "L2", "R2",
    };

    struct HotkeyHashes
    {
        uint32_t pc, xbox, playStation;
    };

    std::vector<HotkeyHashes> HotkeyImageHashes;

    int (__cdecl* LoadChunks)(void* data, int size, int flags) = nullptr;

    void LoadButtonTextures()
    {
        static bool bLoaded = false;
        static SteadyClock::time_point retryAfter{};
        if (bLoaded || !LoadChunks || SteadyClock::now() < retryAfter)
            return;
        bLoaded = true;

        auto textures = TexturePack::LoadResourceTextures();
        if (textures.empty())
            return;

        // The texture pack keeps pointing into this memory, so it's never freed
        auto pack = TexturePack::Build(textures, "WFP_NFSPS_BUTTONS");
        auto buffer = static_cast<uint8_t*>(_aligned_malloc(pack.size(), 0x80));
        if (!buffer)
        {
            bLoaded = false;
            retryAfter = SteadyClock::now() + std::chrono::seconds(5);
            return;
        }
        memcpy(buffer, pack.data(), pack.size());
        LoadChunks(buffer, int(pack.size()), 0);
    }

    void UpdateHotkeyImages()
    {
        bool pad = LastInputWasPad();
        bool playStation = CurrentIconStyle == IconStyle::PlayStation;
        cFEng::ForAllObjects([&](FEObject* object)
        {
            if (object->Type != FE_Image)
                return;

            for (const auto& hashes : HotkeyImageHashes)
            {
                if (object->Handle != hashes.pc && object->Handle != hashes.xbox && object->Handle != hashes.playStation)
                    continue;

                uint32_t texture = !pad ? hashes.pc : (playStation ? hashes.playStation : hashes.xbox);
                if (object->Handle != texture)
                {
                    object->Handle = texture;
                    object->Flags |= 0x2400000;
                }
                break;
            }
        });
    }
}

class InputModule
{
public:
    InputModule()
    {
        WFP::onInitEvent() += []()
        {
            // Invalid controller indices use the game's empty-name path.
            auto controllerName = hook::pattern("83 F8 FF 75 ? B8 ? ? ? ? C2 08 00 8B 04 85 ? ? ? ? 85 C0 74 ? 8D 48 04 85 C9 74 ?");
            static auto emptyControllerName = reinterpret_cast<uintptr_t>(controllerName.get_first<uint8_t>(24) + *controllerName.get_first<int8_t>(23));
            static auto controllerNameHook = safetyhook::create_mid(controllerName.get_first(13), [](SafetyHookContext& regs)
            {
                if (regs.eax >= 8)
                    regs.eip = emptyControllerName;
            });

            // The controller-name scan must use the controller count, not the larger action-source count.
            // Its pointer array holds eight controllers and is followed by keyboard state bytes.
            auto controllerScan = hook::pattern("A1 ? ? ? ? 8B 5F 20 32 C9 8D 54 00 02 33 F6 85 D2 88 4C 24 12");
            injector::WriteMemory<uint32_t>(controllerScan.get_first(10), 0x9090D08B, true); // mov edx, eax
            controllerScan = hook::pattern("A1 ? ? ? ? 83 C6 01 8D 54 00 02 3B F2 7C ? 8A 4C 24 12 0F 57 C9");
            injector::WriteMemory<uint32_t>(controllerScan.get_first(8), 0x9090D08B, true); // mov edx, eax

            pGamepadIcons = &FixSettings().Int("MAIN.GamepadIcons");
            FixSettings().OnChange("MAIN.GamepadIcons", [] { Input::RefreshIconStyle(); });
            CurrentIconStyle = DetectIconStyle();

            for (auto image : HotkeyImages)
            {
                std::string name = image;
                std::string base = name.starts_with("PC_") ? name.substr(3) : name;
                HotkeyImageHashes.push_back({ bStringHash(image), bStringHash(("XBOX_" + base).c_str()), bStringHash(("PS_" + base).c_str()) });
            }

            // Binding table and the number of joysticks, set up by the input init
            auto pattern = hook::pattern("8B 1D ? ? ? ? BF ? ? ? ? BE ? ? ? ? 8B 4E FC 89 4F FC");
            NumJoysticks.SetAddress(*pattern.get_first<int32_t*>(2));
            Actions.SetAddress(*pattern.get_first<uintptr_t>(7) - 4);

            // Joystick defaults for a device, applied for a new device and by "Reset to defaults"
            pattern = hook::pattern("81 EC 20 04 00 00 53 55 56 57 8B BC 24 34 04 00 00 8B 04 BD");
            Joysticks.SetAddress(*pattern.get_first<uintptr_t*>(20));
            shSetJoystickDefaults = safetyhook::create_inline(pattern.get_first(), SetJoystickDefaults);

            pattern = hook::pattern("81 EC 0C 02 00 00 A1 ? ? ? ? 33 C4 89 84 24 08 02 00 00 53 8B 9C 24 18 02 00 00 55 8B AC 24 24 02 00 00");
            shGetBindingName = safetyhook::create_inline(pattern.get_first(), GetBindingName);

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

                // The overlay's pad: A B X Y LB RB Back Start LS RS and the d-pad in its own bits; while it's open
                // the game sees a pad at rest
                uint16_t overlayButtons = 0;
                constexpr uint16_t overlayMapping[] = { 0x4000, 0x2000, 0x8000, 0x1000, 0x400, 0x800, 0x1, 0x8, 0x2, 0x4 };
                for (int button = 0; button < PadButtonCount; ++button)
                {
                    if (state->rgbButtons[button] & 0x80)
                        overlayButtons |= overlayMapping[button];
                }
                auto pov = state->rgdwPOV[0];
                if (LOWORD(pov) != 0xFFFF)
                {
                    if (pov >= 31500 || pov <= 4500) overlayButtons |= 0x10;
                    if (pov >= 4500 && pov <= 13500) overlayButtons |= 0x20;
                    if (pov >= 13500 && pov <= 22500) overlayButtons |= 0x40;
                    if (pov >= 22500 && pov <= 31500) overlayButtons |= 0x80;
                }
                DeveloperOverlay::Gamepad(overlayButtons, int8_t(std::clamp((state->lX - 32767) / 258, -127L, 127L)),
                    int8_t(std::clamp((state->lY - 32767) / 258, -127L, 127L)));
                if (DeveloperOverlay::BlocksInput())
                {
                    state->lX = state->lY = state->lZ = state->lRx = state->lRy = state->lRz = 32767;
                    std::fill(std::begin(state->rgbButtons), std::end(state->rgbButtons), uint8_t(0));
                    state->rgdwPOV[0] = DWORD(-1);
                    Input::Look.stickX = 0.0f;
                    Input::Look.stickY = 0.0f;
                    return;
                }

                Input::Look.stickX = axis(state->lRx);
                Input::Look.stickY = axis(state->lRy);

                bool active = std::fabs(axis(state->lX)) > 0.5f || std::fabs(axis(state->lY)) > 0.5f ||
                    std::fabs(Input::Look.stickX) > 0.5f || std::fabs(Input::Look.stickY) > 0.5f ||
                    axis(state->lZ) > 0.5f || axis(state->lRz) > 0.5f || state->rgdwPOV[0] != 0xFFFFFFFF;
                for (int i = 0; i < PadButtonCount && !active; ++i)
                    active = (state->rgbButtons[i] & 0x80) != 0;
                if (active)
                    LastPadInput = SteadyClock::now();
            });

            // Mouse: deltas at ebp-0x14
            pattern = hook::pattern("8B 45 ? 3B C7 75 ? 3B D7");
            static auto MouseStateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (hWnd != GetForegroundWindow())
                    return;

                struct MouseState { int dx, dy, wheel; uint8_t button; };
                auto state = reinterpret_cast<MouseState*>(regs.ebp - 0x14);
                DeveloperOverlay::MouseButton((state->button & 0x80) != 0);
                if (DeveloperOverlay::BlocksInput())
                {
                    state->dx = state->dy = state->wheel = 0;
                    state->button = 0;
                    regs.edx = regs.ecx = 0;    // dy and the wheel, already loaded
                    RawMouse::Clear();
                    return;
                }
                Input::Look.mouseX += state->dx;
                Input::Look.mouseY += state->dy;
                if (state->dx || state->dy || state->wheel)
                    LastKeyboardMouseInput = SteadyClock::now();
            });

            // Keyboard: key states at ebp-0x130
            pattern = hook::pattern("8B 83 64 04 00 00 8B 80 B0 00 00 00");
            static auto KeyboardStateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                auto keys = reinterpret_cast<uint8_t*>(regs.ebp - 0x130);
                if (DeveloperOverlay::BlocksInput())
                {
                    std::fill_n(keys, 256, uint8_t(0));
                    return;
                }
                if (std::any_of(keys, keys + 256, [](uint8_t key) { return (key & 0x80) != 0; }))
                    LastKeyboardMouseInput = SteadyClock::now();
            });

            // Raw mouse movement for the free look camera
            RawMouse::Enabled.store(FixSettings().Bool("MAIN.RawInputMouse"));
            FixSettings().OnChange("MAIN.RawInputMouse", []()
            {
                RawMouse::Enabled.store(FixSettings().Bool("MAIN.RawInputMouse"));
                RawMouse::Clear();
            });
            WFP::onGameProcessEvent() += []() { RawMouse::Update(hWnd.get()); };
            WFP::onShutdownEvent() += []() { RawMouse::Release(); };
            onWindowMessage() += RawMouse::Message;

            onWindowMessage() += [](HWND, UINT msg, WPARAM wParam, LPARAM)
            {
                if (msg == WM_DEVICECHANGE && (wParam == DBT_DEVNODES_CHANGED || wParam == DBT_DEVICEARRIVAL || wParam == DBT_DEVICEREMOVECOMPLETE))
                    bIconStyleOutdated = true;
            };

            // Button textures and the front end icons
            pattern = hook::pattern("8B 44 24 0C 56 8B 74 24 08 57 8B 7C 24 10 50 57 56 E8");
            LoadChunks = reinterpret_cast<decltype(LoadChunks)>(pattern.get_first());

            WFP::onGameProcessEvent() += []()
            {
                LoadButtonTextures();

                if (bIconStyleOutdated.exchange(false))
                    CurrentIconStyle = DetectIconStyle();

                UpdateHotkeyImages();
            };
        };
    }
} InputModule;

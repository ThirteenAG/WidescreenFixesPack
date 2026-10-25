#pragma once
#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif
#include <dinput.h>
#include <atomic>
#include <functional>
#include <map>
#include <mutex>
#pragma comment(lib, "dxguid.lib")

// Gamepad support shared by the Silent Hill 3 and 4 fixes.
//
// Pads are handled by Xidi (Xidi.32.dll, loaded by the ASI loader named dinput8.dll, with the SDL3 plugin for
// PlayStation, Switch and other pads). Xidi presents each pad as a DirectInput joystick using its XInputNative
// layout: X/Y = left stick, RotX/RotY = right stick, Z = LT, RotZ = RT, POV = d-pad, buttons 1-10 = A B X Y LB RB
// Back Start LS RS. This header intercepts the game's DirectInput joystick reads for such pads, publishes a
// normalised snapshot of the pad for the fix's modules (camera, menus) and lets them rewrite what the game
// sees (for example to take the right stick away from the game while the fix's camera uses it).
namespace SHPad
{
    enum Button : uint32_t
    {
        A = 1 << 0, B = 1 << 1, X = 1 << 2, Y = 1 << 3,
        LB = 1 << 4, RB = 1 << 5, Back = 1 << 6, Start = 1 << 7,
        LS = 1 << 8, RS = 1 << 9,
        DpadUp = 1 << 10, DpadDown = 1 << 11, DpadLeft = 1 << 12, DpadRight = 1 << 13,
        LT = 1 << 14, RT = 1 << 15,
    };

    // How the game's Xidi mapper lays out the pad: DIJOYSTATE axis offsets and button indices (-1 = not present).
    // The default is Xidi's XInputNative layout.
    struct Layout
    {
        DWORD leftX = DIJOFS_X, leftY = DIJOFS_Y;
        DWORD rightX = DIJOFS_RX, rightY = DIJOFS_RY;
        int ltAxis = DIJOFS_Z, rtAxis = DIJOFS_RZ;      // -1 when the triggers are buttons
        int button[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1, -1 };  // A B X Y LB RB Back Start LS RS LT RT
    };

    enum LayoutButton { LayoutA, LayoutB, LayoutX, LayoutY, LayoutLB, LayoutRB, LayoutBack, LayoutStart, LayoutLS, LayoutRS, LayoutLT, LayoutRT };

    struct State
    {
        bool connected = false;
        float lx = 0, ly = 0, rx = 0, ry = 0;   // -1..1, right/down positive
        float lt = 0, rt = 0;                   // 0..1
        uint32_t buttons = 0;
        uint32_t pressed = 0;                   // went down since the previous read
    };

    namespace Detail
    {
        using CreateFn = HRESULT(WINAPI*)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
        using CreateDeviceFn = HRESULT(STDMETHODCALLTYPE*)(IDirectInput8A*, REFGUID, IDirectInputDevice8A**, LPUNKNOWN);
        using GetStateFn = HRESULT(STDMETHODCALLTYPE*)(IDirectInputDevice8A*, DWORD, LPVOID);
        using SetPropertyFn = HRESULT(STDMETHODCALLTYPE*)(IDirectInputDevice8A*, REFGUID, LPCDIPROPHEADER);

        inline CreateFn OriginalCreate = nullptr;
        inline std::map<void**, CreateDeviceFn> OriginalCreateDevice;
        inline std::map<void**, GetStateFn> OriginalGetState;
        inline std::mutex Lock;
        inline std::map<IDirectInputDevice8A*, bool> Devices;   // device -> is a Xidi pad
        inline std::map<IDirectInputDevice8A*, bool> Keyboards;
        inline std::map<IDirectInputDevice8A*, std::array<uint8_t, 256>> KeyboardStates;
        inline std::map<IDirectInputDevice8A*, std::array<uint8_t, 8>> MouseButtons;
        inline std::map<IDirectInputDevice8A*, bool> Mice;      // device -> is a mouse
        using GetDataFn = HRESULT(STDMETHODCALLTYPE*)(IDirectInputDevice8A*, DWORD, LPDIDEVICEOBJECTDATA, LPDWORD, DWORD);
        inline std::map<void**, GetDataFn> OriginalGetData;
        inline std::atomic<long> MouseX = 0, MouseY = 0;       // accumulated counts since the last TakeMouse
        inline std::atomic<bool> HideMouseMotion = false;       // the game doesn't see mouse movement
        inline State Current;
        inline std::atomic<bool> LastInputWasPad = false;
        inline std::vector<std::function<void(DIJOYSTATE&, const State&)>> Filters;
        inline Layout PadLayout;

        inline bool IsXidi(IDirectInputDevice8A* device)
        {
            std::lock_guard lock(Lock);
            if (auto it = Devices.find(device); it != Devices.end())
                return it->second;
            DIDEVICEINSTANCEA info = { sizeof(info) };
            bool xidi = SUCCEEDED(device->GetDeviceInfo(&info)) && memcmp(info.guidInstance.Data4, "XIDIVC", 6) == 0;
            Devices[device] = xidi;
            return xidi;
        }

        // The game may set any axis range; normalise against the range it set
        inline float Normalise(IDirectInputDevice8A* device, DWORD offset, LONG value)
        {
            DIPROPRANGE range = {};
            range.diph.dwSize = sizeof(range);
            range.diph.dwHeaderSize = sizeof(DIPROPHEADER);
            range.diph.dwObj = offset;
            range.diph.dwHow = DIPH_BYOFFSET;
            if (FAILED(device->GetProperty(DIPROP_RANGE, &range.diph)) || range.lMax <= range.lMin)
            {
                range.lMin = 0;
                range.lMax = 65535;
            }
            double center = (double(range.lMin) + range.lMax) * 0.5;
            double half = (double(range.lMax) - range.lMin) * 0.5;
            return float(std::clamp((value - center) / half, -1.0, 1.0));
        }

        inline LONG Denormalise(IDirectInputDevice8A* device, DWORD offset, float value)
        {
            DIPROPRANGE range = {};
            range.diph.dwSize = sizeof(range);
            range.diph.dwHeaderSize = sizeof(DIPROPHEADER);
            range.diph.dwObj = offset;
            range.diph.dwHow = DIPH_BYOFFSET;
            if (FAILED(device->GetProperty(DIPROP_RANGE, &range.diph)) || range.lMax <= range.lMin)
            {
                range.lMin = 0;
                range.lMax = 65535;
            }
            double center = (double(range.lMin) + range.lMax) * 0.5;
            double half = (double(range.lMax) - range.lMin) * 0.5;
            return LONG(std::lround(center + std::clamp(value, -1.0f, 1.0f) * half));
        }

        // Radial dead zone with rescaling, so small stick movements are smooth and diagonals aren't clipped
        inline void Deadzone(float& x, float& y, float deadzone)
        {
            float length = std::sqrt(x * x + y * y);
            if (length <= deadzone)
            {
                x = y = 0;
                return;
            }
            float scaled = std::min(1.0f, (length - deadzone) / (1.0f - deadzone));
            x = x / length * scaled;
            y = y / length * scaled;
        }

        inline bool IsMouse(IDirectInputDevice8A* device)
        {
            std::lock_guard lock(Lock);
            if (auto it = Mice.find(device); it != Mice.end())
                return it->second;
            DIDEVICEINSTANCEA info = { sizeof(info) };
            bool mouse = SUCCEEDED(device->GetDeviceInfo(&info)) && GET_DIDEVICE_TYPE(info.dwDevType) == DI8DEVTYPE_MOUSE;
            Mice[device] = mouse;
            return mouse;
        }

        inline bool IsKeyboard(IDirectInputDevice8A* device)
        {
            std::lock_guard lock(Lock);
            if (auto it = Keyboards.find(device); it != Keyboards.end())
                return it->second;
            DIDEVICEINSTANCEA info{ sizeof(info) };
            const bool keyboard = SUCCEEDED(device->GetDeviceInfo(&info)) && GET_DIDEVICE_TYPE(info.dwDevType) == DI8DEVTYPE_KEYBOARD;
            Keyboards[device] = keyboard;
            return keyboard;
        }

        inline HRESULT STDMETHODCALLTYPE GetDeviceData(IDirectInputDevice8A* device, DWORD size, LPDIDEVICEOBJECTDATA data, LPDWORD count, DWORD flags)
        {
            auto original = OriginalGetData.at(*reinterpret_cast<void***>(device));
            HRESULT result = original(device, size, data, count, flags);
            if (SUCCEEDED(result) && data && count && !(flags & DIGDD_PEEK) && IsKeyboard(device))
            {
                for (DWORD i = 0; i < *count; ++i)
                {
                    const auto& item = *reinterpret_cast<DIDEVICEOBJECTDATA*>(reinterpret_cast<uint8_t*>(data) + i * size);
                    if (item.dwData & 0x80)
                        LastInputWasPad = false;
                }
            }
            if (SUCCEEDED(result) && data && count && !(flags & DIGDD_PEEK) && IsMouse(device))
            {
                for (DWORD i = 0; i < *count; ++i)
                {
                    auto& item = *reinterpret_cast<DIDEVICEOBJECTDATA*>(reinterpret_cast<uint8_t*>(data) + i * size);
                    if ((item.dwOfs == DIMOFS_Z && LONG(item.dwData)) ||
                        (item.dwOfs >= DIMOFS_BUTTON0 && item.dwOfs <= DIMOFS_BUTTON7 && (item.dwData & 0x80)))
                        LastInputWasPad = false;
                    if (item.dwOfs == DIMOFS_X || item.dwOfs == DIMOFS_Y)
                    {
                        (item.dwOfs == DIMOFS_X ? MouseX : MouseY) += LONG(item.dwData);
                        if (LONG(item.dwData) != 0)
                            LastInputWasPad = false;
                        if (HideMouseMotion)
                            item.dwData = 0;
                    }
                }
            }
            return result;
        }

        inline HRESULT STDMETHODCALLTYPE GetDeviceState(IDirectInputDevice8A* device, DWORD size, LPVOID data)
        {
            auto original = OriginalGetState.at(*reinterpret_cast<void***>(device));
            HRESULT result = original(device, size, data);
            if (SUCCEEDED(result) && data && size == 256 && IsKeyboard(device))
            {
                std::lock_guard lock(Lock);
                auto& previous = KeyboardStates[device];
                auto keys = static_cast<const uint8_t*>(data);
                for (size_t i = 0; i < previous.size(); ++i)
                {
                    if ((keys[i] & 0x80) && !(previous[i] & 0x80))
                        LastInputWasPad = false;
                    previous[i] = keys[i];
                }
                return result;
            }
            if (SUCCEEDED(result) && data && (size == sizeof(DIMOUSESTATE) || size == sizeof(DIMOUSESTATE2)) && IsMouse(device))
            {
                auto& mouse = *static_cast<DIMOUSESTATE*>(data);
                MouseX += mouse.lX;
                MouseY += mouse.lY;
                if (mouse.lX || mouse.lY || mouse.lZ)
                    LastInputWasPad = false;
                {
                    std::lock_guard lock(Lock);
                    auto& previous = MouseButtons[device];
                    const size_t count = size == sizeof(DIMOUSESTATE2) ? 8 : 4;
                    auto buttons = static_cast<const uint8_t*>(data) + offsetof(DIMOUSESTATE, rgbButtons);
                    for (size_t i = 0; i < count; ++i)
                    {
                        if ((buttons[i] & 0x80) && !(previous[i] & 0x80))
                            LastInputWasPad = false;
                        previous[i] = buttons[i];
                    }
                }
                if (HideMouseMotion)
                    mouse.lX = mouse.lY = 0;
                return result;
            }
            if (!data || (size != sizeof(DIJOYSTATE) && size != sizeof(DIJOYSTATE2)) || !IsXidi(device))
                return result;

            auto& joy = *static_cast<DIJOYSTATE*>(data);
            State now;
            if (SUCCEEDED(result))
            {
                auto& layout = PadLayout;
                auto axis = [&](DWORD offset) { return *reinterpret_cast<LONG*>(reinterpret_cast<uint8_t*>(&joy) + offset); };
                now.connected = true;
                now.lx = Normalise(device, layout.leftX, axis(layout.leftX));
                now.ly = Normalise(device, layout.leftY, axis(layout.leftY));
                now.rx = Normalise(device, layout.rightX, axis(layout.rightX));
                now.ry = Normalise(device, layout.rightY, axis(layout.rightY));
                if (layout.ltAxis >= 0)
                    now.lt = (Normalise(device, layout.ltAxis, axis(layout.ltAxis)) + 1.0f) * 0.5f;
                if (layout.rtAxis >= 0)
                    now.rt = (Normalise(device, layout.rtAxis, axis(layout.rtAxis)) + 1.0f) * 0.5f;
                Deadzone(now.lx, now.ly, 0.24f);
                Deadzone(now.rx, now.ry, 0.20f);
                static const uint32_t bits[12] = { A, B, X, Y, LB, RB, Back, Start, LS, RS, LT, RT };
                for (int i = 0; i < 12; ++i)
                {
                    int index = layout.button[i];
                    if (index >= 0 && index < 32 && (joy.rgbButtons[index] & 0x80))
                        now.buttons |= bits[i];
                }
                DWORD pov = joy.rgdwPOV[0];
                if (LOWORD(pov) != 0xFFFF)
                {
                    if (pov >= 31500 || pov <= 4500) now.buttons |= DpadUp;
                    if (pov >= 4500 && pov <= 13500) now.buttons |= DpadRight;
                    if (pov >= 13500 && pov <= 22500) now.buttons |= DpadDown;
                    if (pov >= 22500 && pov <= 31500) now.buttons |= DpadLeft;
                }
                if (now.lt > 0.35f) now.buttons |= LT;
                if (now.rt > 0.35f) now.buttons |= RT;
            }
            {
                std::lock_guard lock(Lock);
                now.pressed = now.buttons & ~Current.buttons;
                if (now.pressed || std::abs(now.lx) + std::abs(now.ly) + std::abs(now.rx) + std::abs(now.ry) > 0.3f)
                    LastInputWasPad = true;
                Current = now;
            }
            if (SUCCEEDED(result))
            {
                for (auto& filter : Filters)
                    filter(joy, now);
            }
            return result;
        }

        inline HRESULT STDMETHODCALLTYPE CreateDevice(IDirectInput8A* input, REFGUID guid, IDirectInputDevice8A** out, LPUNKNOWN outer)
        {
            auto original = OriginalCreateDevice.at(*reinterpret_cast<void***>(input));
            HRESULT result = original(input, guid, out, outer);
            if (SUCCEEDED(result) && out && *out)
            {
                {
                    std::lock_guard lock(Lock);
                    Devices.erase(*out);    // a new device can reuse an old address
                    Mice.erase(*out);
                    MouseButtons.erase(*out);
                    Keyboards.erase(*out);
                    KeyboardStates.erase(*out);
                }
                if (IsXidi(*out) || IsMouse(*out) || IsKeyboard(*out))
                {
                    auto vtable = *reinterpret_cast<void***>(*out);
                    std::lock_guard lock(Lock);
                    if (!OriginalGetState.contains(vtable))
                    {
                        OriginalGetState[vtable] = reinterpret_cast<GetStateFn>(vtable[9]);
                        ProtectedGameRef<void*>{ &vtable[9] } = reinterpret_cast<void*>(&GetDeviceState);
                    }
                    if (!OriginalGetData.contains(vtable))
                    {
                        OriginalGetData[vtable] = reinterpret_cast<GetDataFn>(vtable[10]);
                        ProtectedGameRef<void*>{ &vtable[10] } = reinterpret_cast<void*>(&GetDeviceData);
                    }
                }
            }
            return result;
        }

        inline HRESULT WINAPI Create(HINSTANCE instance, DWORD version, REFIID iid, LPVOID* out, LPUNKNOWN outer)
        {
            HRESULT result = OriginalCreate(instance, version, iid, out, outer);
            if (SUCCEEDED(result) && out && *out && iid == IID_IDirectInput8A)
            {
                auto vtable = *reinterpret_cast<void***>(*out);
                std::lock_guard lock(Lock);
                if (!OriginalCreateDevice.contains(vtable))
                {
                    OriginalCreateDevice[vtable] = reinterpret_cast<CreateDeviceFn>(vtable[3]);
                    ProtectedGameRef<void*>{ &vtable[3] } = reinterpret_cast<void*>(&CreateDevice);
                }
            }
            return result;
        }

        inline HMODULE Xidi()
        {
            return GetModuleHandleW(L"Xidi.32.dll");
        }
    }

    // Installs the DirectInput hooks. Call from Init.
    inline void Install(const Layout& layout = {})
    {
        Detail::PadLayout = layout;
        auto hooks = IATHook::Replace(GetModuleHandleW(nullptr), "dinput8.dll", std::forward_as_tuple("DirectInput8Create", &Detail::Create));
        if (auto it = hooks.find("DirectInput8Create"); it != hooks.end())
            Detail::OriginalCreate = reinterpret_cast<Detail::CreateFn>(it->second.get());
    }

    // The latest pad state read by the game
    inline State Get()
    {
        std::lock_guard lock(Detail::Lock);
        return Detail::Current;
    }

    // Lets a module change what the game reads from the pad (runs on the game's input thread)
    inline void AddFilter(std::function<void(DIJOYSTATE&, const State&)> filter)
    {
        Detail::Filters.push_back(std::move(filter));
    }

    // Writes a normalised value back into the game's copy of an axis
    inline void SetAxis(DIJOYSTATE& joy, DWORD offset, float value)
    {
        // Xidi's default range is 0..65535 unless the game sets another; the game set range is honoured by
        // reading the property from any known Xidi device
        IDirectInputDevice8A* device = nullptr;
        {
            std::lock_guard lock(Detail::Lock);
            for (auto& [d, xidi] : Detail::Devices)
            {
                if (xidi)
                {
                    device = d;
                    break;
                }
            }
        }
        LONG raw = device ? Detail::Denormalise(device, offset, value) : LONG(32767.5f + value * 32767.5f);
        *reinterpret_cast<LONG*>(reinterpret_cast<uint8_t*>(&joy) + offset) = raw;
    }

    // Mouse movement the game read since the previous call (counts)
    inline std::pair<long, long> TakeMouse()
    {
        return { Detail::MouseX.exchange(0), Detail::MouseY.exchange(0) };
    }

    // Hides mouse movement from the game (while the fix's camera uses the mouse)
    inline void HideMouseFromGame(bool hide)
    {
        Detail::HideMouseMotion = hide;
    }

    inline bool LastInputWasPad()
    {
        return Detail::LastInputWasPad;
    }

    inline void NoteKeyboardMouseInput()
    {
        Detail::LastInputWasPad = false;
    }

    // Whether the connected pad is a PlayStation controller (for button names): no XInput pad, but a Sony HID gamepad
    inline bool IsPlayStation()
    {
        static std::atomic<int> cached = -1;
        static std::atomic<uint64_t> checked = 0;
        uint64_t now = GetTickCount64();
        if (cached >= 0 && now - checked < 3000)
            return cached != 0;
        checked = now;

        using XInputGetStateFn = DWORD(WINAPI*)(DWORD, void*);
        static auto getState = []() -> XInputGetStateFn
        {
            for (auto dll : { "xinput1_4.dll", "xinput1_3.dll", "xinput9_1_0.dll" })
            {
                if (auto module = LoadLibraryA(dll))
                    return reinterpret_cast<XInputGetStateFn>(GetProcAddress(module, "XInputGetState"));
            }
            return nullptr;
        }();
        if (getState)
        {
            uint8_t state[16] = {};
            for (DWORD i = 0; i < 4; ++i)
            {
                if (getState(i, state) == ERROR_SUCCESS)
                {
                    cached = 0;
                    return false;
                }
            }
        }

        UINT count = 0;
        bool sony = false;
        if (GetRawInputDeviceList(nullptr, &count, sizeof(RAWINPUTDEVICELIST)) == 0 && count)
        {
            std::vector<RAWINPUTDEVICELIST> devices(count);
            if (GetRawInputDeviceList(devices.data(), &count, sizeof(RAWINPUTDEVICELIST)) != UINT(-1))
            {
                for (UINT i = 0; i < count && !sony; ++i)
                {
                    if (devices[i].dwType != RIM_TYPEHID)
                        continue;
                    RID_DEVICE_INFO info = { sizeof(RID_DEVICE_INFO) };
                    UINT size = sizeof(info);
                    if (GetRawInputDeviceInfoA(devices[i].hDevice, RIDI_DEVICEINFO, &info, &size) == UINT(-1))
                        continue;
                    sony = info.hid.dwVendorId == 0x054C && info.hid.usUsagePage == 1 && (info.hid.usUsage == 4 || info.hid.usUsage == 5);
                }
            }
        }
        cached = sony ? 1 : 0;
        return sony;
    }

    // Short name of a pad button for the game's button configuration screens
    inline const char* ButtonName(LayoutButton button)
    {
        static const char* xbox[] = { "A", "B", "X", "Y", "LB", "RB", "Back", "Start", "LS", "RS", "LT", "RT" };
        static const char* sony[] = { "Cross", "Circle", "Square", "Triangle", "L1", "R1", "Share", "Options", "L3", "R3", "L2", "R2" };
        return (IsPlayStation() ? sony : xbox)[button];
    }

    // Rumble through Xidi (0..1 per motor)
    inline void Vibrate(float low, float high)
    {
        using SendFn = bool(__cdecl*)(short, unsigned short, unsigned short);
        static auto send = Detail::Xidi() ? reinterpret_cast<SendFn>(GetProcAddress(Detail::Xidi(), "XidiSendVibration")) : nullptr;
        if (send)
            send(-1, (unsigned short)(std::clamp(low, 0.0f, 1.0f) * 65535), (unsigned short)(std::clamp(high, 0.0f, 1.0f) * 65535));
    }

    // Physical pad elements, in Xidi's element map order (for SetButtonLabel)
    enum Element : int
    {
        ElementLeftX, ElementLeftY, ElementRightX, ElementRightY,
        ElementDpadUp, ElementDpadDown, ElementDpadLeft, ElementDpadRight,
        ElementLT, ElementRT, ElementA, ElementB, ElementX, ElementY, ElementLB, ElementRB,
        ElementBack, ElementStart, ElementLS, ElementRS,
    };

    // Name Xidi reports for a pad element in the game's button configuration screens
    inline void SetButtonLabel(int element, const char* label)
    {
        using LabelFn = void(__cdecl*)(int, const char*);
        static auto set = Detail::Xidi() ? reinterpret_cast<LabelFn>(GetProcAddress(Detail::Xidi(), "XidiSetButtonLabel")) : nullptr;
        if (set)
            set(element, label);
    }
}

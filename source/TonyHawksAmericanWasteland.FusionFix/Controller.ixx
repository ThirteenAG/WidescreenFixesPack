module;

#include <stdafx.h>

export module Controller;

import ComVars;

export namespace THAWController
{
    struct Device
    {
        DWORD size;
        GUID instance, product;
        DWORD type;
    };

    inline SafetyHookInline Enumeration;
    inline SafetyHookInline PadPoll, KeyboardPoll, MousePoll;
    inline SafetyHookMid CameraSpeedHook, CameraInvertHook, DefaultKeysHook;
    inline const float* CameraSpeed = nullptr;
    inline const int32_t* InvertCameraY = nullptr;
    inline bool (*BlockKeys)() = nullptr;
    inline std::atomic<bool> UsingPad = false;
    inline std::atomic<int> ActivePlayer = 0;

    inline bool Focused()
    {
        return WindowedModeWrapper::GameHWND && GetForegroundWindow() == WindowedModeWrapper::GameHWND;
    }

    inline uint8_t* __cdecl ReadPad(int player)
    {
        auto* current = PadPoll.ccall<uint8_t*>(player);
        if (!current || player < 0 || player >= 4 || !Focused())
            return current;
        const auto* previous = PreviousPad.get().data() + 272 * player;
        bool pressed = false;
        for (int i = 48; i < 176 && !pressed; ++i)
            pressed = (current[i] & 0x80) && !(previous[i] & 0x80);
        const auto* axes = reinterpret_cast<const int32_t*>(current);
        const auto* oldAxes = reinterpret_cast<const int32_t*>(previous);
        for (int i = 0; i < 8 && !pressed; ++i)
            pressed = std::abs(static_cast<int64_t>(axes[i])) > 200 &&
                      std::abs(static_cast<int64_t>(axes[i]) - oldAxes[i]) > 200;
        for (int i = 8; i < 12 && !pressed; ++i)
            pressed = axes[i] != -1 && axes[i] != oldAxes[i];
        if (pressed)
        {
            ActivePlayer.store(player, std::memory_order_relaxed);
            UsingPad.store(true, std::memory_order_relaxed);
        }
        return current;
    }

    inline int __cdecl ReadKeyboard()
    {
        const int result = KeyboardPoll.ccall<int>();
        // The open overlay keeps the keys for itself
        if (BlockKeys && BlockKeys())
            std::memset(CurrentKeys.get().data(), 0, 256);
        if (result >= 0 && Focused())
        {
            for (int i = 0; i < 256; ++i)
            {
                if ((CurrentKeys.get()[i] & 0x80) && !(PreviousKeys.get()[i] & 0x80))
                {
                    UsingPad.store(false, std::memory_order_relaxed);
                    break;
                }
            }
        }
        return result;
    }

    inline int32_t Decay(int32_t value)
    {
        return value > 2 ? value - 2 : value < -2 ? value + 2 : 0;
    }

    inline int32_t* __cdecl ReadMouse()
    {
        auto* current = MousePoll.ccall<int32_t*>();
        if (!current || !Focused())
            return current;
        const bool moved = (current[0] || current[1]) &&
            (current[0] != Decay(PreviousMouse.get_ptr()[0]) || current[1] != Decay(PreviousMouse.get_ptr()[1]));
        bool pressed = current[2] != 0;
        auto* buttons = reinterpret_cast<const uint8_t*>(current) + 12;
        auto* previous = reinterpret_cast<const uint8_t*>(PreviousMouse.get_ptr()) + 12;
        for (int i = 0; i < 8 && !pressed; ++i)
            pressed = (buttons[i] & 0x80) && !(previous[i] & 0x80);
        if (moved || pressed) UsingPad.store(false, std::memory_order_relaxed);
        return current;
    }

    inline BOOL __stdcall Enumerate(const Device* device, void* context)
    {
        const auto& product = device->product;
        const bool xidi = (product.Data1 & 0xFFFFu) == 0xFFFFu && product.Data2 == 0 && product.Data3 == 0 &&
                          product.Data4[0] == 'X' && product.Data4[1] == 'I' && product.Data4[2] == 'D' &&
                          product.Data4[3] == 'I' && product.Data4[4] == 'V' && product.Data4[5] == 'C';
        constexpr GUID unassigned{ 0xFFFFFFFFu, 0xFFFFu, 0xFFFFu,
                                   { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF } };
        if (xidi && (IsEqualGUID(GameSelected.get(), GUID_NULL) || IsEqualGUID(GameSelected.get(), unassigned)))
        {
            GameSelected.get() = device->instance;
            // These are native launcher bindings for the bundled Xidi StandardGamepad mapper.
            // Leave a player's existing device and bindings alone.
            constexpr std::array<uint32_t, 24> bindings{
                2, 2, 1, 1, 8, 8, 3, 3, 11, 12, 13, 14, 20, 19, 17, 18, 15, 16, 21, 16, 5, 5, 4, 4
            };
            std::copy(bindings.begin(), bindings.end(), reinterpret_cast<uint32_t*>(GameSelected.get_ptr() + 1));
        }
        return Enumeration.stdcall<BOOL>(device, context);
    }

    inline void Install(const float* cameraSpeed, const int32_t* invertCameraY)
    {
        CameraSpeed = cameraSpeed;
        InvertCameraY = invertCameraY;
        auto callback = hook::pattern("83 EC 50 53 55 8B 6C 24 5C 8B 45 24 56 25 FF 00 00 00 33 F6 83 F8 15");
        auto selected = hook::pattern("83 CB FF 33 D2 B8 ? ? ? ? C7 44 24 10 04 00 00 00");
        GameSelected.SetAddress(*selected.get_first<GUID*>(6));
        Enumeration = safetyhook::create_inline(callback.get_first(), Enumerate);

        auto pad = hook::pattern("53 8B 5C 24 08 8B 04 9D ? ? ? ? 85 C0 74 ? 8B 08 55 50 FF 51 64");
        auto padBuffers = hook::pattern("8D A8 ? ? ? ? 55 8D B8 ? ? ? ? B9 44 00 00 00 8B F5 68 10 01 00 00");
        auto keyboard = hook::pattern("A1 ? ? ? ? 56 57 68 ? ? ? ? B9 40 00 00 00 BE ? ? ? ? BF ? ? ? ? 68 00 01 00 00 F3 A5");
        auto mouse = hook::pattern("A1 ? ? ? ? 85 C0 0F 84 ? ? ? ? 8B 08 50 FF 51 64 85 C0 7D ? A1 ? ? ? ? 8B 10 50 FF 52 1C");
        auto mouseBuffers = hook::pattern("A3 ? ? ? ? A1 ? ? ? ? 89 15 ? ? ? ? 8B 15 ? ? ? ? 68 ? ? ? ? A3 ? ? ? ? A1 ? ? ? ? 6A 14 89 0D");
        PreviousPad.SetAddress(*padBuffers.get_first<uint8_t*>(9));
        CurrentKeys.SetAddress(*keyboard.get_first<uint8_t*>(18));
        PreviousKeys.SetAddress(*keyboard.get_first<uint8_t*>(23));
        PreviousMouse.SetAddress(*mouseBuffers.get_first<int32_t*>(12));
        PadPoll = safetyhook::create_inline(pad.get_first(), ReadPad);
        KeyboardPoll = safetyhook::create_inline(keyboard.get_first(), ReadKeyboard);
        MousePoll = safetyhook::create_inline(mouse.get_first(), ReadMouse);
        // Without the launcher's saved controls the keyboard has no bindings at all, and older saved controls can
        // leave actions such as Pause without a key: those get the launcher's default key when it is still free
        auto keyboardMap = hook::pattern("B9 34 01 00 00 BF ? ? ? ? F3 AB");
        auto readControls = hook::pattern("E8 ? ? ? ? 8B 44 24 0C 83 C4 04 6A 00");
        KeyboardMap.SetAddress(*keyboardMap.get_first<int32_t*>(6));
        DefaultKeysHook = safetyhook::create_mid(readControls.get_first(5), [](SafetyHookContext&)
        {
            static constexpr int32_t defaults[25] = { 17, 31, 30, 32, 78, 74, 181, 55, 76, 77, 75, 72, 28, 33, 82, 18, 71, 73,
                156, 57, 23, 36, 37, 38, 42 };
            const auto bound = [](int32_t key) { return key > 0 && key < 256; };
            for (size_t i = 0; i < std::size(defaults); ++i)
            {
                if (!bound(KeyboardMap.get()[i]) && std::find(KeyboardMap.get().begin(), KeyboardMap.get().end(), defaults[i]) == KeyboardMap.get().end())
                    KeyboardMap.get()[i] = defaults[i];
            }
        });
        auto camera = hook::pattern("F3 0F 10 1D ? ? ? ? 0F 28 C8 F3 0F 5C 46 20 F3 0F 11 44 24 08");
        CameraSpeedHook = safetyhook::create_mid(camera.get_first(8), [](SafetyHookContext& regs)
        {
            if (UsingPad.load(std::memory_order_relaxed))
                regs.xmm3.f32[0] *= std::clamp(*CameraSpeed, 0.1f, 5.0f);
        });
        auto invert = hook::pattern("84 C0 74 ? F3 0F 59 C5 0F 57 F6 0F 2F F0");
        CameraInvertHook = safetyhook::create_mid(invert.get_first(8), [](SafetyHookContext& regs)
        {
            if (UsingPad.load(std::memory_order_relaxed) && *InvertCameraY)
                regs.xmm0.f32[0] = -regs.xmm0.f32[0];
        });
    }
}

module;

#include <stdafx.h>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <algorithm>

export module InputDrain;

export namespace InputFilter
{
    struct Buttons
    {
        uint32_t held = 0;

        uint32_t Apply(std::array<uint32_t, 6>& banks, bool blocked)
        {
            const auto previous = held;
            held = blocked ? held | banks[0] | banks[1] : held & banks[0];
            const auto mask = blocked ? ~uint32_t{} : previous;
            for (auto& bank : banks) bank &= ~mask;
            return mask;
        }
    };

    struct Axes
    {
        std::array<bool, 6> held{};

        void Apply(std::array<int32_t, 6>& values, bool blocked)
        {
            for (size_t i = 0; i < values.size(); ++i)
            {
                held[i] = (blocked || held[i]) && values[i] != 0;
                if (blocked || held[i]) values[i] = 0;
            }
        }
    };
}

export namespace RE6Input
{
    inline bool ready = false;
    inline std::function<bool()> overlayBlocksInput;
    inline std::atomic<HWND> window{ nullptr };
    inline std::atomic<uint32_t> focusGeneration{ 0 };
    inline std::atomic<bool> keyboardBlocked{ false };
    inline SafetyHookInline padHook, keyboardHook, mouseHook;

    template<class T> T Read(const void* object, size_t offset)
    {
        return *reinterpret_cast<const T*>(static_cast<const uint8_t*>(object) + offset);
    }

    template<class T> void Write(void* object, size_t offset, T value)
    {
        if (Read<T>(object, offset) == value) return;
        injector::WriteMemory<T>(static_cast<uint8_t*>(object) + offset, value, true);
    }

    inline bool Blocked()
    {
        const auto foreground = GetForegroundWindow();
        DWORD process = 0;
        GetWindowThreadProcessId(foreground, &process);
        const auto game = window.load();
        return process != GetCurrentProcessId() || (game && foreground != game && GetAncestor(foreground, GA_ROOTOWNER) != game) ||
            (overlayBlocksInput && overlayBlocksInput());
    }

    inline void FocusLost()
    {
        ++focusGeneration;
    }

    inline uint32_t FilterButtons(void* object, size_t offset, InputFilter::Buttons& filter, bool blocked)
    {
        if (!blocked && !filter.held) return 0;
        std::array<uint32_t, 6> banks;
        for (size_t i = 0; i < banks.size(); ++i) banks[i] = Read<uint32_t>(object, offset + i * 4);
        const auto mask = filter.Apply(banks, blocked);
        if (mask)
            for (size_t i = 0; i < banks.size(); ++i) Write(object, offset + i * 4, banks[i]);
        return mask;
    }

    inline unsigned __fastcall Pad(void* object, void*)
    {
        const auto result = padHook.unsafe_thiscall<unsigned>(object);
        static void* previousObject = nullptr;
        static std::array<InputFilter::Buttons, 16> buttons;
        static std::array<InputFilter::Axes, 16> axes;
        static uint32_t generation = 0;
        if (previousObject != object)
        {
            buttons = {};
            axes = {};
            previousObject = object;
        }
        const auto current = focusGeneration.load();
        const bool blocked = Blocked() || current != generation;
        generation = current;
        for (size_t i = 0; i < buttons.size(); ++i)
        {
            auto device = static_cast<uint8_t*>(object) + 60 + 728 * i;
            if (!Read<uint8_t>(device, 7))
            {
                buttons[i] = {};
                axes[i] = {};
                continue;
            }
            FilterButtons(device, 348, buttons[i], blocked);
            if (!blocked && std::none_of(axes[i].held.begin(), axes[i].held.end(), [](bool value) { return value; })) continue;
            std::array<int32_t, 6> values;
            for (size_t j = 0; j < 4; ++j) values[j] = Read<int32_t>(device, 372 + 4 * j);
            values[4] = Read<uint8_t>(device, 388);
            values[5] = Read<uint8_t>(device, 389);
            axes[i].Apply(values, blocked);
            for (size_t j = 0; j < 4; ++j) Write(device, 372 + 4 * j, values[j]);
            Write(device, 388, static_cast<uint8_t>(values[4]));
            Write(device, 389, static_cast<uint8_t>(values[5]));
        }
        return result;
    }

    inline int __fastcall Keyboard(void* object, void*)
    {
        const auto result = keyboardHook.unsafe_thiscall<int>(object);
        static void* previousObject = nullptr;
        static std::array<InputFilter::Buttons, 8> filters;
        static uint32_t generation = 0;
        if (previousObject != object) { filters = {}; previousObject = object; }
        const auto current = focusGeneration.load();
        const bool blocked = Blocked() || current != generation;
        generation = current;
        keyboardBlocked.store(blocked);
        for (size_t i = 0; i < filters.size(); ++i)
        {
            if (!blocked && !filters[i].held) continue;
            std::array<uint32_t, 6> banks;
            for (size_t bank = 0; bank < banks.size(); ++bank) banks[bank] = Read<uint32_t>(object, 296 + 32 * bank + 4 * i);
            const auto mask = filters[i].Apply(banks, blocked);
            for (size_t bank = 0; bank < banks.size(); ++bank) Write(object, 296 + 32 * bank + 4 * i, banks[bank]);
            for (size_t bit = 0; bit < 32; ++bit)
                if (mask & (uint32_t{ 1 } << bit)) Write(object, 488 + (32 * i + bit) * 8, uint64_t{});
        }
        return result;
    }

    inline int __fastcall Mouse(void* object, void*)
    {
        const auto visible = Read<uint8_t>(object, 56);
        const bool overlay = overlayBlocksInput && overlayBlocksInput();
        if (overlay) Write(object, 56, uint8_t{ 1 });
        const auto result = mouseHook.unsafe_thiscall<int>(object);
        if (overlay) Write(object, 56, visible);
        static void* previousObject = nullptr;
        static InputFilter::Buttons buttons;
        static uint32_t generation = 0;
        static bool drainMotion = false;
        if (previousObject != object) { buttons = {}; drainMotion = false; previousObject = object; }
        const auto current = focusGeneration.load();
        const bool blocked = Blocked() || current != generation;
        generation = current;
        const auto mask = FilterButtons(object, 72, buttons, blocked);
        for (size_t bit = 0; bit < 8; ++bit)
            if (mask & (uint32_t{ 1 } << bit)) Write(object, 96 + bit * 8, uint64_t{});
        if (blocked || drainMotion)
            for (size_t offset : { size_t{ 60 }, size_t{ 64 }, size_t{ 68 } }) Write(object, offset, int32_t{});
        drainMotion = blocked;
        return result;
    }

    inline bool Install()
    {
        auto pad = hook::pattern("51 53 55 56 57 8B F9 80 BF C5 2D 00 00 00 74 ? C6 87 C5 2D 00 00 00 8B 0D ? ? ? ?");
        auto keyboard = hook::pattern("83 EC 14 53 55 56 8D A9 28 01 00 00 57 89 4C 24 14 8B C5 BA 08 00 00 00");
        auto mouse = hook::pattern("83 EC 40 53 55 56 57 8B F9 8B 47 48 33 ED 8D 4C 24 3C 51 89 7C 24 18 89 47 4C");
        padHook = safetyhook::create_inline(pad.get_first(), Pad);
        keyboardHook = safetyhook::create_inline(keyboard.get_first(), Keyboard);
        mouseHook = safetyhook::create_inline(mouse.get_first(), Mouse);
        ready = padHook && keyboardHook && mouseHook;
        if (!ready)
        {
            padHook.reset();
            keyboardHook.reset();
            mouseHook.reset();
        }
        return ready;
    }
}

module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/PatternCheck.hpp"

export module Display;

import ComVars;

// The display mode list and the window.

namespace
{
    const int* pPauseOnFocusLoss = nullptr;

    // The game's SetActive(active): inactive pauses the game, its sound and input
    void(__stdcall* SetActive)(bool active) = nullptr;
    void __stdcall SetActiveOnFocus(bool active)
    {
        if (!active && !*pPauseOnFocusLoss)
            return;
        SetActive(active);
    }

    void SetDpiAwareness()
    {
        // Per monitor DPI awareness, so the game gets real pixels on scaled desktops
        using SetContext = BOOL(WINAPI*)(HANDLE);
        if (auto set = reinterpret_cast<SetContext>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetProcessDpiAwarenessContext")))
            set(reinterpret_cast<HANDLE>(-4));
        else
            SetProcessDPIAware();
    }

    // The display mode list of the renderer, moved out of its fixed array of 256 entries (92 bytes each). 8192 is
    // what the per-mode index table behind the array holds (the game clears 0x4000 bytes for it, 2 bytes per mode).
    constexpr uint32_t MaxDisplayModes = 8192;
    constexpr uint32_t DisplayModeSize = 92;
    alignas(16) uint8_t DisplayModes[MaxDisplayModes * DisplayModeSize];

    // lea r32, [base + disp32] (6 bytes) -> mov r32, imm32 + nop
    void LoadAddressInstead(void* lea, uintptr_t address)
    {
        auto modrm = *(static_cast<uint8_t*>(lea) + 1);
        injector::WriteMemory<uint8_t>(lea, 0xB8 + ((modrm >> 3) & 7), true);
        injector::WriteMemory<uint32_t>(static_cast<uint8_t*>(lea) + 1, address, true);
        injector::WriteMemory<uint8_t>(static_cast<uint8_t*>(lea) + 5, 0x90, true);
    }
}

class Display
{
public:
    Display()
    {
        WFP::onInitEvent() += []()
        {
            SetDpiAwareness();

            // Display mode list overflow (crash on monitors with 256 or more display modes)
            //
            // The renderer stores the display modes it enumerates in a fixed array of 256 entries (92 bytes each),
            // with the mode count right behind it. The enumeration fills that array from a linked list without ever
            // checking the bound, and it counts every accepted mode, so on a machine where more than 256 modes pass
            // the filter (%8 width, >= 640x480) entry 257 overwrites the count, the string tables behind it get
            // corrupted by the rest and the code after the copy walks them with a garbage count - the reported
            // startup crash (AV reading a bogus pointer in the strcmp of the unique resolution table).
            //
            // The array moves to a buffer of the fix with room for 8192 modes, so every mode is kept. Six instructions
            // address it, all as lea reg, [renderer + offset]: the copy from the list, the build of the resolution and
            // refresh rate tables, the resolution switch, the refresh rate switch and the closest mode lookup (2, at
            // the entry's height field). They load the buffer's address instead. The count stays where it is (nothing
            // else relies on it sitting behind the array), and the unique resolution (256) and refresh rate (32)
            // tables already check their own bounds. The enumeration starts at (count - capacity) when there are
            // more modes than the buffer holds; D3D returns them in ascending order, so the largest ones are kept.
            // If the instructions aren't all found the array stays in place and only its 256 largest modes are kept.
            uint32_t displayModeCapacity = 256;
            auto modeArray = hook::pattern("8D ? E4 40 45 00");
            auto modeArrayHeight = hook::pattern("8D ? 38 41 45 00");
            if (modeArray.size() == 4 && modeArrayHeight.size() == 2)
            {
                modeArray.for_each_result([](hook::pattern_match match) { LoadAddressInstead(match.get<void>(0), reinterpret_cast<uintptr_t>(DisplayModes)); });
                modeArrayHeight.for_each_result([](hook::pattern_match match) { LoadAddressInstead(match.get<void>(0), reinterpret_cast<uintptr_t>(DisplayModes) + 84); });
                displayModeCapacity = MaxDisplayModes;
            }
            else
                OutputDebugStringA("ResidentEvil6.FusionFix: display mode array references not found, keeping the 256 largest modes\n");

            static uint32_t DisplayModeCapacity = displayModeCapacity;
            auto pattern = hook::pattern("89 6D 04 89 44 24 20 3B F0 0F 86");
            if (PatternAvailable(pattern, "ResidentEvil6.FusionFix: Display"))
            {
                static auto DisplayModeListStartHook = safetyhook::create_mid(pattern.get_first(3), [](SafetyHookContext& regs)
                {
                    regs.eax = regs.esi > DisplayModeCapacity ? regs.esi - DisplayModeCapacity : 0;
                });
            }

            // MAIN.PauseOnFocusLoss: the game pauses when its window loses the focus; 0 keeps it running
            pPauseOnFocusLoss = &FixSettings().Int("MAIN.PauseOnFocusLoss");
            pattern = hook::pattern("E8 ? ? ? ? 8B 54 24 6C 52");
            if (PatternAvailable(pattern, "ResidentEvil6.FusionFix: Display"))
            {
                SetActive = (decltype(SetActive))injector::GetBranchDestination(pattern.get_first()).as_int();
                injector::MakeCALL(pattern.get_first(), SetActiveOnFocus, true);
            }

            // The game's Aspect option fits a 4:3, 16:9 or 16:10 image into the screen; the fix always uses the screen's
            // own aspect (the option's DEFAULT), on which everything else of the fix is based
            for (auto ref : { "0F 84 ? ? ? ? 48 ? ? 48 ? ? 89 8E", "0F 84 ? ? ? ? 48 ? ? 48 ? ? 89 AE" })
            {
                hook::pattern(ref).for_each_result([](hook::pattern_match match)
                {
                    injector::MakeNOP(match.get<void>(0), 12, true);
                });
            }

            // MAIN.WindowMode: the window when the game's own Full Screen option is off; 1 = with a frame, 2 = borderless.
            // A framed window as big as the screen is placed with its frame outside the screen; no rounded corners.
            {
                WindowedModeWrapper::bBorderlessWindowed = FixSettings().Int("MAIN.WindowMode") != 1;
                IATHook::Replace(GetModuleHandleA(NULL), "USER32.DLL",
                    std::forward_as_tuple("CreateWindowExA", WindowedModeWrapper::CreateWindowExA_Hook),
                    std::forward_as_tuple("CreateWindowExW", WindowedModeWrapper::CreateWindowExW_Hook),
                    std::forward_as_tuple("SetWindowLongA", WindowedModeWrapper::SetWindowLongA_Hook),
                    std::forward_as_tuple("SetWindowLongW", WindowedModeWrapper::SetWindowLongW_Hook),
                    std::forward_as_tuple("AdjustWindowRect", WindowedModeWrapper::AdjustWindowRect_Hook),
                    std::forward_as_tuple("AdjustWindowRectEx", WindowedModeWrapper::AdjustWindowRectEx_Hook),
                    std::forward_as_tuple("SetWindowPos", WindowedModeWrapper::SetWindowPos_Hook)
                );
            }
        };
    }
} Display;

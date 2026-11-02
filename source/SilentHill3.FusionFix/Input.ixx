module;

#include <stdafx.h>
#include "Shared/SilentHill/Gamepad.hpp"
#include "FixSettings.hpp"
#pragma warning(disable: 4996)   // sprintf into the game's buffer

export module Input;

import ComVars;
import Camera;
import ButtonIcons;

// Controllers. Pads come through Xidi with the "SilentHill3" mapper of Xidi.ini, which arranges the pad like the
// game's built-in default bindings (a PS2 pad on a USB adapter), so a missing, default or customised key.ini all
// work without the fix rewriting it:
//
//   A / RT  Action, Enter            B       Run, Cancel             X       Flashlight (Zoom on the map)
//   Y       Map                      LB/RB   Step left/right         LT      Ready weapon
//   Back    Inventory, Skip          Start   Pause                   LS      Supplies      RS    Search view
//   Left stick / d-pad: move         Right stick: camera (search view with the classic camera)
//
// The module fixes the analog sticks (the game stacks two square dead zones of 25 % and 37.5 % and doesn't rescale,
// so the stick does nothing and then jumps to 43 %), gives the button configuration screen pad button names
// instead of "btn02", passes the game's rumble to Xidi with a usable small motor, and hands the right stick to
// the modern camera.

namespace
{
    // DIJOYSTATE button index of each pad button with the SilentHill3 mapper
    constexpr int ButtonY = 0, ButtonA = 1, ButtonB = 2, ButtonX = 3, ButtonRS = 4, ButtonLT = 5, ButtonLB = 6, ButtonRB = 7,
        ButtonBack = 8, ButtonStart = 9, ButtonLS = 10, ButtonZoom = 11;



    float SmallThreshold = 0.02f;
    SafetyHookMid SticksHook, RecordHook;
    SafetyHookInline PollHook;
    int(__cdecl* DrawBindingField)(int, int, int, const char*, int) = nullptr;
    uint16_t* (__cdecl* EncodeBinding)(const char*) = nullptr;
    void(__cdecl* SizeBinding)(int, int) = nullptr;

    int __cdecl DrawBinding(int x, int y, int colour, const char* text, int alignment)
    {
        // Keep the native keyboard/action/controller columns separate, including custom bindings.
        const auto encoded = std::string("\\h") + (text ? text : "");
        for (int size = 16; size >= 8; --size)
        {
            SizeBinding(size, 28);
            int widths[8]{};
            SH3Icons::Measure(widths, EncodeBinding(encoded.c_str()));
            if (widths[0] <= 128)
                break;
        }
        int result = DrawBindingField(x, y, colour, text, alignment);
        SizeBinding(16, 28);
        return result;
    }

    // Radial dead zone with rescaling for one stick given as 0..1 per axis (0.5 = centre)
    void FixStick(float& x01, float& y01, float deadzone)
    {
        float x = x01 * 2.0f - 1.0f, y = y01 * 2.0f - 1.0f;
        float length = std::sqrt(x * x + y * y);
        if (length <= deadzone)
        {
            x01 = y01 = 0.5f;
            return;
        }
        // From the edge of the dead zone up, the stick starts just above the game's own threshold
        constexpr float Start = 0.04f;
        float scaled = Start + (1.0f - Start) * std::min(1.0f, (length - deadzone) / (1.0f - deadzone));
        x = x / length * scaled;
        y = y / length * scaled;
        x01 = std::clamp((x + 1.0f) * 0.5f, 0.0f, 1.0f);
        y01 = std::clamp((y + 1.0f) * 0.5f, 0.0f, 1.0f);
    }

    // The button configuration screen prints joystick buttons with sprintf("btn%.2d", index + 1)
    int __cdecl FormatElement(char* buffer, const char* format, int number)
    {
        if (format)
        {
            if (strcmp(format, "pov%d") == 0)
                return sprintf(buffer, "%s", SH3Icons::Texture ? "\\h\\x254\\_208" : "D-Pad");
            if (strcmp(format, "btn%.2d") == 0)
            {
                static const SHPad::LayoutButton names[] =
                {
                    SHPad::LayoutY, SHPad::LayoutA, SHPad::LayoutB, SHPad::LayoutX, SHPad::LayoutRS, SHPad::LayoutLT,
                    SHPad::LayoutLB, SHPad::LayoutRB, SHPad::LayoutBack, SHPad::LayoutStart, SHPad::LayoutLS, SHPad::LayoutX,
                };
                int index = number - 1;
                if (index >= 0 && index < int(std::size(names)))
                {
                    constexpr int images[] = { 3, 0, 1, 2, 9, 6, 4, 5, 10, 11, 8, 2 };
                    if (SH3Icons::Texture)
                        return sprintf(buffer, "\\h\\x254\\_%d", 192 + images[index]);
                    return sprintf(buffer, "%s", SHPad::ButtonName(names[index]));
                }
                return sprintf(buffer, "Button %d", number);
            }
        }
        return vsprintf(buffer, format, reinterpret_cast<va_list>(&number));
    }

    // Rumble: the game keeps two motor slots in logical pad 0 and drives them through DirectInput effects; the
    // small motor's strength is hard-coded to 16/255, which nobody feels. Xidi runs each motor at the stronger of
    // the game's effect and this request.
    void UpdateRumble()
    {
        if (!pPads)
            return;
        uint8_t* pad = pPads;
        auto slot = [&](int motor) { return pad + 0x124 + 16 * motor; };
        bool weakOn = slot(0)[14] && slot(0)[12];
        float strong = slot(1)[14] ? slot(1)[12] / 255.0f : 0.0f;
        float weak = weakOn ? 0.45f : 0.0f;
        static float lastWeak = -1, lastStrong = -1;
        if (weak != lastWeak || strong != lastStrong)
        {
            SHPad::Vibrate(strong, weak);
            lastWeak = weak;
            lastStrong = strong;
        }
    }
}

class Input
{
public:
    Input()
    {
        WFP::onInitEvent() += []()
        {
            SHPad::Layout layout;
            layout.rightX = DIJOFS_Z;
            layout.rightY = DIJOFS_RZ;
            layout.ltAxis = layout.rtAxis = -1;
            int buttons[16] = { ButtonA, ButtonB, ButtonX, ButtonY, ButtonLB, ButtonRB, ButtonBack, ButtonStart, ButtonLS, ButtonRS, ButtonLT, -1, -1, -1, -1, -1 };
            std::copy(std::begin(buttons), std::end(buttons), layout.button);
            SHPad::Install(layout);
            SH3Icons::Init();
            auto updateIcons = []() { SHPad::SetIconFamily(FixSettings().Int("MAIN.GamepadIcons")); };
            updateIcons();
            FixSettings().OnChange("MAIN.GamepadIcons", updateIcons);

            // The input library's own 25 % square dead zone goes; the fix applies a radial one
            auto axisNormalise = hook::pattern("8B 44 24 04 2D 00 80 00 00 89 44 24 04 99 33 C2 2B C2 3D 00 20 00 00 7D 07");
            injector::WriteMemory<uint32_t>(axisNormalise.get_first(0x13), 0x100, true);

            // The game's 37.5 % per-axis thresholds become small ones
            auto thresholds = hook::pattern("D9 05 ? ? ? ? D9 E1 84 C0 ? ? ? ? ? ? DF E0 75 1F F6 C4 05 7A 0A C7 05 ? ? ? ? 00 00 00 00 D9 05 ? ? ? ? D9 E1 ? ? ? ? ? ? EB 53");
            for (int offset : { 0x0C, 0x2D, 0x42, 0x63, 0x82, 0xA1, 0xC0 })
                injector::WriteMemory(thresholds.get_first<float*>(offset), &SmallThreshold, true);

            auto sticks = hook::pattern("6A 12 6A 00 E8 ? ? ? ? D9 1D ? ? ? ? 6A 13 6A 00 E8 ? ? ? ? D9 1D ? ? ? ? 6A 10 6A 00 E8");
            fStickLX.SetAddress(*sticks.get_first<float*>(0x0B));
            fStickLY.SetAddress(*sticks.get_first<float*>(0x1A));
            fStickRX.SetAddress(*sticks.get_first<float*>(0x29));
            fStickRY.SetAddress(*sticks.get_first<float*>(0x38));
            SticksHook = safetyhook::create_mid(sticks.get_first(0x3C), [](SafetyHookContext&)
            {
                FixStick(fStickLX, fStickLY, 0.18f);
                FixStick(fStickRX, fStickRY, 0.18f);
                // The modern camera owns the right stick
                if (IsModernCameraActive())
                    fStickRX = fStickRY = 0.5f;
            });

            // The modern camera replaces the search view
            auto record = hook::pattern("51 56 57 BF ? ? ? ? EB 06 8D 9B 00 00 00 00 8D 47 C0 B9 10 00 00 00 8B F0 F3 A5");
            auto sampler = hook::pattern("E8 ? ? ? ? 8B 15 ? ? ? ? 53 52 57 E8 ? ? ? ? 83 C4 0C 83 F8 FF A3");
            aPlayerInput.SetAddress(*record.get_first<uint8_t*>(4) - 0x240);
            RecordHook = safetyhook::create_mid(sampler.get_first(5), [](SafetyHookContext&)
            {
                if (IsModernCameraActive())
                    (*aPlayerInput)[0x06] = (*aPlayerInput)[0x2E] = 0;
            });

            // Pad button names in the button configuration screen
            auto names = hook::pattern("2B 0C C5 ? ? ? ? 8B 14 C5 ? ? ? ? 41 51 52 56 E8");
            injector::MakeCALL(names.get_first(0x12), &FormatElement, true);
            auto analogNames = hook::pattern("83 F9 06 7C 14 83 C1 FB 51 68 ? ? ? ? 56 E8 ? ? ? ? 83 C4 0C EB 11 8B 04 8D");
            static const char* labels[6] = { "LS X", "LS Y", "RS X", "RX", "RY", "RS Y" };
            auto table = *analogNames.get_first<const char**>(0x1C);
            for (int i = 0; i < 6; ++i)
                injector::WriteMemory(&table[i], labels[i], true);

            auto bindingField = hook::pattern("6A 00 8D 4C 24 30 51 55 53 68 70 01 00 00 E8");
            auto encodeBinding = hook::pattern("83 EC 0C 8B 44 24 10 53 8A 18 55 56");
            auto sizeBinding = hook::pattern("A1 ? ? ? ? 66 8B 4C 24 08 0D 00");
            if (SH3Icons::MeasureHook)
            {
                EncodeBinding = reinterpret_cast<decltype(EncodeBinding)>(encodeBinding.get_first());
                SizeBinding = reinterpret_cast<decltype(SizeBinding)>(sizeBinding.get_first());
                DrawBindingField = reinterpret_cast<decltype(DrawBindingField)>(injector::GetBranchDestination(bindingField.get_first(14)).as_int());
                injector::MakeCALL(bindingField.get_first(14), &DrawBinding, true);
            }

            // Rumble after every input poll
            auto poll = hook::pattern("A0 ? ? ? ? 84 C0 C6 05 ? ? ? ? 00 0F 84 ? ? ? ? 56 57 E8");
            auto pads = hook::pattern("8B 44 24 04 8B 0D ? ? ? ? 69 C0 58 01 00 00 03 C1 8A 0D ? ? ? ? 84 C9 74 0B 85 C0 74 07 8B 80 DC 00 00 00");
            pPads.SetAddress(*pads.get_first<uint8_t**>(6));
            PollHook = safetyhook::create_inline(poll.get_first(), static_cast<char(__cdecl*)()>([]() -> char
            {
                char result = PollHook.ccall<char>();
                UpdateRumble();
                return result;
            }));
        };
    }
} Input;

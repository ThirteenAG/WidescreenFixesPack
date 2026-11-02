module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/PadInput.hpp"
#include "Shared/SplinterCell/PromptCache.hpp"

export module Gamepad;

import ComVars;

// The last device used (pad or keyboard and mouse), the pad's button names in the training prompts, and the pad's
// camera bindings. The pad reaches the game through Xidi as DirectInput buttons Joy1-16 and axes.
namespace
{
    std::atomic<bool> PadIsLastDevice = false;
    std::atomic<uint64_t> PromptRevision = 1;
    thread_local SplinterCell::PromptCache Prompts;

    using Family = PadInput::Family;

    Family CurrentFamily()
    {
        return PadInput::ResolveFamily(FixSettings().Int("MAIN.GamepadIcons"));
    }

    // DirectInput button 1-16 as Xidi maps the pad
    std::wstring MakeButtonName(int button, Family family)
    {
        static const wchar_t* xbox[] = { L"A", L"B", L"X", L"Y", L"LB", L"RB", L"LT", L"RT", L"Back", L"Start", L"LS", L"RS" };
        static const wchar_t* playstation[] = { L"Cross", L"Circle", L"Square", L"Triangle", L"L1", L"R1", L"L2", L"R2", L"Share", L"Options", L"L3", L"R3" };
        static const wchar_t* nintendo[] = { L"B", L"A", L"Y", L"X", L"L", L"R", L"ZL", L"ZR", L"-", L"+", L"L Stick", L"R Stick" };
        static const wchar_t* dpad[] = { L"D-pad Up", L"D-pad Down", L"D-pad Left", L"D-pad Right" };
        if (button >= 13 && button <= 16)
            return dpad[button - 13];
        if (button < 1 || button > 12)
            return L"?";
        switch (family)
        {
            case Family::PlayStation:
                return playstation[button - 1];
            case Family::Nintendo:
                return nintendo[button - 1];
            default:
                return xbox[button - 1];
        }
    }

    void RefreshPrompts()
    {
        const auto revision = PromptRevision.load(std::memory_order_acquire);
        if (Prompts.Current(revision))
            return;
        Prompts.Refresh(revision,
            [](int button) { return FixSettings().String("INPUT.Joy" + std::to_string(button)); },
            [family = CurrentFamily()](int button) { return MakeButtonName(button, family); });
    }

    const std::wstring& ButtonName(int button)
    {
        return Prompts.ButtonName(button);
    }

    std::wstring Button(std::string_view command)
    {
        const int button = Prompts.BoundButton(command);
        return button ? L"(" + ButtonName(button) + L")" : std::wstring{};
    }

    void ReplaceAll(std::wstring& text, std::wstring_view from, std::wstring_view to)
    {
        for (size_t at = text.find(from); at != std::wstring::npos; at = text.find(from, at + to.size()))
            text.replace(at, from.size(), to);
    }

    // Training texts name keyboard keys and the mouse
    bool PadPrompt(std::wstring& text)
    {
        if (text.find(L'(') == std::wstring::npos && text.find(L"mouse") == std::wstring::npos && text.find(L"Space bar") == std::wstring::npos)
            return false;
        const auto original = text;
        ReplaceAll(text, L"Move the mouse around", L"Move the right stick");
        ReplaceAll(text, L"Use (W) to move Forward. Use (S) to move Backwards. Adjust Sam's speed with the mouse wheel. Wheeling up will accelerate. Wheeling down will decelerate.",
            L"Use the left stick to move. The further you push it, the faster Sam moves.");
        ReplaceAll(text, L"Press (W) to move forward. Press (S) to move backward. Adjust Sam's speed with the mouse wheel. Scrolling up will accelerate. Scrolling down will decelerate.",
            L"Use the left stick to move. The further you push it, the faster Sam moves.");
        ReplaceAll(text, L"Use the accelerate key (Mouse Wheel Up) to increase Sam's speed. Use the decelerate key (Mouse Wheel Down) to decrease Sam's speed.",
            L"Push the left stick further to increase Sam's speed and less to decrease it.");
        ReplaceAll(text, L"press the correct movement key (W or S or A or D by default)", L"move the left stick in the correct direction");
        ReplaceAll(text, L"press the same key repeatedly", L"move it the same way repeatedly");
        ReplaceAll(text, L"use your mouse to naviguate", L"use the stick to navigate");
        struct Token
        {
            std::wstring_view key;
            std::string_view command;
            int fallback;
        };
        static constexpr Token tokens[] =
        {
            { L"(Left mouse button)", "Fire", 8 }, { L"(Left Mouse)", "Fire", 8 },
            { L"(левая кнопка мыши)", "Fire", 8 },
            { L"(правая кнопка мыши)", "AltFire", 7 },
            { L"(Space Bar)", "Interaction", 1 }, { L"(Space)", "Interaction", 1 },
            { L"(пробел)", "Interaction", 1 },
            { L"(Shift)", "Jump", 4 }, { L"(C)", "Duck", 2 }, { L"(Q)", "BackToWall", 5 },
            { L"(X)", "ResetCamera", 12 }, { L"(Ctrl)", "QuickInventory", 6 },
            { L"(CRTL)", "QuickInventory", 6 },
            { L"(Z)", "SwitchROF", 11 },
            { L"(ESC)", "", 10 }, { L"(Esc)", "", 10 }, { L"(2)", "DPadRight", 16 },
            { L"(E)", "Scope", 3 }
        };
        std::wstring result;
        for (size_t at = 0; at < text.size();)
        {
            const auto token = std::find_if(std::begin(tokens), std::end(tokens), [&](const Token& entry)
            {
                if (entry.key == L"(Z)" && !std::wstring_view(text).substr(0, at).ends_with(L"В режиме стрельбы нажатие "))
                    return false;
                if (entry.key == L"(E)")
                {
                    const auto before = std::wstring_view(text).substr(0, at);
                    if (!before.ends_with(L"Equip key ") && !before.ends_with(L"'Взять в руки' ") &&
                        !before.ends_with(L"пистолет в руки "))
                        return false;
                }
                return std::wstring_view(text).substr(at).starts_with(entry.key);
            });
            if (token == std::end(tokens))
                result += text[at++];
            else
            {
                const auto button = token->command.empty() ? L"(" + ButtonName(token->fallback) + L")" : Button(token->command);
                result += button.empty() ? token->key : std::wstring_view(button);
                at += token->key.size();
            }
        }
        text = std::move(result);
        if (const auto button = Button("Interaction"); !button.empty())
            ReplaceAll(text, L"Space bar to continue", button + L" to continue");
        return text != original;
    }

    std::wstring Prompt(const wchar_t* text)
    {
        if (!text || !PadIsLastDevice.load(std::memory_order_relaxed))
            return {};
        RefreshPrompts();
        return Prompts.Prompt(text, PadPrompt);
    }

    void(__fastcall* DrawTextAligned)(void*, void*, const wchar_t*, int, float, float) = nullptr;
    void __fastcall DrawTextAlignedHook(void* canvas, void* edx, const wchar_t* text, int align, float scaleX, float scaleY)
    {
        const auto prompt = Prompt(text);
        DrawTextAligned(canvas, edx, prompt.empty() ? text : prompt.c_str(), align, scaleX, scaleY);
    }

    int(__fastcall* GetNumberOfLines)(void*, void*, const wchar_t*, float) = nullptr;
    int __fastcall GetNumberOfLinesHook(void* canvas, void* edx, const wchar_t* text, float scale)
    {
        const auto prompt = Prompt(text);
        return GetNumberOfLines(canvas, edx, prompt.empty() ? text : prompt.c_str(), scale);
    }

    // "Axis aTurn DeadZone=0.3" with its speed multiplied
    std::string ScaledAxis(const std::string& binding, float factor)
    {
        if (binding.rfind("Axis ", 0) != 0)
            return binding;
        auto result = binding;
        float speed = 1.0f;
        auto at = result.find("Speed=");
        if (at != std::string::npos)
        {
            size_t end = at + 6;
            while (end < result.size() && result[end] != ' ')
                ++end;
            speed = static_cast<float>(atof(result.substr(at + 6, end - at - 6).c_str()));
            result.erase(at, end - at);
            while (at < result.size() && result[at] == ' ')
                result.erase(at, 1);
            if (!result.empty() && result.back() == ' ')
                result.pop_back();
        }
        auto name = result.find(' ', 5);
        if (name == std::string::npos)
            name = result.size();
        result.insert(name, std::format(" Speed={:.3f}", speed * factor));
        return result;
    }

    std::atomic<bool> BindingsChanged = false;
}

export namespace Gamepad
{
    // Every input event of the game's viewport (EInputKey, EInputAction)
    void OnInput(int key, int action, float value)
    {
        constexpr int IST_Press = 1, IST_Axis = 4;
        const bool padButton = key >= 0xC8 && key <= 0xD7;
        const bool padAxis = (key >= 0xE0 && key <= 0xE3) || key == 0xE8 || key == 0xE9;
        if (padButton && action == IST_Press)
            PadIsLastDevice = true;
        else if (padAxis && action == IST_Axis && std::abs(value) > 0.5f)
            PadIsLastDevice = true;
        else if (key > 0 && key < 0xC8 && key != VK_LWIN && key != VK_RWIN && action == IST_Press)
            PadIsLastDevice = false;
        else if ((key == 0xEC || key == 0xED) && action == IST_Press)
            PadIsLastDevice = false;
        else if ((key == 0xE4 || key == 0xE5) && action == IST_Axis && std::abs(value) > 1.0f)
            PadIsLastDevice = false;
    }

    // Keyboard or mouse messages of the game's window
    void OnKeyboardMouse()
    {
        PadIsLastDevice = false;
    }

    bool IsPadLastDevice()
    {
        return PadIsLastDevice;
    }

    // The pad's look bindings with MAIN.GamepadCameraSpeed and MAIN.InvertGamepadY
    std::string TurnBinding()
    {
        return ScaledAxis(FixSettings().String("INPUT.JoyZ"), std::clamp(FixSettings().Float("MAIN.GamepadCameraSpeed"), 0.25f, 3.0f));
    }

    std::string LookUpBinding()
    {
        const float invert = FixSettings().Bool("MAIN.InvertGamepadY") ? -1.0f : 1.0f;
        return ScaledAxis(FixSettings().String("INPUT.JoyV"), invert * std::clamp(FixSettings().Float("MAIN.GamepadCameraSpeed"), 0.25f, 3.0f));
    }

    // True once after the look settings changed
    bool TakeBindingsChanged()
    {
        return BindingsChanged.exchange(false);
    }

    void Init()
    {
        for (const auto& setting : FixSettings().GetDescription().settings)
            if (setting.id == "MAIN.GamepadIcons" || setting.id == "MAIN.TextLanguage" || setting.id.starts_with("INPUT.Joy"))
                FixSettings().OnChange(setting.id, []() { PromptRevision.fetch_add(1, std::memory_order_release); });

        for (auto id : { "MAIN.GamepadCameraSpeed", "MAIN.InvertGamepadY", "INPUT.JoyZ", "INPUT.JoyV" })
            FixSettings().OnChange(id, []() { BindingsChanged = true; });
    }

    // The HUD's text drawing (training prompts)
    void InitEchelonHUD()
    {
        auto originals = IATHook::Replace(GetModuleHandle(L"EchelonHUD"), "Echelon.dll",
            std::forward_as_tuple("?DrawTextAligned@UECanvas@@QAEXPBGW4ETextAligned@@MM@Z", reinterpret_cast<void*>(DrawTextAlignedHook)),
            std::forward_as_tuple("?GetNumberOfLines@UECanvas@@QAEHPBGM@Z", reinterpret_cast<void*>(GetNumberOfLinesHook)));
        DrawTextAligned = reinterpret_cast<decltype(DrawTextAligned)>(originals.at("?DrawTextAligned@UECanvas@@QAEXPBGW4ETextAligned@@MM@Z").get());
        GetNumberOfLines = reinterpret_cast<decltype(GetNumberOfLines)>(originals.at("?GetNumberOfLines@UECanvas@@QAEHPBGM@Z").get());
    }
}

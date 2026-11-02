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

    const wchar_t* (__cdecl* GetLanguage)(int) = nullptr;

    std::string Language()
    {
        const auto& chosen = FixSettings().String("MAIN.TextLanguage");
        if (!chosen.empty() && chosen != "auto")
            return chosen;
        const auto language = GetLanguage ? GetLanguage(0) : nullptr;
        if (language)
        {
            if (!_wcsicmp(language, L"fra") || !_wcsicmp(language, L"fre")) return "fr";
            if (!_wcsicmp(language, L"deu")) return "de";
            if (!_wcsicmp(language, L"ita")) return "it";
            if (!_wcsicmp(language, L"esp") || !_wcsicmp(language, L"spa")) return "es";
        }
        return "en";
    }

    std::wstring Text(const std::string& language, const std::string& key)
    {
        thread_local std::map<std::string, asi_settings::Texts> languages;
        auto found = languages.find(language);
        if (found == languages.end())
        {
            auto texts = FixSettings().GetTexts();
            texts.Select(language);
            found = languages.emplace(language, std::move(texts)).first;
        }
        thread_local std::map<std::pair<std::string, std::string>, std::wstring> cache;
        const auto id = std::make_pair(language, key);
        if (const auto cached = cache.find(id); cached != cache.end())
            return cached->second;
        const auto value = found->second.Resolve("@padPrompt." + key);
        const int length = MultiByteToWideChar(CP_UTF8, 0, value.data(), int(value.size()), nullptr, 0);
        std::wstring wide(length, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, value.data(), int(value.size()), wide.data(), length);
        cache.emplace(id, wide);
        return wide;
    }

    // DirectInput button 1-16 as Xidi maps the pad
    std::wstring MakeButtonName(int button, Family family, const std::string& language)
    {
        static const wchar_t* xbox[] = { L"X", L"B", L"A", L"Y", L"LB", L"RB", L"LT", L"RT", L"Back", L"Start", L"LS", L"RS" };
        static const wchar_t* playstation[] = { L"Square", L"Circle", L"Cross", L"Triangle", L"L1", L"R1", L"L2", L"R2", L"Share", L"Options", L"L3", L"R3" };
        static const wchar_t* nintendo[] = { L"Y", L"A", L"B", L"X", L"L", L"R", L"ZL", L"ZR", L"-", L"+", L"L Stick", L"R Stick" };
        if (button >= 13 && button <= 16)
            return Text(language, "dpad" + std::to_string(button - 13));
        if (button < 1 || button > 12)
            return L"?";
        switch (family)
        {
            case Family::PlayStation:
                if (button <= 4)
                {
                    static const char* shapes[] = { "square", "circle", "cross", "triangle" };
                    return Text(language, shapes[button - 1]);
                }
                return playstation[button - 1];
            case Family::Nintendo:
                if (button == 11 || button == 12)
                    return Text(language, button == 11 ? "leftStick" : "rightStick");
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
            [family = CurrentFamily(), language = Language()](int button) { return MakeButtonName(button, family, language); });
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

    bool KnownHint(std::wstring_view text)
    {
        static const auto hints = []()
        {
            std::set<std::wstring, std::less<>> result;
            for (const auto language : { "en", "fr", "de", "it", "es" })
            {
                const auto catalog = Text(language, "keyboardHints");
                size_t start = 0;
                for (size_t end; (end = catalog.find(L'\x1E', start)) != std::wstring::npos; start = end + 1)
                    result.emplace(catalog.substr(start, end - start));
                result.emplace(catalog.substr(start));
            }
            return result;
        }();
        const auto first = text.find_first_not_of(L" \t\r\n");
        const auto last = text.find_last_not_of(L" \t\r\n");
        return first != std::wstring_view::npos && hints.contains(text.substr(first, last - first + 1));
    }

    // Replace native keyboard tokens in one pass so a new face-button name cannot become another key.
    bool PadPrompt(std::wstring& text)
    {
        const auto original = text;
        const bool known = KnownHint(text);
        for (const auto language : { "en", "fr", "de", "it", "es" })
        {
            for (const auto key : { "move", "look", "inventory", "continue", "pick", "pickStart", "wall" })
            {
                auto replacement = Text(language, key);
                bool bound = true;
                for (const auto& [placeholder, command] : std::array{
                    std::pair{ L"{fire}", "Fire" }, std::pair{ L"{wall}", "XboxBTW" },
                    std::pair{ L"{interact}", "Interaction" } })
                {
                    if (replacement.find(placeholder) == std::wstring::npos)
                        continue;
                    const auto button = Button(command);
                    if (button.empty())
                    {
                        bound = false;
                        break;
                    }
                    ReplaceAll(replacement, placeholder, button);
                }
                if (bound)
                    ReplaceAll(text, Text(language, std::string(key) + ".original"), replacement);
            }
        }
        if (!known)
            return text != original;
        struct Token
        {
            std::wstring_view key;
            std::string_view command;
            int fallback;
        };
        static constexpr Token tokens[] =
        {
            { L"(Shift)", "Jump", 4 }, { L"(Maj)", "Jump", 4 }, { L"(Umschalttaste)", "Jump", 4 }, { L"(Mayús)", "Jump", 4 },
            { L"(Space)", "Interaction", 3 }, { L"(Space Bar)", "Interaction", 3 }, { L"(Espace)", "Interaction", 3 },
            { L"(Leertaste)", "Interaction", 3 }, { L"(Spazio)", "Interaction", 3 }, { L"(Espacio)", "Interaction", 3 },
            { L"(C)", "Duck", 2 }, { L"(Q)", "XboxBTW", 11 }, { L"(E)", "Scope", 1 },
            { L"(2)", "NightVision", 16 }, { L"(3)", "ThermalVision", 14 }, { L"(V)", "Whistle", 5 },
            { L"(Ctrl)", "QuickInventory", 6 }, { L"(Strg)", "QuickInventory", 6 }, { L"(Tab)", "XboxNextGadget", 15 },
            { L"(F1)", "", 9 }, { L"(ESC)", "", 10 }, { L"(Esc)", "", 10 }, { L"(Echap)", "", 10 },
            { L"(left mouse button)", "Fire", 8 }, { L"(bouton gauche de la souris)", "Fire", 8 },
            { L"(linke Maustaste)", "Fire", 8 }, { L"(Tasto sinistro mouse)", "Fire", 8 }, { L"(botón izquierdo del ratón)", "Fire", 8 },
            { L"(right mouse button)", "AltFire", 7 }, { L"(bouton droit de la souris)", "AltFire", 7 },
            { L"(rechte Maustaste)", "AltFire", 7 }, { L"(Tasto destro mouse)", "AltFire", 7 },
            { L"(pulsante destro del mouse)", "AltFire", 7 }, { L"(botón derecho del ratón)", "AltFire", 7 },
            { L"(middle mouse button)", "BlackSnipe", 12 }, { L"(bouton central de la souris)", "BlackSnipe", 12 },
            { L"(mittlere Maustaste)", "BlackSnipe", 12 }, { L"(Tasto centrale mouse)", "BlackSnipe", 12 },
            { L"(pulsante centrale del mouse)", "BlackSnipe", 12 }, { L"(botón central del ratón)", "BlackSnipe", 12 }
        };
        std::wstring result;
        for (size_t at = 0; at < text.size();)
        {
            const auto token = std::find_if(std::begin(tokens), std::end(tokens), [&](const Token& entry)
            {
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
        return text != original;
    }

    std::wstring Prompt(const wchar_t* text)
    {
        if (!text || !PadIsLastDevice.load(std::memory_order_relaxed))
            return {};
        RefreshPrompts();
        return Prompts.Prompt(text, PadPrompt);
    }

    SafetyHookInline DrawTextAlignedHook;
    void __fastcall DrawTextAligned(void* canvas, void* edx, const wchar_t* text, int align, float scaleX, float scaleY)
    {
        const auto prompt = Prompt(text);
        DrawTextAlignedHook.unsafe_fastcall(canvas, edx, prompt.empty() ? text : prompt.c_str(), align, scaleX, scaleY);
    }

    SafetyHookInline GetNumberOfLinesHook;
    int __fastcall GetNumberOfLines(void* canvas, void* edx, const wchar_t* text, float scale)
    {
        const auto prompt = Prompt(text);
        return GetNumberOfLinesHook.unsafe_fastcall<int>(canvas, edx, prompt.empty() ? text : prompt.c_str(), scale);
    }

    SafetyHookInline WrappedPrintHook;
    void __cdecl WrappedPrint(void* canvas, int style, int* width, int* height, void* font, int center, const wchar_t* text)
    {
        const auto prompt = Prompt(text);
        WrappedPrintHook.unsafe_ccall(canvas, style, width, height, font, center, prompt.empty() ? text : prompt.c_str());
    }

    SafetyHookInline WrappedPrintScaleHook;
    void __cdecl WrappedPrintScale(void* canvas, int style, float* width, float* height, void* font, const wchar_t* text, float scaleX, float scaleY, DWORD flags)
    {
        const auto prompt = Prompt(text);
        WrappedPrintScaleHook.unsafe_ccall(canvas, style, width, height, font, prompt.empty() ? text : prompt.c_str(), scaleX, scaleY, flags);
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
        GetLanguage = reinterpret_cast<decltype(GetLanguage)>(FindProcAddress(GetModuleHandle(L"Core"), "?GetLanguage@UObject@@SAPB_WH@Z", "?GetLanguage@UObject@@SAPBGH@Z"));
        auto module = GetModuleHandle(L"Echelon");
        auto draw = FindProcAddress(module, "?DrawTextAligned@UECanvas@@QAEXPB_WW4ETextAligned@@MM@Z", "?DrawTextAligned@UECanvas@@QAEXPBGW4ETextAligned@@MM@Z");
        auto measure = FindProcAddress(module, "?GetNumberOfLines@UECanvas@@QAEHPB_WM@Z", "?GetNumberOfLines@UECanvas@@QAEHPBGM@Z");
        auto wrapped = FindProcAddress(GetModuleHandle(L"Engine"), "?WrappedPrint@UCanvas@@IAAXW4ERenderStyle@@AAH1PAVUFont@@HPB_W@Z", "?WrappedPrint@UCanvas@@IAAXW4ERenderStyle@@AAH1PAVUFont@@HPBG@Z");
        auto wrappedScale = FindProcAddress(module, "?WrappedPrintScale@UECanvas@@QAAXW4ERenderStyle@@AAM1PAVUFont@@PB_WMMK@Z", "?WrappedPrintScale@UECanvas@@QAAXW4ERenderStyle@@AAM1PAVUFont@@PBGMMK@Z");
        WrappedPrintHook = safetyhook::create_inline(wrapped, WrappedPrint);
        WrappedPrintScaleHook = safetyhook::create_inline(wrappedScale, WrappedPrintScale);
        DrawTextAlignedHook = safetyhook::create_inline(draw, DrawTextAligned);
        GetNumberOfLinesHook = safetyhook::create_inline(measure, GetNumberOfLines);
    }
}

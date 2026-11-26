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

    // The Joy1-16, JoyX and JoyY commands the game really uses (its user ini), filled in at startup
    std::mutex BindingsMutex;
    std::array<std::string, 18> Bindings;
    std::atomic<uint32_t> CustomBindings{ 0 };
    thread_local unsigned AutomaticBindingDepth = 0;

    constexpr std::pair<const wchar_t*, const char*> DefaultBindings[] =
    {
        { L"Joy1", "Interaction" }, { L"Joy2", "Duck" }, { L"Joy3", "Scope" }, { L"Joy4", "Jump" },
        { L"Joy5", "BackToWall" }, { L"Joy6", "QuickInventory" }, { L"Joy7", "AltFire" }, { L"Joy8", "Fire" },
        { L"Joy9", "" }, { L"Joy10", "" }, { L"Joy11", "SwitchROF" }, { L"Joy12", "ResetCamera" },
        { L"Joy13", "DPadUp" }, { L"Joy14", "DPadDown" }, { L"Joy15", "DPadLeft" }, { L"Joy16", "DPadRight" },
        { L"JoyX", "Axis aStrafe DeadZone=0.3" }, { L"JoyY", "Axis aForward DeadZone=0.3" },
        { L"JoyZ", "Axis aTurn DeadZone=0.3" }, { L"JoyV", "Axis aLookUp DeadZone=0.3" }
    };

    using Family = PadInput::Family;

    Family CurrentFamily()
    {
        return PadInput::ResolveFamily(FixSettings().Int("MAIN.GamepadIcons"));
    }

    constexpr const char* Languages[] = { "en", "fr", "de", "it", "es" };

    const wchar_t* (__cdecl* GetLanguage)() = nullptr;

    std::string Language()
    {
        const auto& chosen = FixSettings().String("MAIN.TextLanguage");
        if (!chosen.empty() && chosen != "auto")
            return chosen;
        const auto language = GetLanguage ? GetLanguage() : nullptr;
        if (language)
        {
            if (!_wcsicmp(language, L"fra")) return "fr";
            if (!_wcsicmp(language, L"deu")) return "de";
            if (!_wcsicmp(language, L"ita")) return "it";
            if (!_wcsicmp(language, L"esp")) return "es";
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
        static const wchar_t* xbox[] = { L"A", L"B", L"X", L"Y", L"LB", L"RB", L"LT", L"RT", L"Back", L"Start", L"LS", L"RS" };
        static const wchar_t* playstation[] = { L"Cross", L"Circle", L"Square", L"Triangle", L"L1", L"R1", L"L2", L"R2", L"Share", L"Options", L"L3", L"R3" };
        static const wchar_t* nintendo[] = { L"B", L"A", L"Y", L"X", L"L", L"R", L"ZL", L"ZR", L"-", L"+", L"L Stick", L"R Stick" };
        static const char* dpad[] = { "dpadUp", "dpadDown", "dpadLeft", "dpadRight" };
        if (button >= 13 && button <= 16)
            return Text(language, dpad[button - 13]);
        if (button < 1 || button > 12)
            return L"?";
        switch (family)
        {
            case Family::PlayStation:
                if (button <= 4)
                {
                    static const char* shapes[] = { "cross", "circle", "square", "triangle" };
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
            [](int button)
            {
                std::lock_guard lock(BindingsMutex);
                return Bindings[button - 1];
            },
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
        if (from.empty())
            return;
        for (size_t at = text.find(from); at != std::wstring::npos; at = text.find(from, at + to.size()))
            text.replace(at, from.size(), to);
    }

    // Training texts that talk about the mouse or the movement keys, replaced as whole sentences in the game's
    // language. An original may list several versions separated by U+001E.
    void ReplaceSentences(std::wstring& text)
    {
        for (const auto language : Languages)
        {
            for (const auto key : { "look", "move", "speed", "lockPicks", "pick", "continue" })
            {
                auto replacement = Text(language, key);
                bool bound = true;
                for (const auto& [placeholder, command] : std::array{
                    std::pair{ L"{interact}", "Interaction" }, std::pair{ L"{inventory}", "QuickInventory" } })
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
                if (!bound)
                    continue;
                const auto originals = Text(language, std::string(key) + ".original");
                size_t start = 0;
                for (size_t end; start <= originals.size(); start = end + 1)
                {
                    end = originals.find(L'\x1E', start);
                    if (end == std::wstring::npos)
                        end = originals.size();
                    ReplaceAll(text, std::wstring_view(originals).substr(start, end - start), replacement);
                }
            }
        }
    }

    // Replace native keyboard tokens in one pass so a new face-button name cannot become another key.
    bool PadPrompt(std::wstring& text)
    {
        const auto original = text;
        ReplaceSentences(text);
        if (text.find(L'(') == std::wstring::npos)
            return text != original;
        struct Token
        {
            std::wstring_view key;
            std::string_view command;
            int fallback;
        };
        static constexpr Token tokens[] =
        {
            { L"(Left mouse button)", "Fire", 8 }, { L"(Left Mouse)", "Fire", 8 },
            { L"(touche gauche de la souris)", "Fire", 8 }, { L"(Touche gauche de la souris)", "Fire", 8 },
            { L"(linke Maustaste)", "Fire", 8 }, { L"(Linke Maustaste)", "Fire", 8 },
            { L"(pulsante sinistro del mouse)", "Fire", 8 }, { L"(Pulsante sinistro del mouse)", "Fire", 8 },
            { L"(pulsante dinistro del mouse)", "Fire", 8 },
            { L"(Botón izquierdo del ratón)", "Fire", 8 }, { L"(Botón Izquierdo Del Ratón)", "Fire", 8 },
            { L"(левая кнопка мыши)", "Fire", 8 },
            { L"(touche droite de la souris)", "AltFire", 7 }, { L"(rechte Maustaste)", "AltFire", 7 },
            { L"(pulsante destro del mouse)", "AltFire", 7 }, { L"(Botón derecho del ratón)", "AltFire", 7 },
            { L"(правая кнопка мыши)", "AltFire", 7 },
            { L"(Space Bar)", "Interaction", 1 }, { L"(Space)", "Interaction", 1 }, { L"(barre espace)", "Interaction", 1 },
            { L"(Leertaste)", "Interaction", 1 }, { L"(Barra spaziatrice)", "Interaction", 1 },
            { L"(Barra espaciadora)", "Interaction", 1 }, { L"(пробел)", "Interaction", 1 },
            { L"(Shift)", "Jump", 4 }, { L"(Umschalt-Taste)", "Jump", 4 }, { L"(Maiusc)", "Jump", 4 }, { L"(Mayús)", "Jump", 4 },
            { L"(C)", "Duck", 2 }, { L"(touche C)", "Duck", 2 }, { L"(C-Taste)", "Duck", 2 },
            { L"(Q)", "BackToWall", 5 }, { L"(X)", "ResetCamera", 12 },
            { L"(Ctrl)", "QuickInventory", 6 }, { L"(CTRL)", "QuickInventory", 6 }, { L"(CRTL)", "QuickInventory", 6 },
            { L"(Strg)", "QuickInventory", 6 },
            { L"(Z)", "SwitchROF", 11 }, { L"(E)", "Scope", 3 }, { L"(2)", "DPadLeft", 15 },
            { L"(ESC)", "", 10 }, { L"(Esc)", "", 10 }
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

    // A converted text drawn through a nested canvas call must not be converted again ("(X)" is a key and a button)
    thread_local int PromptDepth = 0;

    struct PromptScope
    {
        PromptScope() { ++PromptDepth; }
        ~PromptScope() { --PromptDepth; }
    };

    std::wstring Prompt(const wchar_t* text)
    {
        if (!text || PromptDepth > 1 || !PadIsLastDevice.load(std::memory_order_relaxed))
            return {};
        RefreshPrompts();
        return Prompts.Prompt(text, PadPrompt);
    }

    SafetyHookInline DrawTextAlignedHook;
    void __fastcall DrawTextAligned(void* canvas, void* edx, const wchar_t* text, int align, float scaleX, float scaleY)
    {
        PromptScope scope;
        const auto prompt = Prompt(text);
        DrawTextAlignedHook.unsafe_fastcall(canvas, edx, prompt.empty() ? text : prompt.c_str(), align, scaleX, scaleY);
    }

    SafetyHookInline GetNumberOfLinesHook;
    int __fastcall GetNumberOfLines(void* canvas, void* edx, const wchar_t* text, float scale)
    {
        PromptScope scope;
        const auto prompt = Prompt(text);
        return GetNumberOfLinesHook.unsafe_fastcall<int>(canvas, edx, prompt.empty() ? text : prompt.c_str(), scale);
    }

    SafetyHookInline WrappedPrintHook;
    void __cdecl WrappedPrint(void* canvas, int style, int* width, int* height, void* font, int center, const wchar_t* text)
    {
        PromptScope scope;
        const auto prompt = Prompt(text);
        WrappedPrintHook.unsafe_ccall(canvas, style, width, height, font, center, prompt.empty() ? text : prompt.c_str());
    }

    SafetyHookInline WrappedPrintScaleHook;
    void __cdecl WrappedPrintScale(void* canvas, int style, float* width, float* height, void* font, const wchar_t* text, float scaleX, float scaleY, DWORD flags)
    {
        PromptScope scope;
        const auto prompt = Prompt(text);
        WrappedPrintScaleHook.unsafe_ccall(canvas, style, width, height, font, prompt.empty() ? text : prompt.c_str(), scaleX, scaleY, flags);
    }

}

export namespace Gamepad
{
    // Every input event of the game's viewport (EInputKey, EInputAction)
    void OnInput(int key, int action, float value)
    {
        constexpr int IST_Press = 1, IST_Axis = 4;
        // AnalogUp/Down/Left/Right (0xC4-0xC7) are the left stick as keys (UseAnalogAsPad), Joy1-16 follow
        const bool padButton = key >= 0xC4 && key <= 0xD7;
        const bool padAxis = (key >= 0xE0 && key <= 0xE3) || key == 0xE8 || key == 0xE9;
        if (padButton && action == IST_Press)
            PadIsLastDevice = true;
        else if (padAxis && action == IST_Axis && std::abs(value) > 0.5f)
            PadIsLastDevice = true;
        else if (key > 0 && key < 0xC4 && key != VK_LWIN && key != VK_RWIN && action == IST_Press)
            PadIsLastDevice = false;
        else if ((key == 0xEC || key == 0xED) && action == IST_Press)
            PadIsLastDevice = false;
        else if ((key == 0xE4 || key == 0xE5) && action == IST_Axis && value != 0.0f)
        {
            // Only a deliberate move counts, a resting mouse can still report a few counts
            constexpr uint64_t window = 250;
            constexpr float travel = 16.0f;
            static uint64_t windowStart = 0;
            static float moved = 0.0f;
            const auto now = GetTickCount64();
            if (now - windowStart > window)
            {
                windowStart = now;
                moved = 0.0f;
            }
            moved += std::abs(value);
            if (moved > travel)
                PadIsLastDevice = false;
        }
    }

    bool IsPadLastDevice()
    {
        return PadIsLastDevice;
    }

    void SetBinding(int button, const std::string& command)
    {
        if (button < 1 || button > static_cast<int>(std::size(DefaultBindings)))
            return;
        const auto index = static_cast<size_t>(button - 1);
        const auto bit = uint32_t{ 1 } << index;
        if (!command.empty() && _stricmp(command.c_str(), DefaultBindings[index].second))
            CustomBindings.fetch_or(bit);
        else
            CustomBindings.fetch_and(~bit);

        if (index < Bindings.size())
        {
            std::lock_guard lock(BindingsMutex);
            Bindings[index] = command;
        }
        PromptRevision.fetch_add(1, std::memory_order_release);
    }

    struct AutomaticBindingScope
    {
        AutomaticBindingScope() { ++AutomaticBindingDepth; }
        ~AutomaticBindingScope() { --AutomaticBindingDepth; }
        AutomaticBindingScope(const AutomaticBindingScope&) = delete;
        AutomaticBindingScope& operator=(const AutomaticBindingScope&) = delete;
    };

    bool KeepCustomBinding(const wchar_t* command)
    {
        if (!AutomaticBindingDepth || !command)
            return false;
        const auto mask = CustomBindings.load();
        const std::wstring_view text(command);
        const auto name = text.substr(0, text.find_first_of(L" \t"));
        for (size_t i = 0; i < std::size(DefaultBindings); ++i)
        {
            const std::wstring_view key(DefaultBindings[i].first);
            if ((mask & (uint32_t{ 1 } << i)) && name.size() == key.size() && !_wcsnicmp(name.data(), key.data(), key.size()))
                return true;
        }
        return false;
    }

    void Init()
    {
        for (const auto id : { "MAIN.GamepadIcons", "MAIN.TextLanguage" })
            FixSettings().OnChange(id, []() { PromptRevision.fetch_add(1, std::memory_order_release); });
    }

    // The canvas text drawing (training prompts)
    void InitEchelonHUD()
    {
        auto Export = [](const wchar_t* module, const char* name)
        {
            auto address = GetProcAddress(GetModuleHandle(module), name);
            if (!address)
                throw std::runtime_error(std::string("Export not found: ") + name);
            return address;
        };
        GetLanguage = reinterpret_cast<decltype(GetLanguage)>(Export(L"Core", "?GetLanguage@UObject@@SAPBGXZ"));
        WrappedPrintHook = safetyhook::create_inline(Export(L"Engine", "?WrappedPrint@UCanvas@@IAAXW4ERenderStyle@@AAH1PAVUFont@@HPBG@Z"), WrappedPrint);
        WrappedPrintScaleHook = safetyhook::create_inline(Export(L"Echelon", "?WrappedPrintScale@UECanvas@@QAAXW4ERenderStyle@@AAM1PAVUFont@@PBGMMK@Z"), WrappedPrintScale);
        DrawTextAlignedHook = safetyhook::create_inline(Export(L"Echelon", "?DrawTextAligned@UECanvas@@QAEXPBGW4ETextAligned@@MM@Z"), DrawTextAligned);
        GetNumberOfLinesHook = safetyhook::create_inline(Export(L"Echelon", "?GetNumberOfLines@UECanvas@@QAEHPBGM@Z"), GetNumberOfLines);
    }
}

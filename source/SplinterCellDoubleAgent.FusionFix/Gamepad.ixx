module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/PadInput.hpp"

export module Gamepad;

namespace
{
    std::atomic<uint64_t> LabelsRevision = 1;
    const wchar_t* (__cdecl* NativeLanguage)() = nullptr;

    std::string Language()
    {
        const auto& selected = FixSettings().String("MAIN.TextLanguage");
        if (!selected.empty() && selected != "auto")
            return selected;
        const auto native = NativeLanguage ? NativeLanguage() : nullptr;
        if (native)
        {
            if (!_wcsicmp(native, L"fre") || !_wcsicmp(native, L"fra")) return "fr";
            if (!_wcsicmp(native, L"deu")) return "de";
            if (!_wcsicmp(native, L"ita")) return "it";
            if (!_wcsicmp(native, L"spa") || !_wcsicmp(native, L"esp")) return "es";
        }
        return "en";
    }

    // The native DirectInput adapter reorders buttons before dispatching Joy keys.
    std::array<std::wstring, 20> Labels(PadInput::Family family, const std::string& language)
    {
        auto texts = FixSettings().GetTexts();
        texts.Select(language);
        const auto text = [&](const char* key)
        {
            return asi_settings::text::Widen(texts.Resolve(std::string("@padPrompt.") + key));
        };
        std::array<std::wstring, 20> names{ L"Y", L"B", L"A", L"X", L"RB", L"LB", L"LT", L"RT", L"Start", L"LS", L"RS", L"Back" };
        if (family == PadInput::Family::PlayStation)
            names = { text("triangle"), text("circle"), text("cross"), text("square"), L"R1", L"L1", L"L2", L"R2", L"Options", L"L3", L"R3", L"Share" };
        else if (family == PadInput::Family::Nintendo)
            names = { L"X", L"A", L"B", L"Y", L"R", L"L", L"ZL", L"ZR", L"+", text("leftStick"), text("rightStick"), L"-" };
        const char* directions[] = { "dpad0", "dpad1", "dpad2", "dpad3" };
        for (size_t i = 0; i < std::size(directions); ++i)
            names[12 + i] = text(directions[i]);
        names[16] = text("leftStick") + L" X";
        names[17] = text("leftStick") + L" Y";
        names[18] = text("rightStick") + L" X";
        names[19] = text("rightStick") + L" Y";
        for (auto& name : names)
            name = L"(" + name + L")";
        return names;
    }
}

export namespace Gamepad
{

    void Init(const wchar_t* (__cdecl* nativeLanguage)())
    {
        NativeLanguage = nativeLanguage;
        for (auto id : { "MAIN.GamepadIcons", "MAIN.TextLanguage" })
            FixSettings().OnChange(id, []() { LabelsRevision.fetch_add(1, std::memory_order_release); });
        LabelsRevision.fetch_add(1, std::memory_order_release);
    }

    // The game's native key-name path already selects the binding it wants displayed.
    // Input and device ownership remain with the game and Xidi.
    const wchar_t* Label(std::wstring_view key)
    {
        if (!key.starts_with(L"Joy"))
            return nullptr;
        key.remove_prefix(3);
        if (key.starts_with(L" ")) key.remove_prefix(1);
        if (key.empty() || key.size() > 2)
            return nullptr;
        int button = 0;
        if (key == L"X")
        {
            button = 17;
        }
        else if (key == L"Y")
        {
            button = 18;
        }
        else if (key == L"Z")
        {
            button = 19;
        }
        else if (key == L"V")
        {
            button = 20;
        }
        else
        {
            for (const auto c : key)
            {
                if (c < L'0' || c > L'9')
                    return nullptr;
                button = button * 10 + int(c - L'0');
            }
            if (button < 1 || button > 16)
                return nullptr;
        }
        thread_local uint64_t revision = 0;
        thread_local std::map<std::pair<PadInput::Family, std::string>, std::array<std::wstring, 20>> labels;
        thread_local const std::array<std::wstring, 20>* current = nullptr;
        const auto wanted = LabelsRevision.load(std::memory_order_acquire);
        if (revision != wanted)
        {
            const auto id = std::make_pair(PadInput::ResolveFamily(FixSettings().Int("MAIN.GamepadIcons")), Language());
            auto found = labels.find(id);
            if (found == labels.end())
                found = labels.emplace(id, Labels(id.first, id.second)).first;
            current = &found->second;
            revision = wanted;
        }
        return (*current)[button - 1].c_str();
    }
}

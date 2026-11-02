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

    std::array<std::wstring, 20> Labels(PadInput::Family family, const std::string& language)
    {
        auto texts = FixSettings().GetTexts();
        texts.Select(language);
        const auto text = [&](const char* key)
        {
            return asi_settings::text::Widen(texts.Resolve(std::string("@padPrompt.") + key));
        };
        std::array<std::wstring, 20> names{ L"A", L"B", L"X", L"Y", L"LB", L"RB", L"LT", L"RT", L"Back", L"Start", L"LS", L"RS" };
        if (family == PadInput::Family::PlayStation)
            names = { text("cross"), text("circle"), text("square"), text("triangle"), L"L1", L"R1", L"L2", L"R2", L"Share", L"Options", L"L3", L"R3" };
        else if (family == PadInput::Family::Nintendo)
            names = { L"B", L"A", L"Y", L"X", L"L", L"R", L"ZL", L"ZR", L"-", L"+", text("leftStick"), text("rightStick") };
        const char* directions[] = { "dpad0", "dpad2", "dpad3", "dpad1" };
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
        if (!key.starts_with(L"Joy ") || key.size() < 5 || key.size() > 6)
            return nullptr;
        int button = 0;
        for (const auto c : key.substr(4))
        {
            if (c < L'0' || c > L'9')
                return nullptr;
            button = button * 10 + int(c - L'0');
        }
        if (button < 1 || button > 20)
            return nullptr;
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

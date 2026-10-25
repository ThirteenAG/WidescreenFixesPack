#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <thread>
#include <vector>
#include <shellapi.h>
#include "../AsiSettings/asi_settings.hpp"
#include "Text.hpp"

// The fixes' settings in the games' own options menus, built from their ASI Settings description (settings.json,
// see source/Shared/AsiSettings): the pages of the settings shown in menus, the text of a setting's name and value,
// the next value left or right, the presets' rows and the settings file row. Each game's Options module draws them
// in its menu's own way and converts the text (UTF-8) to its font's encoding.
namespace WFPMenu
{
    // A text field of the description: "@KEY" is the fix's text of KEY, anything else is literal
    inline std::string FieldText(const std::string& field)
    {
        if (field.size() >= 2 && field[0] == '@' && field[1] == '@')
            return field.substr(1);
        if (!field.empty() && field[0] == '@')
            return WFPText::Get(field.substr(1));
        return field;
    }

    // The label of a setting's value: its choice, special value or suggestion
    inline const asi_settings::Labelled* ValueLabel(const asi_settings::Setting& setting, const std::string& value)
    {
        bool numeric = !setting.StringValued();
        for (auto list : { &setting.choices, &setting.special, &setting.suggestions })
        {
            for (auto& entry : *list)
            {
                if (asi_settings::detail::JsonEquals(entry.value, value, numeric))
                    return &entry;
            }
        }
        return nullptr;
    }

    // The setting's name; settings that apply at the next start say so
    inline std::string LabelText(const asi_settings::Setting& setting)
    {
        return FieldText(setting.label) + (setting.apply == "restart" ? " " + WFPText::Get("WFP_RESTART") : "");
    }

    // The value in effect, as the menu shows it
    inline std::string ValueText(asi_settings::Plugin& settings, const asi_settings::Setting& setting)
    {
        auto value = settings.Get(setting.id);
        if (setting.type == asi_settings::Type::Bool)
            return WFPText::Get(value != "0" ? "WFP_ON" : "WFP_OFF");
        if (auto label = ValueLabel(setting, value); label && !label->label.empty())
            return FieldText(label->label);
        if (setting.type == asi_settings::Type::Float)
        {
            int decimals = setting.decimals >= 0 ? setting.decimals : (setting.step && *setting.step < 1.0 ? 1 : 0);
            char text[32];
            snprintf(text, sizeof(text), "%.*f", decimals, settings.Float(setting.id));
            return text;
        }
        return value;
    }

    // The value left or right of the one in effect: through the values usable now (choices, or a number's special
    // values and suggestions, in their order), otherwise a step within the range
    inline std::string NextValue(asi_settings::Plugin& settings, const asi_settings::Setting& setting, int direction)
    {
        auto current = settings.Get(setting.id);
        bool listed = setting.type == asi_settings::Type::Bool || setting.type == asi_settings::Type::Choice ||
            !setting.special.empty() || !setting.suggestions.empty();
        if (listed)
        {
            auto values = settings.UsableValues(setting.id);
            if (values.empty())
                return current;
            auto it = std::find(values.begin(), values.end(), current);
            int count = int(values.size());
            // A value that isn't listed (a custom one from the .cfg) continues from the first or the last
            int index = it != values.end() ? int(it - values.begin()) : (direction > 0 ? -1 : 0);
            return values[((index + direction) % count + count) % count];
        }
        if ((setting.type == asi_settings::Type::Float || setting.type == asi_settings::Type::Int) && setting.step)
        {
            double step = *setting.step;
            double value = std::round((std::atof(current.c_str()) + direction * step) / step) * step;
            if (setting.min)
                value = std::max(value, *setting.min);
            if (setting.max)
                value = std::min(value, *setting.max);
            return asi_settings::text::FormatNumber(value);
        }
        return current;
    }

    // Left (-1) or right (+1) on a setting's row; false when nothing changed. Settings that depend on another one being
    // on (enabledIf) keep their value while it's off.
    inline bool Step(asi_settings::Plugin& settings, const asi_settings::Setting& setting, int direction)
    {
        if (!settings.Enabled(setting.id))
            return false;
        auto value = NextValue(settings, setting, direction);
        return value != settings.Get(setting.id) && settings.Set(setting.id, value) == ASI_SETTINGS_OK;
    }

    // Shows the settings file in File Explorer, on a thread of its own so the game goes on while Explorer starts
    inline void ShowSettingsFile(asi_settings::Plugin& settings)
    {
        std::thread([path = settings.CfgPath()]()
        {
            CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
            auto arguments = L"/select,\"" + path.wstring() + L"\"";
            ShellExecuteW(nullptr, L"open", L"explorer.exe", arguments.c_str(), nullptr, SW_SHOWNORMAL);
            CoUninitialize();
        }).detach();
    }

    // A page of the menu: the description's page id, its title (a text field) and its settings
    struct Page
    {
        std::string id;
        std::string title;
        std::vector<const asi_settings::Setting*> settings;
    };

    // The description's pages with the settings shown in menus (visibility "menu"), in their order; pages without
    // such settings are left out
    inline std::vector<Page> Pages(asi_settings::Plugin& settings)
    {
        auto& description = settings.GetDescription();
        std::vector<Page> pages;
        for (auto& page : description.pages)
        {
            Page entry{ page.id, page.title, {} };
            for (auto& setting : description.settings)
            {
                if (setting.page == page.id && setting.visibility == "menu" && setting.HasValue())
                    entry.settings.push_back(&setting);
            }
            std::stable_sort(entry.settings.begin(), entry.settings.end(), [](auto a, auto b) { return a->order < b->order; });
            if (!entry.settings.empty())
                pages.push_back(std::move(entry));
        }
        return pages;
    }

    // The Presets page's rows: the preset row only picks one of the three, the apply row applies it after a second
    // press to confirm
    struct Presets
    {
        enum class ApplyState { Idle, Confirm, Applied, AppliedRestart };

        asi_settings::Plugin* settings = nullptr;
        int shown = -1;             // the preset in the preset row, -1 for "Custom"
        ApplyState apply = ApplyState::Idle;

        static constexpr const char* Names[] = { "WFP_PRESET_VANILLA", "WFP_PRESET_RECOMMENDED", "WFP_PRESET_ENHANCED" };

        // The preset the settings match, -1 for none
        int Current() const
        {
            return int(settings->CurrentPreset());
        }

        // When the page is shown: the preset row starts at the preset in use
        void Reset()
        {
            shown = Current();
            apply = ApplyState::Idle;
        }

        std::string ValueText() const
        {
            return WFPText::Get(shown < 0 ? "WFP_PRESET_CUSTOM" : Names[shown]);
        }

        std::string ApplyText() const
        {
            switch (apply)
            {
                case ApplyState::Confirm:
                    return WFPText::Get("WFP_PRESET_CONFIRM");
                case ApplyState::AppliedRestart:
                    return WFPText::Get("WFP_PRESET_RESTART");
                default:
                    break;
            }
            if (shown < 0)
                return {};
            if (shown == Current())
                return WFPText::Get("WFP_PRESET_INUSE");
            return WFPText::Get("WFP_PRESET_PRESS");
        }

        // Left or right on the preset row
        void Cycle(int direction)
        {
            int count = int(std::size(Names));
            shown = shown < 0 ? (direction > 0 ? 0 : count - 1) : ((shown + direction) % count + count) % count;
            apply = ApplyState::Idle;
        }

        // Accept on the apply row: the first press asks to confirm, the second applies; false when nothing changed
        bool Accept()
        {
            if (shown < 0 || (apply != ApplyState::Confirm && shown == Current()))
                return false;
            if (apply != ApplyState::Confirm)
                apply = ApplyState::Confirm;
            else
                apply = settings->ApplyPreset(asi_settings::Preset(shown)) ? ApplyState::AppliedRestart : ApplyState::Applied;
            return true;
        }

        // Left or right on the apply row, or any other row's message: a pending confirmation is cancelled; true when
        // there was one
        bool Cancel()
        {
            if (apply != ApplyState::Confirm)
                return false;
            apply = ApplyState::Idle;
            return true;
        }
    };
}

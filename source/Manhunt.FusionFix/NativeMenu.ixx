module;

#include <stdafx.h>
#include "common.h"
#include "Shared/RWGTA/FixMenu.hpp"
#include "FixSettings.hpp"
#include <safetyhook.hpp>
#include <filesystem>
#include <map>
#include <set>

export module NativeMenu;

import ComVars;

import NativeMenuABI;
import NativeMenuText;
import NativeMenuView;



export namespace MHMenu
{
    inline int __cdecl SettingsInput();
    inline int __cdecl SettingsDraw();
    inline int __cdecl VideoInput();
    inline int __cdecl VideoDraw();

    inline const std::set<std::string>& JoinedIDs()
    {
        static const std::set<std::string> ids{ "MAIN.FOVFactor", "MAIN.HudAspectRatioConstraint" };
        return ids;
    }

    struct State
    {
        HMODULE module = nullptr;
        bool enabled = false;
        bool opened = false;
        bool refresh = true;
        bool videoOpen = false;
        bool backHeld = false;
        bool enterHeld = false;
        std::string language;
        std::string videoLanguage;
        RWGTAMenu::Model model;
        RWGTAMenu::Model video;
        View view;
        std::map<std::string, std::string> beforeVideo;
        std::map<std::string, std::string> writtenVideo;
        std::set<std::string> messages;
        ULONGLONG restartUntil = 0;
        safetyhook::MidHook defaults;

        bool BackPressed()
        {
            bool held = ABI::Escape() || ABI::FooterBack();
            bool pressed = held && !backHeld;
            backHeld = held;
            return pressed;
        }

        bool EnterPressed()
        {
            bool held = ABI::Enter();
            bool pressed = held && !enterHeld;
            enterHeld = held;
            return pressed;
        }

        void Log(const std::string& message) noexcept
        {
            if (messages.size() >= 32 || !messages.insert(message).second) return;
            OutputDebugStringA(("Manhunt native menu: " + message + "\n").c_str());
        }

        std::wstring Words(const char* key)
        {
            std::wstring result;
            Text::Convert(RWGTAMenu::PluginText(FixSettings(), ABI::Language(), key), result);
            return result;
        }

        void Close()
        {
            opened = false;
            model.Close();
            view = {};
            refresh = true;
        }

        bool Open()
        {
            Close();
            language = ABI::Language();
            model.OpenPlugins(module, language, 32, JoinedIDs(), true);
            opened = model.IsOpen();
            refresh = true;
            return opened && Build();
        }

        bool Build()
        {
            if (!opened) return false;
            if (language != ABI::Language()) return Open();
            if (refresh)
            {
                if (!view.Build(model, ABI::Paused())) { Close(); return false; }
                refresh = false;
            }
            return true;
        }

        void EnsureVideo()
        {
            auto selectedLanguage = ABI::Language();
            if (videoOpen && videoLanguage == selectedLanguage) return;
            video.Close();
            videoLanguage = selectedLanguage;
            video.OpenPlugins(module, videoLanguage, 32, JoinedIDs(), true);
            videoOpen = video.IsOpen();
            beforeVideo.clear();
            writtenVideo.clear();
        }

        const RWGTAMenu::Row* Joined(const char* id)
        {
            EnsureVideo();
            for (const auto& row : video.JoinedRows())
                if (row.setting && row.setting->id == id) return &row;
            return nullptr;
        }

        void ChangeJoined(const char* id, int direction)
        {
            auto row = Joined(id);
            if (!row || !video.Enabled(*row, ABI::Paused())) return;
            auto before = FixSettings().Wanted(id);
            auto previous = writtenVideo.find(id);
            if (previous == writtenVideo.end() || previous->second != before) beforeVideo[id] = before;
            video.Change(*row, direction, ABI::Paused());
            writtenVideo[id] = FixSettings().Wanted(id);
        }

        void FinishVideo(bool cancel)
        {
            if (cancel)
            {
                for (const auto& [id, value] : beforeVideo)
                {
                    auto written = writtenVideo.find(id);
                    // A later editor change belongs to that editor, not this gesture.
                    if (written != writtenVideo.end() && FixSettings().Wanted(id) == written->second)
                        FixSettings().Set(id, value, true);
                }
            }
            beforeVideo.clear();
            writtenVideo.clear();
            video.Close();
            videoOpen = false;
        }

        void RestoreDefaults()
        {
            std::set<std::string> ids;
            for (const auto& setting : FixSettings().GetDescription().settings)
                if (setting.HasValue()) ids.insert(setting.id);
            if (RWGTAMenu::RestoreDefaults(FixSettings(), ids)) restartUntil = GetTickCount64() + 12000;
            refresh = true;
        }
    };

    inline State*& Storage() { static State* state = nullptr; return state; }

    inline std::vector<int> RootRows()
    {
        std::vector<int> rows;
        auto paused = ABI::Paused();
        for (int i = 0; i < 8; ++i)
        {
            bool enabled = !paused || (i != 0 && i != 2);
            injector::WriteMemory<int32_t>(ABI::Address::SettingsRows().get_ptr() + i, enabled ? 1 : 0, true);
            if (enabled) rows.push_back(i);
        }
        return rows;
    }

    inline void RootBack()
    {
        auto paused = ABI::Address::SettingsPaused().get() == 1;
        ABI::Switch(paused ? 8 : 7);
        if (paused) injector::WriteMemory<int32_t>(ABI::Address::SettingsPaused().get_ptr(), 0, true);
    }

    inline int PrivateInput(State& state)
    {
        if (!state.Build()) return 1;
        if (ABI::Down()) state.view.Move(1);
        if (ABI::Up()) state.view.Move(-1);
        bool mouse = ABI::MouseMode();
        bool hoverAction = false;
        if (ABI::HoverActive())
        {
            auto hovered = ABI::Hovered();
            if (hovered >= 0 && static_cast<size_t>(hovered) < state.view.items.size()
                && state.view.items[hovered].enabled)
            {
                state.view.selected = static_cast<size_t>(hovered);
                state.view.Remember();
                hoverAction = true;
            }
        }
        bool back = state.BackPressed(), enter = state.EnterPressed();
        if (back)
        {
            if (!state.view.Back(state.model)) state.Close();
            else state.refresh = true;
            return 1;
        }
        int direction = ABI::Left() ? -1 : ABI::Right() ? 1 : 0;
        if (direction || (enter && (!mouse || hoverAction)))
        {
            if (!state.view.Activate(state.model, ABI::Paused(), direction)) state.Close();
            else state.refresh = true;
        }
        return 1;
    }

    inline int __cdecl SettingsInput()
    {
        auto& state = *Storage();
        if (!state.enabled) return ABI::Function<int(__cdecl*)()>(ABI::Address::SettingsInput())();
        if (state.opened) return PrivateInput(state);
        auto rows = RootRows();
        auto selected = ABI::SettingsSelection();
        auto found = std::find(rows.begin(), rows.end(), selected);
        size_t physical = found == rows.end() ? 0 : static_cast<size_t>(found - rows.begin());
        if (ABI::Down()) physical = (physical + 1) % rows.size();
        if (ABI::Up()) physical = (physical + rows.size() - 1) % rows.size();
        bool mouse = ABI::MouseMode();
        bool hoverAction = false;
        if (ABI::HoverActive())
        {
            auto hovered = ABI::Hovered();
            if (hovered >= 0 && static_cast<size_t>(hovered) < rows.size())
            {
                physical = static_cast<size_t>(hovered);
                hoverAction = true;
            }
        }
        selected = rows[physical];
        injector::WriteMemory<int32_t>(ABI::Address::SettingsSelection().get_ptr(), selected, true);
        bool back = state.BackPressed(), enter = state.EnterPressed();
        if (back) { RootBack(); return 1; }
        if (enter && (!mouse || hoverAction))
        {
            if (selected == 6)
            {
                if (!state.Open()) state.Log("plugin discovery/text fit refused; Settings retained");
            }
            else
            {
                static constexpr int pages[] = { 3, 18, 1, 11, 13, 17, 0, 19 };
                ABI::Switch(pages[selected]);
            }
        }
        return 1;
    }

    inline int __cdecl SettingsDraw()
    {
        auto& state = *Storage();
        if (!state.enabled) return ABI::Function<int(__cdecl*)()>(ABI::Address::SettingsDraw())();
        if (state.opened && state.Build())
        {
            auto title = state.Words(state.view.details ? "menu.details" : "menu.plugins");
            ABI::Header(title.c_str());
            auto y = ABI::Y();
            for (size_t i = 0; i < state.view.items.size(); ++i)
            {
                const auto& item = state.view.items[i];
                ABI::Row(item.text.c_str(), y, item.enabled && i == state.view.selected);
                y += ABI::Spacing();
            }
            auto restart = state.model.RestartRequired() ? state.Words("menu.restartshort") : std::wstring{};
            ABI::Footer(restart.c_str());
            return 1;
        }
        ABI::Header(ABI::Text("SETT"));
        auto rows = RootRows();
        auto plugin = state.Words("menu.plugins");
        static constexpr const char* keys[] = { "DIFF", "CONTR", "LANGUA", "AUDIO", "VIDEO", "BRIGHTN", "", "DEFSET" };
        auto y = ABI::Y();
        for (int row : rows)
        {
            ABI::Row(row == 6 ? plugin.c_str() : ABI::Text(keys[row]), y, ABI::SettingsSelection() == row);
            y += ABI::Spacing();
        }
        auto restart = GetTickCount64() < state.restartUntil ? state.Words("menu.restartshort") : std::wstring{};
        ABI::Footer(restart.c_str());
        return 1;
    }

    inline int __cdecl VideoInput()
    {
        auto& state = *Storage();
        if (!state.enabled) return ABI::Function<int(__cdecl*)()>(ABI::Address::VideoInput())();
        state.EnsureVideo();
        auto selected = (std::clamp)(ABI::VideoSelection(), 0, 6);
        if (ABI::Up()) selected = (selected + 6) % 7;
        if (ABI::Down()) selected = (selected + 1) % 7;
        if (ABI::MouseMode())
        {
            auto hovered = ABI::Hovered();
            if (hovered >= 0 && hovered < 7) selected = hovered;
        }
        injector::WriteMemory<int32_t>(ABI::Address::VideoSelection().get_ptr(), selected, true);
        bool left = ABI::Left(), right = ABI::Right(), enter = state.EnterPressed();
        if (selected < 5)
        {
            auto values = ABI::Address::VideoValues();
            auto maximum = selected < 3 ? 2 : (selected == 3 ? ABI::Address::ResolutionCount().get() : ABI::Address::DetailCount().get());
            maximum = (std::max)(maximum, 1);
            auto value = values[selected].get();
            value = (std::clamp)(value, 0, maximum - 1);
            if (left) value = selected < 3 ? (value + maximum - 1) % maximum : (std::max)(value - 1, 0);
            if (right) value = selected < 3 ? (value + 1) % maximum : (std::min)(value + 1, maximum - 1);
            if (enter && ABI::MouseMode() && selected < 3)
            {
                auto target = ABI::MouseValue() - selected * 2;
                if (target == 0 || target == 1) value = target;
            }
            injector::WriteMemory<int32_t>(values[selected].get_ptr(), value, true);
        }
        else
        {
            const char* id = selected == 5 ? "MAIN.FOVFactor" : "MAIN.HudAspectRatioConstraint";
            if (left) state.ChangeJoined(id, -1);
            if (right) state.ChangeJoined(id, 1);
            if (enter && ABI::MouseMode() && ABI::HoverActive() && ABI::Hovered() == selected)
                state.ChangeJoined(id, 1);
        }
        bool cancel = state.BackPressed();
        if (cancel)
        {
            state.FinishVideo(true);
            ABI::Switch(10);
            ABI::Function<void(__cdecl*)()>(ABI::Address::VideoApply())();
        }
        if (cancel || selected == 2) ABI::Function<void(__cdecl*)()>(ABI::Address::VideoRestore())();
        bool accept = ABI::MouseMode() ? ABI::FooterAccept() : enter;
        if (accept && !cancel) { state.FinishVideo(false); ABI::Switch(10); }
        return 1;
    }

    inline int __cdecl VideoDraw()
    {
        auto& state = *Storage();
        auto result = ABI::Function<int(__cdecl*)()>(ABI::Address::VideoDraw())();
        if (!state.enabled) return result;
        // The original detail rows use two ordinary spacings each.
        auto y = ABI::Y() + ABI::Spacing() * (3.0f + 2.0f * ABI::Address::DetailRows().get());
        for (const auto* id : { "MAIN.FOVFactor", "MAIN.HudAspectRatioConstraint" })
        {
            auto row = state.Joined(id);
            if (!row) continue;
            auto label = state.video.Label(*row) + ": " + state.video.Value(*row);
            std::wstring text;
            if (!Text::Convert(label, text) || !Text::Fits(text))
            {
                state.Log("joined Video text fit refused");
                continue;
            }
            auto index = id == std::string_view("MAIN.FOVFactor") ? 5 : 6;
            ABI::Row(text.c_str(), y, ABI::VideoSelection() == index);
            y += ABI::Spacing();
        }
        return result;
    }

    inline void Tick()
    {
        auto state = Storage();
        if (!state || !state->enabled) return;
        if (ABI::CurrentMenu() != 10 && state->opened) state->Close();
        if (ABI::CurrentMenu() != 13 && state->videoOpen) state->FinishVideo(false);
        state->refresh |= RWGTAMenu::Model::Changed().exchange(false);
    }

    inline void Init()
    {
        if (Storage()) return;
        auto state = new State;
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&Init), &state->module)) { delete state; return; }
        HMODULE pinned = nullptr;
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(&Init), &pinned) || pinned != state->module) { delete state; return; }
        Storage() = state;
        // This call site is reached only by Yes in the game's confirmation.
        state->defaults = safetyhook::create_mid(reinterpret_cast<void*>(ABI::Address::DefaultsConfirmed()), [](SafetyHookContext&)
        {
            auto state = Storage();
            if (!state || !state->enabled) return;
            state->RestoreDefaults();
        });
        ABI::Store<uintptr_t>(ABI::Address::SettingsPage(), reinterpret_cast<uintptr_t>(&SettingsInput));
        ABI::Store<uintptr_t>(ABI::Address::SettingsPage() + 4, reinterpret_cast<uintptr_t>(&SettingsDraw));
        ABI::Store<uintptr_t>(ABI::Address::VideoPage(), reinterpret_cast<uintptr_t>(&VideoInput));
        ABI::Store<uintptr_t>(ABI::Address::VideoPage() + 4, reinterpret_cast<uintptr_t>(&VideoDraw));
        state->enabled = true;
    }
}

#pragma once
#include "NativeMenuView.hpp"
#include <safetyhook.hpp>
#include <filesystem>
#include <map>
#include <set>

namespace MHMenu
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
            try
            {
                if (messages.size() >= 32 || !messages.insert(message).second) return;
                OutputDebugStringA(("Manhunt native menu: " + message + "\n").c_str());
            }
            catch (...) {}
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

        void Fail()
        {
            Close();
            video.Close();
            videoOpen = false;
            beforeVideo.clear();
            writtenVideo.clear();
            enabled = false;
            auto selected = ABI::SettingsSelection();
            if (selected >= 6) ABI::Store<int32_t>(0x7C89E8, selected == 7 ? 6 : 5);
            if (ABI::VideoSelection() >= 5) ABI::Store<int32_t>(0x7C8A38, 4);
            Log("native settings session refused; original menus retained");
        }

        bool Open()
        {
            Close();
            language = ABI::Language();
            model.OpenPlugins(module, language, 32, JoinedIDs());
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
            video.OpenPlugins(module, videoLanguage, 32, JoinedIDs());
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
            ABI::Store<int32_t>(0x7C89F0 + i * 4, enabled ? 1 : 0);
            if (enabled) rows.push_back(i);
        }
        return rows;
    }

    inline void RootBack()
    {
        auto paused = ABI::Data<int32_t>(0x7C89EC) == 1;
        ABI::Switch(paused ? 8 : 7);
        if (paused) ABI::Store<int32_t>(0x7C89EC, 0);
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
        if (!state.enabled) return ABI::Function<int(__cdecl*)()>(0x602800)();
        try
        {
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
            ABI::Store<int32_t>(0x7C89E8, selected);
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
        catch (...) { state.Fail(); return 1; }
    }

    inline int __cdecl SettingsDraw()
    {
        auto& state = *Storage();
        if (!state.enabled) return ABI::Function<int(__cdecl*)()>(0x6029B0)();
        try
        {
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
        catch (...) { state.Fail(); return ABI::Function<int(__cdecl*)()>(0x6029B0)(); }
    }

    inline int __cdecl VideoInput()
    {
        auto& state = *Storage();
        if (!state.enabled) return ABI::Function<int(__cdecl*)()>(0x603290)();
        try
        {
            state.EnsureVideo();
            auto selected = (std::clamp)(ABI::VideoSelection(), 0, 6);
            if (ABI::Up()) selected = (selected + 6) % 7;
            if (ABI::Down()) selected = (selected + 1) % 7;
            if (ABI::MouseMode())
            {
                auto hovered = ABI::Hovered();
                if (hovered >= 0 && hovered < 7) selected = hovered;
            }
            ABI::Store<int32_t>(0x7C8A38, selected);
            bool left = ABI::Left(), right = ABI::Right(), enter = state.EnterPressed();
            if (selected < 5)
            {
                static constexpr uintptr_t values[] = { 0x7D6B14, 0x7D6B30, 0x7D6B18, 0x7D6B28, 0x7D6B2C };
                auto maximum = selected < 3 ? 2 : ABI::Data<int32_t>(selected == 3 ? 0x7C8A50 : 0x7C8A54);
                maximum = (std::max)(maximum, 1);
                auto value = ABI::Data<int32_t>(values[selected]);
                value = (std::clamp)(value, 0, maximum - 1);
                if (left) value = selected < 3 ? (value + maximum - 1) % maximum : (std::max)(value - 1, 0);
                if (right) value = selected < 3 ? (value + 1) % maximum : (std::min)(value + 1, maximum - 1);
                if (enter && ABI::MouseMode() && selected < 3)
                {
                    auto target = ABI::MouseValue() - selected * 2;
                    if (target == 0 || target == 1) value = target;
                }
                ABI::Store<int32_t>(values[selected], value);
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
                ABI::Function<void(__cdecl*)()>(0x5DA960)();
            }
            if (cancel || selected == 2) ABI::Function<void(__cdecl*)()>(0x5DAA20)();
            bool accept = ABI::MouseMode() ? ABI::FooterAccept() : enter;
            if (accept && !cancel) { state.FinishVideo(false); ABI::Switch(10); }
            return 1;
        }
        catch (...) { state.Fail(); return 1; }
    }

    inline int __cdecl VideoDraw()
    {
        auto& state = *Storage();
        auto result = ABI::Function<int(__cdecl*)()>(0x603480)();
        if (!state.enabled) return result;
        try
        {
            // The original detail rows use two ordinary spacings each.
            auto y = ABI::Y() + ABI::Spacing() * (3.0f + 2.0f * ABI::Data<float>(0x7D68C4));
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
        catch (...) { state.Fail(); return result; }
    }

    inline void Tick()
    {
        auto state = Storage();
        if (!state || !state->enabled) return;
        try
        {
            if (ABI::CurrentMenu() != 10 && state->opened) state->Close();
            if (ABI::CurrentMenu() != 13 && state->videoOpen) state->FinishVideo(false);
            state->refresh |= RWGTAMenu::Model::Changed().exchange(false);
        }
        catch (...) { state->Fail(); }
    }

    inline void Init()
    {
        if (Storage()) return;
        State* state = nullptr;
        try
        {
            state = new State;
            if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(&Init), &state->module)) { delete state; return; }
            if (!ABI::Supported())
            {
                state->Log("unsupported frontend contracts; original menus retained");
                delete state;
                return;
            }
            HMODULE pinned = nullptr;
            if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                reinterpret_cast<LPCWSTR>(&Init), &pinned) || pinned != state->module) { delete state; return; }
            Storage() = state;
            // This call site is reached only by Yes in the game's confirmation.
            state->defaults = safetyhook::create_mid(reinterpret_cast<void*>(0x602594), [](SafetyHookContext&)
            {
                auto state = Storage();
                if (!state || !state->enabled) return;
                try { state->RestoreDefaults(); } catch (...) { state->Fail(); }
            });
            if (!state->defaults) { state->Log("defaults callback transaction refused"); return; }
            ABI::Store<uintptr_t>(0x7D6764, reinterpret_cast<uintptr_t>(&SettingsInput));
            ABI::Store<uintptr_t>(0x7D6768, reinterpret_cast<uintptr_t>(&SettingsDraw));
            ABI::Store<uintptr_t>(0x7D6914, reinterpret_cast<uintptr_t>(&VideoInput));
            ABI::Store<uintptr_t>(0x7D6918, reinterpret_cast<uintptr_t>(&VideoDraw));
            state->enabled = true;
        }
        catch (...)
        {
            if (state && Storage() == state) state->Fail();
            else delete state;
        }
    }
}

#pragma once
#include "SettingsModel.hpp"
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include <mutex>
#include <cstdio>
#include <atomic>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace asi_overlay
{
    // Each host owns a context. Every public entry point restores the previous context
    // so other mods using ImGui do not accidentally draw into ours.
    class Overlay
    {
    public:
        struct Scope
        {
            std::unique_lock<std::recursive_mutex> lock;
            ImGuiContext* previous;
            Scope(Overlay& o) : lock(o.mutex), previous(ImGui::GetCurrentContext()) { ImGui::SetCurrentContext(o.context); }
            ~Scope() { ImGui::SetCurrentContext(previous); }
        };
        SettingsModel model;
        ImGuiContext* context = nullptr;
        HWND window = nullptr;
        HMODULE host = nullptr;
        int hotkey = VK_F10;
        std::string fallbackLanguage = "en";
        bool dark = true;

        bool Initialize(HWND hwnd, HMODULE module)
        {
            if (context) return window == hwnd;
            auto previous = ImGui::GetCurrentContext();
            context = ImGui::CreateContext();
            Scope scope(*this);
            scope.previous = previous;
            window = hwnd;
            host = module;
            auto& io = ImGui::GetIO();
            io.IniFilename = nullptr;
            io.LogFilename = nullptr;
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
            io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
            io.ConfigDebugHighlightIdConflicts = false; // never show ImGui's programmer popups to players
            // Segoe UI covers the common European translations. ImGui 1.92 loads glyphs on demand.
            wchar_t system[MAX_PATH]{};
            GetWindowsDirectoryW(system, MAX_PATH);
            auto path = asi_settings::text::Narrow(std::wstring(system) + L"\\Fonts\\segoeui.ttf");
            if (!io.Fonts->AddFontFromFileTTF(path.c_str(), 18.0f)) io.Fonts->AddFontDefault();
            if (!ImGui_ImplWin32_Init(hwnd)) { ImGui::DestroyContext(context); context = nullptr; return false; }
            return true;
        }

        void Shutdown()
        {
            if (!context) return;
            Scope scope(*this);
            SetOpen(false);
            ImGui_ImplWin32_Shutdown();
            if (scope.previous == context) scope.previous = nullptr;
            ImGui::DestroyContext(context);
            context = nullptr;
            window = nullptr;
        }

        bool IsOpen() const { return open; }
        bool BlocksInput() const { return open || waitPadRelease || GetTickCount64() < releaseUntil; }
        void SetTheme(bool useDark) { Scope scope(*this); dark = useDark; lightTheme = !useDark; }
        void SelectPlugin(std::string_view id)
        {
            Scope scope(*this);
            selectedPlugin = -1;
            for (size_t i = 0; i < model.plugins.size(); ++i) if (model.plugins[i].file.id == id) selectedPlugin = int(i);
        }

        void SetOpen(bool value)
        {
            if (!context || value == open) return;
            Scope scope(*this);
            open = value;
            edits.clear();
            if (open)
            {
                waitPadRelease = false;
                model.Discover(host, fallbackLanguage);
                model.message.clear();
                GetClipCursor(&oldClip);
                GetCursorPos(&oldCursor);
                oldCapture = GetCapture();
                ReleaseCapture();
                ClipCursor(nullptr);
                // Do not modify ShowCursor's counter; ImGui draws the cursor itself.
                oldCursorShape = GetCursor();
                ImGui::GetIO().MouseDrawCursor = true;
                SetCursor(nullptr);
                firstFrame = true;
            }
            else
            {
                pending.clear();
                ImGui::GetIO().MouseDrawCursor = false;
                ImGui::GetIO().ClearInputKeys();
                ImGui::GetIO().ClearInputMouse();
                ReleaseCapture();
                if (GetForegroundWindow() == window)
                {
                    if (oldCapture && IsWindow(oldCapture)) SetCapture(oldCapture);
                    ClipCursor(&oldClip);
                    SetCursorPos(oldCursor.x, oldCursor.y);
                }
                else ClipCursor(nullptr);
                SetCursor(oldCursorShape);
                // Consume opening/closing button edges rather than leaking them into the game.
                releaseUntil = GetTickCount64() + 200;
                waitPadRelease = padButtons != 0 || padCombo;
            }
        }

        // Call before the game's WndProc/raw-input handler. Never swallow window lifecycle messages.
        bool Message(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, LRESULT& result)
        {
            if (!context || hwnd != window) return false;
            Scope scope(*this);
            if ((msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) && wp == WPARAM(hotkey) && !(lp & (1LL << 30)))
            {
                SetOpen(!open);
                result = 0;
                return true;
            }
            if (msg == WM_KILLFOCUS || (msg == WM_ACTIVATEAPP && !wp)) SetOpen(false);
            if (!open)
            {
                if ((msg == WM_KEYUP || msg == WM_SYSKEYUP) && wp == WPARAM(hotkey)) { result = 0; return true; }
                return false;
            }
            // A lone Alt would toggle ImGui's menu layer (title bar close button), so a following
            // Enter/A closes the overlay; Alt+Tab returns trigger it too. The UI has no Alt shortcuts.
            bool alt = (msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP || msg == WM_KEYDOWN || msg == WM_KEYUP) && (wp == VK_MENU || wp == VK_LMENU || wp == VK_RMENU);
            if (!alt) ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp);
            if (msg == WM_MOUSEWHEEL || msg == WM_MOUSEHWHEEL) legacyWheel = true;
            if (msg == WM_INPUT) RawMouse(lp);
            if (msg == WM_SETCURSOR && LOWORD(lp) == HTCLIENT) { SetCursor(nullptr); result = TRUE; return true; }
            if ((msg >= WM_KEYFIRST && msg <= WM_KEYLAST) || (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST) || msg == WM_INPUT)
            {
                result = msg == WM_INPUT ? DefWindowProcW(hwnd, msg, wp, lp) : 0;
                return true;
            }
            return false;
        }

        // Games registering raw mouse input with RIDEV_NOLEGACY (Max Payne 3) never receive
        // WM_xBUTTON messages. Read buttons/wheel from the raw packet; ImGui ignores duplicate
        // button states, and the wheel is only used when the window gets no legacy wheel messages.
        void RawMouse(LPARAM lp)
        {
            RAWINPUT raw{};
            UINT size = sizeof(raw);
            if (GetRawInputData(HRAWINPUT(lp), RID_INPUT, &raw, &size, sizeof(RAWINPUTHEADER)) == UINT(-1) || raw.header.dwType != RIM_TYPEMOUSE) return;
            auto& io = ImGui::GetIO();
            auto flags = raw.data.mouse.usButtonFlags;
            const USHORT down[] = { RI_MOUSE_LEFT_BUTTON_DOWN, RI_MOUSE_RIGHT_BUTTON_DOWN, RI_MOUSE_MIDDLE_BUTTON_DOWN };
            const USHORT up[] = { RI_MOUSE_LEFT_BUTTON_UP, RI_MOUSE_RIGHT_BUTTON_UP, RI_MOUSE_MIDDLE_BUTTON_UP };
            for (int i = 0; i < 3; ++i)
            {
                if (flags & down[i]) io.AddMouseButtonEvent(i, true);
                if (flags & up[i]) io.AddMouseButtonEvent(i, false);
            }
            if ((flags & RI_MOUSE_WHEEL) && !legacyWheel) io.AddMouseWheelEvent(0, float(SHORT(raw.data.mouse.usButtonData)) / WHEEL_DELTA);
        }

        // Hosts whose game owns the mouse through DirectInput (exclusive/foreground cooperative levels
        // suppress window mouse messages) feed buttons and wheel steps from the game's device reads.
        void MouseButton(int button, bool down)
        {
            if (!context || !open) return;
            Scope scope(*this);
            ImGui::GetIO().AddMouseButtonEvent(button, down);
        }
        void MouseWheel(float steps)
        {
            if (!context || !open || steps == 0) return;
            Scope scope(*this);
            ImGui::GetIO().AddMouseWheelEvent(0, steps);
        }

        // PS2-style mapped pad after the game's DirectInput/XInput mapper, before game consumers.
        // For other games call Gamepad with their normalized button state.
        void Gamepad(uint16_t buttons, int8_t x = 0, int8_t y = 0)
        {
            if (!context) return;
            Scope scope(*this);
            bool combo = (buttons & 0x9) == 0x9; // Select + Start
            if (combo && !padCombo && GetForegroundWindow() == window) SetOpen(!open);
            padCombo = combo;
            if (!open && buttons == 0 && std::abs(int(x)) < 24 && std::abs(int(y)) < 24) waitPadRelease = false;
            padButtons = combo ? 0 : buttons;
            padX = x; padY = y;
        }

        void NewFrame(float framebufferWidth = 0, float framebufferHeight = 0, bool scaledBackend = false)
        {
            for (auto& p : model.plugins) if (p.module == host)
            {
                std::string language;
                if (SettingsModel::Read(p.api, "MAIN.TextLanguage", language))
                {
                    if (language == "auto" || language.empty()) language = fallbackLanguage;
                    if (language != model.language) model.SetLanguage(language);
                }
            }
            ImGui_ImplWin32_NewFrame();
            auto& frameIO = ImGui::GetIO();
            frameIO.DisplayFramebufferScale = scaledBackend && frameIO.DisplaySize.x > 0 && frameIO.DisplaySize.y > 0
                ? ImVec2(framebufferWidth / frameIO.DisplaySize.x, framebufferHeight / frameIO.DisplaySize.y) : ImVec2(1, 1);
            // Only supply the game's pad when it has activity; the Win32 backend handles ordinary XInput otherwise.
            if (padButtons || padX || padY || hadMappedPad)
            {
                auto& io = ImGui::GetIO();
                io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
                auto button = [&](ImGuiKey key, uint16_t mask) { io.AddKeyEvent(key, (padButtons & mask) != 0); };
                button(ImGuiKey_GamepadDpadUp, 0x10); button(ImGuiKey_GamepadDpadRight, 0x20);
                button(ImGuiKey_GamepadDpadDown, 0x40); button(ImGuiKey_GamepadDpadLeft, 0x80);
                button(ImGuiKey_GamepadFaceUp, 0x1000); button(ImGuiKey_GamepadFaceRight, 0x2000);
                button(ImGuiKey_GamepadFaceDown, 0x4000); button(ImGuiKey_GamepadFaceLeft, 0x8000);
                button(ImGuiKey_GamepadL1, 0x400); button(ImGuiKey_GamepadR1, 0x800);
                auto axis = [&](ImGuiKey key, int value) { float f = std::clamp((value - 24) / 103.0f, 0.0f, 1.0f); io.AddKeyAnalogEvent(key, f > 0, f); };
                axis(ImGuiKey_GamepadLStickLeft, -int(padX)); axis(ImGuiKey_GamepadLStickRight, padX);
                axis(ImGuiKey_GamepadLStickUp, -int(padY)); axis(ImGuiKey_GamepadLStickDown, padY);
                hadMappedPad = padButtons || padX || padY;
            }
            auto scale = std::clamp(ImGui_ImplWin32_GetDpiScaleForHwnd(window), 1.0f, 3.0f);
            scale = std::min(scale, std::max(0.65f, ImGui::GetIO().DisplaySize.y / 560.0f));
            if (scale != uiScale || styleDark != dark)
            {
                ImGui::GetStyle() = ImGuiStyle();
                if (dark) ImGui::StyleColorsDark(); else ImGui::StyleColorsLight();
                auto& style = ImGui::GetStyle();
                style.WindowRounding = 12; style.FrameRounding = 5; style.ChildRounding = 8;
                style.FramePadding = ImVec2(9, 6); style.ItemSpacing = ImVec2(10, 9);
                style.Colors[ImGuiCol_CheckMark] = ImVec4(0.25f, 0.65f, 1, 1);
                style.Colors[ImGuiCol_PopupBg].w = 1.0f;
                style.ScaleAllSizes(scale);
                style.FontScaleDpi = scale;
                uiScale = scale; styleDark = dark;
            }
            ImGui::NewFrame();
            Draw();
            ImGui::Render();
        }

        // The official DX9 backend has no framebuffer-scale support. Scale this frame's
        // geometry before handing it to the unmodified backend, keeping input in client coordinates.
        static void ScaleD3D9DrawData(ImDrawData* data, float width, float height)
        {
            if (!data || data->DisplaySize.x <= 0 || data->DisplaySize.y <= 0) return;
            ImVec2 factor(width / data->DisplaySize.x, height / data->DisplaySize.y);
            if (factor.x == 1 && factor.y == 1) return;
            for (auto list : data->CmdLists)
            {
                for (auto& vertex : list->VtxBuffer) { vertex.pos.x *= factor.x; vertex.pos.y *= factor.y; }
                for (auto& command : list->CmdBuffer)
                {
                    command.ClipRect.x *= factor.x; command.ClipRect.z *= factor.x;
                    command.ClipRect.y *= factor.y; command.ClipRect.w *= factor.y;
                }
            }
            data->DisplaySize = ImVec2(width, height);
            data->DisplayPos.x *= factor.x; data->DisplayPos.y *= factor.y;
        }

    private:
        std::recursive_mutex mutex;
        std::atomic<bool> open = false;
        std::atomic<bool> waitPadRelease = false;
        bool firstFrame = false, focusSidebar = false, legacyWheel = false, padCombo = false, hadMappedPad = false, styleDark = true;
        uint16_t padButtons = 0;
        int8_t padX = 0, padY = 0;
        std::atomic<ULONGLONG> releaseUntil = 0;
        float uiScale = 0, fittedScale = 0;
        ImVec2 fittedDisplay{};
        RECT oldClip{};
        POINT oldCursor{};
        HWND oldCapture = nullptr;
        HCURSOR oldCursorShape = nullptr;
        int selectedPlugin = -1;
        std::vector<Change> pending;
        std::map<std::string, std::string> edits;

        // Bit p set: every preset setting of the snapshot equals preset p. Presets can share values
        // (e.g. Recommended == Enhanced), so all matches count, not only the first.
        static int PresetMatches(const std::vector<asi_settings::Slot>& values)
        {
            int mask = 0;
            bool any = false;
            for (int p = 0; p < 3; ++p)
            {
                bool all = true;
                for (auto& slot : values)
                {
                    if (!slot.setting->InPresets()) continue;
                    any = true;
                    if (!asi_settings::detail::JsonEquals((*slot.setting->presets)[p], asi_settings::FormatValue(slot), !slot.setting->StringValued())) { all = false; break; }
                }
                if (all) mask |= 1 << p;
            }
            return any ? mask : -1; // -1: the plugin has no preset settings
        }

        void Presets(int plugin)
        {
            int matches = 7;
            bool participates = false;
            for (size_t i = 0; i < model.plugins.size(); ++i)
            {
                if (plugin >= 0 && int(i) != plugin) continue;
                int m = PresetMatches(model.Snapshot(i, true)); // the player's choices, not fallbacks in effect
                if (m >= 0) { matches &= m; participates = true; }
            }
            if (!participates) matches = 0;
            static const char* names[] = { "Vanilla", "Recommended", "Enhanced" };
            ImGui::TextUnformatted("Preset");
            std::string current;
            for (int p = 0; p < 3; ++p)
            {
                bool active = (matches >> p) & 1;
                if (p) ImGui::SameLine();
                if (active) { ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.4f, 0.7f, 1)); ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1)); }
                if (ImGui::Button(names[p])) { pending = model.PresetChanges(p, plugin); ImGui::OpenPopup("Apply preset?"); }
                if (active) { ImGui::PopStyleColor(2); current += (current.empty() ? "" : " / ") + std::string(names[p]); }
            }
            ImGui::SameLine(); ImGui::TextDisabled("%s", current.empty() ? "Custom" : current.c_str());
        }

        void Confirmation()
        {
            ImGui::SetNextWindowSize(ImVec2(std::min(680.0f * uiScale, ImGui::GetIO().DisplaySize.x - 32), 0), ImGuiCond_Appearing);
            if (!ImGui::BeginPopupModal("Apply preset?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) return;
            if (ImGui::IsKeyPressed(ImGuiKey_Escape) || ImGui::IsKeyPressed(ImGuiKey_GamepadFaceRight))
            {
                pending.clear(); ImGui::CloseCurrentPopup(); ImGui::EndPopup(); return;
            }
            ImGui::TextWrapped("Only menu settings change. The plugins validate and save each change.");
            ImGui::BeginChild("Changes", ImVec2(0, std::min(340.0f * uiScale, ImGui::GetIO().DisplaySize.y * 0.45f)));
            size_t lastPlugin = size_t(-1);
            for (auto& change : pending)
            {
                if (lastPlugin != change.plugin)
                {
                    lastPlugin = change.plugin;
                    ImGui::SeparatorText(model.plugins[change.plugin].file.texts.Resolve(model.plugins[change.plugin].file.description.name).c_str());
                }
                ImGui::TextWrapped("%s: %s -> %s%s", change.label.c_str(), change.beforeText.c_str(), change.afterText.c_str(), change.restart ? " (after restart)" : "");
                ImGui::TextWrapped("%s", change.description.c_str());
            }
            if (pending.empty()) ImGui::TextUnformatted("These values are already selected.");
            ImGui::EndChild();
            if (ImGui::Button("Apply")) { model.Apply(pending); pending.clear(); ImGui::CloseCurrentPopup(); }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) { pending.clear(); ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }

        void Setting(size_t index, const asi_settings::Setting& s, const std::vector<asi_settings::Slot>& values)
        {
            auto& p = model.plugins[index];
            if (s.visibility == "file" || !SettingsModel::Condition(s.visibleIf, values) || SettingsModel::Hidden(p.api, s)) return;
            auto choices = SettingsModel::Choices(p.api, s);
            if ((s.type == asi_settings::Type::Choice || s.dynamic) && choices.empty()) return;
            std::string value;
            if (s.HasValue() && !SettingsModel::Read(p.api, s.id, value)) return;
            auto label = p.file.texts.Resolve(s.label);
            auto key = p.file.id + ":" + s.id;
            ImGui::PushID(s.id.c_str());
            ImGui::TextUnformatted(label.c_str());
            if (s.apply == "restart") { ImGui::SameLine(); ImGui::TextDisabled("%s", model.restartChanges.count(key) ? "Restart to apply" : "After restart"); }
            auto description = p.file.texts.Resolve(s.description);
            if (!description.empty()) ImGui::TextWrapped("%s", description.c_str());
            ImGui::BeginDisabled(!SettingsModel::Condition(s.enabledIf, values));
            ImGui::SetNextItemWidth(std::min(420.0f * uiScale, ImGui::GetContentRegionAvail().x));
            auto commit = [&](const std::string& v) { model.Set(index, s, v); };
            using Type = asi_settings::Type;
            if (s.type == Type::Bool)
            {
                int b = 0; asi_settings::detail::ParseBool(value, b); bool checked = b != 0;
                if (ImGui::Checkbox("##value", &checked)) commit(checked ? "1" : "0");
            }
            else if (s.type == Type::Choice || s.dynamic)
            {
                auto choiceLabel = [&](const asi_settings::Labelled& c) { return c.label.empty() ? asi_settings::detail::JsonToText(c.value) : p.file.texts.Resolve(c.label); };
                std::string preview = value;
                for (auto& c : choices) if (asi_settings::detail::JsonEquals(c.value, value, !s.StringValued())) preview = choiceLabel(c);
                if (ImGui::BeginCombo("##value", preview.c_str()))
                {
                    for (auto& c : choices)
                    {
                        auto v = asi_settings::detail::JsonToText(c.value);
                        if (ImGui::Selectable(choiceLabel(c).c_str(), v == value)) commit(v);
                    }
                    ImGui::EndCombo();
                }
            }
            else if (s.type == Type::Int || s.type == Type::Float)
            {
                auto& edit = edits[key];
                double number = std::strtod(edit.empty() ? value.c_str() : edit.c_str(), nullptr);
                bool changed;
                bool slider = s.min && s.max && (s.controls.empty() || std::find(s.controls.begin(), s.controls.end(), "slider") != s.controls.end());
                // Decimals: the description's, else as many as the step needs (step 1 -> "25", 0.05 -> "0.25"), else 2.
                int decimals = s.decimals;
                if (decimals < 0 && s.step && *s.step > 0) { decimals = 0; for (double f = *s.step; decimals < 4 && std::abs(f - std::round(f)) > 1e-6; f *= 10) ++decimals; }
                auto format = s.type == Type::Int ? std::string("%.0f") : "%." + std::to_string(decimals < 0 ? 2 : decimals) + "f";
                if (slider) changed = ImGui::SliderScalar("##value", ImGuiDataType_Double, &number, &*s.min, &*s.max, format.c_str());
                else { double step = s.step.value_or(s.type == Type::Int ? 1.0 : 0.1); changed = ImGui::InputScalar("##value", ImGuiDataType_Double, &number, &step, nullptr, format.c_str()); }
                // Do not save once per drag frame; the API persists each accepted change.
                if (changed)
                {
                    if (s.step && *s.step > 0) number = s.min.value_or(0) + std::round((number - s.min.value_or(0)) / *s.step) * *s.step;
                    if (slider) number = std::clamp(number, *s.min, *s.max);
                    if (s.type == Type::Int) number = std::round(number);
                    edit = asi_settings::text::FormatNumber(number);
                }
                if (ImGui::IsItemDeactivatedAfterEdit() && !edit.empty()) { commit(edit); edit.clear(); }
                if (!s.unit.empty()) { ImGui::SameLine(); ImGui::TextUnformatted(s.unit.c_str()); }
            }
            else if (s.type == Type::Action)
            {
                if (ImGui::Button("Run") && OVERLAY_HAS(p.api, Invoke))
                    if (p.api->Invoke(s.id.c_str()) < 0) model.message = label + ": action refused.";
            }
            else if (s.type != Type::Info)
            {
                // Keep editing buffers (including an empty string) stable while another editor changes a value.
                if (!edits.count(key)) edits[key] = value;
                auto& edit = edits[key];
                std::vector<char> buffer(4097, 0);
                std::memcpy(buffer.data(), edit.data(), std::min(edit.size(), buffer.size() - 1));
                bool enter = ImGui::InputText("##value", buffer.data(), s.maxLength > 0 ? std::min<size_t>(s.maxLength + 1, buffer.size()) : buffer.size(), ImGuiInputTextFlags_EnterReturnsTrue);
                edit = buffer.data();
                if (enter || ImGui::IsItemDeactivatedAfterEdit()) { commit(edit); edits.erase(key); }
                else if (!ImGui::IsItemActive()) edits.erase(key);
            }
            // Choice lists already offer their values; the extra list is for free-form values.
            if ((!s.special.empty() || !s.suggestions.empty()) && s.type != Type::Choice && !s.dynamic)
            {
                if (ImGui::BeginCombo("Values", "Special / suggested / custom"))
                {
                    for (auto list : { &s.special, &s.suggestions }) for (auto& c : *list)
                    {
                        auto v = asi_settings::detail::JsonToText(c.value);
                        auto text = c.label.empty() ? v : p.file.texts.Resolve(c.label);
                        if (ImGui::Selectable(text.c_str(), value == v)) commit(v);
                    }
                    ImGui::EndCombo();
                }
            }
            if (s.HasValue())
            {
                ImGui::SameLine();
                if (ImGui::SmallButton("Reset")) commit(asi_settings::detail::JsonToText(s.defaultValue));
                // The control shows the value in effect; name the saved choice when it can't be used now
                std::string stored;
                if (model.Stored(index, s, stored) && model.Display(index, s, stored) != model.Display(index, s, value))
                {
                    ImGui::SameLine();
                    ImGui::TextDisabled("Saved: %s (not available now)", model.Display(index, s, stored).c_str());
                }
            }
            ImGui::EndDisabled();
            ImGui::Separator();
            ImGui::PopID();
        }

        void Pages(size_t index, int tier)
        {
            auto& p = model.plugins[index];
            auto values = model.Snapshot(index);
            auto pages = p.file.description.pages;
            std::stable_sort(pages.begin(), pages.end(), [](auto& a, auto& b) { return a.order < b.order; });
            if (pages.empty()) pages.push_back({ "", "Options", "" });
            // The same page appears once per tier: keep the tiers' header IDs apart.
            ImGui::PushID(tier);
            for (auto& page : pages)
            {
                std::vector<const asi_settings::Setting*> settings;
                for (auto& s : p.file.description.settings)
                {
                    int stier = s.compatibility ? 2 : s.visibility == "menu" ? 0 : 1;
                    if (s.page == page.id && stier == tier && s.visibility != "file") settings.push_back(&s);
                }
                if (settings.empty()) continue;
                std::stable_sort(settings.begin(), settings.end(), [](auto a, auto b) { return a->order < b->order; });
                ImGui::PushID(page.id.c_str());
                // Advanced/Compatibility are collapsed by their tier header; pages inside them start open.
                if (ImGui::CollapsingHeader(p.file.texts.Resolve(page.title).c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                {
                    std::string group;
                    for (auto s : settings)
                    {
                        if (s->group != group) { group = s->group; if (!group.empty()) ImGui::SeparatorText(p.file.texts.Resolve(group).c_str()); }
                        Setting(index, *s, values);
                    }
                }
                ImGui::PopID();
            }
            ImGui::PopID();
        }

        void Draw()
        {
            if (!open) return;
            ClipCursor(nullptr);
            auto display = ImGui::GetIO().DisplaySize;
            // Re-fit after a resolution/window size change; otherwise keep the user's placement.
            auto fit = display.x != fittedDisplay.x || display.y != fittedDisplay.y || uiScale != fittedScale ? ImGuiCond_Always : ImGuiCond_Appearing;
            fittedDisplay = display; fittedScale = uiScale;
            ImGui::SetNextWindowSize(ImVec2(std::min(1050.0f * uiScale, display.x - 32), std::min(760.0f * uiScale, display.y - 32)), fit);
            ImGui::SetNextWindowPos(ImVec2(display.x / 2, display.y / 2), fit, ImVec2(0.5f, 0.5f));
            bool keepOpen = true;
            ImGui::Begin("Settings", &keepOpen, ImGuiWindowFlags_NoCollapse);
            if (firstFrame) { ImGui::SetWindowFocus(); firstFrame = false; focusSidebar = true; }
            ImGui::TextDisabled("Live settings | Hotkey / Select + Start to close");
            ImGui::SameLine(); ImGui::Checkbox("Light theme", &lightTheme); dark = !lightTheme;
            ImGui::Separator();
            // Flattened children: arrows/d-pad move between the plugin list and settings directly.
            ImGui::BeginChild("Plugins", ImVec2(210 * uiScale, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_NavFlattened);
            // Start keyboard/pad navigation on the selected plugin rather than the close button.
            if (focusSidebar && selectedPlugin == -1) { ImGui::SetKeyboardFocusHere(); focusSidebar = false; }
            if (ImGui::Selectable("Presets", selectedPlugin == -1)) selectedPlugin = -1;
            for (size_t i = 0; i < model.plugins.size(); ++i)
            {
                ImGui::PushID(int(i));
                if (focusSidebar && selectedPlugin == int(i)) { ImGui::SetKeyboardFocusHere(); focusSidebar = false; }
                if (ImGui::Selectable(model.plugins[i].file.texts.Resolve(model.plugins[i].file.description.name).c_str(), selectedPlugin == int(i))) selectedPlugin = int(i);
                ImGui::PopID();
            }
            ImGui::EndChild(); ImGui::SameLine();
            ImGui::BeginChild("Settings", ImVec2(0, 0), ImGuiChildFlags_NavFlattened);
            if (selectedPlugin >= int(model.plugins.size())) selectedPlugin = -1;
            Presets(selectedPlugin);
            ImGui::Separator();
            if (selectedPlugin >= 0)
            {
                Pages(selectedPlugin, 0);
                if (ImGui::CollapsingHeader("Advanced")) Pages(selectedPlugin, 1);
                if (ImGui::CollapsingHeader("Compatibility")) Pages(selectedPlugin, 2);
            }
            else ImGui::TextWrapped("Choose a preset for all plugins, or choose a plugin on the left to edit its settings.");
            if (!model.message.empty()) ImGui::TextWrapped("%s", model.message.c_str());
            Confirmation();
            ImGui::EndChild();
            ImGui::End();
            if (!keepOpen) SetOpen(false);
        }
        bool lightTheme = false;
    };
}

module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include <d3d9.h>
#include "Shared/RWGTA/FixMenu.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"
#include "Shared/RainDroplets.hpp"

export module NativeMenu;

export namespace USMMenu
{
    inline RWGTAMenu::Model Model;
    inline SafetyHookInline LoopHook, EventHook, DestroyHook;
    inline SafetyHookMid RenderHook;
    inline std::function<void(IDirect3DDevice9*, HWND)> Render;
    inline uintptr_t OptionsVtable = 0, Manager = 0, Tab = 0;
    inline void* (__cdecl* Allocate)(size_t) = nullptr;
    inline uintptr_t(__thiscall* ConstructButton)(void*, uintptr_t, int, const char*, int) = nullptr;
    inline std::vector<uintptr_t> Widgets;
    inline std::vector<std::pair<uintptr_t, bool>> Hidden, SavedVisibility;
    inline bool Opened = false, Details = false;
    // The list shown (the plugins list, a plugin's page or a category); every list scrolls through the 7 row slots
    inline size_t Page = RWGTAMenu::Model::RootPage;
    inline int HelpOffset = 0, HelpLines = 0;
    inline constexpr int VisibleSlots = 7;
    inline int VisibleRows = 0;
    inline std::array<const RWGTAMenu::Row*, VisibleSlots> RenderedRows{};
    inline MenuList::WheelNotches Notches;
    // Widgets: 0-6 rows, 7 Back, 8 Details, 9 title, 10-16 values, 17 help, 18 restart notice, 19/20 scroll marks
    inline constexpr int UpMark = 19, DownMark = 20;
    inline std::array<int, 24> MenuKeys{};
    inline bool PadShortcutsEnabled = false;
    inline std::string Language;
    struct UpdateNotice
    {
        std::string version;
        std::string page;
        bool hub;
    };
    inline std::optional<UpdateNotice> Notice;
    inline std::function<std::string()> GameLanguage;
    inline std::map<std::string, std::string> RestartValues;
    inline std::map<HMODULE, std::map<std::string, std::string>> OtherRestartValues;

    template<class Result = void, class... Args> Result Call(uintptr_t object, size_t method, Args... args)
    {
        return reinterpret_cast<Result(__thiscall*)(uintptr_t, Args...)>((*reinterpret_cast<uintptr_t**>(object))[method])(object, args...);
    }

    inline HMODULE Module()
    {
        HMODULE module = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&Module), &module);
        return module;
    }

    inline std::string Words(const char* key)
    {
        return RWGTAMenu::PluginText(FixSettings(), Language.empty() ? "en" : Language, key);
    }

    inline std::string CurrentLanguage()
    {
        const auto language = FixSettings().Get("MAIN.TextLanguage");
        return language == "auto" ? (GameLanguage ? GameLanguage() : "en") : language;
    }

    inline std::string NativeText(const std::string& text)
    {
        const auto wide = asi_settings::text::Widen(text);
        const auto size = WideCharToMultiByte(1252, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, "?", nullptr);
        std::string result(size, '\0');
        WideCharToMultiByte(1252, 0, wide.data(), static_cast<int>(wide.size()), result.data(), size, "?", nullptr);
        return result;
    }

    inline void Caption(uintptr_t widget, const std::string& text)
    {
        const auto native = NativeText(text);
        Call(widget, 11, native.c_str());
    }

    inline bool IsOptionControl(uintptr_t object)
    {
        const auto id = *reinterpret_cast<const int*>(object + 44);
        const auto caption = *reinterpret_cast<const char* const*>(object + 40);
        return id > 6 || (id == 0 && caption && (caption[0] == '+' || caption[0] == '-') && !caption[1]);
    }

    inline void RememberVisibility(uintptr_t object)
    {
        while (object && std::none_of(SavedVisibility.begin(), SavedVisibility.end(), [object](const auto& value) { return value.first == object; }))
        {
            SavedVisibility.emplace_back(object, *reinterpret_cast<const uint8_t*>(object + 81) != 0);
            object = *reinterpret_cast<const uintptr_t*>(object + 32);
        }
    }

    inline void RememberRestartValues()
    {
        for (const auto& page : Model.Pages())
        {
            if (!page.plugin || page.plugin->module == Module())
                continue;
            auto& values = OtherRestartValues[page.plugin->module];
            for (const auto& setting : page.plugin->file.description.settings)
                if (setting.apply == "restart" && !values.contains(setting.id))
                    values.emplace(setting.id, OtherPlugins::Get(*page.plugin, setting));
        }
    }

    inline void Refresh();
    inline void Back();
    inline void Activate(int direction);

    inline void __cdecl Click(uintptr_t widget, int event, uintptr_t)
    {
        // 3: the mouse came onto a button; a row it comes onto is selected, like the game's own rows
        if (event == 3 && Opened && !Details && !Notice)
        {
            const auto found = std::find(Widgets.begin(), Widgets.end(), widget);
            const auto index = static_cast<int>(found - Widgets.begin());
            if (found != Widgets.end() && (index < 7 || (index >= 10 && index < 17)))
            {
                const auto slot = index < 7 ? index : index - 10;
                if (slot < VisibleRows && Model.Window(Page).Point(slot))
                    Refresh();
            }
        }
        if (event != 2)
            return;
        if (widget == Tab)
        {
            if (!Opened)
            {
                Hidden.clear();
                SavedVisibility.clear();
                const auto count = *reinterpret_cast<int*>(Manager + 28);
                const auto objects = *reinterpret_cast<uintptr_t**>(Manager + 20);
                for (int i = 0; i < count; ++i)
                {
                    const auto object = objects[i];
                    if (object == Tab || std::find(Widgets.begin(), Widgets.end(), object) != Widgets.end())
                        continue;
                    RememberVisibility(object);
                    if (IsOptionControl(object))
                        Hidden.emplace_back(object, *reinterpret_cast<const uint8_t*>(object + 81) != 0);
                }
                for (const auto& [widget, visible] : Hidden)
                    Call(widget, 6, 0);
                Model.OpenMenu(Module(), Language, VisibleSlots);
                RememberRestartValues();
                Opened = true;
                Page = RWGTAMenu::Model::RootPage;
                Model.Window(Page).Reset();
                Notches.Clear();
            }
            Refresh();
            return;
        }
        auto found = std::find(Widgets.begin(), Widgets.end(), widget);
        if (found == Widgets.end() || !Opened)
            return;
        const auto index = static_cast<int>(found - Widgets.begin());
        if (index < 7 || (index >= 10 && index < 17))
        {
            const auto slot = index < 7 ? index : index - 10;
            if (slot >= VisibleRows)
                return;
            Model.Window(Page).Point(slot);
            Activate(0);
        }
        else if (index == UpMark || index == DownMark)
        {
            // The marks scroll the list by a row; the selection stays unless it would leave the window
            if (Model.Window(Page).Scroll(index == UpMark ? -1 : 1))
                Refresh();
        }
        else if (index == 7)
            Back();
        else if (index == 8)
        {
            Details = !Details;
            HelpOffset = 0;
            Refresh();
        }
    }

    inline uintptr_t Button(int id, int x, int y, int width, float scale = 0.8f)
    {
        const auto widget = ConstructButton(Allocate(0x94), Manager, id, "data\\packs\\inv.dat", 0);
        Call(widget, 9, x, y, width, 24);
        Call(widget, 13, uint32_t(0xFFDCDCDC));
        Call(widget, 14, uint32_t(0xFFE6D03F));
        Call(widget, 16, scale, scale);
        Call(widget, 17, scale, scale);
        Call(widget, 18, scale, scale);
        *reinterpret_cast<decltype(&Click)*>(widget + 72) = Click;
        *reinterpret_cast<uintptr_t*>(widget + 76) = 0;
        Call(widget, 6, 0);
        return widget;
    }

    // The rows in the window of the shown list
    inline std::vector<const RWGTAMenu::Row*> Rows()
    {
        return Model.Shown(Page, false);
    }

    inline bool RestartPending()
    {
        for (const auto& [id, value] : RestartValues)
            if (FixSettings().Wanted(id) != value)
                return true;
        for (const auto& page : Model.Pages())
        {
            if (!page.plugin || page.plugin->module == Module())
                continue;
            const auto values = OtherRestartValues.find(page.plugin->module);
            if (values == OtherRestartValues.end())
                continue;
            for (const auto& setting : page.plugin->file.description.settings)
            {
                const auto initial = values->second.find(setting.id);
                if (initial != values->second.end() && OtherPlugins::Get(*page.plugin, setting) != initial->second)
                    return true;
            }
        }
        return false;
    }

    inline std::string NoticeText(const UpdateNotice& notice)
    {
        auto text = Words(notice.hub ? "update.availablehub" : "update.availablesite");
        for (const auto& [key, value] : { std::pair{ "{version}", &notice.version }, std::pair{ "{page}", &notice.page } })
            if (const auto at = text.find(key); at != std::string::npos)
                text.replace(at, std::char_traits<char>::length(key), *value);
        return text;
    }

    inline std::string Help()
    {
        if (Notice)
            return NoticeText(*Notice);
        if (Page == RWGTAMenu::Model::RootPage)
            return Words("menu.pluginshelp");
        const auto rows = Rows();
        const auto slot = static_cast<size_t>(Model.Window(Page).Slot());
        if (slot < rows.size() && rows[slot] && !RWGTAMenu::Model::Opens(*rows[slot]))
            return Model.Help(*rows[slot]);
        return {};
    }

    inline void Refresh()
    {
        if (!Opened)
            return;
        for (const auto widget : Widgets)
            Call(widget, 6, 0);
        if (Page >= Model.Pages().size())
            Page = RWGTAMenu::Model::RootPage;
        const auto rows = Rows();
        const auto& window = Model.Window(Page);
        RenderedRows.fill(nullptr);
        VisibleRows = Details || Notice ? 0 : std::min(VisibleSlots, static_cast<int>(rows.size()));
        for (int i = 0; i < VisibleRows; ++i)
        {
            const auto row = rows[i];
            RenderedRows[i] = row;
            const auto value = Model.Value(*row);
            Caption(Widgets[i], Model.Label(*row));
            Call(Widgets[i], 24, static_cast<size_t>(i) == window.Slot() ? 1 : 0);
            Call(Widgets[i], 6, 1);
            Caption(Widgets[10 + i], value);
            Call(Widgets[10 + i], 6, value.empty() ? 0 : 1);
        }
        Call(Widgets[UpMark], 6, VisibleRows && window.Above() ? 1 : 0);
        Call(Widgets[DownMark], 6, VisibleRows && window.Below() ? 1 : 0);
        Caption(Widgets[7], Words("menu.back"));
        Call(Widgets[7], 6, 1);
        Caption(Widgets[8], Words(Details ? "menu.back" : "menu.details"));
        Call(Widgets[8], 6, Notice ? 0 : 1);
        Caption(Widgets[9], Page == RWGTAMenu::Model::RootPage ? Words("menu.plugins") : Model.Pages()[Page].title);
        Call(Widgets[9], 6, 1);
        auto help = Help();
        std::istringstream words(help);
        std::string word, wrapped, line;
        std::vector<std::string> helpLines;
        const bool full = Details || Notice;
        const size_t columns = full ? 85 : 90;
        while (words >> word)
        {
            if (!line.empty() && line.size() + word.size() + 1 > columns)
            {
                helpLines.push_back(line);
                line.clear();
            }
            if (!line.empty()) line += ' ';
            line += word;
        }
        if (!line.empty())
            helpLines.push_back(line);
        HelpLines = static_cast<int>(helpLines.size());
        // Under the rows there is room for four lines, the whole help of a row or of the plugins list
        const auto visibleLines = full ? 10 : 4;
        HelpOffset = full ? std::clamp(HelpOffset, 0, std::max(0, HelpLines - visibleLines)) : 0;
        for (int i = HelpOffset; i < std::min(HelpLines, HelpOffset + visibleLines); ++i)
        {
            if (!wrapped.empty())
                wrapped += '\n';
            wrapped += helpLines[i];
        }
        Caption(Widgets[17], wrapped);
        Call(Widgets[17], 9, 48, full ? 170 : 347, 548, full ? 220 : 84);
        Call(Widgets[17], 6, 1);
        Caption(Widgets[18], RestartPending() ? Words("menu.restartwarning") : "");
        Call(Widgets[18], 6, RestartPending() ? 1 : 0);
    }

    inline void Back()
    {
        HelpOffset = 0;
        if (Notice)
        {
            Notice.reset();
            Details = false;
            Page = RWGTAMenu::Model::RootPage;
        }
        else if (Details)
            Details = false;
        else if (const auto parent = Model.Back(Page); parent != SIZE_MAX)
        {
            // The list it came from, on the row it came from
            Page = parent;
        }
        else
        {
            Opened = false;
            for (const auto widget : Widgets) Call(widget, 6, 0);
            // Native visibility calls propagate to children; restore each saved flag exactly.
            for (const auto& [widget, visible] : SavedVisibility)
                injector::WriteMemory<uint8_t>(reinterpret_cast<uint8_t*>(widget + 81), visible ? 1 : 0, true);
            Hidden.clear();
            SavedVisibility.clear();
            Model.Close();
        }
        Refresh();
    }

    inline void Activate(int direction)
    {
        if (Details || Notice)
            return;
        const auto rows = Rows();
        const auto slot = static_cast<int>(Model.Window(Page).Slot());
        if (slot >= VisibleRows || slot >= static_cast<int>(rows.size()) || rows[slot] != RenderedRows[slot])
            return;
        const auto row = rows[slot];
        if (RWGTAMenu::Model::Opens(*row))
        {
            if (direction)
                return;
            const auto target = Model.Enter(*row);
            if (target != SIZE_MAX)
                Page = target;
        }
        else
            Model.Change(*row, direction, true);
        Refresh();
    }

    // Up/Down: one row, the window following by one row, wrapping at the ends like the game's lists
    inline void Navigate(int direction)
    {
        if (Details || Notice)
            HelpOffset += direction;
        else
            Model.Window(Page).Move(direction);
        Refresh();
    }

    // The wheel (one row per notch), Page Up/Down, Home/End
    inline void Scroll(int direction)
    {
        if (Details || Notice)
            HelpOffset += direction;
        else if (!Model.Window(Page).Scroll(direction))
            return;
        Refresh();
    }

    inline void __fastcall Event(uintptr_t manager, void*, UINT message, WPARAM key, LPARAM parameter)
    {
        if (USMOverlayBlocksInput() && ((message >= WM_KEYFIRST && message <= WM_KEYLAST) ||
            (message >= WM_MOUSEFIRST && message <= WM_MOUSELAST)))
            return;
        if (manager == Manager && message == WM_KEYUP)
        {
            if (!Opened && key == 'P')
            {
                Click(Tab, 2, 0);
                return;
            }
            if (Opened)
            {
                if (key == VK_ESCAPE) Back();
                else if (key == VK_F1 && !Notice) { Details = !Details; HelpOffset = 0; Refresh(); }
                else if (key == VK_RETURN || key == VK_SPACE) Activate(0);
                else if (key == VK_LEFT || key == VK_RIGHT) Activate(key == VK_LEFT ? -1 : 1);
                else if (key == VK_UP || key == VK_DOWN)
                    Navigate(key == VK_UP ? -1 : 1);
                return;
            }
        }
        if (manager == Manager && Opened && message == WM_KEYDOWN)
            return;
        EventHook.thiscall<void>(manager, message, key, parameter);
    }

    inline bool IsOptionsManager(uintptr_t manager)
    {
        if (!manager || *reinterpret_cast<const uintptr_t*>(manager) != OptionsVtable)
            return false;
        const auto count = *reinterpret_cast<const int*>(manager + 28);
        const auto objects = *reinterpret_cast<uintptr_t* const*>(manager + 20);
        if (!objects || count < 3 || count > 512)
            return false;
        unsigned tabs = 0;
        for (int i = 0; i < count; ++i)
        {
            const auto id = *reinterpret_cast<const int*>(objects[i] + 44);
            if (id >= 7 && id <= 9)
                tabs |= 1u << (id - 7);
        }
        // Confirmation dialogs share this vtable but have no Options tabs.
        return tabs == 7;
    }

    inline bool Message(UINT message, WPARAM key, LPARAM parameter)
    {
        if (Opened && message == WM_MOUSEWHEEL && IsOptionsManager(Manager) && !USMOverlayBlocksInput())
        {
            const auto notches = Notches.Take(GET_WHEEL_DELTA_WPARAM(key));
            for (int i = 0; i < std::abs(notches); ++i)
                Scroll(notches > 0 ? -1 : 1);
            return true;
        }
        if (Opened && message == WM_KEYDOWN && (key == VK_PRIOR || key == VK_NEXT || key == VK_HOME || key == VK_END) &&
            IsOptionsManager(Manager) && !USMOverlayBlocksInput())
        {
            if (!Details && !Notice)
            {
                auto& window = Model.Window(Page);
                if (key == VK_PRIOR || key == VK_NEXT ? window.Page(key == VK_NEXT ? 1 : -1) : window.Edge(key == VK_END ? 1 : -1))
                    Refresh();
            }
            return true;
        }
        // The native input loop synthesizes navigation keys only.
        if ((message != WM_KEYDOWN && message != WM_KEYUP) || (key != 'P' && key != VK_F1) ||
            !IsOptionsManager(Manager) || USMOverlayBlocksInput())
            return false;
        if (message == WM_KEYUP)
            Event(Manager, nullptr, message, key, parameter);
        return true;
    }

    inline char __fastcall Loop(uintptr_t manager, void*)
    {
        if (Manager != manager && IsOptionsManager(manager))
        {
            Manager = manager;
            Opened = Details = false;
            Notice.reset();
            Widgets.clear();
            Hidden.clear();
            SavedVisibility.clear();
            Language = CurrentLanguage();
            const auto nativeCount = *reinterpret_cast<int*>(Manager + 28);
            const auto nativeWidgets = *reinterpret_cast<uintptr_t**>(Manager + 20);
            for (int i = 0; i < nativeCount; ++i)
            {
                const auto id = *reinterpret_cast<int*>(nativeWidgets[i] + 44);
                if (id >= 7 && id <= 9)
                    Call(nativeWidgets[i], 9, 150 + (id - 7) * 115, 122, 110, 24);
            }
            Tab = Button(1000, 495, 122, 105);
            Caption(Tab, Words("menu.plugins"));
            Call(Tab, 6, 1);
            for (int i = 0; i < 7; ++i) Widgets.push_back(Button(1001 + i, 48, 165 + i * 24, 350));
            Widgets.push_back(Button(1008, 48, 410, 200));
            Widgets.push_back(Button(1009, 390, 410, 200));
            Widgets.push_back(Button(1010, 48, 143, 548, 0.85f));
            for (int i = 0; i < 7; ++i) Widgets.push_back(Button(1011 + i, 400, 165 + i * 24, 195));
            Widgets.push_back(Button(1018, 48, 347, 548, 0.6f));
            Widgets.push_back(Button(1019, 48, 435, 548, 0.6f));
            // Scroll marks, an ellipsis in the rows' font over the rows' column, clickable
            Widgets.push_back(Button(1020, 48, 151, 350, 0.6f));
            Widgets.push_back(Button(1021, 48, 331, 350, 0.6f));
            Caption(Widgets[UpMark], "...");
            Caption(Widgets[DownMark], "...");
        }
        if (manager == Manager)
        {
            const auto language = CurrentLanguage();
            if (language != Language)
            {
                Language = language;
                Caption(Tab, Words("menu.plugins"));
                if (Opened)
                {
                    Model.SelectLanguage(Language);
                    RememberRestartValues();
                    Refresh();
                }
            }
            if (Opened)
            {
                if (RWGTAMenu::Model::Changed().exchange(false))
                    Refresh();
                for (const auto& [widget, visible] : Hidden) Call(widget, 6, 0);
            }
        }
        if (manager == Manager)
        {
            RWGTA::UpdateCheck::Tick();
            // The Options screen is up: no drops over it
            RainDroplets::MenuOpen(true);
        }
        return LoopHook.thiscall<char>(manager);
    }

    inline int __fastcall Destroy(uintptr_t manager, void*)
    {
        if (manager == Manager)
        {
            RainDroplets::MenuOpen(false);
            Model.Close();
            Manager = Tab = 0;
            Opened = Details = false;
            Notice.reset();
            Widgets.clear();
            Hidden.clear();
            SavedVisibility.clear();
        }
        return DestroyHook.thiscall<int>(manager);
    }

    inline void EnablePadShortcuts(uintptr_t start)
    {
        auto first = hook::range_pattern(start, start + 0x200, "BE ? ? ? ? EB 06 8D 9B 00 00 00 00 8B 46 FC");
        auto repeat = hook::range_pattern(start, start + 0x200, "81 FE ? ? ? ? 7D 05 48 89 06");
        auto end = hook::range_pattern(start, start + 0x200, "83 C6 0C 81 FE ? ? ? ? 0F 8C");
        const auto counter = *first.get_first<uintptr_t>(1);
        const auto original = reinterpret_cast<const int*>(counter - 8);
        std::copy_n(original, 18, MenuKeys.begin());
        MenuKeys[18] = 'P';
        MenuKeys[19] = 48;
        MenuKeys[21] = VK_F1;
        MenuKeys[22] = 49;
        // Extend the engine's event table; native polling, release edges and repeat stay intact.
        const auto keys = reinterpret_cast<uintptr_t>(MenuKeys.data());
        WriteGameValue(first.get_first(1), keys + 8);
        WriteGameValue(repeat.get_first(2), keys + 14 * sizeof(int));
        WriteGameValue(end.get_first(5), keys + 26 * sizeof(int));
        PadShortcutsEnabled = true;
    }

    inline void Init()
    {
        auto options = hook::pattern("81 EC D0 01 00 00 53 55 56 57 E8 ? ? ? ? E8");
        auto loop = hook::pattern("83 EC 24 53 8B 1D ? ? ? ? 56 57 6A 01 6A 00 8B F9 8B 47 04");
        auto construct = hook::pattern("51 8B 44 24 18 53 55 56 57 8B E9");
        auto destroy = hook::pattern("56 8B F1 8B 46 1C 57 33 FF 85 C0 C7 06");
        const auto start = reinterpret_cast<uintptr_t>(options.get_first());
        auto vtable = hook::range_pattern(start, start + 0x100, "C7 44 24 34 ? ? ? ? DB 05");
        auto allocation = hook::range_pattern(start, start + 0x800, "68 94 00 00 00 E8");
        OptionsVtable = *vtable.get_first<uintptr_t>(4);
        Allocate = reinterpret_cast<decltype(Allocate)>(injector::GetBranchDestination(allocation.get_first(5)).as_int());
        ConstructButton = reinterpret_cast<decltype(ConstructButton)>(construct.get_first(-21));
        for (const auto& setting : FixSettings().GetDescription().settings)
            if (setting.apply == "restart") RestartValues.emplace(setting.id, FixSettings().Wanted(setting.id));
        EventHook = safetyhook::create_inline((*reinterpret_cast<uintptr_t*>(OptionsVtable)), Event);
        LoopHook = safetyhook::create_inline(loop.get_first(), Loop);
        DestroyHook = safetyhook::create_inline(destroy.get_first(), Destroy);
        const auto loopStart = reinterpret_cast<uintptr_t>(loop.get_first());
        auto render = hook::range_pattern(loopStart, loopStart + 0x300,
            "8B 47 08 8B 08 50 FF 91 A8 00 00 00 8B 7F 08");
        RenderHook = safetyhook::create_mid(render.get_first(), [](SafetyHookContext& regs)
        {
            if (Render)
                Render(*reinterpret_cast<IDirect3DDevice9**>(regs.edi + 8),
                    *reinterpret_cast<HWND*>(regs.edi + 4));
        });
        EnablePadShortcuts(loopStart);
    }
}

#pragma once
#include "NativeMenuContracts.hpp"
#include "MenuLayout.hpp"
#include "../Shared/RWGTA/FixMenu.hpp"
#include <array>
#include <deque>

namespace PunMenu
{
    constexpr uintptr_t Options = 0x74B9B8, OptionsList = 0x74B938, OptionsBackground = 0x74B4B8;
    constexpr uintptr_t GameSettings = 0x74A4F0, GameLabels = 0x749FE0, GameValues = 0x74A470;
    inline const std::set<std::string> joined{ "MAIN.HudAspectRatioConstraint", "MAIN.FOVFactor" };
    template <class T> T Read(const void* base, size_t offset = 0)
    {
        T value{};
        MEMORY_BASIC_INFORMATION memory{};
        auto address = reinterpret_cast<uintptr_t>(base) + offset;
        if (base && VirtualQuery(reinterpret_cast<void*>(address), &memory, sizeof(memory)) && memory.State == MEM_COMMIT &&
            !(memory.Protect & (PAGE_NOACCESS | PAGE_GUARD)) && address + sizeof(T) <= reinterpret_cast<uintptr_t>(memory.BaseAddress) + memory.RegionSize)
            std::memcpy(&value, reinterpret_cast<void*>(address), sizeof(value));
        return value;
    }
    template <class T> T Read(uintptr_t base, size_t offset = 0) { return Read<T>(reinterpret_cast<void*>(base), offset); }
    template <class T> void Write(uintptr_t base, size_t offset, T value)
    {
        ProtectedGameRef<T>{ reinterpret_cast<T*>(base + offset) } = value;
    }
    inline void Log(const char* message)
    {
        OutputDebugStringA(("The Punisher native menu: " + std::string(message) + "\n").c_str());
    }
    inline std::string NativeText(const std::string& text)
    {
        auto count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);
        if (count <= 0 || count > 8192) return {};
        std::wstring wide(count, L'\0');
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), wide.data(), count);
        CharUpperBuffW(wide.data(), count);
        count = WideCharToMultiByte(1252, WC_NO_BEST_FIT_CHARS, wide.data(), static_cast<int>(wide.size()), nullptr, 0, "?", nullptr);
        std::string result(count, '\0');
        WideCharToMultiByte(1252, WC_NO_BEST_FIT_CHARS, wide.data(), static_cast<int>(wide.size()), result.data(), count, "?", nullptr);
        return result;
    }
    inline std::pair<int, int> Measure(const std::string& text, uintptr_t list)
    {
        int width{}, height{};
        auto font = Read<int>(Read<void*>(list, 76));
        reinterpret_cast<void(__cdecl*)(int*, int*, const char*, int, int)>(0x5185E0)(&width, &height, text.c_str(), -1, font);
        return { width, height };
    }
    inline std::vector<std::string> Wrap(const std::string& text, uintptr_t list, int width)
    {
        std::vector<std::string> lines;
        auto native = NativeText(text);
        while (!native.empty())
        {
            size_t length = native.find('\n');
            if (length == std::string::npos) length = native.size();
            while (length && Measure(native.substr(0, length), list).first > width) --length;
            if (!length) length = 1;
            if (length < native.size() && native[length] != '\n')
            {
                auto space = native.rfind(' ', length);
                if (space != std::string::npos && space > 0) length = space;
            }
            lines.push_back(native.substr(0, length));
            native.erase(0, length);
            while (!native.empty() && (native.front() == '\n' || native.front() == ' ')) native.erase(0, 1);
        }
        if (lines.empty()) lines.emplace_back();
        return lines;
    }
    struct Binding
    {
        uintptr_t widget{}, provider{};
        int count{};
        const char*** source{};
        uint8_t* availability{};
        std::array<uint8_t, 20> disabled{};
        std::array<int, 4> rectangle{};
        std::vector<std::string> text;
        std::vector<const char*> values;
        std::vector<const char**> slots;
        std::vector<uint8_t> enabled;
        bool bound{};
        bool Capture(uintptr_t list)
        {
            if (bound) return widget == list;
            auto data = Read<uintptr_t>(list, 100);
            if (Read<uintptr_t>(list) != 0x5FBAC0 || Read<uintptr_t>(data) != 0x5FE378) return false;
            int rows = Read<int>(data, 360);
            auto labels = Read<const char***>(data, 376);
            if (rows < 0 || rows > 18 || !labels) return false;
            widget = list;
            provider = data;
            count = rows;
            source = labels;
            availability = Read<uint8_t*>(data, 388);
            for (size_t i = 0; i < disabled.size(); ++i) disabled[i] = Read<uint8_t>(data, 4 + i);
            for (size_t i = 0; i < rectangle.size(); ++i) rectangle[i] = Read<int>(list, 44 + i * 4);
            bound = true;
            return true;
        }
        const char** Original(size_t index) const { return index < static_cast<size_t>(count) ? Read<const char**>(source, index * 4) : nullptr; }
        void Publish(std::vector<std::string> labels, std::vector<bool> selectable, size_t originals = 0)
        {
            text = std::move(labels);
            values.clear();
            slots.clear();
            enabled.clear();
            values.reserve(text.size());
            slots.reserve(text.size());
            enabled.reserve(text.size());
            for (const auto& label : text) values.push_back(label.c_str());
            for (size_t i = 0; i < text.size(); ++i)
            {
                auto original = i < originals ? Original(i) : nullptr;
                slots.push_back(original ? original : &values[i]);
                bool available = selectable[i];
                if (i < originals && i < static_cast<size_t>(count))
                    available &= !disabled[i] && (!availability || Read<uint8_t>(availability, i) != 0);
                enabled.push_back(available ? 1 : 0);
            }
            for (size_t i = 0; i < text.size(); ++i) Write<uint8_t>(provider, 4 + i, enabled[i] ? 0 : 1);
            Write<int>(provider, 360, static_cast<int>(text.size()));
            Write<const char***>(provider, 376, slots.data());
            Write<uint8_t*>(provider, 388, enabled.data());
            Write<int>(widget, 56, std::max(rectangle[3], static_cast<int>(text.size()) * Read<int>(widget, 92)));
        }
        void Restore()
        {
            if (!bound) return;
            Write<int>(provider, 360, count);
            Write<const char***>(provider, 376, source);
            Write<uint8_t*>(provider, 388, availability);
            for (size_t i = 0; i < disabled.size(); ++i) Write<uint8_t>(provider, 4 + i, disabled[i]);
            for (size_t i = 0; i < rectangle.size(); ++i) Write<int>(widget, 44 + i * 4, rectangle[i]);
            bound = false;
        }
    };
    struct Item
    {
        RWGTAMenu::Row row{};
        size_t page{ SIZE_MAX };
        std::string text;
        bool next{}, previous{}, back{}, details{}, selectable{ true };
    };
    struct State
    {
        HMODULE module{};
        RWGTAMenu::Model model;
        SafetyHookInline optionsInit, optionsInput, gameInit, gameInput, defaults;
        SafetyHookInline draw;
        Binding options, labels, values;
        std::vector<Item> items;
        uintptr_t activeRoot{};
        size_t page{ SIZE_MAX }, slice{}, helpSlice{};
        std::string title, help;
        const char* titlePointer{};
        const char** originalTitle{};
        std::array<int, 2> originalTitleSize{};
        std::array<int, 2> originalBackgroundSize{};
        ULONGLONG lastDiscrete{};
        bool enabled{}, busy{}, open{}, pause{}, details{}, capturedTitle{}, modelOpen{};
    };
    inline State& Get() { static auto* state = new State; return *state; }
    inline bool Current(uintptr_t root)
    {
        return Get().activeRoot == root && Read<int>(root, 164) == 0;
    }
    inline int Selected(uintptr_t list) { return Read<int>(list, 96); }
    inline void Select(uintptr_t list, int index)
    {
        reinterpret_cast<void(__thiscall*)(void*, int)>(0x478BB0)(reinterpret_cast<void*>(list), index);
    }
    inline void Model(bool force = false)
    {
        auto& state = Get();
        if (state.modelOpen && !force) return;
        state.model.OpenPlugins(state.module, "en", 32, joined);
        state.modelOpen = true;
    }
    inline void Title(const std::string& title)
    {
        auto& state = Get();
        if (!state.capturedTitle)
        {
            state.originalTitle = Read<const char**>(Options, 132);
            state.originalTitleSize = { Read<int>(Options, 152), Read<int>(Options, 156) };
            state.originalBackgroundSize = { Read<int>(OptionsBackground, 52), Read<int>(OptionsBackground, 56) };
            state.capturedTitle = true;
        }
        state.title = NativeText(title);
        state.titlePointer = state.title.c_str();
        Write<const char**>(Options, 132, &state.titlePointer);
        int width{}, height{};
        reinterpret_cast<void(__cdecl*)(int*, int*, const char*, int, int)>(0x5185E0)(&width, &height, state.titlePointer, -1, Read<int>(0x67604C));
        Write<int>(Options, 152, width);
        Write<int>(Options, 156, height);
    }
    inline void RestoreTitle()
    {
        auto& state = Get();
        if (!state.capturedTitle) return;
        Write<const char**>(Options, 132, state.originalTitle);
        Write<int>(Options, 152, state.originalTitleSize[0]);
        Write<int>(Options, 156, state.originalTitleSize[1]);
        Write<int>(OptionsBackground, 52, state.originalBackgroundSize[0]);
        Write<int>(OptionsBackground, 56, state.originalBackgroundSize[1]);
        state.capturedTitle = false;
    }
    inline bool Entry()
    {
        auto& state = Get();
        if (!state.options.Capture(OptionsList) || state.options.count != 3) return false;
        Model();
        std::vector<std::string> text(3);
        text.push_back(NativeText(state.model.Text("menu.plugins")));
        std::vector<bool> enabled(4, true);
        state.options.Publish(std::move(text), std::move(enabled), 3);
        return true;
    }
    inline void Joined()
    {
        auto& state = Get();
        if (!state.labels.Capture(GameLabels) || !state.values.Capture(GameValues) || state.labels.count != 6) return;
        Model();
        const auto& rows = state.model.JoinedRows();
        std::vector<std::string> labels(6), values(6);
        for (const auto& row : rows)
        {
            labels.push_back(NativeText(state.model.Label(row)));
            values.push_back(NativeText(state.model.Value(row)));
        }
        state.labels.Publish(std::move(labels), std::vector<bool>(6 + rows.size(), true), 6);
        state.values.Publish(std::move(values), std::vector<bool>(6 + rows.size(), true), 6);
    }
    inline void Close()
    {
        auto& state = Get();
        state.open = state.details = false;
        state.items.clear();
        RestoreTitle();
        Entry();
        Select(OptionsList, 3);
    }
    inline bool Build()
    {
        auto& state = Get();
        if (!Current(Options) || !state.options.bound) return false;
        state.busy = true;
        state.items.clear();
        const auto& pages = state.model.Pages();
        std::vector<Item> all;
        if (state.details)
        {
            auto lines = Wrap(state.help.empty() ? state.model.Text("menu.presethelp") : state.help, OptionsList, 510);
            size_t begin = state.helpSlice * 6;
            for (size_t i = begin; i < std::min(begin + 6, lines.size()); ++i) all.push_back({ {}, SIZE_MAX, lines[i], false, false, false, false, false });
            if (begin) all.push_back({ {}, SIZE_MAX, NativeText(state.model.Text("menu.previous")), false, true });
            if (begin + 6 < lines.size()) all.push_back({ {}, SIZE_MAX, NativeText(state.model.Text("menu.next")), true });
            all.push_back({ {}, SIZE_MAX, NativeText(state.model.Text("menu.back")), false, false, true });
            Title(state.model.Text("menu.details"));
        }
        else
        {
            auto visible = state.page == SIZE_MAX ? std::vector<const RWGTAMenu::Row*>{} : state.model.VisibleRows(state.page);
            auto count = state.page == SIZE_MAX ? pages.size() : visible.size();
            size_t begin = state.slice * 2;
            for (size_t i = begin; i < std::min(begin + 2, count); ++i)
            {
                Item item;
                std::string text;
                if (state.page == SIZE_MAX)
                {
                    item.page = i;
                    text = pages[i].title;
                }
                else
                {
                    item.row = *visible[i];
                    text = state.model.Label(item.row) + ": " + state.model.Value(item.row);
                }
                auto lines = Wrap(text, OptionsList, 510);
                for (size_t line = 0; line < lines.size() && all.size() < 12; ++line)
                {
                    auto part = item;
                    part.text = lines[line];
                    part.selectable = line == 0;
                    all.push_back(std::move(part));
                }
            }
            if (begin) all.push_back({ {}, SIZE_MAX, NativeText(state.model.Text("menu.previous")), false, true });
            if (begin + 2 < count) all.push_back({ {}, SIZE_MAX, NativeText(state.model.Text("menu.next")), true });
            if (state.page != SIZE_MAX)
                all.push_back({ {}, SIZE_MAX, NativeText(state.model.Text("menu.details")) + (state.model.RestartRequired() ? ": " + NativeText(state.model.Text("menu.restartshort")) : ""), false, false, false, true });
            all.push_back({ {}, SIZE_MAX, NativeText(state.model.Text("menu.back")), false, false, true });
            Title(state.page == SIZE_MAX ? state.model.Text("menu.plugins") : "The Punisher");
        }
        if (all.empty() || all.size() > 18)
        {
            state.busy = false;
            return false;
        }
        state.items = std::move(all);
        std::vector<std::string> text;
        std::vector<bool> enabled;
        for (const auto& item : state.items)
        {
            text.push_back(item.text);
            enabled.push_back(item.selectable);
        }
        state.options.Publish(std::move(text), std::move(enabled));
        Write<int>(OptionsList, 52, 520);
        Write<int>(OptionsBackground, 52, 580);
        Write<int>(OptionsBackground, 56, 30 + static_cast<int>(state.items.size()) * Read<int>(OptionsList, 92));
        auto first = std::find_if(state.items.begin(), state.items.end(), [](const auto& item) { return item.selectable; });
        Select(OptionsList, static_cast<int>(first - state.items.begin()));
        state.busy = false;
        return true;
    }
    inline bool Press(int input, int key) { return reinterpret_cast<int(__cdecl*)(int, int)>(0x53D870)(input, key) != 0; }
    inline bool Repeat(int input, int key) { return reinterpret_cast<int(__cdecl*)(int, int)>(0x53D790)(input, key) != 0; }
    struct Input { bool back{}, choose{}, left{}, right{}; };
    inline Input Events(int input, uintptr_t root, uintptr_t list)
    {
        int back = -1, row = -1, direction = -1;
        auto* hit = reinterpret_cast<void*(__thiscall*)(void*, int*, int*, int*, int)>(0x4794C0)(reinterpret_cast<void*>(root), &back, &row, &direction, 0);
        if (reinterpret_cast<uintptr_t>(hit) == list && row >= 0) Select(list, row);
        return { Press(input, 1) || back == 0, Press(input, 28) || back == 1 || row >= 0,
                 Repeat(input, 203) || direction == 0, Repeat(input, 205) || direction == 1 };
    }
    inline uintptr_t __cdecl OptionsInput(int input, void* origin)
    {
        auto& state = Get();
        if (!state.enabled || state.busy || !Current(Options)) return state.optionsInput.ccall<uintptr_t>(input, origin);
        auto event = Events(input, Options, OptionsList);
        if (!state.open)
        {
            if (Selected(OptionsList) != 3 || !event.choose) return state.optionsInput.ccall<uintptr_t>(input, origin);
            if (GetTickCount64() - state.lastDiscrete < 180) return 0;
            state.lastDiscrete = GetTickCount64();
            Model(true);
            state.open = true;
            state.details = false;
            state.page = SIZE_MAX;
            state.slice = 0;
            if (!Build()) Close();
            return 0;
        }
        if ((event.choose || event.back) && GetTickCount64() - state.lastDiscrete < 180) return 0;
        if (!(event.choose || event.back || event.left || event.right)) return 0;
        if (event.choose || event.back) state.lastDiscrete = GetTickCount64();
        int index = Selected(OptionsList);
        if (index < 0 || static_cast<size_t>(index) >= state.items.size()) return 0;
        auto item = state.items[index];
        if (event.back || (event.choose && item.back))
        {
            if (state.details)
            {
                state.details = false;
            }
            else if (state.page == SIZE_MAX)
            {
                Close();
                return 0;
            }
            else
            {
                state.page = SIZE_MAX;
                state.slice = 0;
            }
        }
        else if (event.choose && item.next)
        {
            if (state.details) ++state.helpSlice;
            else ++state.slice;
        }
        else if (event.choose && item.previous)
        {
            if (state.details && state.helpSlice) --state.helpSlice;
            else if (state.slice) --state.slice;
        }
        else if (event.choose && item.details)
        {
            state.details = true;
            state.helpSlice = 0;
        }
        else if (event.choose && item.page != SIZE_MAX)
        {
            state.page = item.page;
            state.slice = 0;
            state.model.PageShown(state.page);
        }
        else if (!state.details && item.selectable && state.model.Enabled(item.row, state.pause))
        {
            state.model.Change(item.row, event.left ? -1 : event.right ? 1 : 0, state.pause);
        }
        if (!Build()) Close();
        RWGTAMenu::Model::Changed() = false;
        return 0;
    }
    inline uintptr_t __cdecl GameInput(int input, void* origin)
    {
        auto& state = Get();
        if (!state.enabled || state.busy || !Current(GameSettings) || !state.labels.bound)
            return state.gameInput.ccall<uintptr_t>(input, origin);
        auto event = Events(input, GameSettings, GameLabels);
        int row = Selected(GameLabels) - state.labels.count;
        if (row < 0 || static_cast<size_t>(row) >= state.model.JoinedRows().size() || event.back)
            return state.gameInput.ccall<uintptr_t>(input, origin);
        if (event.choose || event.left || event.right)
        {
            state.model.Change(state.model.JoinedRows()[row], event.left ? -1 : event.right ? 1 : 0, state.pause);
            Joined();
            RWGTAMenu::Model::Changed() = false;
        }
        return 0;
    }
    inline uintptr_t __cdecl OptionsInit(int* previous)
    {
        auto& state = Get();
        state.options.Restore();
        RestoreTitle();
        state.open = state.details = false;
        auto result = state.optionsInit.ccall<uintptr_t>(previous);
        state.activeRoot = Options;
        state.pause = previous && *previous == 55;
        Model(true);
        Entry();
        return result;
    }
    inline uintptr_t __cdecl GameInit()
    {
        auto& state = Get();
        state.labels.Restore();
        state.values.Restore();
        auto result = state.gameInit.ccall<uintptr_t>();
        state.activeRoot = GameSettings;
        Model();
        Joined();
        return result;
    }
    inline uintptr_t __cdecl Defaults()
    {
        auto result = Get().defaults.ccall<uintptr_t>();
        RWGTAMenu::RestoreDefaults(FixSettings(), joined);
        return result;
    }
    inline void UpdateDraw(uintptr_t root)
    {
        auto& state = Get();
        if (!state.enabled || state.busy) return;
        state.activeRoot = root;
        if (root != Options)
        {
            state.options.Restore();
            RestoreTitle();
            state.open = state.details = false;
        }
        else if (!state.options.bound)
        {
            Entry();
        }
        if (root != GameSettings)
        {
            state.labels.Restore();
            state.values.Restore();
        }
        else if (!state.labels.bound)
        {
            Joined();
        }
        if (state.open && !state.details)
        {
            auto index = Selected(OptionsList);
            if (index >= 0 && static_cast<size_t>(index) < state.items.size())
            {
                const auto& item = state.items[index];
                if (item.page == SIZE_MAX && !item.next && !item.previous && !item.back && !item.details && item.selectable)
                    state.help = state.model.Label(item.row) + "\n" + state.model.Help(item.row);
            }
        }
        if (RWGTAMenu::Model::Changed().exchange(false))
        {
            if (state.open)
            {
                if (!Build()) Close();
            }
            else if (root == GameSettings)
            {
                Joined();
            }
        }
    }
    inline uintptr_t __fastcall Draw(void* root, void*)
    {
        UpdateDraw(reinterpret_cast<uintptr_t>(root));
        PunMenuLayout::Scope layout(reinterpret_cast<uintptr_t>(root));
        return Get().draw.fastcall<uintptr_t>(root, nullptr);
    }
    inline void Init()
    {
        if (!SupportedImage())
        {
            Log("Native menu unavailable for this executable");
            return;
        }
        auto& state = Get();
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN, reinterpret_cast<LPCWSTR>(&Init), &state.module);
        state.optionsInit = safetyhook::create_inline(reinterpret_cast<void*>(0x486F40), OptionsInit);
        state.optionsInput = safetyhook::create_inline(reinterpret_cast<void*>(0x486FE0), OptionsInput);
        state.gameInit = safetyhook::create_inline(reinterpret_cast<void*>(0x484160), GameInit);
        state.gameInput = safetyhook::create_inline(reinterpret_cast<void*>(0x484200), GameInput);
        state.defaults = safetyhook::create_inline(reinterpret_cast<void*>(0x483FD0), Defaults);
        state.draw = safetyhook::create_inline(reinterpret_cast<void*>(0x48AB20), Draw);
        state.enabled = state.optionsInit && state.optionsInput && state.gameInit && state.gameInput && state.defaults && state.draw;
        if (!state.enabled)
        {
            state.optionsInit.reset();
            state.optionsInput.reset();
            state.gameInit.reset();
            state.gameInput.reset();
            state.defaults.reset();
            state.draw.reset();
            Log("Native menu hooks unavailable");
        }
    }
} // namespace PunMenu

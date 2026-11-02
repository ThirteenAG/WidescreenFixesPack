module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/FixMenu.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"
#include <array>
#include <climits>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <unordered_map>

export module NativeMenu;

import ComVars;

export namespace RevMenuNative
{
    constexpr size_t MaximumRows = 9;
    constexpr size_t ActiveFlags = 328;
    constexpr size_t Parent = 648;
    constexpr size_t State = 696;
    constexpr size_t Selection = 704;
    constexpr size_t Actions = 708;
    constexpr size_t RowCount = 744;
    constexpr size_t Header = 748;
    constexpr size_t Rows = 756;
    constexpr size_t Labels = 844;

    template<class T> const T& Read(const void* object, size_t offset)
    {
        return *reinterpret_cast<const T*>(static_cast<const uint8_t*>(object) + offset);
    }

    template<class T> void Write(void* object, size_t offset, T value)
    {
        if (Read<T>(object, offset) != value)
            std::memcpy(static_cast<uint8_t*>(object) + offset, &value, sizeof(value));
    }

    inline void* Initialize{};
    inline void* Input{};
    inline void* Destroy{};
    inline void* Lookup{};
    inline void (__thiscall* Select)(void*, uint8_t, uint8_t){};
    inline void (__thiscall* SetMessage)(void*, int){};
    inline void (__thiscall* ShowHelp)(void*, int, int){};
    inline void (__thiscall* HideHelp)(void*, uint8_t){};
    inline uint8_t (__thiscall* HitTest)(void*, void*, const float*, float){};
    inline void* (__thiscall* HitPart)(void*, int){};
    inline void (__thiscall* AnimateReference)(void*, int, uint8_t){};
    inline void (__thiscall* AnimateWidget)(void*, int){};
    inline uint8_t (__cdecl* HasNetwork)(){};

    inline void* Find(const char* signature)
    {
        return hook::get_pattern(signature);
    }

    inline bool Bind()
    {
        Initialize = reinterpret_cast<decltype(Initialize)>(Find("8B 44 24 04 83 EC 08 53 56 57 8B F1 89 86 88 02 00 00 8B 0D ? ? ? ? 8B 11 8B 42 30 BB 01 00 00 00 53 68 ? ? ? ?"));
        Input = reinterpret_cast<decltype(Input)>(Find("55 8B EC 83 E4 F0 83 EC 24 53 56 57 8B F1 6A 01 8D 8E EC 02 00 00 E8 ? ? ? ? 3C 01 74 ? 6A 03 8D 8E EC 02 00 00 E8 ? ? ? ?"));
        Destroy = reinterpret_cast<decltype(Destroy)>(Find("56 8B F1 C7 06 ? ? ? ? A1 ? ? ? ? 80 78 34 00 75 ? A1 ? ? ? ? 85 C0 74 ? 83 48 24 04 8D 8E 70 02 00 00 51"));
        Lookup = reinterpret_cast<decltype(Lookup)>(Find("8B 44 24 04 39 81 88 00 00 00 77 ? B8 ? ? ? ? C2 04 00"));
        Select = reinterpret_cast<decltype(Select)>(Find("53 56 BB 01 00 00 00 57 8B F1 38 5C 24 14 75 ? 55 8B AE E8 02 00 00 85 ED 7E ? 8D BE F4 02 00 00 6A 00 6A 04 8B CF E8 ? ? ? ?"));
        SetMessage = reinterpret_cast<decltype(SetMessage)>(Find("56 57 8B 7C 24 0C 8B F1 39 7E 04 0F 84 ? ? ? ? 89 7E 04 83 FF FF 75 ? A1 ? ? ? ? 05 68 07 00 00 50 FF 15 ? ? ? ?"));
        ShowHelp = reinterpret_cast<decltype(ShowHelp)>(Find("8B 44 24 08 8B 54 24 04 50 8B 81 9C 02 00 00 52 50 E8 ? ? ? ? C2 08 00"));
        HideHelp = reinterpret_cast<decltype(HideHelp)>(Find("8B 89 90 02 00 00 33 C0 80 7C 24 04 01 0F 95 C0 83 C0 02 89 44 24 04 E9 ? ? ? ? CC CC CC CC 56 8B F1 8B 8E 90 02 00 00"));
        HitTest = reinterpret_cast<decltype(HitTest)>(Find("55 8B EC 83 E4 F0 8B C1 8B 4D 08 83 EC 10 85 C9 75 ? 32 C0 8B E5 5D C2 0C 00"));
        HitPart = reinterpret_cast<decltype(HitPart)>(Find("57 8B 7C 24 08 8B C1 39 78 50 74 ? 56 8B 70 68 85 F6 74 ? 57 8B CE E8 ? ? ? ? 85 C0 75 ? 8B 76 6C 85 F6 75 ? 33 C0"));
        AnimateReference = reinterpret_cast<decltype(AnimateReference)>(Find("8B 44 24 04 56 8B F1 8B 4E 04 39 41 0C 75 ? 80 7C 24 0C 01 75 ? 8B 0E 8B 01 8B 50 64 FF D2 EB ? 6A 00 50 E8 ? ? ? ?"));
        AnimateWidget = reinterpret_cast<decltype(AnimateWidget)>(Find("56 8B F1 8B 8E EC 00 00 00 85 C9 74 ? 8B 81 94 00 00 00 85 C0 74 ? 8B 00 EB ? 83 C8 FF 8B 54 24 08 3B C2 74 ? 52 E8 ? ? ? ?"));
        HasNetwork = reinterpret_cast<decltype(HasNetwork)>(Find("A1 ? ? ? ? 85 C0 74 ? 8B 48 70 85 C9 75 ? 32 C0 C3"));
        const auto input = static_cast<uint8_t*>(Find("A1 ? ? ? ? 8B 88 70 08 00 00 66 0F 6E 80 00 06 00 00 69 C9 CC 02 00 00 8B 94 01 A0 01 00 00"));
        const auto help = static_cast<uint8_t*>(Find("8B 04 B5 ? ? ? ? 8B 0D ? ? ? ? 8D 56 F9 F7 DA 1B D2 83 E2 E3 83 C2 21 52 50 E8 ? ? ? ?"));
        pInputManager.SetAddress(*reinterpret_cast<uintptr_t**>(input + 1));
        pHelpManager.SetAddress(*reinterpret_cast<uintptr_t**>(help + 9));
        return Initialize && Input && Destroy && Lookup && Select && SetMessage && ShowHelp && HideHelp && HitTest &&
            HitPart && AnimateReference && AnimateWidget && HasNetwork && pInputManager.get_ptr() && pHelpManager.get_ptr();
    }
}

export namespace RevMenu
{
    namespace Native = RevMenuNative;
    constexpr int MessageBase = 0x6F450000;
    enum class Kind { Empty, Title, Plugin, Setting, Previous, Next, Details, Back };
    struct Item
    {
        Kind kind{ Kind::Empty };
        std::string label;
        RWGTAMenu::Row row{};
        size_t target{};
    };
    struct Host
    {
        void* object{};
        RWGTAMenu::Model model;
        std::array<int, Native::MaximumRows> actions{}, labels{}, files{};
        std::array<Item, Native::MaximumRows> items{};
        size_t slots{}, logical{}, offset{}, page{}, first{}, selected{ 1 }, helpFirst{}, helpReturnSelection{ 1 };
        uint32_t language{};
        int mouseX{ INT_MIN }, mouseY{ INT_MIN };
        bool plugins{}, details{}, transition{}, helpHeld{}, inputSeen{}, alive{ true };
        std::string helpTitle, previewTitle, previewHelp;
        std::string renderKey;
        std::vector<std::string> help;
    };
    inline HMODULE module{};
    inline int (*Width)(){};
    inline int (*Height)(){};
    inline SafetyHookInline initializeHook, inputHook, destroyHook, messageHook;
    inline std::unordered_map<void*, std::shared_ptr<Host>> hosts;
    inline std::map<std::pair<HMODULE, std::string>, std::string> restartValues;
    inline std::mutex messagesMutex;
    inline std::map<int, std::string> messages;
    inline std::map<std::string, int> messageIds;
    inline std::atomic<uint32_t> languageGeneration{};
    inline std::string updateNotice;

    inline bool Current(const std::shared_ptr<Host>& host)
    {
        const auto found = host ? hosts.find(host->object) : hosts.end();
        return host && host->alive && found != hosts.end() && found->second == host;
    }

    inline std::string Language()
    {
        auto language = FixSettings().String("MAIN.TextLanguage");
        if (!language.empty() && language != "auto")
            return language;
        switch (PRIMARYLANGID(GetUserDefaultUILanguage()))
        {
        case LANG_GERMAN: return "de";
        case LANG_FRENCH: return "fr";
        case LANG_ITALIAN: return "it";
        case LANG_SPANISH: return "es";
        default: return "en";
        }
    }

    inline int Intern(const std::string& text)
    {
        if (text.empty())
            return -1;
        std::lock_guard lock(messagesMutex);
        if (const auto found = messageIds.find(text); found != messageIds.end())
            return found->second;
        if (messages.size() >= 0x100000)
            return -1;
        const int id = MessageBase + static_cast<int>(messages.size());
        messages.emplace(id, text);
        messageIds.emplace(text, id);
        return id;
    }

    inline const char* __fastcall Message(void* object, void*, unsigned id)
    {
        if (id >= MessageBase && id < MessageBase + 0x100000)
        {
            std::lock_guard lock(messagesMutex);
            if (const auto found = messages.find(static_cast<int>(id)); found != messages.end())
                return found->second.c_str();
        }
        return messageHook.unsafe_thiscall<const char*>(object, id);
    }

    inline std::vector<std::string> Wrap(const std::string& text, size_t columns = 58)
    {
        std::vector<std::string> lines;
        std::string line;
        size_t characters = 0;
        for (size_t i = 0; i < text.size();)
        {
            const unsigned char first = text[i];
            const size_t length = first < 0x80 ? 1 : first < 0xE0 ? 2 : first < 0xF0 ? 3 : 4;
            if (text[i] == '\n' || characters >= columns)
            {
                const auto space = line.find_last_of(' ');
                if (text[i] != '\n' && space != std::string::npos)
                {
                    lines.push_back(line.substr(0, space));
                    line.erase(0, space + 1);
                    characters = 0;
                    for (const unsigned char c : line)
                        characters += (c & 0xC0) != 0x80;
                }
                else
                {
                    lines.push_back(std::move(line));
                    line.clear();
                    characters = 0;
                }
                if (text[i] == '\n')
                {
                    ++i;
                    continue;
                }
            }
            if (text[i] != '\r' && !(line.empty() && text[i] == ' '))
            {
                line.append(text, i, (std::min)(length, text.size() - i));
                ++characters;
            }
            i += (std::min)(length, text.size() - i);
        }
        if (!line.empty())
            lines.push_back(std::move(line));
        return lines;
    }

    inline void Remember(const RWGTAMenu::Model& model)
    {
        for (const auto& page : model.Pages())
            if (page.plugin)
                for (const auto& setting : page.plugin->file.description.settings)
                    if (setting.HasValue() && setting.apply == "restart")
                        restartValues.try_emplace({ page.plugin->module, setting.id }, OtherPlugins::Get(*page.plugin, setting));
    }

    inline bool RestartRequired(const RWGTAMenu::Model& model)
    {
        for (const auto& page : model.Pages())
            if (page.plugin)
                for (const auto& setting : page.plugin->file.description.settings)
                {
                    if (!setting.HasValue() || setting.apply != "restart")
                        continue;
                    const auto initial = restartValues.find({ page.plugin->module, setting.id });
                    if (initial != restartValues.end() && initial->second != OtherPlugins::Get(*page.plugin, setting))
                        return true;
                }
        return false;
    }

    inline void OpenModel(Host& host)
    {
        host.model.OpenPlugins(module, Language(), host.slots - 5, {}, true);
        host.language = languageGeneration.load();
        Remember(host.model);
        if (host.page > host.model.Pages().size())
            host.page = host.first = 0;
    }

    // A row label is { widget, message id, message file }; rows may read different files.
    inline void Label(Host& host, size_t row, int message, int file)
    {
        const auto offset = Native::Labels + 12 * row;
        if (Native::Read<int>(host.object, offset + 8) != file)
        {
            Native::Write<int>(host.object, offset + 8, file);
            Native::Write<int>(host.object, offset + 4, INT_MIN);
        }
        Native::SetMessage(static_cast<uint8_t*>(host.object) + offset, message);
    }

    inline void Help(const std::string& text)
    {
        if (pHelpManager.is_initialized() && pHelpManager.get())
            Native::ShowHelp(reinterpret_cast<void*>(pHelpManager.get()), Intern(text), 4);
    }

    inline void Restore(Host& host, size_t selection)
    {
        for (size_t i = 0; i < host.slots; ++i)
        {
            Native::Write<int>(host.object, Native::Actions + 4 * i, host.actions[i]);
            Label(host, i, host.labels[i], host.files[i]);
        }
        Native::Write<int>(host.object, Native::Selection, static_cast<int>((std::min)(selection, host.slots - 1)));
        host.renderKey.clear();
        Native::Select(host.object, 1, 1);
    }

    inline void Root(Host& host)
    {
        host.logical = (std::min)(host.logical, host.slots);
        host.offset = host.logical >= host.slots ? 1 : (std::min)(host.offset, host.logical);
        for (size_t i = 0; i < host.slots; ++i)
        {
            const size_t logical = i + host.offset;
            Native::Write<int>(host.object, Native::Actions + 4 * i, logical < host.slots ? host.actions[logical] : -1);
            if (logical < host.slots)
                Label(host, i, host.labels[logical], host.files[logical]);
            else
                Label(host, i, Intern(host.model.Text("menu.plugins")), host.files[0]);
        }
        Native::Write<int>(host.object, Native::Selection, static_cast<int>(host.logical - host.offset));
        const auto key = std::string("root:") + std::to_string(host.logical) + ":" + std::to_string(host.offset) + ":" + std::to_string(host.language);
        if (host.renderKey != key)
        {
            host.renderKey = key;
            Native::Select(host.object, 1, 1);
        }
        if (host.logical == host.slots)
            Help(host.model.Text("menu.pluginshelp"));
    }

    inline std::string Description(const Host& host, const Item& item)
    {
        if (item.kind == Kind::Setting)
            return host.model.Help(item.row);
        if (item.kind == Kind::Plugin)
            return host.model.Text("menu.pluginshelp");
        return {};
    }

    inline bool Selectable(const Item& item)
    {
        return item.kind != Kind::Empty && item.kind != Kind::Title;
    }

    inline void Populate(Host& host)
    {
        host.items = {};
        const size_t rows = host.slots - (host.page && !host.details ? 5 : 4);
        auto& items = host.items;
        items[0] = { Kind::Title, host.details ? host.helpTitle : host.page ? host.model.Pages()[host.page - 1].title : host.model.Text("menu.plugins") };
        if (host.details)
        {
            for (size_t i = 0; i < rows && i + host.helpFirst < host.help.size(); ++i)
                items[i + 1] = { Kind::Title, host.help[i + host.helpFirst] };
            if (host.helpFirst)
                items[host.slots - 3] = { Kind::Previous, host.model.Text("menu.previous") };
            if (host.helpFirst + rows < host.help.size())
                items[host.slots - 2] = { Kind::Next, host.model.Text("menu.next") };
        }
        else if (!host.page)
        {
            const auto& pages = host.model.Pages();
            for (size_t i = 0; i < rows && i + host.first < pages.size(); ++i)
                items[i + 1] = { Kind::Plugin, pages[i + host.first].title, {}, i + host.first + 1 };
            if (host.first)
                items[host.slots - 3] = { Kind::Previous, host.model.Text("menu.previous") };
            if (host.first + rows < pages.size())
                items[host.slots - 2] = { Kind::Next, host.model.Text("menu.next") };
        }
        else
        {
            const auto visible = host.model.VisibleRows(host.page - 1);
            if (host.first >= visible.size())
                host.first = 0;
            for (size_t i = 0; i < rows && i + host.first < visible.size(); ++i)
            {
                const auto row = *visible[i + host.first];
                auto label = host.model.Label(row);
                const auto value = host.model.Value(row);
                if (!value.empty())
                    label += ": " + value;
                items[i + 1] = { Kind::Setting, std::move(label), row };
            }
            if (host.first)
                items[host.slots - 3] = { Kind::Previous, host.model.Text("menu.previous") };
            if (host.first + rows < visible.size())
                items[host.slots - 2] = { Kind::Next, host.model.Text("menu.next") };
        }
        if (!host.details && host.page)
        {
            if (host.selected < host.slots && items[host.selected].kind == Kind::Setting)
            {
                host.previewTitle = host.model.Label(items[host.selected].row);
                host.previewHelp = Description(host, items[host.selected]);
            }
            if (!host.previewHelp.empty())
                items[host.slots - 4] = { Kind::Details, host.model.Text("menu.details") };
        }
        items[host.slots - 1] = { Kind::Back, host.model.Text("menu.back") };
        host.selected = (std::min)(host.selected, host.slots - 1);
        if (!Selectable(items[host.selected]))
        {
            host.selected = 1;
            while (host.selected + 1 < host.slots && !Selectable(items[host.selected]))
                ++host.selected;
        }
        std::string key = std::to_string(host.selected) + ":";
        for (size_t i = 0; i < host.slots; ++i)
        {
            Native::Write<int>(host.object, Native::Actions + 4 * i, -1);
            const int message = Intern(items[i].label);
            key += std::to_string(message) + ":";
            Label(host, i, message, host.files[0]);
        }
        Native::Write<int>(host.object, Native::Selection, static_cast<int>(host.selected));
        if (host.renderKey != key)
        {
            host.renderKey = std::move(key);
            Native::Select(host.object, 1, 1);
        }
        std::string footer;
        if (!host.details && !host.page && !updateNotice.empty() && FixSettings().Bool("MAIN.CheckForUpdates"))
        {
            // The update notice on the plugin list
            const auto lines = Wrap(updateNotice);
            for (size_t i = 0; i < lines.size() && i < 2; ++i)
                footer += (i ? "\n" : "") + lines[i];
        }
        else if (!host.details)
        {
            const auto description = Wrap(Description(host, items[host.selected]));
            if (!description.empty())
                footer = description.front() + "\n" + host.model.Text("menu.helphint");
        }
        if (RestartRequired(host.model))
        {
            if (!footer.empty())
                footer += '\n';
            footer += host.model.Text("menu.restartwarning");
        }
        Help(footer);
    }

    inline void Back(Host& host)
    {
        if (host.details)
        {
            host.details = false;
            host.selected = host.helpReturnSelection;
        }
        else if (host.page)
        {
            host.page = host.first = 0;
            host.selected = 1;
        }
        else
        {
            host.plugins = false;
            host.logical = host.slots;
            Root(host);
            return;
        }
        Populate(host);
    }

    inline void Activate(const std::shared_ptr<Host>& host, int direction)
    {
        const auto item = host->items[host->selected];
        const size_t rows = host->slots - (host->page && !host->details ? 5 : 4);
        switch (item.kind)
        {
        case Kind::Setting:
            host->model.Change(item.row, direction, false);
            break;
        case Kind::Plugin:
            if (direction)
                return;
            host->page = item.target;
            host->first = 0;
            host->selected = 1;
            host->model.PageShown(host->page - 1);
            break;
        case Kind::Details:
            if (direction || host->previewHelp.empty())
                return;
            host->helpTitle = host->previewTitle;
            host->help = Wrap(host->previewHelp);
            host->helpFirst = 0;
            host->helpReturnSelection = host->selected;
            host->details = true;
            host->selected = host->slots - 1;
            break;
        case Kind::Previous:
        case Kind::Next:
        {
            if (direction)
                return;
            auto& first = host->details ? host->helpFirst : host->first;
            first = item.kind == Kind::Next ? first + rows : first >= rows ? first - rows : 0;
            break;
        }
        case Kind::Back:
            if (!direction)
                Back(*host);
            return;
        default:
            return;
        }
        if (Current(host))
            Populate(*host);
    }

    inline int Hover(Host& host, int x, int y)
    {
        const int width = Width ? Width() : 0;
        const int height = Height ? Height() : 0;
        if (width <= 0 || height <= 0)
            return -1;
        const float diff = (static_cast<float>(width) / height) / (16.0f / 9.0f);
        const float scaleX = (std::min)(1.0f, 1.0f / diff);
        const float scaleY = (std::min)(1.0f, diff);
        const float point[4]{ (x - width * 0.5f) / scaleX + width * 0.5f,
            (y - height * 0.5f) / scaleY + height * 0.5f, 0.0f, 0.0f };
        for (size_t i = 0; i < host.slots; ++i)
        {
            if (host.plugins && !Selectable(host.items[i]))
                continue;
            auto widget = Native::Read<void*>(host.object, Native::Rows + i * 8);
            auto node = widget ? Native::Read<void*>(widget, 236) : nullptr;
            auto part = node ? Native::HitPart(node, 3) : nullptr;
            if (widget && part && Native::HitTest(widget, part, point, 1.0f))
                return static_cast<int>(i);
        }
        return -1;
    }

    inline bool NativeAction(Host& host)
    {
        const size_t selection = host.logical;
        const int action = host.actions[selection];
        if (action == 7 && Native::HasNetwork() && !Native::Read<uint8_t>(host.object, 693))
            return false;
        Restore(host, selection);
        auto object = static_cast<uint8_t*>(host.object);
        Native::AnimateReference(object + Native::Header, 3, 1);
        Native::AnimateReference(object + Native::Rows + selection * 8, 6, 1);
        host.transition = true;
        if (action == 8)
        {
            Native::Write<int>(object, Native::State, 3);
            return false;
        }
        if (action == 1)
        {
            Native::Write<int>(object, Native::State, 11);
            Native::Write<uint8_t>(object, 996, 1);
            return false;
        }
        if (action == 7)
        {
            for (size_t i = 0; i < host.slots; ++i)
                Native::AnimateReference(object + Native::Rows + i * 8, 5, 0);
            Native::AnimateReference(object + Native::Rows + selection * 8, 2, 0);
            if (Native::HasNetwork() && !Native::Read<uint8_t>(object, 693))
                Native::AnimateReference(object + Native::Rows + 7 * 8, 12, 0);
            Native::AnimateWidget(Native::Read<void*>(object, 832), 3);
            if (pHelpManager.get())
                Native::HideHelp(reinterpret_cast<void*>(pHelpManager.get()), 0);
            Native::AnimateWidget(Native::Read<void*>(object, 828), 3);
        }
        return true;
    }

    inline uint8_t __fastcall Input(void* object, void*)
    {
        const auto found = hosts.find(object);
        if (found == hosts.end() || !found->second->slots || !pInputManager.is_initialized() || !pInputManager.get())
            return inputHook.unsafe_thiscall<uint8_t>(object);
        auto host = found->second;
        if (host->transition)
        {
            host->transition = false;
            host->logical = static_cast<size_t>((std::max)(0, Native::Read<int>(object, Native::Selection)));
            host->offset = 0;
            host->inputSeen = false;
        }
        auto parent = Native::Read<uint8_t*>(object, Native::Parent);
        if (!parent || Native::Read<uint8_t>(parent, 48) == 1 || Native::Read<uint8_t>(parent, 51) == 1)
            return false;
        if (Native::HasNetwork() && !Native::Read<uint8_t>(object, 693))
        {
            Native::Write<uint8_t>(object, 693, 1);
            Native::AnimateReference(static_cast<uint8_t*>(object) + Native::Rows + 7 * 8,
                Native::Read<int>(object, Native::Selection) == 7 ? 1 : 4, 0);
        }
        const auto input = reinterpret_cast<uint8_t*>(pInputManager.get());
        const int device = Native::Read<int>(input, 2160);
        if (device < 0 || device >= 3)
        {
            Restore(*host, host->logical);
            host->plugins = false;
            return inputHook.unsafe_thiscall<uint8_t>(object);
        }
        const auto pressed = Native::Read<uint32_t>(input, 416 + 716 * device);
        const auto repeated = Native::Read<uint32_t>(input, 428 + 716 * device);
        const bool accept = (pressed & Native::Read<uint32_t>(input, 1492)) != 0;
        const bool back = (pressed & Native::Read<uint32_t>(input, 1496)) != 0;
        const int x = Native::Read<int>(input, 1536);
        const int y = Native::Read<int>(input, 1540);
        const bool mouseMoved = host->inputSeen && (x != host->mouseX || y != host->mouseY);
        host->inputSeen = true;
        host->mouseX = x;
        host->mouseY = y;
        if (host->language != languageGeneration.load())
            OpenModel(*host);
        if (host->plugins)
        {
            RWGTA::UpdateCheck::Tick();
            Populate(*host);
        }
        else
            Root(*host);
        if (back)
        {
            if (host->plugins)
            {
                Back(*host);
                return false;
            }
            Restore(*host, (std::min)(host->logical, host->slots - 1));
            host->transition = true;
            return inputHook.unsafe_thiscall<uint8_t>(object);
        }
        const int hover = mouseMoved || (pressed & 0x1000000) ? Hover(*host, x, y) : -1;
        if (hover >= 0)
        {
            if (host->plugins)
                host->selected = static_cast<size_t>(hover);
            else
                host->logical = static_cast<size_t>(hover) + host->offset;
        }
        const int direction = repeated & 0x10010 ? -1 : repeated & 0x40040 ? 1 : 0;
        if (direction)
        {
            if (host->plugins)
            {
                do
                {
                    host->selected = (host->selected + host->slots + direction) % host->slots;
                } while (!Selectable(host->items[host->selected]));
            }
            else
            {
                const auto count = host->slots + 1;
                host->logical = (host->logical + count + direction) % count;
                if (host->logical < host->offset)
                    host->offset = host->logical;
                else if (host->logical >= host->offset + host->slots)
                    host->offset = host->logical - host->slots + 1;
            }
        }
        DWORD foregroundProcess = 0;
        GetWindowThreadProcessId(GetForegroundWindow(), &foregroundProcess);
        const bool helpHeld = foregroundProcess == GetCurrentProcessId() && (GetAsyncKeyState(VK_F1) & 0x8000);
        if (host->plugins && helpHeld && !host->helpHeld && !host->details)
        {
            const auto& item = host->items[host->selected];
            const auto text = item.kind == Kind::Details ? host->previewHelp : Description(*host, item);
            if (!text.empty())
            {
                host->helpTitle = item.kind == Kind::Details ? host->previewTitle : item.kind == Kind::Setting ? host->model.Label(item.row) : item.label;
                host->help = Wrap(text);
                host->helpFirst = 0;
                host->helpReturnSelection = host->selected;
                host->details = true;
                host->selected = host->slots - 1;
            }
        }
        host->helpHeld = helpHeld;
        if (host->plugins)
        {
            const int change = repeated & 0x20020 ? -1 : repeated & 0x80080 ? 1 : 0;
            if (change)
                Activate(host, change);
            else if (accept && (!(pressed & 0x1000000) || hover >= 0))
                Activate(host, 0);
            if (Current(host))
                Populate(*host);
            return false;
        }
        Root(*host);
        if (accept && (!(pressed & 0x1000000) || hover >= 0))
        {
            if (host->logical < host->slots)
                return NativeAction(*host);
            OpenModel(*host);
            host->plugins = true;
            host->details = false;
            host->page = host->first = 0;
            host->selected = 1;
            Populate(*host);
        }
        return false;
    }

    inline int __fastcall Initialize(void* object, void*, void* parent)
    {
        const auto result = initializeHook.unsafe_thiscall<int>(object, parent);
        const int count = Native::Read<int>(object, Native::RowCount);
        if (count != 7 && count != 9)
            return result;
        auto host = std::make_shared<Host>();
        host->object = object;
        host->slots = static_cast<size_t>(count);
        for (size_t i = 0; i < host->slots; ++i)
        {
            host->actions[i] = Native::Read<int>(object, Native::Actions + 4 * i);
            host->labels[i] = Native::Read<int>(object, Native::Labels + 12 * i + 4);
            host->files[i] = Native::Read<int>(object, Native::Labels + 12 * i + 8);
        }
        if (const auto prior = hosts.find(object); prior != hosts.end())
            prior->second->alive = false;
        hosts[object] = host;
        OpenModel(*host);
        return result;
    }

    inline void* __fastcall Destroy(void* object, void*, unsigned flags)
    {
        std::shared_ptr<Host> host;
        if (const auto found = hosts.find(object); found != hosts.end())
        {
            host = found->second;
            host->alive = false;
            hosts.erase(found);
        }
        return destroyHook.unsafe_thiscall<void*>(object, flags);
    }

    inline void WINAPI Exit(UINT code)
    {
        RWGTA::UpdateCheck::OnExit();
        ExitProcess(code);
    }

    inline void Init(int (*width)(), int (*height)())
    {
        Width = width;
        Height = height;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&Init), &module);
        if (!Native::Bind())
            return;
        for (const auto& setting : FixSettings().GetDescription().settings)
            if (setting.HasValue() && setting.apply == "restart")
                restartValues.try_emplace({ module, setting.id }, FixSettings().Get(setting.id));
        FixSettings().OnChange("MAIN.TextLanguage", []() { ++languageGeneration; });
        messageHook = safetyhook::create_inline(Native::Lookup, Message);
        inputHook = safetyhook::create_inline(Native::Input, Input);
        destroyHook = safetyhook::create_inline(Native::Destroy, Destroy);
        if (messageHook && inputHook && destroyHook)
            initializeHook = safetyhook::create_inline(Native::Initialize, Initialize);
        if (!initializeHook)
        {
            destroyHook.reset();
            inputHook.reset();
            messageHook.reset();
            OutputDebugStringA("Resident Evil Revelations: native Plugins menu unavailable.\n");
            return;
        }

        // MAIN.CheckForUpdates: the notice shows on the plugin list; FusionFixHub opens when the game closes.
        IATHook::Replace(GetModuleHandleW(nullptr), "KERNEL32.dll", std::forward_as_tuple("ExitProcess", Exit));
        RWGTA::UpdateCheck::Config updates;
        updates.pluginId = "ResidentEvilRevelations.FusionFix";
        updates.fileVersion = rsc_FileVersion;
        updates.settings = &FixSettings();
        updates.playable = []()
        {
            return std::any_of(hosts.begin(), hosts.end(), [](const auto& entry)
            {
                return entry.second->plugins && !entry.second->page && !entry.second->details;
            });
        };
        updates.show = [](const std::wstring& notice)
        {
            updateNotice = asi_settings::text::Narrow(notice);
        };
        RWGTA::UpdateCheck::Init(std::move(updates));
    }
}

class NativeMenu
{
public:
    NativeMenu()
    {
        WFP::onInitEvent() += []()
        {
            RevMenu::Init(GetResX, GetResY);
        };
    }
} NativeMenu;

module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/SilentHill/Display.hpp"
#include "Shared/RWGTA/FixMenu.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"


export module NativeMenu;

import ComVars;


export namespace SH2Menu
{
    inline SafetyHookInline PanelHook, PanelVertexHook;
    inline bool RootPanel = false, PanelExtended = false;
    inline int PluginY = 440;
    inline RWGTAMenu::Model Model;
    inline SafetyHookInline ControlHook, DrawHook;
    inline int(__cdecl* NativeInput)(int) = nullptr;
    inline int(__cdecl* PadInput)(int, uint64_t) = nullptr;
    inline int(__cdecl* PadHeld)(int, uint64_t) = nullptr;
    inline void(__cdecl* Background)() = nullptr;
    inline void(__cdecl* SetColour)(int, int, int, int) = nullptr;
    inline uint16_t* (__cdecl* Encode)(const char*) = nullptr;
    inline void(__cdecl* DrawMessage)(const uint16_t*, int, int, int) = nullptr;
    inline int(__cdecl* Measure)(int*, const uint16_t*) = nullptr;
    inline void(__cdecl* CentreOn)() = nullptr;
    inline void(__cdecl* CentreOff)() = nullptr;
    inline int(__cdecl* MouseX)() = nullptr;
    inline int(__cdecl* MouseY)() = nullptr;
    inline int(__cdecl* MouseClick)(int) = nullptr;
    inline bool Selected = false, Active = false, HelpOpen = false;
    inline int View = -1, Row = 0, Scroll = 0, HelpScroll = 0;
    inline std::string UpdateNotice;
    inline std::vector<const RWGTAMenu::Row*> Rows;
    constexpr int VisibleRows = 8;

    inline bool IsRootPanel()
    {
        return MenuState.is_initialized() && MenuState.get()[0] == 2 && MenuState.get()[1] == 0 && MenuState.get()[8] == 0 && MenuState.get()[0x1C] == 0 && !Active && UpdateNotice.empty();
    }

    inline int MessageY(int y)
    {
        return PanelExtended && IsRootPanel() && y >= 375 && y <= 430 ? y + 32 : y;
    }

    inline int __cdecl Panel()
    {
        const bool previous = RootPanel;
        RootPanel = IsRootPanel();
        const int result = PanelHook.ccall<int>();
        RootPanel = previous;
        return result;
    }

    inline void* __cdecl PanelVertex(void* vertex, float x, float y, float z, DWORD colour)
    {
        if (RootPanel && MenuPanelHeight.is_initialized() && y >= float(MenuPanelHeight.get()) * 0.75f)
            y += float(MenuPanelHeight.get()) * (32.0f / 480.0f);
        return PanelVertexHook.ccall<void*>(vertex, x, y, z, colour);
    }

    inline void ExtendPanel()
    {
        auto vertex = hook::pattern("8B 44 24 04 D9 44 24 08 8B 4C 24 0C D9 18 8B 54 24 10 89 48 04 8B 4C 24 14 89 50 08 C7 40 0C 00 00 80 3F 89 48 10 C3");
        const auto background = reinterpret_cast<uint8_t*>(Background);
        if (background[12] != 0xDB || background[13] != 0x05)
            return;
        if (auto* height = *reinterpret_cast<uint32_t**>(background + 14))
            MenuPanelHeight.SetAddress(height);
        PanelVertexHook = safetyhook::create_inline(vertex.get_first(), &PanelVertex);
        PanelHook = safetyhook::create_inline(background, &Panel);
        PluginY = 365;
        PanelExtended = true;
    }

    inline std::string ToGame(const std::string& text)
    {
        int count = MultiByteToWideChar(CP_UTF8, 0, text.data(), int(text.size()), nullptr, 0);
        std::wstring wide(count, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, text.data(), int(text.size()), wide.data(), count);
        count = WideCharToMultiByte(1252, 0, wide.data(), int(wide.size()), nullptr, 0, nullptr, nullptr);
        std::string result(count, '\0');
        WideCharToMultiByte(1252, 0, wide.data(), int(wide.size()), result.data(), count, nullptr, nullptr);
        std::replace(result.begin(), result.end(), '\\', '/');
        return "\\h" + result;
    }

    inline int Width(const std::string& text)
    {
        int widths[64]{};
        Measure(widths, Encode(text.c_str()));
        return widths[0];
    }

    inline std::string Fit(std::string text, int width)
    {
        std::replace(text.begin(), text.end(), '\n', ' ');
        if (Width(text) <= width)
            return text;
        while (text.size() > 2 && Width(text + "...") > width)
            text.pop_back();
        return text + "...";
    }

    inline void Text(const std::string& text, int x, int y, bool selected = false, bool centre = false)
    {
        SetColour(63, 63, 63, selected ? 255 : 160);
        auto encoded = Encode(text.c_str());
        size_t size = 0;
        while (size < 2048 && encoded[size] != 0xFFFF)
            ++size;
        if (size == 2048)
            return;
        std::vector<uint16_t> table{ 1, 2 };
        table.insert(table.end(), encoded, encoded + size + 1);
        if (centre)
            CentreOn();
        DrawMessage(table.data(), 0, x, y);
        if (centre)
            CentreOff();
    }

    inline std::vector<std::string> Wrap(const std::string& text, int width)
    {
        std::vector<std::string> lines;
        std::istringstream words(text);
        std::string word, line;
        while (words >> word)
        {
            if (!line.empty() && Width(ToGame(line + " " + word)) > width)
            {
                lines.push_back(ToGame(line));
                line.clear();
            }
            while (Width(ToGame(word)) > width && word.size() > 1)
            {
                size_t length = word.size() - 1;
                while (length > 1 && Width(ToGame(word.substr(0, length))) > width)
                    --length;
                lines.push_back(ToGame(word.substr(0, length)));
                word.erase(0, length);
            }
            line += (line.empty() ? "" : " ") + word;
        }
        if (!line.empty())
            lines.push_back(ToGame(line));
        return lines;
    }

    inline std::string Language()
    {
        auto language = FixSettings().String("MAIN.TextLanguage");
        if (!language.empty() && language != "auto")
            return language;
        constexpr const char* tags[] = { "en", "en", "fr", "de", "it", "es" };
        return tags[GameLanguage.is_initialized() ? std::min<int>(GameLanguage.get(), 5) : 1];
    }

    inline void Open()
    {
        HMODULE module = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&Open), &module);
        Model.OpenPlugins(module, Language(), VisibleRows, {}, true);
        Active = true;
        HelpOpen = false;
        View = -1;
        Row = Scroll = 0;
    }

    inline bool Input(int mask)
    {
        uint64_t native = mask;
        if (mask == 1) native |= 0x4000000;
        if (mask == 2) native |= 0x8000000;
        if (mask == 4) native |= 0x2000000;
        if (mask == 8) native |= 0x1000000;
        if (mask == 16) native = uint64_t(int64_t(ConfirmMask.get()));
        if (mask == 32) native = uint64_t(int64_t(BackMask.get()));
        return NativeInput(mask) || PadInput(0, native);
    }

    inline int __cdecl Control()
    {
        static std::string selectedLanguage;
        auto language = Language();
        if (language != selectedLanguage)
        {
            FixSettings().GetTexts().Select(language);
            selectedLanguage = language;
        }
        bool root = MenuState.get()[0] == 2 && MenuState.get()[1] == 0 && MenuState.get()[8] == 0 && MenuState.get()[0x1C] == 0;
        if (!root)
        {
            if (Active)
                Model.Close();
            Active = Selected = false;
            return ControlHook.ccall<int>();
        }
        const bool up = Input(1), down = Input(2), accept = Input(16), back = Input(32) || MouseClick(2);
        const bool click = MouseClick(1) && MouseX() >= 70 && MouseX() <= 450;
        const bool pluginClick = click && MouseY() >= PluginY - 8 && MouseY() <= PluginY + 26;
        const bool rowClick = click && MouseY() >= 83 && MouseY() < 83 + VisibleRows * 32;
        RWGTA::UpdateCheck::Tick();
        if (!UpdateNotice.empty())
        {
            if (accept || back || click)
                UpdateNotice.clear();
            Background();
            return 0;
        }
        if (!Active)
        {
            if (Selected && (back || (click && !pluginClick)))
            {
                Selected = false;
                return ControlHook.ccall<int>();
            }
            if (Selected && (up || down))
            {
                Selected = false;
                MenuSelection = up ? 9 : 0;
                Background();
                return 0;
            }
            if ((down && MenuSelection.get() == 9) || (up && MenuSelection.get() == 0) || pluginClick)
                Selected = true;
            if (Selected && (accept || pluginClick))
                Open();
            if (!Selected)
                return ControlHook.ccall<int>();
            Background();
            return 0;
        }

        Rows = View >= 0 ? Model.VisibleRows(size_t(View)) : std::vector<const RWGTAMenu::Row*>{};
        const int count = (View < 0 ? int(Model.Pages().size()) : int(Rows.size())) + 1;
        if (HelpOpen)
        {
            if (up) HelpScroll = std::max(0, HelpScroll - 1);
            if (down) ++HelpScroll;
            if (back || accept || (click && MouseY() >= 380 && MouseY() <= 420)) HelpOpen = false;
        }
        else
        {
            if (up) Row = (Row + count - 1) % count;
            if (down) Row = (Row + 1) % count;
            if (rowClick)
                Row = std::min(count - 1, Scroll + (MouseY() - 83) / 32);
            if (back || ((accept || rowClick) && Row == count - 1))
            {
                if (View < 0)
                {
                    Active = false;
                    Model.Close();
                }
                else
                    View = -1;
                Row = Scroll = 0;
            }
            else if (View < 0 && (accept || rowClick))
            {
                View = Row;
                Model.PageShown(size_t(View));
                Row = Scroll = 0;
            }
            else if (View >= 0 && Row < int(Rows.size()))
            {
                if (Input(4)) Model.Change(*Rows[Row], -1, true);
                if (Input(8)) Model.Change(*Rows[Row], 1, true);
                if (accept || rowClick) Model.Change(*Rows[Row], 0, true);
            }
        }
        static bool held = false;
        const bool help = GetForegroundWindow() == SHDisplay::Get().window &&
            ((GetAsyncKeyState(VK_F1) & 0x8000) || PadHeld(0, uint64_t{ 1 } << 13));
        if (help && !held)
        {
            HelpOpen = !HelpOpen;
            HelpScroll = 0;
        }
        held = help;
        Row = std::clamp(Row, 0, count - 1);
        Scroll = std::clamp(Scroll, std::max(0, Row - VisibleRows + 1), Row);
        Background();
        return 0;
    }

    inline int __cdecl Draw()
    {
        if (!UpdateNotice.empty())
        {
            Text(ToGame(FixSettings().GetTexts().Resolve("@plugin.name")), 256, 52, true, true);
            auto lines = Wrap(UpdateNotice, 420);
            for (size_t i = 0; i < std::min<size_t>(lines.size(), 9); ++i)
                Text(lines[i], 46, 110 + int(i) * 28, true);
            Text(ToGame(FixSettings().GetTexts().Resolve("@menu.back")), 256, 395, true, true);
            return 0;
        }
        if (!Active)
        {
            const int16_t saved = MenuSelection.get();
            if (Selected)
                MenuSelection = 10;
            int result = DrawHook.ccall<int>();
            MenuSelection = saved;
            if (MenuState.get()[0] == 2 && MenuState.get()[1] == 0)
                Text(ToGame(FixSettings().GetTexts().Resolve("@menu.plugins")), 256, PluginY, Selected, true);
            return result;
        }
        const std::string title = View < 0 ? Model.Text("menu.plugins") : Model.Pages()[View].title;
        Text(Fit(ToGame(title), 430), 256, 52, true, true);
        const int count = View < 0 ? int(Model.Pages().size()) : int(Rows.size());
        const RWGTAMenu::Row* selected = View >= 0 && Row < count ? Rows[Row] : nullptr;
        std::string help = selected ? Model.Help(*selected) : Model.Text("menu.back");
        if (HelpOpen)
        {
            auto lines = Wrap(help, 420);
            HelpScroll = std::clamp(HelpScroll, 0, std::max(0, int(lines.size()) - 9));
            for (int i = HelpScroll; i < std::min(int(lines.size()), HelpScroll + 9); ++i)
                Text(lines[i], 46, 95 + (i - HelpScroll) * 28, true);
            Text(ToGame(Model.Text("menu.back")), 256, 395, true, true);
        }
        else
        {
            for (int i = Scroll; i <= std::min(count, Scroll + VisibleRows - 1); ++i)
            {
                int y = 96 + (i - Scroll) * 32;
                if (i == count)
                    Text(ToGame(Model.Text("menu.back")), 256, y, Row == i, true);
                else if (View < 0)
                    Text(Fit(ToGame(Model.Pages()[i].title), 420), 256, y, Row == i, true);
                else
                {
                    Text(Fit(ToGame(Model.Label(*Rows[i])), 245), 42, y, Row == i);
                    Text(Fit(ToGame(Model.Value(*Rows[i])), 170), 300, y, Row == i);
                }
            }
            auto lines = Wrap(help, 420);
            if (!lines.empty())
                Text(Fit(ToGame("F1: ") + lines.front().substr(2), 420), 46, 395);
        }
        if (Model.RestartRequired())
            Text(Fit(ToGame(Model.Text("menu.restartwarning")), 440), 256, 450, true, true);
        return 0;
    }

    template<typename T>
    inline void Resolve(T& function, const char* signature)
    {
        function = reinterpret_cast<T>(hook::get_pattern(signature));
    }

    inline void Init()
    {
        auto control = hook::pattern("83 EC 50 A1 ? ? ? ? 33 44 24 50 55 89 44 24 50 A1");
        if (control.empty())
            control = hook::pattern("83 EC 4C A1 ? ? ? ? 33 44 24 4C 53 89 44 24 4C A1");
        auto draw = hook::pattern("83 EC 08 53 55 56 57 68 FF 00 00 00 6A 3F 6A 3F 6A 3F E8 ? ? ? ? A0");
        auto language = hook::pattern("A0 ? ? ? ? 3C 02 74 11 76 0B 3C 05 77 07 BF 32 00 00 00");
        if (auto* languageAddress = *language.get_first<uint8_t*>(1))
            GameLanguage.SetAddress(languageAddress);
        Resolve(NativeInput, "A1 ? ? ? ? 23 44 24 04 F7 D8 1B C0 F7 D8 C3");
        auto input = hook::pattern("6A 00 68 02 00 00 08 6A 00 E8 ? ? ? ? 83 C4 0C 85 C0 75 0E 6A 02 E8 ? ? ? ? 83 C4 04 85 C0 74 3D A1 ? ? ? ? 99 52 50 6A 00 E8 ? ? ? ? 83 C4 0C 85 C0 75 27 A1 ? ? ? ? 99");
        PadInput = reinterpret_cast<decltype(PadInput)>(injector::GetBranchDestination(input.get_first(9)).as_int());
        BackMask.SetAddress(*input.get_first<int*>(36));
        ConfirmMask.SetAddress(*input.get_first<int*>(58));
        Resolve(PadHeld, "8B 44 24 04 85 C0 74 03 33 C0 C3 8B 44 24 08 53 8B 5C 24 10 55 56 57 53 50 E8 ? ? ? ? 83 C4 08 8B F0 F7 DE 1B F6 81 E6 80 00 00 00 33 ED");
        Resolve(Background, "51 53 55 56 57 6A 00 E8 ? ? ? ? DB 05 ? ? ? ? 8B F0");
        Resolve(SetColour, "0F B6 4C 24 04 0F B6 54 24 10 33 C0 8A 64 24 0C");
        Resolve(Encode, "83 EC 0C 8B 44 24 10 8A 08 53 55 56 57 0F B7 3D");
        Resolve(DrawMessage, "8B 44 24 04 85 C0 74 17 66 8B 4C 24 08 66 3B 08 73 0D 0F B7 C9 0F B7 54 48 02 8D 04 50 EB 02 33 C0 85 C0 89 44 24 04 74 ? 8B 44 24 0C");
        Resolve(Measure, "83 EC 0C 55 8B 6C 24 18 33 D2 3B EA 75 07 33 C0");
        Resolve(CentreOn, "81 0D ? ? ? ? 00 01 00 00 C3 90");
        Resolve(CentreOff, "81 25 ? ? ? ? FF FE FF FF C3 90 90 90 90 90 81");
        Resolve(MouseX, "A1 ? ? ? ? C3 90 90 90 90 90 90 90 90 90 90 A1 ? ? ? ? C3 90 90 90 90 90 90 90 90 90 90 A0 ? ? ? ? C3");
        MouseY = reinterpret_cast<decltype(MouseY)>(reinterpret_cast<uintptr_t>(MouseX) + 16);
        Resolve(MouseClick, "0F BE 05 ? ? ? ? 23 44 24 04 C3 90 90 90 90 0F BE 05");
        MenuState.SetAddress(*draw.get_first<uint8_t*>(24));
        MenuSelection.SetAddress(reinterpret_cast<uint8_t*>(MenuState.get_ptr()) + 2);
        ControlHook = safetyhook::create_inline(control.get_first(), &Control);
        DrawHook = safetyhook::create_inline(draw.get_first(), &Draw);
        RWGTA::UpdateCheck::Config updates;
        updates.pluginId = "SilentHill2.FusionFix";
        updates.fileVersion = rsc_FileVersion;
        updates.settings = &FixSettings();
        updates.playable = []() { return MenuState.get()[0] == 2 && MenuState.get()[8] == 0; };
        updates.show = [](const std::wstring& notice)
        {
            UpdateNotice = asi_settings::text::Narrow(notice);
        };
        RWGTA::UpdateCheck::Init(std::move(updates));
    }
}

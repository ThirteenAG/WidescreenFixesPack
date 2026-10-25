#pragma once

extern int posType; // existing native HUD coordinate policy in dllmain.cpp

// The paused game owns its HUD fonts; frontend artwork/fonts are destroyed
// before gameplay. Keep this adapter on the native pause draw and key paths.
namespace GTA2PauseMenu
{
    inline SafetyHookInline DrawHook, KeyHook;
    inline void* (__stdcall* DrawText)(const wchar_t*, int, int, int, int*, int, int, int) = nullptr;
    inline int (__stdcall* TextWidth)(const wchar_t*, int) = nullptr;
    inline uint16_t* HeadingFont = nullptr, * RowFont = nullptr;
    inline RWGTAMenu::Model Model;
    inline bool Opened = false, Ready = false;
    inline void* OwnerGame = nullptr, * OwnerPlayer = nullptr;
    inline size_t Page = SIZE_MAX, RootStart = 0, Selected = 0, RootSelected = 0;
    inline std::pair<size_t, size_t> HelpSelection{ SIZE_MAX, SIZE_MAX };
    inline ULONGLONG HelpStarted{};
    inline constexpr size_t RootRows = 7;
    inline bool PadHeld[7]{};
    inline DWORD PadRepeat[4]{};
    inline bool InputActive = false;

    inline bool Available(void* player)
    {
        using GTA2Input::Field;
        auto game = GTA2Input::Game ? *GTA2Input::Game : nullptr;
        return game && Field<int>(game, 0) == 2 && Field<void*>(game, 56) == player && player &&
            !Field<uint8_t>(player, 1930) && !Field<uint8_t>(player, 1940) &&
            !*GTA2Input::Multiplayer && !GTA2Input::IsReplay(GTA2Input::Replay, 0) &&
            Field<int>(GTA2Input::Replay, 0x38) != 2;
    }
    inline void Close()
    {
        Opened = false; OwnerGame = OwnerPlayer = nullptr;
        Model.Close(); Page = SIZE_MAX; RootStart = Selected = RootSelected = 0;
    }
    inline void Open(void* player)
    {
        if (!GTA2Options::HostModule)
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(&GTA2Options::HostModule), &GTA2Options::HostModule);
        GTA2Options::Model.Close();
        Model.OpenPlugins(GTA2Options::HostModule, GTA2Options::LanguageTag(), 8);
        OwnerGame = *GTA2Input::Game; OwnerPlayer = player;
        Page = SIZE_MAX; RootStart = Selected = RootSelected = 0; Opened = true;
        HelpSelection = { SIZE_MAX, SIZE_MAX };
    }
    inline size_t Links()
    {
        return std::min(RootRows, Model.Pages().size() - std::min(RootStart, Model.Pages().size()));
    }
    inline size_t Count()
    {
        if (Page != SIZE_MAX) return Model.VisibleRows(Page).size() + 1;
        return Links() + (RootStart ? 1 : 0) + (RootStart + Links() < Model.Pages().size() ? 1 : 0) + 1;
    }
    inline void Activate(int direction)
    {
        if (Page != SIZE_MAX)
        {
            auto rows = Model.VisibleRows(Page);
            if (Selected < rows.size()) Model.Change(*rows[Selected], direction, true);
            else if (!direction) { Page = SIZE_MAX; Selected = RootSelected; }
            return;
        }
        if (direction) return;
        auto links = Links();
        size_t position = Selected;
        if (position < links)
        {
            RootSelected = Selected;
            Page = RootStart + position; Selected = 0; Model.PageShown(Page);
        }
        else
        {
            position -= links;
            if (RootStart && !position) { RootStart -= RootRows; Selected = 0; return; }
            if (RootStart) --position;
            if (RootStart + links < Model.Pages().size() && !position) { RootStart += RootRows; Selected = 0; return; }
            Close();
        }
    }
    inline void Dispatch(void* player, int key)
    {
        bool available = Available(player);
        if (Opened && (!available || OwnerGame != *GTA2Input::Game || OwnerPlayer != player)) Close();
        if (available && InputDevices::Focused())
        {
            if (!Opened && key == 28) { Open(player); return; }
            if (Opened)
            {
                if (key == 64) Close(); // native pause action resumes through its own lifecycle
                else
                {
                    auto count = Count();
                    if (key == 200) Selected = (Selected + count - 1) % count;
                    else if (key == 208) Selected = (Selected + 1) % count;
                    else if (key == 203) Activate(-1);
                    else if (key == 205) Activate(1);
                    else if (key == 28) Activate(0);
                    else if (key == 1 || key == 14)
                    {
                        if (Page == SIZE_MAX) Close();
                        else { Page = SIZE_MAX; Selected = RootSelected; }
                    }
                    return; // prevent menu keys becoming gameplay actions/quit prompt
                }
            }
        }
        KeyHook.thiscall<void>(player, key);
    }
    inline void __fastcall Key(void* player, int, int key)
    {
        // HUD polling owns menu transitions while paused. Late player packets
        // must not apply the same keyboard transition a second time.
        if (Available(player) && InputDevices::Focused())
        {
            if (key == 28 || (Opened && (key == 200 || key == 208 || key == 203 || key == 205 || key == 1 || key == 14)))
                return;
            if (Opened && key == 64) Close();
        }
        KeyHook.thiscall<void>(player, key);
    }
    inline void Poll(void* player)
    {
        using namespace InputDevices;
        if (!Ready || !Available(player) || !Focused())
        {
            std::fill(std::begin(PadHeld), std::end(PadHeld), false); InputActive = false; return;
        }
        auto [x, y] = Stick();
        const bool down[] = { InputDevices::Key(VK_LEFT) || x < -0.35f || Button(XINPUT_GAMEPAD_DPAD_LEFT),
            InputDevices::Key(VK_RIGHT) || x > 0.35f || Button(XINPUT_GAMEPAD_DPAD_RIGHT),
            InputDevices::Key(VK_UP) || y < -0.35f || Button(XINPUT_GAMEPAD_DPAD_UP),
            InputDevices::Key(VK_DOWN) || y > 0.35f || Button(XINPUT_GAMEPAD_DPAD_DOWN),
            InputDevices::Key(VK_RETURN) || Button(XINPUT_GAMEPAD_A),
            InputDevices::Key(VK_ESCAPE) || InputDevices::Key(VK_BACK) || Button(XINPUT_GAMEPAD_B),
            (ModernControls && InputDevices::Key(PauseKey)) || Button(XINPUT_GAMEPAD_START) };
        constexpr int keys[] = { 203, 205, 200, 208, 28, 1, 64 };
        // Ignore buttons already held when the pause HUD first appears. In
        // particular the P/Start press that paused must not immediately resume.
        if (!InputActive)
        {
            std::copy(std::begin(down), std::end(down), std::begin(PadHeld));
            InputActive = true; return;
        }
        auto now = GetTickCount();
        for (int i = 0; i < 7; ++i)
        {
            bool pressed = down[i] && !PadHeld[i];
            if (i < 4)
            {
                if (pressed) PadRepeat[i] = now + 400;
                else if (down[i] && int32_t(now - PadRepeat[i]) >= 0) { pressed = true; PadRepeat[i] = now + 120; }
            }
            if (pressed && (Opened || i == 4 || i == 6)) Dispatch(player, keys[i]);
            PadHeld[i] = down[i];
        }
    }
    inline std::wstring Fit(std::wstring text, int width, uint16_t font)
    {
        return GTA2MenuText::Fit(std::move(text), width, SIZE_MAX, [font](const std::wstring& text)
        {
            return TextWidth(text.c_str(), font);
        });
    }
    inline void Text(const std::wstring& text, int x, int y, uint16_t font, int width)
    {
        auto clipped = Fit(text, width, font);
        int style = 2;
        // 4C7280 consumes eight stack arguments (RET 20h): wide text,
        // two 14-bit fixed-point coordinates, font, style pointer, 3 flags.
        // Its ECX is only a scratch destination overwritten by 401B60.
        auto previous = posType;
        posType = 1; // one centred 640x480 canvas, whatever the HUD Aspect Ratio
        DrawText(clipped.c_str(), x * 16384, y * 16384, font, &style, 0, 0, 0);
        posType = previous;
    }
    inline void Description(std::wstring text)
    {
        auto lines = GTA2MenuText::Wrap(std::move(text), 544, SIZE_MAX, [](const std::wstring& line)
        {
            return TextWidth(line.c_str(), *RowFont);
        });
        auto pages = (lines.size() + 1) / 2;
        size_t part = pages ? size_t((GetTickCount64() - HelpStarted) / 12000) % pages : 0;
        for (size_t line = 0; line < 2 && part * 2 + line < lines.size(); ++line)
            Text(lines[part * 2 + line], 48, 410 + int(line) * 28, *RowFont, 544);
        if (pages > 1)
            Text(std::to_wstring(part + 1) + L"/" + std::to_wstring(pages), 576, 380, *RowFont, 64);
    }
    inline void __fastcall Draw(void* nativePause, int)
    {
        using GTA2Options::FontText;
        auto game = GTA2Input::Game ? *GTA2Input::Game : nullptr;
        auto player = game ? GTA2Input::Field<void*>(game, 56) : nullptr;
        // The simulation player decoder may stop while paused. HUD draw is
        // still called on the game thread and is therefore the menu input tick.
        if (Available(player))
        {
            InputDevices::BeginInputFrame();
            InputDevices::Poll();
            Poll(player);
        }
        else InputActive = false;
        bool available = Available(player);
        if (Opened && (!available || game != OwnerGame || player != OwnerPlayer)) Close();
        if (!Opened)
        {
            DrawHook.thiscall<void>(nativePause);
            if (available)
            {
                auto label = FontText(RWGTAMenu::PluginText(FixSettings(), GTA2Options::LanguageTag(), "menu.plugins"));
                Text(L"ENTER  " + label, 160, 438, *RowFont, 440);
            }
            return;
        }
        RWGTAMenu::Model::Changed().exchange(false);
        auto count = Count();
        if (Selected >= count) Selected = 0;
        auto title = Page == SIZE_MAX ? Model.Text("menu.plugins") : Model.Pages()[Page].title;
        Text(FontText(title), 48, 68, *HeadingFont, 544);
        auto rows = Page == SIZE_MAX ? std::vector<const RWGTAMenu::Row*>{} : Model.VisibleRows(Page);
        for (size_t i = 0; i < count; ++i)
        {
            std::wstring label, value;
            if (Page != SIZE_MAX)
            {
                if (i < rows.size())
                {
                    label = FontText(Model.Label(*rows[i])); value = FontText(Model.Value(*rows[i]));
                    if (!Model.Enabled(*rows[i], true)) value = FontText(Model.Text("menu.unavailable"));
                }
                else label = FontText(Model.Text("menu.back"));
            }
            else
            {
                auto n = i;
                if (n < Links()) label = FontText(Model.Pages()[RootStart + n].title);
                else
                {
                    n -= Links();
                    if (RootStart && n == 0) label = FontText(Model.Text("menu.previous"));
                    else
                    {
                        if (RootStart) --n;
                        label = FontText(Model.Text(RootStart + Links() < Model.Pages().size() && n == 0 ? "menu.next" : "menu.back"));
                    }
                }
            }
            auto valueWidth = value.empty() ? 0 : std::min(580, TextWidth(value.c_str(), *RowFont));
            auto width = value.empty() ? 580 : std::max(0, 580 - valueWidth - 20);
            if (i == Selected)
                label = L"(" + Fit(label, width - TextWidth(L"()", *RowFont), *RowFont) + L")";
            else label = L" " + label;
            Text(label, 48, 116 + int(i) * 28, *RowFont, width);
            if (!value.empty()) Text(value, 628 - valueWidth, 116 + int(i) * 28, *RowFont, valueWidth);
        }
        auto selection = std::pair{ Page, Selected };
        if (HelpSelection != selection)
        {
            HelpSelection = selection;
            HelpStarted = GetTickCount64();
        }
        std::wstring description;
        if (Model.RestartRequired())
            description = FontText(Model.Text("menu.restartwarning")) + L"\n";
        if (Page != SIZE_MAX && Selected < rows.size())
            description += FontText(Model.Help(*rows[Selected]));
        Description(std::move(description));
    }
    inline void Install()
    {
        auto draw = hook::pattern("83 EC 08 53 55 56 57 8B 3D ? ? ? ? 89 4C 24 14 8B CF 33 F6 E8 ? ? ? ? 84 C0");
        auto key = hook::pattern("53 8B 5C 24 08 56 57 8B FB 8B F1 8B 0D ? ? ? ? 81 E7 FF FF 00 00");
        auto text = hook::pattern("51 56 8B 35 ? ? ? ? 81 C6 A8 00 00 00 57 8D 44 24 08 56 50 8D 4C 24 20 E8 ? ? ? ? 8D 4C 24 18 56 51 8D 4C 24 1C 8B F8 E8 ? ? ? ? 8B 00 8B 54 24 2C 8B 4C 24 28 52 8B 54 24 28");
        auto width = hook::pattern("8B 0D ? ? ? ? 53 55 56 8D 44 24 14 57 8B 7C 24 14 50 33 F6");
        auto heading = hook::pattern("66 8B 0D ? ? ? ? 8B F8 51 57 E8 ? ? ? ? 56 8B D8");
        auto row = hook::pattern("66 8B 15 ? ? ? ? 52 68 ? ? ? ? E8 ? ? ? ? 66 85 F6");
        if (draw.size() != 1 || key.size() != 1 || text.size() != 1 || width.size() != 1 || heading.size() != 1 || row.size() != 1)
        {
            Log::Write("GTA2 native pause menu signatures unavailable."); return;
        }
        DrawText = reinterpret_cast<decltype(DrawText)>(text.get_first());
        TextWidth = reinterpret_cast<decltype(TextWidth)>(width.get_first());
        HeadingFont = *heading.get_first<uint16_t*>(3); RowFont = *row.get_first<uint16_t*>(3);
        DrawHook = safetyhook::create_inline(draw.get_first(), Draw);
        KeyHook = safetyhook::create_inline(key.get_first(), Key);
        Ready = bool(DrawHook) && bool(KeyHook);
        Log::Write("GTA2 native pause menu installed.");
    }
}

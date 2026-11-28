module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/GTAClassic/InputDevices.hpp"
#include "Shared/RWGTA/FixMenu.hpp"

export module PauseMenu;

import ComVars;
import Hud;
import Options;
import MenuText;
import Log;

// The paused game owns its HUD fonts; frontend artwork/fonts are destroyed
// before gameplay. Keep this adapter on the native pause draw and key paths.
// Enter on the pause screen opens the plugins list; a plugin opens its settings,
// or the list of its categories, each opening its settings. Every list is one
// list that scrolls by a row through the rows shown (Back scrolls with them,
// Up/Down wrap at the ends), Page Up/Page Down and the shoulders move by a
// window, Home/End go to the ends, and three dots mark the rows above and
// below; Esc goes back to the row a list was opened from.
export namespace GTA2PauseMenu
{
    SafetyHookInline DrawHook, KeyHook;
    void* (__stdcall* DrawText)(const wchar_t*, int, int, int, int*, int, int, int) = nullptr;
    int (__stdcall* TextWidth)(const wchar_t*, int) = nullptr;
    RWGTAMenu::Model Model;
    bool Opened = false;
    void* OwnerGame = nullptr, * OwnerPlayer = nullptr;
    size_t Page = SIZE_MAX;              // the model's page shown
    std::pair<size_t, size_t> HelpSelection{ SIZE_MAX, SIZE_MAX };
    ULONGLONG HelpStarted{};
    constexpr size_t WindowRows = 9;     // rows shown at once, Back included
    bool PadHeld[11]{};
    DWORD PadRepeat[4]{};
    bool InputActive = false;

    bool Available(void* player)
    {
        void* game = pGame;
        return game && Field<int>(game, 0) == 2 && Field<void*>(game, 56) == player && player &&
            !Field<uint8_t>(player, 1930) && !Field<uint8_t>(player, 1940) &&
            !nMultiplayer && !IsReplay(Replay, 0) &&
            Field<int>(Replay, 0x38) != 2;
    }
    void Close()
    {
        Opened = false; OwnerGame = OwnerPlayer = nullptr;
        Model.Close(); Page = SIZE_MAX;
    }
    void Open(void* player)
    {
        if (!GTA2Options::HostModule)
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(&GTA2Options::HostModule), &GTA2Options::HostModule);
        GTA2Options::Model.Close();
        Model.OpenMenu(GTA2Options::HostModule, GTA2Options::LanguageTag(), WindowRows);
        OwnerGame = pGame; OwnerPlayer = player;
        Page = RWGTAMenu::Model::RootPage; Opened = true;
        HelpSelection = { SIZE_MAX, SIZE_MAX };
    }
    // The rows of the page shown, Back last (null); its window follows them
    std::vector<const RWGTAMenu::Row*> Rows()
    {
        return Page == SIZE_MAX ? std::vector<const RWGTAMenu::Row*>{} : Model.ListRows(Page, true);
    }
    // Back: to the list the page was opened from, on its row, or out of the plugins list
    void Back()
    {
        size_t parent = Model.Back(Page);
        if (parent == SIZE_MAX)
            Close();
        else
            Page = parent;
    }
    void Activate(int direction)
    {
        auto rows = Rows();
        auto& window = Model.Window(Page);
        if (window.Selected() >= rows.size())
            return;
        auto row = rows[window.Selected()];
        if (!row)
        {
            if (!direction) Back();
            return;
        }
        if (RWGTAMenu::Model::Opens(*row))
        {
            if (!direction)
            {
                size_t page = Model.Enter(*row);
                if (page != SIZE_MAX)
                    Page = page;
            }
            return;
        }
        Model.Change(*row, direction, true);
    }
    void Dispatch(void* player, int key)
    {
        bool available = Available(player);
        if (Opened && (!available || OwnerGame != pGame || OwnerPlayer != player)) Close();
        if (available && InputDevices::Focused())
        {
            if (!Opened && key == 28) { Open(player); return; }
            if (Opened)
            {
                if (key == 64) Close(); // native pause action resumes through its own lifecycle
                else
                {
                    Rows();
                    auto& window = Model.Window(Page);
                    if (key == 200) window.Move(-1, true);
                    else if (key == 208) window.Move(1, true);
                    else if (key == 201) window.Page(-1);
                    else if (key == 209) window.Page(1);
                    else if (key == 199) window.Edge(-1);
                    else if (key == 207) window.Edge(1);
                    else if (key == 203) Activate(-1);
                    else if (key == 205) Activate(1);
                    else if (key == 28) Activate(0);
                    else if (key == 1 || key == 14) Back();
                    return; // prevent menu keys becoming gameplay actions/quit prompt
                }
            }
        }
        KeyHook.thiscall<void>(player, key);
    }
    void __fastcall Key(void* player, int, int key)
    {
        // HUD polling owns menu transitions while paused. Late player packets
        // must not apply the same keyboard transition a second time.
        if (Available(player) && InputDevices::Focused())
        {
            if (key == 28 || (Opened && (key == 200 || key == 208 || key == 203 || key == 205 || key == 1 || key == 14 ||
                key == 201 || key == 209 || key == 199 || key == 207)))
                return;
            if (Opened && key == 64) Close();
        }
        KeyHook.thiscall<void>(player, key);
    }
    void Poll(void* player)
    {
        using namespace InputDevices;
        if (!Available(player) || !Focused())
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
            (ModernControls && InputDevices::Key(PauseKey)) || Button(XINPUT_GAMEPAD_START),
            InputDevices::Key(VK_PRIOR) || Button(XINPUT_GAMEPAD_LEFT_SHOULDER),
            InputDevices::Key(VK_NEXT) || Button(XINPUT_GAMEPAD_RIGHT_SHOULDER),
            InputDevices::Key(VK_HOME), InputDevices::Key(VK_END) };
        constexpr int keys[] = { 203, 205, 200, 208, 28, 1, 64, 201, 209, 199, 207 };
        // Ignore buttons already held when the pause HUD first appears. In
        // particular the P/Start press that paused must not immediately resume.
        if (!InputActive)
        {
            std::copy(std::begin(down), std::end(down), std::begin(PadHeld));
            InputActive = true; return;
        }
        auto now = GetTickCount();
        for (int i = 0; i < int(std::size(keys)); ++i)
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
    std::wstring Fit(std::wstring text, int width, uint16_t font)
    {
        return GTA2MenuText::Fit(std::move(text), width, SIZE_MAX, [font](const std::wstring& text)
        {
            return TextWidth(text.c_str(), font);
        });
    }
    void Text(const std::wstring& text, int x, int y, uint16_t font, int width)
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
    void Description(std::wstring text)
    {
        auto lines = GTA2MenuText::Wrap(std::move(text), 544, SIZE_MAX, [](const std::wstring& line)
        {
            return TextWidth(line.c_str(), wRowFont);
        });
        auto pages = (lines.size() + 1) / 2;
        size_t part = pages ? size_t((GetTickCount64() - HelpStarted) / 12000) % pages : 0;
        for (size_t line = 0; line < 2 && part * 2 + line < lines.size(); ++line)
            Text(lines[part * 2 + line], 48, 410 + int(line) * 28, wRowFont, 544);
        if (pages > 1)
            Text(std::to_wstring(part + 1) + L"/" + std::to_wstring(pages), 576, 380, wRowFont, 64);
    }
    void __fastcall Draw(void* nativePause, int)
    {
        using GTA2Options::FontText;
        void* game = pGame;
        auto player = game ? Field<void*>(game, 56) : nullptr;
        // The simulation player decoder may stop while paused. HUD draw is
        // still called on the game thread and is therefore the menu input tick.
        if (Available(player))
        {
            InputDevices::BeginInputFrame();
            InputDevices::Poll();
            Poll(player);
            InputDevices::UpdateRumble(true);
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
                std::wstring key = InputDevices::PadLast ? InputDevices::ButtonName(XINPUT_GAMEPAD_A) : L"ENTER";
                Text(key + L"  " + label, 160, 438, wRowFont, 440);
            }
            return;
        }
        RWGTAMenu::Model::Changed().exchange(false);
        auto rows = Rows();
        auto& window = Model.Window(Page);
        auto title = Model.Pages()[Page].title;
        Text(FontText(title), 48, 68, wHeadingFont, 544);
        for (size_t slot = 0; slot < window.Shown(); ++slot)
        {
            size_t i = window.First() + slot;
            std::wstring label, value;
            if (auto row = rows[i])
            {
                label = FontText(Model.Label(*row));
                if (!RWGTAMenu::Model::Opens(*row))
                {
                    value = FontText(Model.Value(*row));
                    if (!Model.Enabled(*row, true)) value = FontText(Model.Text("menu.unavailable"));
                }
            }
            else label = FontText(Model.Text("menu.back"));
            auto valueWidth = value.empty() ? 0 : std::min(580, TextWidth(value.c_str(), wRowFont));
            auto width = value.empty() ? 580 : std::max(0, 580 - valueWidth - 20);
            if (i == window.Selected())
                label = L"(" + Fit(label, width - TextWidth(L"()", wRowFont), wRowFont) + L")";
            else label = L" " + label;
            const int y = 116 + int(slot) * 28;
            Text(label, 48, y, wRowFont, width);
            if (!value.empty()) Text(value, 628 - valueWidth, y, wRowFont, valueWidth);
        }
        // More rows above or below: three dots of the font (it has no arrows) above the first and below the last row
        if (window.Above())
            Text(L"...", 48, 116 - 20, wRowFont, 64);
        if (window.Below())
            Text(L"...", 48, 116 + int(window.Shown()) * 28 - 8, wRowFont, 64);
        auto selection = std::pair{ Page, window.Selected() };
        if (HelpSelection != selection)
        {
            HelpSelection = selection;
            HelpStarted = GetTickCount64();
        }
        std::wstring description;
        if (Model.RestartRequired())
            description = FontText(Model.Text("menu.restartwarning")) + L"\n";
        if (window.Selected() < rows.size() && rows[window.Selected()] && !RWGTAMenu::Model::Opens(*rows[window.Selected()]))
            description += FontText(Model.Help(*rows[window.Selected()]));
        Description(std::move(description));
    }
    void Install()
    {
        auto draw = hook::pattern("83 EC 08 53 55 56 57 8B 3D ? ? ? ? 89 4C 24 14 8B CF 33 F6 E8 ? ? ? ? 84 C0");
        auto key = hook::pattern("53 8B 5C 24 08 56 57 8B FB 8B F1 8B 0D ? ? ? ? 81 E7 FF FF 00 00");
        auto text = hook::pattern("51 56 8B 35 ? ? ? ? 81 C6 A8 00 00 00 57 8D 44 24 08 56 50 8D 4C 24 20 E8 ? ? ? ? 8D 4C 24 18 56 51 8D 4C 24 1C 8B F8 E8 ? ? ? ? 8B 00 8B 54 24 2C 8B 4C 24 28 52 8B 54 24 28");
        auto width = hook::pattern("8B 0D ? ? ? ? 53 55 56 8D 44 24 14 57 8B 7C 24 14 50 33 F6");
        auto heading = hook::pattern("66 8B 0D ? ? ? ? 8B F8 51 57 E8 ? ? ? ? 56 8B D8");
        auto row = hook::pattern("66 8B 15 ? ? ? ? 52 68 ? ? ? ? E8 ? ? ? ? 66 85 F6");
        DrawText = reinterpret_cast<decltype(DrawText)>(text.get_first());
        TextWidth = reinterpret_cast<decltype(TextWidth)>(width.get_first());
        wHeadingFont.SetAddress(*heading.get_first<uint16_t*>(3)); wRowFont.SetAddress(*row.get_first<uint16_t*>(3));
        DrawHook = safetyhook::create_inline(draw.get_first(), Draw);
        KeyHook = safetyhook::create_inline(key.get_first(), Key);
        Log::Write("GTA2 native pause menu installed.");
    }
}

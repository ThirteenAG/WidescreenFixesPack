#pragma once

#include "NativeMenuPopup.hpp"

namespace DeerMenu
{
    struct Launcher : std::enable_shared_from_this<Launcher>
    {
        inline static constexpr int EntryID = 0x7101;
        inline static constexpr UINT OpenMessage = WM_APP + 0x5E;
        TitleContext context;
        HMODULE module = nullptr;
        HWND entry = nullptr;
        bool opening = false;
        bool requested = false;
        bool invalid = false;
        const char* refusal = "title entry contract refused";

        void Remove() noexcept
        {
            const auto owned = entry;
            entry = nullptr;
            if (owned && IsWindow(owned) && GetParent(owned) == context.dialog &&
                GetDlgCtrlID(owned) == EntryID && GetDlgItem(context.dialog, EntryID) == owned &&
                TitleContext::Class(owned, L"Button") &&
                reinterpret_cast<HMODULE>(GetWindowLongPtrW(owned, GWLP_HINSTANCE)) == module)
                DestroyWindow(owned);
        }

        bool Alive() const
        {
            return !invalid && context.Alive() && entry && IsWindow(entry) &&
                GetDlgItem(context.dialog, EntryID) == entry && GetParent(entry) == context.dialog &&
                TitleContext::Class(entry, L"Button");
        }

        void Retire() noexcept
        {
            invalid = true;
            requested = false;
            for (const auto& item : popups)
                if (item.second->source.dialog == context.dialog)
                    item.second->Retire();
        }

        bool Build(HWND dialog, HMODULE owner)
        {
            auto lease = shared_from_this();
            module = owner;
            RECT band{};
            if (!owner)
            {
                refusal = "title module unavailable";
                return false;
            }
            if (!context.Capture(dialog))
            {
                refusal = "title native control identity refused";
                return false;
            }
            if (GetDlgItem(dialog, EntryID))
            {
                refusal = "title entry ID already in use";
                return false;
            }
            if (!context.Space(band))
            {
                refusal = "title client space refused";
                return false;
            }
            auto font = reinterpret_cast<HFONT>(SendMessageW(context.controls[2], WM_GETFONT, 0, 0));
            // WM_GETFONT returns null when the stock control uses SYSTEM_FONT.
            if (!font) font = reinterpret_cast<HFONT>(GetStockObject(SYSTEM_FONT));
            HDC dc = GetDC(dialog);
            if (!dc || !font)
            {
                if (dc) ReleaseDC(dialog, dc);
                refusal = "title native font unavailable";
                return false;
            }
            const auto previous = SelectObject(dc, font);
            SIZE extent{};
            TEXTMETRICW metric{};
            const bool fits = GetTextMetricsW(dc, &metric) && metric.tmHeight >= 12 &&
                GetTextExtentPoint32W(dc, L"Plugins", 7, &extent) &&
                extent.cx + 12 <= band.right - band.left && extent.cy + 4 <= band.bottom - band.top;
            if (previous) SelectObject(dc, previous);
            ReleaseDC(dialog, dc);
            if (!fits || !context.Alive())
            {
                refusal = fits ? "title source retired during text measurement" : "title native text does not fit";
                return false;
            }
            entry = CreateWindowExW(0, L"BUTTON", L"Plugins", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
                band.left, band.top, band.right - band.left, band.bottom - band.top, dialog,
                reinterpret_cast<HMENU>(EntryID), owner, nullptr);
            if (!Alive())
            {
                Remove();
                return false;
            }
            SendMessageW(entry, WM_SETFONT, reinterpret_cast<WPARAM>(font), FALSE);
            if (!Alive())
            {
                Remove();
                return false;
            }
            ShowWindow(entry, SW_SHOW);
            if (!Alive()) { Remove(); return false; }
            return true;
        }

        bool Request(HWND sender)
        {
            if (!Alive() || sender != entry || opening || requested || !IsWindowEnabled(context.dialog))
                return false;
            requested = true;
            if (!PostMessageW(context.dialog, OpenMessage, 0, 0))
            {
                requested = false;
                return false;
            }
            return true;
        }

        void Open()
        {
            auto lease = shared_from_this();
            if (!requested || opening || !Alive() || !IsWindowEnabled(context.dialog))
                return;
            requested = false;
            Session::Busy scope(opening);
            auto popup = std::make_shared<Popup>();
            popup->source = context;
            popup->module = module;
            // An owned empty Win32 dialog template; all row controls are built
            // transactionally by Popup. The native launcher remains its owner.
            std::vector<WORD> bytes;
            bytes.resize((sizeof(DLGTEMPLATE) + 1) / 2);
            auto* definition = reinterpret_cast<DLGTEMPLATE*>(bytes.data());
            definition->style = WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME;
            definition->dwExtendedStyle = 0;
            definition->cdit = 0;
            definition->x = 0; definition->y = 0;
            definition->cx = 330; definition->cy = 230;
            bytes.push_back(0); // no menu
            bytes.push_back(0); // native dialog class
            for (const wchar_t c : std::wstring(L"Plugins")) bytes.push_back(WORD(c));
            bytes.push_back(0);
            if (!Alive())
                return;
            DialogBoxIndirectParamW(module, reinterpret_cast<const DLGTEMPLATE*>(bytes.data()),
                context.dialog, PopupProcedure, reinterpret_cast<LPARAM>(popup.get()));
            if (Alive())
                SetFocus(entry);
        }
    };

    inline std::unordered_map<HWND, std::shared_ptr<Launcher>> launchers;
}

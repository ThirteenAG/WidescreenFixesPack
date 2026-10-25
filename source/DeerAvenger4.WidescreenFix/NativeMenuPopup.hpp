#pragma once

#include "NativeMenuSession.hpp"
#include "FixSettings.hpp"
#include "NativeMenuTitle.hpp"
#include <unordered_map>

namespace DeerMenu
{
    inline std::string Language()
    {
        // Native glyph bounds remain fixed; translated text is folded to ASCII.
        const auto selected = FixSettings().String("MAIN.TextLanguage");
        return selected.empty() || selected == "auto" ? "en" : selected;
    }

    struct Popup;
    struct ListBinding
    {
        std::weak_ptr<Popup> owner;
        HWND window = nullptr;
        WNDPROC original = nullptr;
        bool expectedDestroy = false;
    };
    inline std::unordered_map<HWND, std::shared_ptr<Popup>> popups;
    inline std::unordered_map<HWND, std::shared_ptr<ListBinding>> listBindings;
    inline INT_PTR CALLBACK PopupProcedure(HWND, UINT, WPARAM, LPARAM);
    inline LRESULT CALLBACK ListProcedure(HWND, UINT, WPARAM, LPARAM);

    struct Popup : std::enable_shared_from_this<Popup>
    {
        inline static constexpr UINT ServiceMessage = WM_APP + 0x5D;
        TitleContext source;
        HMODULE module = nullptr;
        HWND window = nullptr;
        HWND heading = nullptr;
        HWND help = nullptr;
        uint64_t helpSerial = 0;
        size_t helpSelected = SIZE_MAX;
        HWND list = nullptr;
        HFONT font = nullptr; // Stock GDI object; never deleted by this host.
        std::shared_ptr<Session> session;
        uint64_t paintedSerial = 0;
        HWND expectedFocus = nullptr;
        bool servicing = false;
        bool publishing = false;
        bool closing = false;
        bool invalid = false;
        bool started = false;

        bool Current() const
        {
            return !invalid && !closing && window && IsWindow(window) && source.Alive() &&
                (!servicing || publishing || GetFocus() == expectedFocus);
        }

        bool Input(HWND sender) const
        {
            return Current() && sender == list && list && IsWindow(list) &&
                GetFocus() == list && GetForegroundWindow() == window &&
                session && session->open;
        }

        void Cancel() noexcept
        {
            if (session)
                session->Close();
            if (window && IsWindow(window))
                PostMessageW(window, ServiceMessage, 0, 0);
        }

        void Retire() noexcept
        {
            invalid = true;
            if (session)
                session->Retire();
            if (window && IsWindow(window))
                PostMessageW(window, ServiceMessage, 0, 0);
        }

        bool Measure(HDC dc, const std::string& text, int width, int height) const
        {
            if (!NativeText(text) || text.size() > 4096)
                return false;
            const std::wstring wide(text.begin(), text.end());
            SIZE size{};
            return GetTextExtentPoint32W(dc, wide.data(), int(wide.size()), &size) &&
                size.cx <= width && size.cy <= height;
        }

        static void DestroyList(HWND control)
        {
            const auto it = listBindings.find(control);
            const auto binding = it == listBindings.end() ? std::shared_ptr<ListBinding>{} : it->second;
            if (it != listBindings.end())
                it->second->expectedDestroy = true;
            if (control && IsWindow(control))
                DestroyWindow(control);
            // Failed subclass installation keeps the original WNDPROC, so
            // our WM_NCDESTROY cannot remove this exact weak binding.
            const auto remaining = listBindings.find(control);
            if (!IsWindow(control) && remaining != listBindings.end() && remaining->second == binding)
                listBindings.erase(remaining);
        }

        bool Publish()
        {
            auto lease = shared_from_this();
            if (!Current() || !session || !session->ready || !session->open)
                return false;
            const auto generation = session->generation;
            const auto serial = session->serial;
            const View snapshot = session->view;
            const auto current = [&] { return Current() && session->Current(generation) && session->serial == serial; };
            RECT client{};
            if (!GetClientRect(window, &client) || !current())
                return false;
            HDC dc = GetDC(window);
            if (!dc)
                return false;
            const auto previous = SelectObject(dc, font);
            struct DCLease
            {
                HWND window;
                HDC dc;
                HGDIOBJ previous;
                ~DCLease() { if (previous) SelectObject(dc, previous); ReleaseDC(window, dc); }
            } dcLease{window, dc, previous};
            TEXTMETRICW metrics{};
            const bool metricOK = GetTextMetricsW(dc, &metrics) && metrics.tmHeight >= 12 && metrics.tmHeight <= 96;
            const int rowHeight = metricOK ? metrics.tmHeight + 4 : 0;
            const int width = client.right - client.left - 32;
            bool fits = metricOK && current();
            for (const auto& line : snapshot.headings)
                fits = fits && Measure(dc, line, width, metrics.tmHeight + 2);
            for (const auto& item : snapshot.items)
                fits = fits && Measure(dc, item.text, width - 8, metrics.tmHeight + 2);
            if (!fits || !current())
                return false;
            const int headingHeight = int(snapshot.headings.size()) * rowHeight;
            const int listHeight = int(snapshot.items.size()) * rowHeight + 8;
            const int top = 12 + headingHeight + 8;
            if (top + listHeight + 12 > client.bottom || width < 80)
                return false;
            Session::Busy building(publishing);
            const auto next = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
                WS_CHILD | WS_TABSTOP | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
                12, top, client.right - 24, listHeight, window, reinterpret_cast<HMENU>(0x7100), module, nullptr);
            struct PendingWindow
            {
                HWND value;
                ~PendingWindow() { if (value) Popup::DestroyList(value); }
            } pendingWindow{next};
            if (!next || !current())
            {
                return false;
            }
            auto binding = std::make_shared<ListBinding>();
            binding->owner = lease;
            binding->window = next;
            binding->original = reinterpret_cast<WNDPROC>(GetWindowLongPtrW(next, GWLP_WNDPROC));
            if (!binding->original)
                return false;
            listBindings.emplace(next, binding);
            SetLastError(0);
            const auto original = SetWindowLongPtrW(next, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(ListProcedure));
            if (!original && GetLastError())
            {
                return false;
            }
            SendMessageW(next, WM_SETFONT, reinterpret_cast<WPARAM>(font), FALSE);
            for (size_t i = 0; i < snapshot.items.size(); ++i)
            {
                const auto& text = snapshot.items[i].text;
                const std::wstring wide(text.begin(), text.end());
                if (!current() || SendMessageW(next, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(wide.c_str())) != LRESULT(i))
                {
                    return false;
                }
            }
            if (!current() || session->selected >= snapshot.items.size())
            {
                return false;
            }
            SendMessageW(next, LB_SETCURSEL, session->selected, 0);
            std::wstring title;
            for (const auto& line : snapshot.headings)
            {
                if (!title.empty())
                    title += L"\r\n";
                title.append(line.begin(), line.end());
            }
            MoveWindow(heading, 12, 12, client.right - 24, headingHeight, TRUE);
            SetWindowTextW(heading, title.c_str());
            if (!current())
            {
                return false;
            }
            const auto old = list;
            list = next;
            pendingWindow.value = nullptr;
            paintedSerial = serial;
            ShowWindow(next, SW_SHOW);
            expectedFocus = next;
            SetFocus(next);
            DestroyList(old);
            return current() && GetFocus() == next;
        }

        bool PublishHelp()
        {
            if (!Current() || !session || !session->ready || !list || !help)
                return false;
            if (helpSerial == session->serial && helpSelected == session->selected)
                return true;
            const auto serial = session->serial;
            const auto selected = session->selected;
            std::string text;
            if (!session->ReadHelp(text) || !Current() || session->serial != serial || session->selected != selected)
                return false;
            RECT client{}, bounds{};
            if (!GetClientRect(window, &client) || !GetWindowRect(list, &bounds))
                return false;
            MapWindowPoints(nullptr, window, reinterpret_cast<POINT*>(&bounds), 2);
            const int top = bounds.bottom + 12;
            if (client.right < 104 || client.bottom - top < 24)
                return false;
            const std::wstring wide(text.begin(), text.end());
            MoveWindow(help, 12, top, client.right - 24, client.bottom - top - 12, TRUE);
            SetWindowTextW(help, wide.c_str());
            if (!Current() || session->serial != serial || session->selected != selected)
                return false;
            helpSerial = serial;
            helpSelected = selected;
            return true;
        }

        void Service()
        {
            auto lease = shared_from_this();
            if (servicing)
                return;
            Session::Busy service(servicing);
            expectedFocus = GetFocus();
            if (!Current() || !session)
            {
                Finish();
                return;
            }
            if (!session->open)
            {
                if (closing || started)
                {
                    Finish();
                    return;
                }
                started = true;
                if (!session->Open(Language()))
                {
                    Finish();
                    return;
                }
            }
            else
                session->Frame();
            if (!Current() || !session->open || !session->ready ||
                (paintedSerial != session->serial && !Publish()) || !PublishHelp())
                Finish();
        }

        void Finish() noexcept
        {
            closing = true;
            if (session)
                session->Close();
            if (window && IsWindow(window))
                EndDialog(window, 0);
        }

        bool Queue(int direction, bool back)
        {
            if (!session || !Current())
                return false;
            const auto index = list && IsWindow(list) ? SendMessageW(list, LB_GETCURSEL, 0, 0) : LB_ERR;
            if (!session->opening && (index == LB_ERR || size_t(index) != session->selected ||
                paintedSerial != session->serial))
                return false;
            if (!session->Queue(direction, back, session->selected))
                return false;
            PostMessageW(window, ServiceMessage, 0, 0);
            return true;
        }
    };

    inline LRESULT CALLBACK ListProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        const auto it = listBindings.find(window);
        if (it == listBindings.end())
            return DefWindowProcW(window, message, wParam, lParam);
        const auto binding = it->second;
        const auto owner = binding->owner.lock();
        try
        {
            if (owner && window == owner->list && message == WM_KILLFOCUS &&
                !binding->expectedDestroy && !owner->publishing && owner->session && owner->session->busy)
                owner->Cancel();
            if (owner && window == owner->list && owner->Input(window))
            {
                if (message == WM_GETDLGCODE)
                    return CallWindowProcW(binding->original, window, message, wParam, lParam) | DLGC_WANTARROWS | DLGC_WANTALLKEYS;
                if (message == WM_KEYDOWN && (wParam == VK_RETURN || wParam == VK_ESCAPE || wParam == VK_LEFT || wParam == VK_RIGHT))
                {
                    const int direction = wParam == VK_LEFT ? -1 : wParam == VK_RIGHT ? 1 : 0;
                    if (owner->Queue(direction, wParam == VK_ESCAPE))
                        return 0;
                }
                if (message == WM_CHAR && (wParam == VK_RETURN || wParam == VK_ESCAPE))
                    return 0;
            }
        }
        catch (...) { if (owner) owner->Retire(); }
        const auto result = CallWindowProcW(binding->original, window, message, wParam, lParam);
        if (message == WM_NCDESTROY)
        {
            listBindings.erase(window);
            if (owner && !binding->expectedDestroy && window == owner->list)
                owner->Retire();
        }
        return result;
    }

    inline INT_PTR CALLBACK PopupProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        std::shared_ptr<Popup> popup;
        const auto it = popups.find(window);
        if (it != popups.end())
            popup = it->second;
        try
        {
            if (message == WM_INITDIALOG)
            {
                popup = reinterpret_cast<Popup*>(lParam)->shared_from_this();
                popup->window = window;
                popups.emplace(window, popup);
                popup->font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
                popup->heading = CreateWindowExW(0, L"STATIC", L"Plugins", WS_CHILD | WS_VISIBLE | SS_NOPREFIX,
                    12, 12, 300, 24, window, nullptr, popup->module, nullptr);
                popup->help = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY,
                    12, 160, 300, 80, window, reinterpret_cast<HMENU>(0x7102), popup->module, nullptr);
                if (!popup->font || !popup->heading || !popup->help || !popup->source.Alive())
                {
                    popup->Finish();
                    return TRUE;
                }
                SendMessageW(popup->heading, WM_SETFONT, reinterpret_cast<WPARAM>(popup->font), FALSE);
                SendMessageW(popup->help, WM_SETFONT, reinterpret_cast<WPARAM>(popup->font), FALSE);
                RECT client{};
                GetClientRect(window, &client);
                popup->session = std::make_shared<Session>();
                popup->session->module = popup->module;
                popup->session->pause = false;
                popup->session->width = client.right - 48;
                popup->session->owner = [weak = std::weak_ptr<Popup>(popup)]
                {
                    const auto current = weak.lock();
                    return current && current->Current();
                };
                PostMessageW(window, Popup::ServiceMessage, 0, 0);
                SetTimer(window, 1, 100, nullptr);
                return TRUE;
            }
            if (!popup)
                return FALSE;
            if (message == Popup::ServiceMessage || (message == WM_TIMER && wParam == 1))
            {
                popup->Service();
                return TRUE;
            }
            if (message == WM_COMMAND && LOWORD(wParam) == IDCANCEL)
            {
                popup->Queue(0, true);
                return TRUE;
            }
            if (message == WM_COMMAND && reinterpret_cast<HWND>(lParam) == popup->list && popup->Input(popup->list))
            {
                if (HIWORD(wParam) == LBN_SELCHANGE)
                {
                    const auto selected = SendMessageW(popup->list, LB_GETCURSEL, 0, 0);
                    if (popup->session->busy)
                        popup->Cancel();
                    else if (selected != LB_ERR && size_t(selected) < popup->session->view.items.size())
                    {
                        popup->session->selected = size_t(selected);
                        PostMessageW(window, Popup::ServiceMessage, 0, 0);
                    }
                }
                else if (HIWORD(wParam) == LBN_DBLCLK)
                    popup->Queue(0, false);
                return TRUE;
            }
            if (message == WM_CLOSE)
            {
                popup->Cancel();
                popup->closing = true;
                return TRUE;
            }
            if (message == WM_NCDESTROY)
            {
                popup->invalid = true;
                popup->window = nullptr;
                if (popup->session)
                    popup->session->Retire();
                popups.erase(window);
            }
        }
        catch (...)
        {
            if (popup)
                popup->Finish();
            else
                EndDialog(window, -1);
            return TRUE;
        }
        return FALSE;
    }
}

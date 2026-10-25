#pragma once

// Included inside SCMenu after the normal page builder.
inline std::string DetailsUtf8(const std::wstring& value)
{
    if (value.empty()) return {};
    const int count = WideCharToMultiByte(CP_UTF8, 0, value.data(), int(value.size()), nullptr, 0, nullptr, nullptr);
    std::string result(count, '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), int(value.size()), result.data(), count, nullptr, nullptr);
    return result;
}

inline void SelectDetailsNavigation(Host& host, bool next)
{
    host.detailsNext = next && host.detailsLines.size() > PageRows;
    bool backSelected = !host.detailsNext, nextSelected = host.detailsNext;
    Bool(host.buttons[PageRows], L"m_bSelected", &backSelected);
    Bool(host.values[PageRows], L"m_bSelected", &nextSelected);
    ActivateWindow(host.detailsNext ? host.values[PageRows] : host.buttons[PageRows]);
}

inline std::wstring DetailsWide(const std::string& text)
{
    if (text.size() > size_t(INT_MAX)) return {};
    const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), nullptr, 0);
    if (count <= 0) return {};
    std::wstring result(count, L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), result.data(), count);
    result = MenuCharacters(result);
    for (auto& c : result) if (c < 32 || c == 8232 || c == 8233) c = L' ';
    const auto locale = Wide(Language());
    const int upperCount = LCMapStringEx(locale.c_str(), LCMAP_UPPERCASE, result.data(), int(result.size()), nullptr, 0, nullptr, nullptr, 0);
    if (upperCount > 0)
    {
        std::wstring upper(upperCount, L'\0');
        if (LCMapStringEx(locale.c_str(), LCMAP_UPPERCASE, result.data(), int(result.size()), upper.data(), upperCount, nullptr, nullptr, 0)) result = std::move(upper);
    }
    return result;
}

inline void BeginDetails(Host& host)
{
    if (host.page == SIZE_MAX || !host.helpRow) return;
    const auto help = model.Help(*host.helpRow);
    if (help.empty()) return;
    host.detailsText = DetailsWide(help);
    if (host.detailsText.empty()) return;
    host.details = true;
    host.detailsDirty = true;
    host.detailsNext = false;
    host.detailsPage = 0;
    host.detailsFont = nullptr;
    host.detailsLines.clear();
    ActivateWindow(host.panel);
}

inline void EndDetails(Host& host)
{
    host.details = false;
    host.detailsLines.clear();
    host.detailsText.clear();
    Populate(host);
}

inline void DetailsStep(Host& host, int direction)
{
    const auto pages = std::max(size_t(1), (host.detailsLines.size() + PageRows - 1) / PageRows);
    host.detailsPage = direction < 0 ? (host.detailsPage + pages - 1) % pages : (host.detailsPage + 1) % pages;
    host.detailsDirty = true;
}

inline bool DetailsKey(Host& host, int key)
{
    if (key == VK_ESCAPE || key == VK_F1)
        EndDetails(host);
    else if (key == VK_LEFT || key == VK_PRIOR)
        DetailsStep(host, -1);
    else if (key == VK_RIGHT || key == VK_NEXT)
        DetailsStep(host, 1);
    else if (key == VK_UP || key == VK_DOWN || key == VK_TAB)
        SelectDetailsNavigation(host, !host.detailsNext);
    else if (key == VK_RETURN)
    {
        if (host.detailsNext) DetailsStep(host, 1); else EndDetails(host);
    }
    else
        return false;
    return true;
}

inline void RenderDetails(Host& host, void* canvas)
{
    if (!canvas || !textSize || !host.details || host.buttons.size() <= PageRows || host.values.size() <= PageRows)
        return;
    const auto font = Object(canvas, L"Font");
    float height = 0, spacing = 0, margin = 0;
    if (!font || !Get(host.buttons[0], L"WinHeight", L"FloatProperty", height)) return;
    Get(host.buttons[0], L"m_fFontSpacing", L"FloatProperty", spacing);
    Get(host.buttons[0], L"m_fLMarge", L"FloatProperty", margin);
    const auto width = host.rowWidth - std::max(0.0f, margin) * 2 - 2;
    if (!std::isfinite(width) || !std::isfinite(height) || !std::isfinite(spacing) || width <= 0 || height <= 0)
        return;
    if (host.detailsFont != font || host.detailsWidth != width || host.detailsSpacing != spacing)
    {
        auto measure = [&](const std::wstring& value)
        {
            float w = 0, h = 0;
            textSize(canvas, nullptr, value.c_str(), &w, &h);
            return std::isfinite(w) && std::isfinite(h) && h <= height ? w + float(value.size()) * spacing : INFINITY;
        };
        host.detailsLines.clear();
        size_t begin = 0;
        while (begin < host.detailsText.size())
        {
            while (begin < host.detailsText.size() && host.detailsText[begin] == L' ') ++begin;
            if (begin == host.detailsText.size()) break;
            size_t end = begin, space = std::wstring::npos;
            while (end < host.detailsText.size())
            {
                size_t next = end + 1;
                if (host.detailsText[end] >= 0xD800 && host.detailsText[end] <= 0xDBFF && next < host.detailsText.size()) ++next;
                // Individual native captions are bounded independently of the
                // complete description; a long line continues on another row.
                if (next - begin > 240 || measure(host.detailsText.substr(begin, next - begin)) > width) break;
                if (host.detailsText[end] == L' ') space = end;
                end = next;
            }
            // Native fonts can refuse a glyph; retain it for the existing
            // visible missing-glyph handling rather than dropping text.
            if (end == begin)
            {
                end = begin + 1;
                if (host.detailsText[begin] >= 0xD800 && host.detailsText[begin] <= 0xDBFF && end < host.detailsText.size()) ++end;
            }
            else if (end < host.detailsText.size() && space != std::wstring::npos && space > begin) end = space;
            host.detailsLines.push_back(host.detailsText.substr(begin, end - begin));
            begin = end;
        }
        host.detailsFont = font;
        host.detailsWidth = width;
        host.detailsSpacing = spacing;
        host.detailsDirty = true;
    }
    if (!host.detailsDirty) return;
    const bool previous = busy; busy = true;
    const auto pages = std::max(size_t(1), (host.detailsLines.size() + PageRows - 1) / PageRows);
    host.detailsPage = std::min(host.detailsPage, pages - 1);
    Label(host.title, host.helpLabel + " (" + std::to_string(host.detailsPage + 1) + "/" + std::to_string(pages) + ")");
    for (size_t i = 0; i < PageRows; ++i)
    {
        auto button = host.buttons[i];
        Invoke(host.values[i], L"HideWindow");
        Set(button, L"WinWidth", L"FloatProperty", host.rowWidth);
        bool disabled = true, selected = false;
        Bool(button, L"bDisabled", &disabled);
        Bool(button, L"m_bSelected", &selected);
        const auto line = host.detailsPage * PageRows + i;
        if (line < host.detailsLines.size())
        {
            Label(button, DetailsUtf8(host.detailsLines[line]));
            Invoke(button, L"ShowWindow");
        }
        else
            Invoke(button, L"HideWindow");
    }
    auto back = host.buttons[PageRows], next = host.values[PageRows];
    bool disabled = false;
    Bool(back, L"bDisabled", &disabled);
    Set(back, L"WinWidth", L"FloatProperty", host.rowWidth * 0.60f - 8);
    Label(back, model.Text("menu.back"));
    Invoke(back, L"ShowWindow");
    Bool(next, L"bDisabled", &disabled);
    Label(next, model.Text("menu.next"));
    Invoke(next, pages > 1 ? L"ShowWindow" : L"HideWindow");
    Label(host.help, model.Text("menu.detailshint"));
    host.detailsDirty = false;
    SelectDetailsNavigation(host, host.detailsNext);
    busy = previous;
}

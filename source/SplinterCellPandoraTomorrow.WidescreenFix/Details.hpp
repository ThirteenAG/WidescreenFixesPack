#pragma once

inline std::wstring DetailWide(const std::string& text)
{
    if (text.size() > size_t(INT_MAX)) return {};
    const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), nullptr, 0);
    if (count <= 0) return {};
    std::wstring result(count, L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), result.data(), count);
    for (auto& c : result) if (c < 32 && c != L'\n') c = L' ';
    return result;
}
inline std::string DetailUtf8(const std::wstring& text)
{
    if (text.empty()) return {};
    const int count = WideCharToMultiByte(CP_UTF8, 0, text.data(), int(text.size()), nullptr, 0, nullptr, nullptr);
    std::string result(count, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), int(text.size()), result.data(), count, nullptr, nullptr);
    return result;
}
inline void* PageFocus(const Host& host)
{
    auto current = host.parent;
    std::set<void*> seen;
    while (Alive(current) && seen.size() < 32 && seen.insert(current).second)
    {
        auto next = Object(current, L"FocusedControl");
        if (!next) return current;
        current = next;
    }
    return nullptr;
}
inline void BeginDetails(Host& host)
{
    const auto keep = Keep(host);
    if (!Current(keep) || !host.open || host.page == SIZE_MAX || host.helpIndex >= host.items.size()) return;
    const auto& row = host.items[host.helpIndex];
    if (row.link || row.help.empty()) return;
    auto text = row.label;
    if (!row.value.empty()) text += ": " + row.value;
    text += "\n" + row.help;
    auto wide = DetailWide(text);
    if (wide.empty()) return;
    host.detailsText = std::move(wide);
    host.details = host.detailsDirty = true;
    host.detailsPage = 0; host.detailsFont = nullptr; host.detailsLines.clear();
}
inline void EndDetails(Host& host)
{
    const auto selected = host.helpIndex;
    host.details = false; host.detailsText.clear(); host.detailsLines.clear(); host.detailsFont = nullptr;
    Populate(host);
    if (!host.retired && selected < host.items.size()) Focus(host.labels[selected]);
}
inline void StepDetails(Host& host, int direction)
{
    if (!host.details || host.labels.size() < 2) return;
    const auto count = host.labels.size() - 1;
    const auto pages = std::max(size_t(1), (host.detailsLines.size() + count - 1) / count);
    host.detailsPage = direction < 0 ? (host.detailsPage + pages - 1) % pages : (host.detailsPage + 1) % pages;
    host.detailsDirty = true;
}
inline std::vector<std::wstring> WrapDetails(const std::wstring& text, float width, const std::function<float(const std::wstring&)>& measure)
{
    std::vector<std::wstring> lines;
    std::wstring line;
    auto flush = [&]() { if (!line.empty()) { lines.push_back(std::move(line)); line.clear(); } };
    for (size_t at = 0; at < text.size();)
    {
        if (text[at] == L'\n') { flush(); ++at; continue; }
        if (iswspace(text[at])) { ++at; continue; }
        auto end = at;
        while (end < text.size() && !iswspace(text[end])) ++end;
        auto word = text.substr(at, end - at); at = end;
        if (!line.empty() && measure(line + L" " + word) > width) flush();
        while (!word.empty() && measure(word) > width)
        {
            size_t take = 0;
            for (size_t i = 0; i < word.size();)
            {
                size_t next = i + 1;
                if (word[i] >= 0xD800 && word[i] <= 0xDBFF && next < word.size() && word[next] >= 0xDC00 && word[next] <= 0xDFFF) ++next;
                if (measure(word.substr(0, next)) > width) break;
                take = next; i = next;
            }
            if (!take) return {};
            lines.push_back(word.substr(0, take)); word.erase(0, take);
        }
        if (!word.empty()) { if (!line.empty()) line += L' '; line += word; }
    }
    flush();
    return lines;
}
inline void DrawDetails(void* object, void* canvas)
{
    const auto host = active;
    if (!Current(host) || !host->open || !host->details || !canvas || !textSize || host->labels.size() < 2) return;
    const auto count = host->labels.size() - 1;
    if (std::find(host->labels.begin(), host->labels.end() - 1, object) != host->labels.end() - 1)
        Set(object, L"MenuState", L"ByteProperty", uint8_t(0));
    const auto key = ReadString(host->labels.front(), L"FontName");
    std::vector<void*> fonts;
    if (key.empty() || !ArrayValues(host->controller, fonts, L"FontStack")) return;
    void* font = nullptr;
    for (auto candidate : fonts)
        if (_wcsicmp(ReadString(candidate, L"KeyName").c_str(), key.c_str()) == 0) { font = Object(candidate, L"OnlyDefaultFont"); break; }
    const auto left = Bounds(host->labels.front()), value = Bounds(host->values.front());
    const float width = value.x + value.w - left.x - 18;
    if (!Alive(font) || !std::isfinite(width) || width < 100) return;
    if (font != host->detailsFont || width != host->detailsWidth)
    {
        auto old = Object(canvas, L"Font");
        if (!Set(canvas, L"Font", L"ObjectProperty", font)) return;
        auto measure = [&](const std::wstring& text)
        {
            float w = 0, h = 0; textSize(canvas, nullptr, text.c_str(), &w, &h);
            return std::isfinite(w) && std::isfinite(h) && h <= left.h - 2 && (text.empty() || w > 0) ? w : INFINITY;
        };
        auto lines = WrapDetails(host->detailsText, width, measure);
        Set(canvas, L"Font", L"ObjectProperty", old);
        if (lines.empty()) return;
        host->detailsLines = std::move(lines); host->detailsFont = font; host->detailsWidth = width; host->detailsDirty = true;
    }
    if (!host->detailsDirty) return;
    host->detailsDirty = false;
    const auto pages = std::max(size_t(1), (host->detailsLines.size() + count - 1) / count);
    host->detailsPage = std::min(host->detailsPage, pages - 1);
    StringValue(host->title, L"Caption", model.Text("menu.details") + " (" + std::to_string(host->detailsPage + 1) + "/" + std::to_string(pages) + ")", true);
    for (size_t i = 0; i < count; ++i)
    {
        auto rect = Bounds(host->labels[i]); rect.w = width + 18; Position(host->labels[i], rect);
        const auto index = host->detailsPage * count + i;
        SetBool(host->labels[i], L"bAcceptsInput", false); SetBool(host->labels[i], L"bTabStop", false);
        Set(host->labels[i], L"MenuState", L"ByteProperty", uint8_t(0));
        if (index < host->detailsLines.size()) StringValue(host->labels[i], L"Caption", DetailUtf8(host->detailsLines[index]));
        Visible(host->labels[i], index < host->detailsLines.size()); Visible(host->values[i], false);
    }
    auto rect = Bounds(host->labels[count]); const auto right = Bounds(host->values[count]);
    rect.w = pages > 1 ? right.x - rect.x : right.x + right.w - rect.x; Position(host->labels[count], rect);
    StringValue(host->labels[count], L"Caption", model.Text("menu.back")); SetBool(host->labels[count], L"bAcceptsInput", true); SetBool(host->labels[count], L"bTabStop", true); Visible(host->labels[count], true);
    StringValue(host->values[count], L"Caption", model.Text("menu.next")); SetBool(host->values[count], L"bAcceptsInput", pages > 1); SetBool(host->values[count], L"bTabStop", pages > 1); Visible(host->values[count], pages > 1);
    StringValue(host->help, L"Caption", model.Text("menu.detailshint")); Visible(host->help, true);
    Focus(host->labels[count]);
}

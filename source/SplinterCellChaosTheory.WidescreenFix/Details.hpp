#pragma once

inline constexpr size_t DetailLines = 5;
inline size_t DetailPages()
{
    return screen ? std::max(size_t(1), (screen->detailsLines.size() + DetailLines - 1) / DetailLines) : 1;
}
inline std::wstring DetailsWide(const std::string& text)
{
    if (text.size() > 65536) return {};
    const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), nullptr, 0);
    if (length <= 0) return {};
    std::wstring wide(length, L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), wide.data(), length);
    for (auto& c : wide) if (c < L' ' && c != L'\n') c = L' ';
    return wide;
}
inline std::vector<std::wstring> WrapDetails(const std::wstring& text, double width,
    const std::function<double(const std::wstring&)>& measure)
{
    if (!std::isfinite(width) || width <= 0) return {};
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
inline void PopulateDetails()
{
    if (!screen || !screen->details) return;
    const auto pages = DetailPages();
    screen->detailsPage = std::min(screen->detailsPage, pages - 1);
    clear(screen->list, nullptr);
    Row(model.Text("menu.details") + " (" + std::to_string(screen->detailsPage + 1) + "/" + std::to_string(pages) + ")", true);
    for (size_t i = 0; i < DetailLines; ++i)
    {
        const auto at = screen->detailsPage * DetailLines + i;
        RowWide(at < screen->detailsLines.size() ? screen->detailsLines[at] : L"", true);
    }
    if (pages > 1) Row(model.Text("menu.next"), false);
    Row(model.Text("menu.back"), false);
    select(screen->list, nullptr, int16_t(Count(screen->list) - 1), 1, 1);
}
inline void EndDetails()
{
    if (!screen || !screen->details) return;
    screen->details = false; screen->detailsLines.clear();
    screen->selected = screen->detailsReturn;
    Populate();
}
inline void StepDetails(int direction)
{
    if (!screen || !screen->details) return;
    const auto pages = DetailPages();
    screen->detailsPage = direction < 0 ? (screen->detailsPage + pages - 1) % pages : (screen->detailsPage + 1) % pages;
    PopulateDetails();
}
inline void BeginDetails()
{
    if (!screen || screen->descriptorPage == SIZE_MAX) return;
    const auto token = screen->page;
    const auto snapshot = model;
    const auto rows = snapshot.VisibleRows(screen->descriptorPage);
    const auto at = screen->rowStart + screen->helpIndex;
    if (at >= rows.size()) return;
    auto title = snapshot.Label(*rows[at]);
    if (!screen || screen->page != token) return;
    auto value = snapshot.Value(*rows[at]);
    if (!screen || screen->page != token) return;
    auto help = snapshot.Help(*rows[at]);
    if (!screen || screen->page != token || help.empty()) return;
    if (!value.empty()) title += ": " + value;
    const auto metrics = RowMetrics(screen->list);
    const auto locale = Wide(Language());
    if (!metrics.font || !std::isfinite(metrics.width) || metrics.width <= 0) return;
    auto text = CTText::Style(DetailsWide(title), metrics, locale.c_str()) + L"\n" +
        CTText::Style(DetailsWide(help), metrics, locale.c_str());
    auto lines = WrapDetails(text, metrics.width, [&](const auto& part)
    {
        const auto width = metrics.Measure(part);
        return std::isfinite(width) && (part.empty() || width > 0) ? width : INFINITY;
    });
    if (lines.empty()) return;
    screen->detailsReturn = screen->selected; screen->detailsPage = 0;
    screen->detailsLines = std::move(lines); screen->details = true;
    PopulateDetails();
}

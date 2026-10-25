#pragma once
#include <string>
#include <vector>

namespace GTA1MenuText
{
    inline size_t Next(const std::string& text, size_t at)
    {
        ++at;
        while (at < text.size() && (static_cast<unsigned char>(text[at]) & 0xC0) == 0x80)
            ++at;
        return at;
    }

    inline void Pop(std::string& text)
    {
        auto at = text.size() - 1;
        while (at && (static_cast<unsigned char>(text[at]) & 0xC0) == 0x80)
            --at;
        text.resize(at);
    }

    template<typename Measure>
    std::string Fit(std::string text, int width, Measure measure)
    {
        if (measure(text) <= width)
            return text;
        const std::string dots{ "..." };
        if (measure(dots) > width)
            return {};
        while (!text.empty() && measure(text + dots) > width)
            Pop(text);
        return text + dots;
    }

    template<typename Measure>
    std::string Join(const std::string& label, const std::string& value, int width, Measure measure)
    {
        if (value.empty())
            return Fit(label, width, measure);
        if (measure(value) > width)
            return {};
        const std::string gap{ ": " };
        auto fitted = Fit(label, width - measure(value) - measure(gap), measure);
        return fitted.empty() ? value : fitted + gap + value;
    }

    template<typename Measure>
    std::vector<std::string> Wrap(std::string text, int width, Measure measure)
    {
        std::vector<std::string> lines;
        while (!text.empty())
        {
            size_t count{};
            while (count < text.size())
            {
                auto next = Next(text, count);
                if (measure(text.substr(0, next)) > width)
                    break;
                count = next;
            }
            if (!count)
                return {};
            auto consumed = count;
            if (count < text.size())
            {
                auto space = text.rfind(' ', count);
                if (space != std::string::npos && space)
                    count = consumed = space;
            }
            lines.push_back(text.substr(0, count));
            text.erase(0, consumed);
            while (!text.empty() && text[0] == ' ')
                text.erase(0, 1);
        }
        return lines;
    }
}

#pragma once
#include <algorithm>
#include <atomic>
#include <charconv>
#include <cmath>
#include <string_view>

namespace RevHudLayout
{
    enum class Mode
    {
        Center,
        Left,
        Right,
        Offset,
        Subtitles,
        Vanilla,
        Hidden,
    };

    inline std::atomic<float> Aspect{ 0.0f };

    inline std::string_view Trim(std::string_view text)
    {
        const auto begin = text.find_first_not_of(" \t\r\n");
        if (begin == std::string_view::npos)
            return {};
        const auto end = text.find_last_not_of(" \t\r\n");
        return text.substr(begin, end - begin + 1);
    }

    inline float Number(std::string_view text)
    {
        text = Trim(text);
        if (text.empty())
            return 0.0f;
        float value = 0.0f;
        const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
        if (parsed.ec != std::errc() || parsed.ptr != text.data() + text.size() || !std::isfinite(value) || value <= 0.0f)
            return 0.0f;
        return value;
    }

    inline float ParseAspect(std::string_view text)
    {
        text = Trim(text);
        const auto colon = text.find(':');
        float value = Number(colon == std::string_view::npos ? text : text.substr(0, colon));
        if (colon != std::string_view::npos)
        {
            const auto height = Number(text.substr(colon + 1));
            value = height > 0.0f ? value / height : 0.0f;
        }
        return std::isfinite(value) && value > 0.0f ? std::clamp(value, 4.0f / 3.0f, 32.0f / 9.0f) : 0.0f;
    }

    inline void SetAspect(std::string_view text)
    {
        Aspect.store(ParseAspect(text));
    }

    inline void Transform(float* constants, Mode mode, float screenAspect, float constraint)
    {
        if (!std::isfinite(screenAspect) || screenAspect <= 0.0f || mode == Mode::Vanilla || mode == Mode::Hidden)
            return;
        const float diff = screenAspect / (16.0f / 9.0f);
        const float scaleX = std::min(1.0f, 1.0f / diff);
        const float scaleY = std::min(1.0f, diff);
        switch (mode)
        {
            case Mode::Center:
                constants[0] *= scaleX;
                constants[2] *= scaleX;
                break;
            case Mode::Left:
            case Mode::Right:
            {
                const float box = std::isfinite(constraint) && constraint > 0.0f
                    ? std::min(1.0f, std::clamp(constraint, 4.0f / 3.0f, 32.0f / 9.0f) / screenAspect) : 1.0f;
                constants[0] *= scaleX;
                constants[2] = (constants[2] + 1.0f) * scaleX + (mode == Mode::Left ? -box : box - 2.0f * scaleX);
                break;
            }
            case Mode::Offset:
                constants[2] += 1.0f - scaleX;
                return;
            case Mode::Subtitles:
                constants[0] *= scaleX;
                constants[2] += 1.0f - scaleX;
                break;
            default:
                return;
        }
        constants[1] *= scaleY;
        constants[3] *= scaleY;
    }
}

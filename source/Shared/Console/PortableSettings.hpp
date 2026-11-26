#pragma once
#include "PortableIni.hpp"
#include <cmath>
#include <cstdlib>
#include <initializer_list>

namespace console::portable
{
    class IniSnapshot
    {
        char contents[IniCapacity];
        size_t length{};

        static void Trim(const char*& begin, const char*& end)
        {
            while (begin < end && (*begin == ' ' || *begin == '\t'))
                ++begin;
            while (end > begin && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r'))
                --end;
        }

        static char Lower(char ch)
        {
            return ch >= 'A' && ch <= 'Z' ? char(ch + 'a' - 'A') : ch;
        }

        static bool Match(const char* begin, const char* end, const char* key)
        {
            if (size_t(end - begin) != std::strlen(key))
                return false;
            while (begin != end)
                if (Lower(*begin++) != Lower(*key++))
                    return false;
            return true;
        }

    public:
        std::string_view Bytes() const
        {
            return { contents, length };
        }

        bool Read(const char* path)
        {
            length = 0;
            SceIoStat stat{};
            if (sceIoGetstat(path, &stat) < 0 || stat.st_size <= 0 || uint64_t(stat.st_size) >= sizeof(contents))
                return false;
            const auto file = sceIoOpen(path, PSP_O_RDONLY, 0);
            if (file < 0)
                return false;
            const auto size = sceIoRead(file, contents, sizeof(contents));
            const auto closed = sceIoClose(file);
            if (size != stat.st_size || closed < 0)
                return false;
            length = size_t(size);
            return true;
        }

    private:
        bool Find(const char* section, const char* key, char* value, size_t capacity) const
        {
            const char* cursor = contents;
            const char* const end = contents + length;
            if (length >= 3 && std::memcmp(cursor, "\xEF\xBB\xBF", 3) == 0)
                cursor += 3;
            bool active = false;
            while (cursor != end)
            {
                const char* begin = cursor;
                const auto newline = static_cast<const char*>(std::memchr(cursor, '\n', size_t(end - cursor)));
                const char* stop = newline ? newline : end;
                cursor = newline ? newline + 1 : end;
                for (auto comment = begin; comment != stop; ++comment)
                {
                    if (*comment == ';' || *comment == '#')
                    {
                        stop = comment;
                        break;
                    }
                }
                Trim(begin, stop);
                if (begin == stop)
                    continue;
                if (*begin == '[')
                {
                    active = stop[-1] == ']';
                    if (active)
                    {
                        ++begin;
                        --stop;
                        Trim(begin, stop);
                        active = Match(begin, stop, section);
                        if (active && !key)
                            return true;
                    }
                    continue;
                }
                if (!active)
                    continue;
                const auto equals = static_cast<const char*>(std::memchr(begin, '=', size_t(stop - begin)));
                if (!equals)
                    continue;
                auto keyEnd = equals;
                Trim(begin, keyEnd);
                if (!Match(begin, keyEnd, key))
                    continue;
                begin = equals + 1;
                Trim(begin, stop);
                const auto size = size_t(stop - begin);
                if (size == 0 || size >= capacity)
                    return false;
                std::memcpy(value, begin, size);
                value[size] = '\0';
                return true;
            }
            return false;
        }

    public:
        bool HasSection(const char* section) const
        {
            return Find(section, nullptr, nullptr, 0);
        }

        bool Text(const char* section, const char* key, char* value, size_t capacity) const
        {
            return Find(section, key, value, capacity);
        }

        float Real(const char* section, const char* key, float fallback) const
        {
            char token[32];
            if (!Text(section, key, token, sizeof(token)))
                return fallback;
            char* end{};
            const float value = std::strtof(token, &end);
            return end != token && !*end && std::isfinite(value) ? value : fallback;
        }

        bool Toggle(const char* section, const char* key, bool fallback) const
        {
            char token[8];
            if (!Text(section, key, token, sizeof(token)))
                return fallback;
            const auto end = token + std::strlen(token);
            for (const char* option : {"1", "true", "yes", "on", "y"})
                if (Match(token, end, option))
                    return true;
            for (const char* option : {"0", "false", "no", "off", "n"})
                if (Match(token, end, option))
                    return false;
            return fallback;
        }
    };
}

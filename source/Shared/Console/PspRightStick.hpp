#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string_view>
extern "C"
{
#include <pspiofilemgr.h>
}

namespace console::portable
{
    enum class RightStickBinding { Unavailable, Unbound, Mapped };

    inline RightStickBinding ReadRightStickBinding(const char* path)
    {
        const auto file = sceIoOpen(path, PSP_O_RDONLY, 0);
        if (file < 0)
            return RightStickBinding::Unavailable;

        const auto trim = [](std::string_view text)
        {
            const auto first = text.find_first_not_of(" \t\r");
            if (first == text.npos)
                return std::string_view{};
            text.remove_prefix(first);
            text.remove_suffix(text.size() - 1 - text.find_last_not_of(" \t\r"));
            return text;
        };
        const auto same = [](std::string_view left, std::string_view right)
        {
            const auto lower = [](char ch) { return ch >= 'A' && ch <= 'Z' ? char(ch + 'a' - 'A') : ch; };
            return left.size() == right.size() && std::equal(left.begin(), left.end(), right.begin(),
                [&](char a, char b) { return lower(a) == lower(b); });
        };
        constexpr std::string_view keys[]{"RightAn.Up", "RightAn.Down", "RightAn.Left", "RightAn.Right"};
        bool section = false;
        bool finished = false;
        bool mapped = false;
        unsigned seen = 0;
        const auto process = [&](std::string_view text)
        {
            if (text.size() >= 3 && std::memcmp(text.data(), "\xEF\xBB\xBF", 3) == 0)
                text.remove_prefix(3);
            text = trim(text);
            if (text.empty())
                return;
            const auto closingBracket = text.find(']');
            if (text.front() == '[' && closingBracket != text.npos)
            {
                finished = section;
                if (!section)
                    section = same({text.data() + 1, closingBracket - 1}, "ControlMapping");
            }
            else if (section && text.front() != ';')
            {
                const auto comment = text.find('#');
                if (comment != text.npos)
                    text.remove_suffix(text.size() - comment);
                const auto equals = text.find('=');
                if (equals == text.npos)
                    return;
                const auto key = trim({text.data(), equals});
                for (unsigned i = 0; i < std::size(keys); ++i)
                {
                    const unsigned bit = 1u << i;
                    if ((seen & bit) || !same(key, keys[i]))
                        continue;
                    seen |= bit;
                    text.remove_prefix(equals + 1);
                    auto value = trim(text);
                    if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
                    {
                        value.remove_prefix(1);
                        value.remove_suffix(1);
                    }
                    if (value.find_first_not_of(" \t\r,:") != value.npos)
                        finished = mapped = true;
                    break;
                }
            }
        };

        // Only the section and mapping lines need inspection. Large profiles
        // may put this section after many unrelated emulator settings.
        char chunk[1024];
        char line[512];
        size_t length = 0;
        int size = 0;
        while (!finished && (size = sceIoRead(file, chunk, sizeof(chunk))) > 0)
        {
            for (int i = 0; i < size && !finished; ++i)
            {
                if (chunk[i] == '\n')
                {
                    process({line, length});
                    length = 0;
                }
                else if (length < sizeof(line))
                    line[length++] = chunk[i];
            }
        }
        if (!finished && size == 0 && length)
            process({line, length});
        const int closed = sceIoClose(file);
        if (size < 0 || closed < 0)
            return RightStickBinding::Unavailable;
        // PPSSPP restores its default mappings if the section is absent.
        return !section || mapped ? RightStickBinding::Mapped : RightStickBinding::Unbound;
    }

    class PspRightStick
    {
        char gameProfile[64]{};
        uint32_t lastRead{};
        bool mapped{true};

    public:
        void Initialize(uint32_t now)
        {
            // PPSSPP names per-game profiles with the UMD ID without its dash.
            char disc[11]{};
            const auto file = sceIoOpen("disc0:/UMD_DATA.BIN", PSP_O_RDONLY, 0);
            if (file >= 0)
            {
                const auto size = sceIoRead(file, disc, sizeof(disc));
                const auto closed = sceIoClose(file);
                if (size == sizeof(disc) && closed >= 0 && disc[4] == '-' && disc[10] == '|')
                    std::snprintf(gameProfile, sizeof(gameProfile), "ms0:/PSP/SYSTEM/%.4s%.5s_ppsspp.ini", disc, disc + 5);
            }
            Refresh(now);
        }

        void Poll(uint32_t now)
        {
            if (now - lastRead >= 250000u)
                Refresh(now);
        }

        bool Mapped() const
        {
            return mapped;
        }

    private:
        void ReadProfile(const char* path)
        {
            const auto binding = ReadRightStickBinding(path);
            if (binding != RightStickBinding::Unavailable)
                mapped = binding == RightStickBinding::Mapped;
        }

        void Refresh(uint32_t now)
        {
            lastRead = now;
            // Read the active profile again after PPSSPP saves a mapping change.
            // The file can be replaced in the same timestamp tick.
            constexpr uint32_t fileMissing = 0x80010002u;
            SceIoStat stat{};
            if (gameProfile[0])
            {
                const auto status = sceIoGetstat(gameProfile, &stat);
                if (status >= 0)
                {
                    ReadProfile(gameProfile);
                    return;
                }
                if (uint32_t(status) != fileMissing)
                    return;
            }
            constexpr const char* globalProfile = "ms0:/PSP/SYSTEM/controls.ini";
            const auto status = sceIoGetstat(globalProfile, &stat);
            if (status >= 0)
                ReadProfile(globalProfile);
            else if (uint32_t(status) == fileMissing)
                mapped = true;
        }
    };
}

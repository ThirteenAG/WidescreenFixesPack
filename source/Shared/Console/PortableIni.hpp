#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
extern "C" {
#include <pspiofilemgr.h>
}

namespace console::portable {
// Bounded INI update for a module's own settings file. Existing lines, comments
// and unrelated keys are kept; changed keys are rewritten in place and missing
// keys are appended to their section. No heap or stdio is used.
struct IniValue {
    const char* section;
    const char* key;
    char value[16];
};
enum class IniUpdate { Replace, AddMissing };
// Keep one byte available for the terminator in the settings reader.
inline constexpr size_t IniCapacity = 8192;

template<size_t Capacity = IniCapacity> class IniWriter {
    static_assert(Capacity > 1);
    static constexpr uint32_t fileNotFound = 0x80010002u;
    static constexpr size_t pathCapacity = 192;
    inline static char input[Capacity];
    inline static char output[Capacity];
    size_t used = 0;
    bool overflow = false;
    void append(const char* text, size_t length) {
        if (length >= Capacity - used) { overflow = true; return; }
        std::memcpy(output + used, text, length); used += length;
    }
    void append(const char* text) { append(text, std::strlen(text)); }
    // Insert at an earlier output position, used for keys missing in a section.
    void insert(size_t at, const char* text, size_t length) {
        if (length >= Capacity - used || at > used) { overflow = true; return; }
        std::memmove(output + at + length, output + at, used - at);
        std::memcpy(output + at, text, length); used += length;
    }
    static bool same(const char* a, size_t length, const char* b) {
        if (std::strlen(b) != length) return false;
        for (size_t i = 0; i < length; ++i) {
            char x = a[i], y = b[i];
            if (x >= 'A' && x <= 'Z') x = char(x - 'A' + 'a');
            if (y >= 'A' && y <= 'Z') y = char(y - 'A' + 'a');
            if (x != y) return false;
        }
        return true;
    }
    static void trim(const char*& begin, const char*& end) {
        while (begin < end && (*begin == ' ' || *begin == '\t')) ++begin;
        while (end > begin && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r')) --end;
    }

    static size_t bomLength(size_t size)
    {
        return size >= 3 && std::memcmp(input, "\xEF\xBB\xBF", 3) == 0 ? 3 : 0;
    }

    static const char* lineSection(const char* begin, const char* end)
    {
        for (auto cursor = begin; cursor != end; ++cursor)
            if (*cursor == ';' || *cursor == '#') { end = cursor; break; }
        trim(begin, end);
        return end > begin && end[-1] == ']' ? end - 1 : nullptr;
    }

    static const char* lineEquals(const char* begin, const char* end)
    {
        if (begin == end || *begin == ';' || *begin == '#')
            return nullptr;
        return static_cast<const char*>(std::memchr(begin, '=', size_t(end - begin)));
    }

    // Scan the checked snapshot before inserting defaults so a key in a later
    // occurrence of the same section is still treated as existing.
    static uint32_t existingKeys(size_t size, const IniValue* values, unsigned count)
    {
        const char* section = nullptr;
        size_t sectionLength = 0;
        uint32_t found = 0;
        const char* end = input + size;
        for (const char* cursor = input + bomLength(size); cursor < end;)
        {
            const auto lineEnd = static_cast<const char*>(std::memchr(cursor, '\n', size_t(end - cursor)));
            const char* begin = cursor;
            const char* stop = lineEnd ? lineEnd : end;
            cursor = lineEnd ? lineEnd + 1 : end;
            trim(begin, stop);
            if (begin != stop && *begin == '[')
            {
                section = nullptr;
                if (auto close = lineSection(begin, stop))
                {
                    section = begin + 1;
                    trim(section, close);
                    sectionLength = size_t(close - section);
                }
            }
            else if (const auto equals = lineEquals(begin, stop); section && equals)
            {
                stop = equals;
                trim(begin, stop);
                for (unsigned i = 0; i < count; ++i)
                    if (same(section, sectionLength, values[i].section) && same(begin, size_t(stop - begin), values[i].key))
                        found |= 1u << i;
            }
        }
        return found;
    }
    static bool FileMissing(int result)
    {
        return static_cast<uint32_t>(result) == fileNotFound;
    }

    template<size_t SuffixLength>
    static bool SidePath(char (&out)[pathCapacity], const char* path, const char (&suffix)[SuffixLength])
    {
        const size_t length = std::strlen(path);
        if (length > sizeof(out) - SuffixLength)
            return false;
        std::memcpy(out, path, length);
        std::memcpy(out + length, suffix, SuffixLength);
        return true;
    }

    // A previous replacement may have stopped after moving the original.
    static bool RecoverFile(const char* path, const char* previous)
    {
        SceIoStat stat{};
        const int current = sceIoGetstat(path, &stat);
        if (current >= 0)
            return true;
        if (!FileMissing(current))
            return false;

        const int recovery = sceIoGetstat(previous, &stat);
        return FileMissing(recovery) || (recovery >= 0 && sceIoRename(previous, path) >= 0);
    }

    static bool MatchesSnapshot(const char* path, size_t size)
    {
        const SceUID file = sceIoOpen(path, PSP_O_RDONLY, 0);
        if (file < 0)
            return false;
        char chunk[512];
        size_t offset = 0;
        bool matches = true;
        while (offset < size)
        {
            const size_t length = size - offset < sizeof(chunk) ? size - offset : sizeof(chunk);
            if (sceIoRead(file, chunk, unsigned(length)) != int(length) ||
                std::memcmp(input + offset, chunk, length) != 0)
            {
                matches = false;
                break;
            }
            offset += length;
        }
        if (matches)
            matches = sceIoRead(file, chunk, 1) == 0;
        return sceIoClose(file) >= 0 && matches;
    }

public:
    bool recover(const char* path)
    {
        char previous[pathCapacity];
        return SidePath(previous, path, ".wfp-old") && RecoverFile(path, previous);
    }

    // An expected snapshot ties edits derived from previous values to that
    // exact input. Returns false on a conflict or a read/write failure.
    bool update(const char* path, const IniValue* values, unsigned count, IniUpdate mode = IniUpdate::Replace,
        std::string_view expected = {}) {
        if (count > 32) return false;
        if (!count) return true;

        char temporary[pathCapacity], previous[pathCapacity];
        if (!SidePath(temporary, path, ".tmp") || !SidePath(previous, path, ".wfp-old") ||
            !RecoverFile(path, previous))
            return false;

        SceIoStat stat{};
        const int status = sceIoGetstat(path, &stat);
        const bool exists = status >= 0;
        if (!exists && !FileMissing(status))
            return false;

        used = 0;
        overflow = false;
        size_t size = 0;
        if (exists)
        {
            if (stat.st_size < 0 || uint64_t(stat.st_size) >= Capacity)
                return false;
            const SceUID in = sceIoOpen(path, PSP_O_RDONLY, 0);
            if (in < 0)
                return false;
            const int read = sceIoRead(in, input, Capacity);
            const int closed = sceIoClose(in);
            if (read != stat.st_size || closed < 0)
                return false;
            size = size_t(read);
        }
        if (!expected.empty() && (expected.size() != size || std::memcmp(input, expected.data(), size) != 0))
            return false;

        uint32_t written = mode == IniUpdate::AddMissing ? existingKeys(size, values, count) : 0;
        if (written == (UINT32_MAX >> (32 - count)))
            return true;
        const char* newline = "\r\n";
        for (size_t i = 0; i < size; ++i) if (input[i] == '\n') { newline = i && input[i-1] == '\r' ? "\r\n" : "\n"; break; }
        const size_t newlineLength = std::strlen(newline);
        const char* section = nullptr; size_t sectionLength = 0, insertAt = 0;
        const auto finishSection = [&]() {
            if (!section) return;
            for (unsigned i = 0; i < count; ++i) {
                if ((written & (1u << i)) || !same(section, sectionLength, values[i].section)) continue;
                char text[96]; size_t length = 0;
                const auto add = [&](const char* value) {
                    const size_t n = std::strlen(value);
                    if (length + n <= sizeof(text)) { std::memcpy(text + length, value, n); length += n; }
                    else overflow = true;
                };
                add(values[i].key); add(" = "); add(values[i].value); add(newline);
                insert(insertAt, text, length); insertAt += length;
                written |= 1u << i;
            }
        };
        const size_t prefixSize = bomLength(size);
        append(input, prefixSize);
        const char* cursor = input + prefixSize; const char* end = input + size;
        while (cursor < end) {
            const char* lineEnd = static_cast<const char*>(std::memchr(cursor, '\n', size_t(end - cursor)));
            const char* next = lineEnd ? lineEnd + 1 : end;
            const char* begin = cursor; const char* stop = lineEnd ? lineEnd : end;
            trim(begin, stop);
            if (begin < stop && *begin == '[') {
                finishSection();
                section = nullptr;
                if (auto close = lineSection(begin, stop)) {
                    section = begin + 1;
                    trim(section, close);
                    sectionLength = size_t(close - section);
                }
                append(cursor, size_t(next - cursor));
                if (!lineEnd) append(newline, newlineLength);
                insertAt = used;
                cursor = next; continue;
            }
            const char* equals = lineEquals(begin, stop);
            bool replaced = false;
            if (equals && section) {
                const char* keyBegin = begin; const char* keyEnd = equals;
                trim(keyBegin, keyEnd);
                for (unsigned i = 0; i < count; ++i) {
                    if ((written & (1u << i)) || !same(section, sectionLength, values[i].section) ||
                        !same(keyBegin, size_t(keyEnd - keyBegin), values[i].key)) continue;
                    const char* valueBegin = equals + 1;
                    const char* valueEnd = valueBegin;
                    while (valueEnd < stop && *valueEnd != '#' && *valueEnd != ';')
                        ++valueEnd;
                    trim(valueBegin, valueEnd);
                    append(cursor, size_t(valueBegin - cursor));
                    append(values[i].value);
                    append(valueEnd, size_t(next - valueEnd));
                    if (!lineEnd)
                        append(newline, newlineLength);
                    written |= 1u << i;
                    replaced = true;
                    break;
                }
            }
            if (!replaced) {
                append(cursor, size_t(next - cursor));
                if (!lineEnd) append(newline, newlineLength);
            }
            // Missing keys go after the section's last non-blank line.
            if (begin < stop) insertAt = used;
            cursor = next;
        }
        finishSection();
        for (unsigned i = 0; i < count; ++i) {
            if (written & (1u << i)) continue;
            if (used > prefixSize) append(newline, newlineLength);
            append("["); append(values[i].section); append("]"); append(newline, newlineLength);
            for (unsigned j = i; j < count; ++j)
                if (!(written & (1u << j)) && same(values[i].section, std::strlen(values[i].section), values[j].section)) {
                    append(values[j].key); append(" = "); append(values[j].value); append(newline, newlineLength);
                    written |= 1u << j;
                }
        }
        if (overflow) return false;
        const SceUID out = sceIoOpen(temporary, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
        if (out < 0) return false;
        const int result = sceIoWrite(out, output, unsigned(used));
        const int closed = sceIoClose(out);
        if (result != int(used) || closed < 0)
        {
            sceIoRemove(temporary);
            return false;
        }

        // PSP rename cannot replace the destination. Keep the original until
        // the complete, closed replacement reaches its final path.
        if (exists)
        {
            const int oldStatus = sceIoGetstat(previous, &stat);
            const bool available = FileMissing(oldStatus) || (oldStatus >= 0 && sceIoRemove(previous) >= 0);
            if (!available || sceIoRename(path, previous) < 0)
            {
                sceIoRemove(temporary);
                return false;
            }
            // The editor can commit while the replacement is being prepared.
            // Compare the file actually displaced, including all unrelated keys.
            if (!MatchesSnapshot(previous, size))
            {
                sceIoRename(previous, path); // Leave the recovery file if this also fails.
                sceIoRemove(temporary);
                return false;
            }
        }
        else if (!FileMissing(sceIoGetstat(path, &stat)))
        {
            sceIoRemove(temporary);
            return false;
        }
        if (sceIoRename(temporary, path) < 0)
        {
            if (exists)
                sceIoRename(previous, path); // Leave the recovery file if this also fails.
            sceIoRemove(temporary);
            return false;
        }
        if (exists)
            sceIoRemove(previous);
        return true;
    }
};
}

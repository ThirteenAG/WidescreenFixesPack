#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
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
template<size_t Capacity = 8192> class IniWriter {
    inline static char input[Capacity];
    inline static char output[Capacity];
    size_t used = 0;
    bool overflow = false;
    void append(const char* text, size_t length) {
        if (used + length > Capacity) { overflow = true; return; }
        std::memcpy(output + used, text, length); used += length;
    }
    void append(const char* text) { append(text, std::strlen(text)); }
    // Insert at an earlier output position, used for keys missing in a section.
    void insert(size_t at, const char* text, size_t length) {
        if (used + length > Capacity || at > used) { overflow = true; return; }
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
public:
    // Returns false if the file is too large, unreadable or cannot be written.
    bool update(const char* path, const IniValue* values, unsigned count) {
        if (count > 32) return false;
        if (!count) return true;
        used = 0; overflow = false;
        size_t size = 0;
        const SceUID in = sceIoOpen(path, PSP_O_RDONLY, 0);
        if (in >= 0) {
            const int read = sceIoRead(in, input, Capacity);
            sceIoClose(in);
            if (read < 0 || size_t(read) >= Capacity) return false;
            size = size_t(read);
        }
        uint32_t written = 0;
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
        const char* cursor = input; const char* end = input + size;
        while (cursor < end) {
            const char* lineEnd = static_cast<const char*>(std::memchr(cursor, '\n', size_t(end - cursor)));
            const char* next = lineEnd ? lineEnd + 1 : end;
            const char* begin = cursor; const char* stop = lineEnd ? lineEnd : end;
            trim(begin, stop);
            if (begin < stop && *begin == '[') {
                const char* close = static_cast<const char*>(std::memchr(begin, ']', size_t(stop - begin)));
                if (close) {
                    finishSection();
                    section = begin + 1; sectionLength = size_t(close - begin - 1);
                }
                append(cursor, size_t(next - cursor));
                if (!lineEnd) append(newline, newlineLength);
                insertAt = used;
                cursor = next; continue;
            }
            const char* equals = begin < stop && *begin != ';' && *begin != '#'
                ? static_cast<const char*>(std::memchr(begin, '=', size_t(stop - begin))) : nullptr;
            bool replaced = false;
            if (equals && section) {
                const char* keyBegin = begin; const char* keyEnd = equals;
                trim(keyBegin, keyEnd);
                for (unsigned i = 0; i < count; ++i) {
                    if ((written & (1u << i)) || !same(section, sectionLength, values[i].section) ||
                        !same(keyBegin, size_t(keyEnd - keyBegin), values[i].key)) continue;
                    append(cursor, size_t(equals - cursor + 1));
                    append(" "); append(values[i].value); append(newline, newlineLength);
                    written |= 1u << i; replaced = true; break;
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
            if (used) append(newline, newlineLength);
            append("["); append(values[i].section); append("]"); append(newline, newlineLength);
            for (unsigned j = i; j < count; ++j)
                if (!(written & (1u << j)) && same(values[i].section, std::strlen(values[i].section), values[j].section)) {
                    append(values[j].key); append(" = "); append(values[j].value); append(newline, newlineLength);
                    written |= 1u << j;
                }
        }
        if (overflow) return false;
        char temporary[192];
        const size_t pathLength = std::strlen(path);
        if (pathLength + 5 > sizeof(temporary)) return false;
        std::memcpy(temporary, path, pathLength); std::memcpy(temporary + pathLength, ".tmp", 5);
        const SceUID out = sceIoOpen(temporary, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
        if (out < 0) return false;
        const int result = sceIoWrite(out, output, unsigned(used));
        sceIoClose(out);
        if (result != int(used)) { sceIoRemove(temporary); return false; }
        // PSP rename does not replace an existing file. The complete new file
        // exists before the old one is removed.
        sceIoRemove(path);
        return sceIoRename(temporary, path) >= 0;
    }
};
}

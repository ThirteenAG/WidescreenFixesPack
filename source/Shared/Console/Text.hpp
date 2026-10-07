#pragma once
#include <cstddef>
#include <cstdint>
#include <cmath>

namespace console {
// Small ASCII menu labels. No stdio, locale state or newlib dtoa allocations.
template<size_t Size> struct Text {
    static_assert(Size > 0);
    char (&buffer)[Size];
    size_t used = 0;
    explicit Text(char (&output)[Size]) : buffer(output) { buffer[0] = 0; }
    void character(char value) {
        if (used + 1 < Size) { buffer[used++] = value; buffer[used] = 0; }
    }
    void append(const char* value) {
        while (*value && used + 1 < Size) character(*value++);
    }
    void integer(uint32_t value) {
        char digits[10]; unsigned count = 0;
        do { digits[count++] = char('0' + value % 10); value /= 10; } while (value);
        while (count) character(digits[--count]);
    }
    void fixed(float value, unsigned precision) {
        // Menu settings are small bounded values. Reject unexpected input
        // before conversion, and cap precision to avoid integer overflow.
        if (!std::isfinite(value) || value < -1000 || value > 1000 || precision > 4) {
            character('?'); return;
        }
        if (value < 0) { character('-'); value = -value; }
        uint32_t factor = 1;
        for (unsigned i = 0; i < precision; ++i) factor *= 10;
        const uint32_t rounded = uint32_t(value * float(factor) + 0.5f);
        integer(rounded / factor);
        if (!precision) return;
        character('.');
        for (uint32_t digit = factor / 10; digit; digit /= 10)
            character(char('0' + rounded / digit % 10));
    }
};
template<size_t Size> void format(char (&output)[Size], const char* label, float value, unsigned precision) {
    Text<Size> text(output); text.append(label); text.fixed(value, precision);
}
template<size_t Size> void format(char (&output)[Size], const char* label, const char* value) {
    Text<Size> text(output); text.append(label); text.append(value);
}
}

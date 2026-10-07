#pragma once
#include "StoriesBindings.hpp"
#include <cstring>

// Help text that names PS2 buttons in plain words ("R3 button", "left analog
// stick", "TRIANGLE button"), rewritten for the PC control scheme when the
// text is fetched. Templates are ASCII with placeholders:
//   {Action}  primary key of a binding (actions[].ini, e.g. {Mission})
//   {enter} {back} {mouse}  fixed keys (menu select, menu back, mouse look)
// The writer supplies how a key is shown (artwork token or key name).
namespace console::stories {
struct TextRewrite { const char* key; const char* text; };

inline uint8_t PlaceholderKey(const char* name, unsigned length, const Bindings& bindings) {
    auto is = [&](const char* word) { return std::strlen(word) == length && !std::strncmp(word, name, length); };
    if (is("enter")) return code::enter;
    if (is("back")) return code::back;
    if (is("mouse")) return code::mouseMove;
    for (unsigned i = 0; i < ActionCount; ++i)
        if (is(actions[i].ini)) return bindings.primary(Action(i));
    return 0;
}
// Writer: put(char16_t) and key(uint8_t code); key(0) is never called.
template<class Writer> void FormatText(const char* text, const Bindings& bindings, Writer& writer) {
    while (*text) {
        if (*text == '{') {
            const char* end = text + 1;
            while (*end && *end != '}') ++end;
            if (*end == '}') {
                const uint8_t key = PlaceholderKey(text + 1, unsigned(end - text - 1), bindings);
                if (key) writer.key(key); else writer.put(u'-');
                text = end + 1;
                continue;
            }
        }
        writer.put(char16_t(uint8_t(*text++)));
    }
}
template<size_t Count> const TextRewrite* FindRewrite(const TextRewrite (&table)[Count], const char* key) {
    for (const auto& entry : table) if (!std::strcmp(entry.key, key)) return &entry;
    return nullptr;
}
}

#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace console
{
struct CheatEvents
{
    uint32_t generation, sequence, count;
    char events[32];
};
static_assert(sizeof(CheatEvents) == 44 && alignof(CheatEvents) == 4);

// Discard everything before the first gameplay tick and consume each release once.
struct TypedCheats
{
    char word[25]{};
    uint32_t generation{}, consumed{};
    bool active{};

    static bool matches(const char* history, const char* keyword)
    {
        const std::size_t length = std::strlen(keyword);
        if (length > sizeof(word)) return false;
        for (std::size_t i = 0; i < length; ++i)
            if (history[i] != keyword[length - 1 - i]) return false;
        return true;
    }

    template<class Check>
    void tick(bool gameplay, char (&legacy)[sizeof(word)], const volatile CheatEvents& source, Check&& check)
    {
        if (!gameplay)
        {
            active = false;
            word[0] = 0;
        }
        if (!source.generation)
        {
            if (!active || check(legacy))
                std::memset(legacy, 0, sizeof(legacy));
            active = gameplay;
            return;
        }

        const uint32_t sequence = source.sequence;
        if (sequence & 1u) return;
        std::atomic_thread_fence(std::memory_order_acquire);
        const uint32_t currentGeneration = source.generation;
        const uint32_t count = source.count;
        constexpr uint32_t capacity = sizeof(source.events);
        char events[capacity];
        const bool read = active && currentGeneration == generation && consumed != count;
        if (read)
            for (unsigned i = 0; i < capacity; ++i)
                events[i] = source.events[i];
        std::atomic_thread_fence(std::memory_order_acquire);
        if (sequence != source.sequence || currentGeneration != source.generation) return;

        if (read)
        {
            if (count - consumed > capacity)
            {
                consumed = count - capacity;
                word[0] = 0;
            }
            while (consumed != count)
            {
                const char key = events[consumed++ % capacity];
                if (key)
                {
                    std::memmove(word + 1, word, sizeof(word) - 1);
                    word[0] = key;
                    if (check(word)) word[0] = 0;
                }
                else
                    word[0] = 0;
            }
        }
        else if (!active || currentGeneration != generation)
            word[0] = 0;

        consumed = count;
        generation = currentGeneration;
        active = gameplay;
    }
};
}

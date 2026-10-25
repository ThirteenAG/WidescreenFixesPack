#pragma once
#include <cstdint>

namespace WFPMenu
{
    class Owner
    {
    public:
        struct Token
        {
            void* screen{};
            uint64_t generation{};
        };

        void Bind(void* screen)
        {
            ++generation;
            current = screen;
        }

        void Clear()
        {
            ++generation;
            current = nullptr;
        }

        bool Clear(void* screen)
        {
            if (!screen || screen != current)
                return false;
            Clear();
            return true;
        }

        void* Get() const
        {
            return current;
        }

        Token Capture() const
        {
            return { current, generation };
        }

        bool Matches(Token token) const
        {
            return token.screen && token.screen == current && token.generation == generation;
        }

    private:
        void* current{};
        uint64_t generation{};
    };
}

#pragma once
#include <atomic>

namespace ScarfaceProfile
{
    struct Context
    {
        bool(__cdecl* isPaused)();
        const uintptr_t* flowManager;
        void* const* character;
        uint32_t pilotOffset;
    };

    inline std::atomic<const Context*> Bindings{ nullptr };
    inline std::atomic<const wchar_t*> Selected{ nullptr };

    inline const wchar_t* Read()
    {
        return Selected.load(std::memory_order_acquire);
    }

    inline void Update()
    {
        const wchar_t* profile = nullptr;
        const auto context = Bindings.load(std::memory_order_acquire);
        if (context && context->isPaused)
        {
            const auto flow = context->flowManager ? *context->flowManager : 1;
            if (flow && flow != 0xDDDDDDDD && !context->isPaused())
            {
                const auto player = context->character ? *context->character : nullptr;
                if (player && reinterpret_cast<uintptr_t>(player) != 0xDDDDDDDD &&
                    *reinterpret_cast<const uint32_t*>(static_cast<const uint8_t*>(player) + context->pilotOffset))
                    profile = L"InCar";
                else
                    profile = L"OnFoot";
            }
        }
        Selected.store(profile, std::memory_order_release);
    }

    inline void Shutdown()
    {
        Bindings.store(nullptr, std::memory_order_release);
        Selected.store(nullptr, std::memory_order_release);
    }
}

#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <string>

namespace NFSPSDialog
{
    struct Configuration
    {
        void* table;
        uint8_t custom;
        uint8_t hide;
        uint16_t reserved;
        uint32_t title;
        uint8_t buttons;
        uint8_t padding[3];
        wchar_t* body;
        wchar_t* labels[3];
        uint32_t opacity;
        uint32_t context;
    };
    static_assert(sizeof(Configuration) == 40);
    static_assert(offsetof(Configuration, body) == 16);

    inline Configuration** Pending = nullptr;
    inline void** Instance = nullptr;
    inline void* Table = nullptr;
    inline Configuration* Publishing = nullptr;
    inline void* OwnedInstance = nullptr;
    inline void* (__cdecl* Allocate)(size_t) = nullptr;
    inline void (__cdecl* Free)(void*) = nullptr;
    inline void (__cdecl* Open)() = nullptr;

    inline bool Busy()
    {
        return !Pending || !Instance || *Pending || *Instance;
    }

    inline bool Show(std::wstring body, std::wstring label, const std::function<bool()>& current)
    {
        if (Busy() || !Table || !Allocate || !Free || !Open || body.empty() || label.empty() || !current())
            return false;
        auto config = static_cast<Configuration*>(Allocate(sizeof(Configuration)));
        if (!config)
            return false;
        std::memset(config, 0, sizeof(Configuration));
        config->table = Table;
        config->hide = 1;
        config->buttons = 1;
        config->opacity = 128;
        auto destroy = [&]()
        {
            Free(config->body);
            Free(config->labels[0]);
            Free(config);
        };
        auto copy = [&](const std::wstring& text)
        {
            auto bytes = (text.size() + 1) * sizeof(wchar_t);
            auto result = static_cast<wchar_t*>(Allocate(bytes));
            if (result)
                std::memcpy(result, text.c_str(), bytes);
            return result;
        };
        if (current())
            config->body = copy(body);
        if (config->body && current())
            config->labels[0] = copy(label);
        if (!config->labels[0] || !current() || Busy())
        {
            destroy();
            return false;
        }
        // The native dialog consumes this configuration and owns all three CRT allocations.
        *Pending = config;
        Publishing = config;
        Open();
        if (*Pending != config)
            Publishing = nullptr;
        return true;
    }
}

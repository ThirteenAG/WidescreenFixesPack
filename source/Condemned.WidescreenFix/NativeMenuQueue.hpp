#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

// Disconnected, allocation-free callback storage. Admission still requires the
// future host's exact sender/current-selection/modal checks. No engine calls,
// strings, plugin API calls or native ownership changes occur here.
namespace CondemnedMenuNative
{
    enum class CommandKind : uint8_t { Open, Accept, Left, Right, Back };
    struct Request
    {
        CommandKind kind{};
        uint64_t owner{}, pool{};
        uint16_t selected{};
        uintptr_t sender{};
    };
    class Pending
    {
        std::array<Request, 32> entries{};
        size_t first{}, count{};
    public:
        bool Full() const noexcept { return count == entries.size(); }
        bool Empty() const noexcept { return count == 0; }
        // A caller consumes input only on true. Capacity is checked before any
        // logical Back cancellation, so a rejected key cannot retire a session.
        bool Push(Request request) noexcept
        {
            if (Full()) return false;
            entries[(first + count) % entries.size()] = request;
            ++count;
            return true;
        }
        std::optional<Request> Pop() noexcept
        {
            if (!count) return {};
            auto request = entries[first];
            first = (first + 1) % entries.size();
            --count;
            return request;
        }
        void Clear() noexcept { first = count = 0; }
    };
}

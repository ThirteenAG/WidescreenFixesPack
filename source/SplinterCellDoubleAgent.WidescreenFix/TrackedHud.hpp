#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <mutex>

namespace DAHud
{
    class Positions
    {
        struct Position
        {
            int16_t* value;
            int baseline;
            int16_t written;
            int direction;
        };
        std::mutex mutex;
        std::map<void*, Position> positions;
        int shift = 0;

        static int16_t Clamp(int value)
        {
            return int16_t(std::clamp(value, int(std::numeric_limits<int16_t>::min()), int(std::numeric_limits<int16_t>::max())));
        }
        void Apply(Position& position)
        {
            if (*position.value != position.written)
                position.baseline = *position.value;
            position.written = Clamp(position.baseline + position.direction * shift);
            *position.value = position.written;
        }

    public:
        static int Offset(float value)
        {
            if (!std::isfinite(value))
                return 0;
            return int(std::clamp(value, float(std::numeric_limits<int16_t>::min()), float(std::numeric_limits<int16_t>::max())));
        }
        void Add(void* owner, int16_t& value, int direction)
        {
            if (!owner || (direction != -1 && direction != 1))
                return;
            std::lock_guard lock(mutex);
            auto [at, inserted] = positions.try_emplace(owner, Position{ &value, value, value, direction });
            if (!inserted && (at->second.value != &value || at->second.direction != direction))
                at->second = { &value, value, value, direction };
            Apply(at->second);
        }
        void Remove(void* owner)
        {
            std::lock_guard lock(mutex);
            positions.erase(owner);
        }
        void SetOffset(float value)
        {
            std::lock_guard lock(mutex);
            shift = Offset(value);
            for (auto& [owner, position] : positions)
                Apply(position);
        }
    };
}

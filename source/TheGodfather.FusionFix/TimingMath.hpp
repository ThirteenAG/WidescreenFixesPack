#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace GodfatherTiming
{
    struct Remainders
    {
        double ticks = 0.0;
        double microseconds = 0.0;
    };

    struct Frame
    {
        bool valid = false, running = false;
        uint32_t ticks = 0, milliseconds = 0, gameMilliseconds = 0, elapsedMilliseconds = 0;
        uint64_t gameMicroseconds = 0;
        float tickError = 0.0f, seconds = 0.0f, elapsed = 0.0f, scaled = 0.0f, gameSeconds = 0.0f;
    };

    inline Frame Advance(Remainders& remainder, double elapsed, float quantum, float frequency, uint32_t maximumTicks,
        float scale, bool paused, uint32_t ticks, uint64_t gameMicroseconds, float previousScaled)
    {
        if (!std::isfinite(elapsed) || elapsed < 0.0 || !std::isfinite(quantum) || quantum <= 0.0f
            || !std::isfinite(frequency) || frequency <= 0.0f || !maximumTicks || !std::isfinite(scale) || scale < 0.0f)
            return {};
        const double maximum = double(quantum) * maximumTicks * 0.001;
        elapsed = std::min(elapsed, maximum);
        const double wantedTicks = elapsed * 1000.0 / quantum + remainder.ticks;
        const double steps = std::floor(wantedTicks + 0.5);
        const double wantedMicroseconds = elapsed * scale * 1000000.0 + remainder.microseconds;
        const double microseconds = std::floor(wantedMicroseconds + 0.5);
        if (steps < 0.0 || steps > maximumTicks || !std::isfinite(microseconds)
            || microseconds < 0.0 || microseconds >= 18446744073709551616.0)
            return {};
        remainder.ticks = wantedTicks - steps;
        Frame result;
        result.valid = true;
        result.running = !paused;
        result.ticks = ticks + uint32_t(steps);
        result.tickError = float(-remainder.ticks);
        result.milliseconds = uint32_t(uint64_t(double(result.ticks) * quantum + 0.5));
        result.seconds = float(double(result.ticks) / frequency);
        result.elapsed = float(elapsed);
        result.elapsedMilliseconds = uint32_t(std::floor(elapsed * 1000.0 + 0.5));
        result.scaled = paused ? previousScaled : float(elapsed * scale);
        result.gameMicroseconds = gameMicroseconds;
        if (!paused)
        {
            result.gameMicroseconds += uint64_t(microseconds);
            remainder.microseconds = wantedMicroseconds - microseconds;
        }
        result.gameMilliseconds = uint32_t(uint64_t(double(result.gameMicroseconds) * 0.001 + 0.5));
        result.gameSeconds = float(double(result.gameMicroseconds) * 0.000001);
        return result;
    }
}

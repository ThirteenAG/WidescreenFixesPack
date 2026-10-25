#pragma once
#include <initializer_list>
#include <string>
#include <mutex>
#include <unordered_set>

namespace PatternPreflight
{
    struct Requirement
    {
        const char* Signature;
        size_t Count{ 1 };
        bool Exact{ false };
    };

    inline void Report(const char* feature)
    {
        static std::mutex mutex;
        static std::unordered_set<std::string> reported;
        const std::lock_guard lock(mutex);
        if (reported.emplace(feature).second)
        {
            const auto message = std::string(feature) + ": required pattern unavailable; feature disabled.\n";
            OutputDebugStringA(message.c_str());
        }
    }

    // Resolve every required site before installing any part of a feature.
    inline bool Check(const char* feature, std::initializer_list<Requirement> requirements)
    {
        for (const auto& requirement : requirements)
        {
            auto pattern = hook::pattern(requirement.Signature);
            const auto count = pattern.size();
            if (requirement.Exact ? count != requirement.Count : count < requirement.Count)
            {
                Report(feature);
                return false;
            }
        }
        return true;
    }
}

#pragma once
#include <initializer_list>
#include <mutex>
#include <string>
#include <unordered_set>

inline void ReportPatternFailure(const char* feature)
{
    static std::mutex mutex;
    static std::unordered_set<std::string> reported;
    std::lock_guard lock(mutex);
    if (reported.emplace(feature).second)
        OutputDebugStringA((std::string(feature) + ": required pattern unavailable or ambiguous; feature disabled.\n").c_str());
}

inline bool PatternAvailable(hook::pattern& pattern, const char* feature)
{
    if (!pattern.empty())
        return true;
    ReportPatternFailure(feature);
    return false;
}

inline bool PatternUnique(hook::pattern& pattern, const char* feature)
{
    if (pattern.size() == 1)
        return true;
    ReportPatternFailure(feature);
    return false;
}

// Check a dependent patch group before writing any of its sites.
inline bool CheckPatterns(const char* feature, std::initializer_list<const char*> signatures)
{
    for (auto signature : signatures)
    {
        auto pattern = hook::pattern(signature);
        if (!PatternUnique(pattern, feature))
            return false;
    }
    return true;
}

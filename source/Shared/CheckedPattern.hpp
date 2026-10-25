#pragma once

#include <mutex>
#include <set>
#include <string>

namespace wfp::patterns
{
    inline void Report(const char* message)
    {
        static std::mutex lock;
        static std::set<std::string> reported;
        std::lock_guard guard(lock);
        if (reported.emplace(message).second)
            OutputDebugStringA(message);
    }

    inline void Missing(const char* signature)
    {
        const auto message = std::string("Widescreen Fix: feature disabled; pattern not found: ") + signature + "\n";
        Report(message.c_str());
    }

    template<class T = void> T* First(const char* signature, ptrdiff_t offset = 0)
    {
        auto pattern = hook::pattern(signature);
        if (pattern.empty())
        {
            Missing(signature);
            return nullptr;
        }
        return pattern.get_first<T>(offset);
    }
}

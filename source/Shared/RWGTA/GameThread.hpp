#pragma once
#include <functional>
#include <mutex>
#include <vector>

// Work for the game's main thread. Settings change on the thread that sets them: the game's menu runs on the main
// thread, but editors change them through the live link on its own. Appliers that touch the game's state post their
// work here; the fix runs it once per frame (gameplay and frontend idle).
namespace RWGTA::GameThread
{
    inline std::mutex& Mutex()
    {
        static std::mutex mutex;
        return mutex;
    }

    inline std::vector<std::function<void()>>& Queue()
    {
        static std::vector<std::function<void()>> queue;
        return queue;
    }

    inline void Post(std::function<void()> work)
    {
        std::lock_guard lock(Mutex());
        Queue().push_back(std::move(work));
    }

    // From the game's main thread, once per frame
    inline void Run()
    {
        std::vector<std::function<void()>> work;
        {
            std::lock_guard lock(Mutex());
            if (Queue().empty())
                return;
            work.swap(Queue());
        }
        for (auto& f : work)
            f();
    }
}

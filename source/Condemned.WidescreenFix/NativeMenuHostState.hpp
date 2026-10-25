#pragma once
#include "NativeMenuModel.hpp"
#include "NativeMenuQueue.hpp"
#include <array>
#include <atomic>
#include <limits>

// Disconnected host bookkeeping, not hook installation. The future wrappers
// must enter every proved whole engine scope, including owning deletion callers.
// Zero counters alone do not prove an arbitrary callback is a service boundary.
namespace CondemnedMenuNative
{
    enum class Phase : size_t { Update, Input, Render, Transition, Lifecycle, Foreign, Service, Count };
    class Traversals
    {
        std::atomic<DWORD> thread{};
        std::array<uint32_t, static_cast<size_t>(Phase::Count)> depths{};
        bool afterUpdate{};
    public:
        class Scope
        {
            Traversals* owner{};
            size_t index{};
        public:
            Scope() = default;
            Scope(Traversals& value, size_t at) : owner(&value), index(at) { ++owner->depths[index]; }
            Scope(const Scope&) = delete;
            Scope& operator=(const Scope&) = delete;
            Scope(Scope&& other) noexcept : owner(std::exchange(other.owner, nullptr)), index(other.index) {}
            ~Scope() { if (owner) --owner->depths[index]; }
            explicit operator bool() const { return owner != nullptr; }
        };
        // Called by the exact engine-facing Update wrapper, never by input or
        // discovery callbacks. Other threads must delegate stock behavior only.
        bool BindUpdateThread()
        {
            const auto now = GetCurrentThreadId();
            DWORD empty{};
            thread.compare_exchange_strong(empty, now, std::memory_order_acq_rel);
            return thread.load(std::memory_order_acquire) == now;
        }
        bool Here() const
        {
            const auto bound = thread.load(std::memory_order_acquire);
            return bound && bound == GetCurrentThreadId();
        }
        Scope Enter(Phase phase)
        {
            const auto index = static_cast<size_t>(phase);
            if (!Here() || index >= depths.size() || depths[index] == UINT32_MAX) return {};
            if (phase == Phase::Update) afterUpdate = false;
            return Scope(*this, index);
        }
        bool Quiet() const
        {
            return Here() && std::all_of(depths.begin(), depths.end(), [](uint32_t value) { return value == 0; });
        }
        // Invoked only after original whole10051150 and its Update Scope return.
        // If an enclosing broker/render/lifecycle is still active, its return
        // cannot be replaced with an assumed leaf/broker service point.
        bool MarkCompleteUpdate()
        {
            if (!Quiet()) return false;
            afterUpdate = true;
            return true;
        }
        Scope Service()
        {
            if (!Quiet() || !afterUpdate) return {};
            afterUpdate = false;
            return Enter(Phase::Service);
        }
    };
    struct LogicalOwner
    {
        uint64_t identity{}, pool{};
        Session settings;
        Pending pending;
        bool returnPending{};
        // admission supplied by the host includes exact native owner, selected
        // widget, current pool and modal/focus gates; this function does not
        // pretend a cached raw pointer is a native lifetime lease.
        bool Queue(Request request, bool admitted)
        {
            if (!admitted || request.owner != identity || request.pool != pool || !pending.Push(request)) return false;
            if (request.kind == CommandKind::Back && settings.InForeignCall())
            {
                returnPending = true;
                settings.Cancel(); // logical only; shared API burst stops now
            }
            return true;
        }
        void Invalidate()
        {
            settings.Cancel();
            returnPending = true;
            ++pool;
            pending.Clear();
        }
    };
}

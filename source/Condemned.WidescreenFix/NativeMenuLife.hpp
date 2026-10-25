#pragma once
#include "NativeMenuOwner.hpp"

namespace CondemnedMenuNative
{
    // Terms run before nondeleting/scalar destructors. Interface/manager dtors
    // do NOT replace a queued Term: their machine bodies do not call Term.
    enum class LifeOp : uint8_t
    {
        ShellTerm, InterfaceTerm, ManagerTerm, ScreenTerm, RemoveAll,
        ShellDtor, InterfaceScalar, InterfaceDtor, ManagerScalar, ManagerDtor,
        ScreenScalar, ScreenDtor, TextScalar
    };
    struct DeleteJob { LifeOp operation{}; void* receiver{}; uint32_t flags{}; };
    class Lifecycle
    {
        std::array<DeleteJob, 64> pending{};
        size_t count{};
        std::optional<DeleteJob> authorized;
        bool cleaning{};
        static bool TermCovers(LifeOp parent, LifeOp child)
        {
            if (parent == LifeOp::ShellTerm)
                return child == LifeOp::InterfaceTerm || TermCovers(LifeOp::InterfaceTerm, child);
            if (parent == LifeOp::InterfaceTerm)
                return child == LifeOp::ManagerTerm || TermCovers(LifeOp::ManagerTerm, child);
            if (parent == LifeOp::ManagerTerm)
                return child == LifeOp::ScreenTerm || child == LifeOp::ScreenScalar ||
                    child == LifeOp::ScreenDtor || child == LifeOp::RemoveAll;
            return parent == LifeOp::ScreenTerm && child == LifeOp::RemoveAll;
        }
        static bool Covers(LifeOp parent, LifeOp child)
        {
            if (parent == child || TermCovers(parent, child)) return true;
            if (parent == LifeOp::ShellDtor)
                return child == LifeOp::InterfaceTerm || TermCovers(LifeOp::InterfaceTerm, child) ||
                    child == LifeOp::InterfaceDtor || child == LifeOp::ManagerDtor;
            if (parent == LifeOp::InterfaceScalar || parent == LifeOp::InterfaceDtor)
                return child == LifeOp::InterfaceDtor || child == LifeOp::ManagerDtor;
            if (parent == LifeOp::ManagerScalar) return child == LifeOp::ManagerDtor;
            if (parent == LifeOp::ScreenScalar) return child == LifeOp::ScreenDtor || child == LifeOp::RemoveAll;
            return parent == LifeOp::ScreenDtor && child == LifeOp::RemoveAll;
        }
        // Pending physical destruction is broader than authorized child calls:
        // an enclosing destructor makes later member scalar deletion stale even
        // when it invokes the member's nondeleting destructor, not its scalar.
        static bool Supersedes(LifeOp parent, LifeOp child)
        {
            if (Covers(parent, child)) return true;
            if (parent == LifeOp::ShellDtor)
                return child == LifeOp::InterfaceScalar || child == LifeOp::ManagerScalar;
            return (parent == LifeOp::InterfaceScalar || parent == LifeOp::InterfaceDtor) &&
                child == LifeOp::ManagerScalar;
        }
        static unsigned Rank(LifeOp operation)
        {
            // Preserve required Terms first. Destructors that do not own screen
            // deletion must wait requested child destruction before member free.
            if (operation <= LifeOp::RemoveAll) return static_cast<unsigned>(operation);
            if (operation == LifeOp::ScreenScalar || operation == LifeOp::ScreenDtor) return 5;
            if (operation == LifeOp::ManagerScalar || operation == LifeOp::ManagerDtor) return 6;
            if (operation == LifeOp::InterfaceScalar || operation == LifeOp::InterfaceDtor) return 7;
            return 8;
        }
    public:
        static bool Related(const Owner& source, LifeOp operation, void* object)
        {
            switch (operation)
            {
                case LifeOp::ShellTerm: case LifeOp::ShellDtor:
                    return object == reinterpret_cast<uint8_t*>(source.ready.view.interfaceManager) - 8;
                case LifeOp::InterfaceTerm: case LifeOp::InterfaceDtor: case LifeOp::InterfaceScalar:
                    return object == source.ready.view.interfaceManager;
                case LifeOp::ManagerTerm: case LifeOp::ManagerDtor: case LifeOp::ManagerScalar:
                    return object == source.ready.view.manager;
                case LifeOp::TextScalar: return source.OwnedNode(object);
                default: return object == source.ready.view.screen;
            }
        }
        bool Authorized(const Owner& source, LifeOp operation, void* object) const
        {
            if (!Related(source, operation, object)) return false;
            if (cleaning && operation == LifeOp::TextScalar) return true;
            return authorized && authorized->operation != operation && Covers(authorized->operation, operation);
        }
        bool Executing(DeleteJob job) const
        {
            return authorized && authorized->operation == job.operation && authorized->receiver == job.receiver;
        }
        bool Queue(Owner& source, DeleteJob job)
        {
            if (!Related(source, job.operation, job.receiver)) return false;
            // Logical cancellation precedes source loss. The queue is fixed-size;
            // no allocation or engine API occurs in a lifecycle callback.
            source.Cancel();
            if (job.operation == LifeOp::TextScalar) return true;
            // Owned text requests retire the whole pool; Dispose deletes each
            // exact owned text once before any parent/provider shutdown.
            for (size_t i = 0; i < count; ++i)
            {
                if (pending[i].operation == job.operation && pending[i].receiver == job.receiver)
                { pending[i].flags |= job.flags; return true; }
                if (Supersedes(pending[i].operation, job.operation)) return true;
            }
            for (size_t i = 0; i < count; )
            {
                if (Supersedes(job.operation, pending[i].operation)) pending[i] = pending[--count];
                else ++i;
            }
            // At most12 non-text operations exist for this singleton owner.
            // Text operations never occupy slots, so64 cannot fill normally.
            if (count == pending.size()) return false;
            pending[count++] = job;
            return true;
        }
        bool Empty() const { return count == 0; }
        template<class Cleanup> void CleanOwned(Cleanup cleanup)
        {
            const auto previous = cleaning;
            cleaning = true;
            try { cleanup(); }
            catch (...) { cleaning = previous; throw; }
            cleaning = previous;
        }
        template<class Dispose, class Original> bool Collect(Owner& source, Dispose dispose, Original original)
        {
            // Caller is AFTER original wholeUpdate with ALL native scopes zero.
            // This authorization covers only exact owned text cleanup, not a
            // reentrant higher owner operation triggered by a native destructor.
            cleaning = true;
            bool detached{};
            try { detached = dispose(count != 0); }
            catch (...) { cleaning = false; throw; }
            cleaning = false;
            if (!detached) return false; // retain jobs/storage until exact detach
            while (count)
            {
                size_t first = 0;
                for (size_t i = 1; i < count; ++i)
                    if (Rank(pending[i].operation) < Rank(pending[first].operation)) first = i;
                const auto job = pending[first];
                pending[first] = pending[--count];
                // Remove parent-covered pending descendants before native entry.
                for (size_t i = 0; i < count; )
                {
                    if (Supersedes(job.operation, pending[i].operation)) pending[i] = pending[--count];
                    else ++i;
                }
                authorized = job;
                try { original(job); }
                catch (...) { authorized.reset(); throw; }
                authorized.reset();
            }
            (void)source;
            return true;
        }
    };
}

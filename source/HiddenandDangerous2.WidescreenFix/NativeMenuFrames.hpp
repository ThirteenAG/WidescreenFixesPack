#pragma once
#include "NativeMenuABI.hpp"
#include "NativeMenuContracts.hpp"
#include "gameref.hpp"
#include <memory>
#include <unordered_set>
#include <vector>

namespace HD2Menu
{
    class FrameLease
    {
    public:
        FrameLease() = default;
        FrameLease(const FrameLease&) = delete;
        FrameLease& operator=(const FrameLease&) = delete;
        ~FrameLease() { Reset(); }

        bool Acquire(void* value, uintptr_t module, uintptr_t table)
        {
            Reset();
            if (!value || !module || !Contracts::Readable(value, 250)
                || *reinterpret_cast<uintptr_t*>(value) != module + table - 0x10000000)
                return false;
            auto references = reinterpret_cast<uint16_t*>(static_cast<uint8_t*>(value) + 228);
            if (!*references || *references >= INT16_MAX) return false;
            // Common I3D_frame ownership, including Scene13: factory84 sets
            // WORD228=1; native Sector124 increments it before registration;
            // Release0 decrements it and retires only at zero. Not a weak link.
            ProtectedGameRef<uint16_t>{ references } = static_cast<uint16_t>(*references + 1);
            frame = value;
            return true;
        }

        void Reset()
        {
            if (auto value = std::exchange(frame, nullptr)) ABI::ReleaseFrame(value);
        }
        void* Get() const { return frame; }
    private:
        void* frame = nullptr;
    };

    // Native parents link children without retaining them. This separate pool
    // owns every clone and must outlive the controls which borrow those frames.
    // Release controls before their backing frames during host teardown.
    class FrameTree
    {
    public:
        FrameTree(uintptr_t module, void* driver) : module(module), driver(driver) {}
        FrameTree(const FrameTree&) = delete;
        FrameTree& operator=(const FrameTree&) = delete;
        FrameTree(FrameTree&&) = delete;
        FrameTree& operator=(FrameTree&&) = delete;
        ~FrameTree() { Reset(); }

        void* Clone(void* source)
        {
            // One transaction per pool. A failed transaction never leaves an
            // unattached clone registered in a native scene or parent.
            if (!frames.empty() || !module || !driver || !ABI::ready)
                return nullptr;
            try
            {
                // Reserve before a native allocation can become untracked.
                frames.reserve(64);
                std::unordered_set<void*> visited;
                visited.reserve(64);
                auto result = Copy(source, nullptr, visited, 0);
                if (result)
                    return result;
            }
            catch (...)
            {
                // A recursive visited-set allocation can fail after native
                // frames were created. Roll those owned refs back as well.
            }
            Reset();
            return nullptr;
        }

        void Detach()
        {
            // Detach before the native manager destroys its asset scene. Link
            // removal is safe while all pool frames retain their initial refs.
            for (auto i = frames.rbegin(); i != frames.rend(); ++i)
                ABI::LinkFrame(*i, nullptr);
        }

        bool Register(void* sector)
        {
            // The host owns this sector's initial reference until after this
            // pool is reset. Native slot124 retains each registered frame;
            // slot128 removes the exact registration and releases that ref.
            if (!sector || registration || !Table(sector, 0x1009BA50))
                return false;
            for (auto frame : frames)
                if (Read<void*>(frame, 256))
                    return false;
            registration = sector;
            for (auto frame : frames)
            {
                ABI::Virtual<int32_t(__stdcall*)(void*, void*)>(sector, 124)(sector, frame);
                // The return is allocator-derived, not semantic success.
                if (Read<void*>(frame, 256) != sector)
                {
                    Unregister();
                    return false;
                }
            }
            return true;
        }

        void Unregister()
        {
            if (!registration)
                return;
            for (auto i = frames.rbegin(); i != frames.rend(); ++i)
                if (Read<void*>(*i, 256) == registration)
                    ABI::Virtual<int32_t(__stdcall*)(void*, void*)>(registration, 128)(registration, *i);
            registration = nullptr;
        }

        void Reset()
        {
            Unregister();
            Detach();
            for (auto i = frames.rbegin(); i != frames.rend(); ++i)
                ABI::ReleaseFrame(*i);
            frames.clear();
        }

    private:
        template<class T> static T Read(void* object, size_t offset)
        {
            return *reinterpret_cast<const T*>(static_cast<const uint8_t*>(object) + offset);
        }

        bool Table(void* frame, uintptr_t original) const
        {
            return Read<uintptr_t>(frame, 0) == module + original - 0x10000000;
        }

        bool Kind(void* source, uint32_t& type, uint32_t& fourcc) const
        {
            if (!Contracts::Readable(source, 288)) return false;
            // The native frame kind is WORD+248; WORD+250 is a visual index,
            // whereas CreateVisual requires the independently verified FOURCC.
            type = Read<uint16_t>(source, 248);
            fourcc = 0;
            if (type == 5)
                return Contracts::Readable(source, 600) && Table(source, 0x1009BC20) && Read<uint32_t>(source, 596) == 0;
            if (type == 6)
                return Table(source, 0x1009BBA8);
            if (type != 1)
                return false;
            if (Table(source, 0x1009C7C8)) fourcc = 0x5F4A424F; // OBJ_
            else if (Table(source, 0x1009C6D8)) fourcc = 0x50414D4C; // LMAP
            else if (Table(source, 0x1009C5F8)) fourcc = 0x4D474E53; // SNGM
            else if (Table(source, 0x1009C440)) fourcc = 0x44524242; // BBRD
            else return false;
            // Each accepted SetMesh retains the mesh; Copy does not clone the
            // linked child tree. Unsupported animated/visual classes fail closed.
            return true;
        }

        void* Copy(void* source, void* parent, std::unordered_set<void*>& visited, size_t depth)
        {
            if (!source || depth > 8 || visited.size() >= 64 || !visited.insert(source).second)
                return nullptr;
            uint32_t type = 0, fourcc = 0;
            if (!Kind(source, type, fourcc))
                return nullptr;
            auto destination = type == 1 ? ABI::CreateVisual(driver, fourcc) : ABI::CreateFrame(driver, type);
            if (!destination)
                return nullptr;
            frames.push_back(destination);
            if (Read<uintptr_t>(destination, 0) != Read<uintptr_t>(source, 0)
                || ABI::CopyFrame(destination, source) < 0)
                return nullptr;
            if (parent && ABI::LinkFrame(destination, parent) < 0)
                return nullptr;
            // Model5's native Copy omits children, so clone them explicitly.
            // Parent/next links are checked before each recursive allocation.
            for (auto child = Read<void*>(source, 268); child; child = Read<void*>(child, 272))
            {
                if (!Contracts::Readable(child, 288) || Read<void*>(child, 264) != source || !Copy(child, destination, visited, depth + 1))
                    return nullptr;
            }
            return destination;
        }

        uintptr_t module;
        void* driver;
        void* registration = nullptr;
        std::vector<void*> frames;
    };
}

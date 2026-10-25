#pragma once
#include <cmath>
#include <cstdint>
#include <new>
#include <unordered_map>

namespace NFSPSHud
{
    struct Identity
    {
        const void* frame;
        uint32_t guid;
        bool operator==(const Identity&) const = default;
    };

    class Positions
    {
        struct Placement
        {
            Identity identity;
            float originalX;
            float lastX;
            bool right;
            bool refused;
        };
        std::unordered_map<void*, Placement> entries;

        static bool Same(float first, float second)
        {
            return std::abs(first - second) <= 0.001f;
        }

    public:
        void Retire(void* object)
        {
            entries.erase(object);
        }

        template<class Read, class Write>
        bool Apply(void* object, Identity identity, bool right, float offset, Read&& read, Write&& write)
        {
            if (!object || !identity.frame || !std::isfinite(offset))
                return false;
            float x{}, y{};
            if (!read(object, x, y) || !std::isfinite(x) || !std::isfinite(y))
                return false;
            auto found = entries.find(object);
            bool inserted = found == entries.end();
            if (inserted)
            {
                try
                {
                    found = entries.emplace(object, Placement{ identity, x, x, right, false }).first;
                }
                catch (const std::bad_alloc&)
                {
                    return false;
                }
            }
            auto& placement = found->second;
            if (!inserted && (placement.identity.guid != identity.guid || placement.right != right ||
                (!Same(x, placement.originalX) && !Same(x, placement.lastX))))
                placement.refused = true;
            if (placement.refused)
                return false;
            placement.identity = identity;
            auto target = placement.originalX + (right ? offset : -offset);
            if (!std::isfinite(target))
                return false;
            if (!Same(x, target))
                write(object, target, y);
            placement.lastX = target;
            return true;
        }
    };
}

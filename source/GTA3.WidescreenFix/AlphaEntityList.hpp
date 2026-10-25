#pragma once

#include <cstddef>
#include <memory>
#include <new>
#include <vector>

namespace AlphaEntityList
{
    struct Info
    {
        void* entity;
        float distance;
    };

    struct Node
    {
        Info item;
        Node* prev;
        Node* next;
    };

    struct List
    {
        Node head, tail, freeHead, freeTail;
        Node* links;
    };

    static_assert(sizeof(Node) == 16 && sizeof(List) == 68);
    static_assert(offsetof(List, freeHead) == 32 && offsetof(List, links) == 64);

    class Growth
    {
        std::vector<std::unique_ptr<Node[]>> chunks;

    public:
        // Call only after the native initializer has detached the old nodes.
        void Reset() noexcept { chunks.clear(); }

        bool EnsureFreeNode(List& list) noexcept
        {
            if (list.freeHead.next != &list.freeTail)
                return true;

            // Never relocate the game's allocation: the active sorted list
            // contains pointers to its nodes, and Shutdown owns that allocation.
            constexpr size_t count = 256;
            try
            {
                auto nodes = std::make_unique<Node[]>(count);
                auto first = nodes.get();
                chunks.push_back(std::move(nodes));
                for (size_t i = 0; i < count; ++i)
                {
                    first[i].prev = i ? &first[i - 1] : &list.freeHead;
                    first[i].next = i + 1 < count ? &first[i + 1] : &list.freeTail;
                }
                list.freeHead.next = first;
                list.freeTail.prev = &first[count - 1];
                return true;
            }
            catch (const std::bad_alloc&)
            {
                // Preserve the native failure path if memory is exhausted.
                return false;
            }
        }
    };
}

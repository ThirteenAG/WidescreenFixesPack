#pragma once
#include <array>
#include <cstdint>
#include "../Lights.hpp"
#include <cassert>

// Shared fixed-capacity storage for embedded distant-vehicle geometry.
namespace DistantCarMesh
{
    // Fixed storage keeps mesh construction and render batches off the game's
    // heap. Capacity is checked in the standalone tests as well as debug builds.
    template<class T, size_t Capacity> struct Buffer
    {
        std::array<T, Capacity> values;
        size_t count = 0;
        void push_back(const T& value)
        {
            assert(count < Capacity);
            if (count < Capacity) values[count++] = value;
        }
        T* append(size_t amount)
        {
            assert(amount <= Capacity - count);
            if (amount > Capacity - count) return nullptr;
            T* result = data() + count;
            count += amount;
            return result;
        }
        void clear() { count = 0; }
        size_t size() const { return count; }
        bool empty() const { return count == 0; }
        T* data() { return values.data(); }
        const T* data() const { return values.data(); }
        T* begin() { return data(); }
        T* end() { return data() + count; }
        const T* begin() const { return values.data(); }
        const T* end() const { return values.data() + count; }
        T& operator[](size_t index) { return values[index]; }
        const T& operator[](size_t index) const { return values[index]; }
    };

    enum Material { Paint, Glass, Rubber, Chrome, Headlight, Taillight, BoatHull };
    struct Vertex { console::Vector3 position, normal; Material material; uint32_t color = 0xFFFFFFu; };
    struct Geometry
    {
        Buffer<Vertex, 1024> vertices;
        Buffer<short, 1536> indices;
    };
}

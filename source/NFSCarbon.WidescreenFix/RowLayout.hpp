#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <unordered_map>
#include <vector>

namespace NFSCRowLayout
{
    struct Pose
    {
        float x{};
        float scale{ 1.0f };
    };

    struct Bounds
    {
        float left{ std::numeric_limits<float>::max() };
        float right{ std::numeric_limits<float>::lowest() };
    };

    class Layout
    {
        struct Change
        {
            float* address;
            uint8_t* object;
            float original;
            float applied;
        };
        std::vector<Change> arrows;
        std::unordered_map<void*, std::vector<Change>> rows;

        static float* State(uint8_t* object)
        {
            if (!object || *reinterpret_cast<uint32_t*>(object + 0x30) < 68)
                return nullptr;
            return *reinterpret_cast<float**>(object + 0x2C);
        }

        static bool Axis(float* state)
        {
            if (!state || !std::isfinite(state[7]) || !std::isfinite(state[14]) || state[14] <= 0.0f)
                return false;
            for (size_t i : { 4u, 5u, 6u, 10u, 11u, 12u })
            {
                if (!std::isfinite(state[i]) || std::abs(state[i]) > 0.001f)
                    return false;
            }
            return std::isfinite(state[13]) && std::abs(state[13] - 1.0f) <= 0.001f;
        }

        static bool Parent(uint8_t* package, uint8_t* target, Pose& result)
        {
            size_t budget{ 4096 };
            auto visit = [&](auto&& self, uint8_t* head, Pose parent, size_t depth) -> bool
            {
                for (auto object = head; object && budget && depth < 32;
                    object = *reinterpret_cast<uint8_t**>(object + 4))
                {
                    --budget;
                    if (object == target)
                    {
                        result = parent;
                        return std::isfinite(parent.x) && std::isfinite(parent.scale) && parent.scale > 0.0f;
                    }
                    if (*reinterpret_cast<uint32_t*>(object + 0x18) == 5)
                    {
                        auto state = State(object);
                        if (Axis(state) && self(self, *reinterpret_cast<uint8_t**>(object + 0x64),
                            { parent.x + state[7] * parent.scale, parent.scale * state[14] }, depth + 1))
                            return true;
                    }
                }
                return false;
            };
            return package && target && visit(visit, *reinterpret_cast<uint8_t**>(package + 0x4C), {}, 0);
        }

        static void Restore(std::vector<Change>& changes)
        {
            for (auto& change : changes)
            {
                // A native animation that subsequently changed the field keeps its new value.
                if (*change.address == change.applied)
                {
                    *change.address = change.original;
                    if (change.object)
                        *reinterpret_cast<uint32_t*>(change.object + 0x1C) |= 0x2400000;
                }
            }
            changes.clear();
        }

        static void Set(std::vector<Change>& changes, float* address, uint8_t* object, float value)
        {
            if (*address == value)
                return;
            changes.push_back({ address, object, *address, value });
            *address = value;
            if (object)
                *reinterpret_cast<uint32_t*>(object + 0x1C) |= 0x2400000;
        }

        static bool TextBounds(uint8_t* package, uint8_t* group,
            const std::function<float(uint8_t*)>& measure, Bounds& bounds)
        {
            size_t budget{ 4096 };
            bool measured{};
            bool valid{ true };
            auto visit = [&](auto&& self, uint8_t* object, size_t depth) -> void
            {
                if (!object || !budget || depth >= 32)
                {
                    valid = false;
                    return;
                }
                --budget;
                auto type = *reinterpret_cast<uint32_t*>(object + 0x18);
                if (type == 2)
                {
                    Pose parent;
                    auto state = State(object);
                    auto width = measure(object);
                    if (!Axis(state) || !Parent(package, object, parent) || !std::isfinite(width))
                    {
                        valid = false;
                        return;
                    }
                    width *= parent.scale;
                    auto justify = *reinterpret_cast<uint32_t*>(object + 0x6C) & 3;
                    auto left = parent.x + state[7] * parent.scale - width * (justify == 1 ? 0.5f : justify == 2 ? 1.0f : 0.0f);
                    bounds.left = std::min(bounds.left, left);
                    bounds.right = std::max(bounds.right, left + width);
                    measured = true;
                }
                else if (type == 5)
                {
                    for (auto child = *reinterpret_cast<uint8_t**>(object + 0x64); child && budget;
                        child = *reinterpret_cast<uint8_t**>(child + 4))
                        self(self, child, depth + 1);
                }
            };
            visit(visit, group, 0);
            return valid && measured;
        }

        static float Radius(uint8_t* arrow, float parentScale)
        {
            auto state = State(arrow);
            if (!state || !std::isfinite(state[14]) || state[14] <= 0.0f ||
                *reinterpret_cast<uint32_t*>(arrow + 0x18) != 5)
                return std::numeric_limits<float>::quiet_NaN();
            float radius{};
            size_t budget{ 64 };
            for (auto child = *reinterpret_cast<uint8_t**>(arrow + 0x64); child && budget--;
                child = *reinterpret_cast<uint8_t**>(child + 4))
            {
                auto image = State(child);
                if (*reinterpret_cast<uint32_t*>(child + 0x18) != 1 || !image ||
                    !std::isfinite(image[7]) || !std::isfinite(image[8]) ||
                    !std::isfinite(image[14]) || !std::isfinite(image[15]))
                    return std::numeric_limits<float>::quiet_NaN();
                radius = std::max(radius, std::hypot(image[7], image[8]) + 0.5f * std::hypot(image[14], image[15]));
            }
            return radius > 0.0f ? radius * std::abs(state[14]) * parentScale : std::numeric_limits<float>::quiet_NaN();
        }

    public:
        void RestoreArrows()
        {
            Restore(arrows);
        }

        void RestoreAll()
        {
            RestoreArrows();
            for (auto& [widget, changes] : rows)
                Restore(changes);
            rows.clear();
        }

        static float Capacity(uint8_t* package, uint8_t* widget, uint8_t* text, bool slider)
        {
            Pose parent;
            auto state = State(text);
            if (!Axis(state) || !Parent(package, text, parent))
                return std::numeric_limits<float>::quiet_NaN();
            if (slider)
            {
                auto base = *reinterpret_cast<uint8_t**>(widget + 48);
                auto baseState = State(base);
                Pose baseParent;
                if (!baseState || !Parent(package, base, baseParent) || !std::isfinite(baseState[7]) ||
                    !std::isfinite(baseState[14]) || baseState[14] <= 0.0f ||
                    (*reinterpret_cast<uint32_t*>(text + 0x6C) & 3) != 2)
                    return std::numeric_limits<float>::quiet_NaN();
                auto left = baseParent.x + (baseState[7] - baseState[14] * 0.5f) * baseParent.scale;
                auto right = parent.x + state[7] * parent.scale;
                return (right - left - 64.0f * baseParent.scale - 8.0f) / parent.scale;
            }
            auto left = *reinterpret_cast<uint8_t**>(widget + 40);
            auto right = *reinterpret_cast<uint8_t**>(widget + 44);
            Pose leftParent, rightParent;
            if (!Parent(package, left, leftParent) || !Parent(package, right, rightParent))
                return std::numeric_limits<float>::quiet_NaN();
            auto radius = Radius(left, leftParent.scale) + Radius(right, rightParent.scale);
            return (616.0f - 2.0f * radius - 16.0f) / parent.scale;
        }

        bool Place(uint8_t* package, uint8_t* widget, bool slider, bool selected,
            const std::function<float(uint8_t*)>& measure)
        {
            auto& changes = rows[widget];
            Restore(changes);
            if (selected)
                RestoreArrows();
            auto group = *reinterpret_cast<uint8_t**>(widget + (slider ? 64 : 28));
            Bounds value;
            if (!TextBounds(package, group, measure, value))
                return false;
            auto leftArrow = *reinterpret_cast<uint8_t**>(widget + 40);
            auto rightArrow = *reinterpret_cast<uint8_t**>(widget + 44);
            Pose leftParent, rightParent;
            auto leftState = State(leftArrow);
            auto rightState = State(rightArrow);
            float leftRadius{}, rightRadius{};
            if (!slider)
            {
                if (!leftState || !rightState || !std::isfinite(leftState[7]) || !std::isfinite(rightState[7]) ||
                    !Parent(package, leftArrow, leftParent) || !Parent(package, rightArrow, rightParent))
                    return false;
                leftRadius = Radius(leftArrow, leftParent.scale);
                rightRadius = Radius(rightArrow, rightParent.scale);
                if (!std::isfinite(leftRadius) || !std::isfinite(rightRadius))
                    return false;
            }
            auto minimum = -308.0f + 2.0f * leftRadius + (slider ? 0.0f : 8.0f);
            auto maximum = 308.0f - 2.0f * rightRadius - (slider ? 0.0f : 8.0f);
            auto width = value.right - value.left;
            if (!std::isfinite(width) || width > maximum - minimum)
                return false;
            auto shift = std::max(0.0f, minimum - value.left) + std::min(0.0f, maximum - value.right);
            Pose parent;
            auto state = State(group);
            float shiftedX{};
            if (shift)
            {
                if (!Axis(state) || !Parent(package, group, parent))
                    return false;
                shiftedX = state[7] + shift / parent.scale;
                value.left += shift;
                value.right += shift;
            }
            auto moveCaption = [&]()
            {
                if (shift)
                    Set(changes, state + 7, group, shiftedX);
            };
            if (slider)
            {
                auto base = *reinterpret_cast<uint8_t**>(widget + 48);
                auto baseState = State(base);
                if (!baseState || !Parent(package, base, parent) || !std::isfinite(baseState[7]) ||
                    !std::isfinite(baseState[14]) || baseState[14] <= 0.0f ||
                    !std::isfinite(*reinterpret_cast<float*>(widget + 176)) || *reinterpret_cast<float*>(widget + 176) <= 0.0f)
                    return false;
                auto left = parent.x + (baseState[7] - baseState[14] * 0.5f) * parent.scale;
                auto available = (value.left - 8.0f - left) / parent.scale;
                if (!std::isfinite(available) || available < 64.0f)
                    return false;
                auto size = std::min(baseState[14], available);
                moveCaption();
                Set(changes, baseState + 7, base, baseState[7] + (size - baseState[14]) * 0.5f);
                Set(changes, baseState + 14, base, size);
                Set(changes, reinterpret_cast<float*>(widget + 176), nullptr, size);
                return true;
            }
            if (!selected)
            {
                moveCaption();
                return true;
            }
            auto left = std::max(-308.0f + leftRadius,
                std::min(leftParent.x + leftState[7] * leftParent.scale, value.left - leftRadius - 8.0f));
            auto right = std::min(308.0f - rightRadius,
                std::max(rightParent.x + rightState[7] * rightParent.scale, value.right + rightRadius + 8.0f));
            if (left + leftRadius + 7.99f > value.left || right - rightRadius - 7.99f < value.right)
                return false;
            moveCaption();
            Set(arrows, leftState + 7, leftArrow, (left - leftParent.x) / leftParent.scale);
            Set(arrows, rightState + 7, rightArrow, (right - rightParent.x) / rightParent.scale);
            return true;
        }
    };
}

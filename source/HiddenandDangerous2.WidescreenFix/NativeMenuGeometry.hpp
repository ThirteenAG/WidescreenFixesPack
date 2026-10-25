#pragma once
#include "NativeMenuText.hpp"
#include <array>
#include <cmath>

namespace HD2Menu
{
    struct Geometry
    {
        std::array<void*, 6> frames{};
        std::array<ABI::Vector, 6> original{};
        std::array<float, 6> centers{};
        uint16_t screenHeight = 0;
        bool changed = false;
        float direction = 1.0f;

        static bool Position(void* frame, ABI::Vector& result)
        {
            if (!Contracts::Readable(frame, 288)) return false;
            auto update = reinterpret_cast<void(__thiscall*)(void*)>(Context::graphics + 0x1E390);
            update(frame);
            return Context::Read(frame, 112, result) && std::isfinite(result.x)
                && std::isfinite(result.y) && std::isfinite(result.z);
        }

        static bool Bounds(void* button)
        {
            void* frame = nullptr;
            void* text = nullptr;
            void* scene = nullptr;
            if (!Context::Read(button, 112, frame) || !Context::Read(button, 108, text)
                || !Context::Read(button, 104, scene) || !frame || !text || !scene) return false;
            ABI::Virtual<int32_t(__stdcall*)(void*)>(frame, 36)(frame);
            ABI::Function<void(__thiscall*)(void*, void*, void*)>(0x644A80)(text, scene, frame);
            return true;
        }

        static void Move(void* frame, const ABI::Vector& position)
        {
            ABI::PositionFrame(frame, position);
            // SetWorldPos dirties this root. Propagate that change through its
            // visuals before reading their cached world bounds for text layout.
            ABI::Virtual<int32_t(__stdcall*)(void*)>(frame, 36)(frame);
        }

        bool Capture(const Context& context, const std::function<bool()>& current)
        {
            float first = 0, last = 0, previous = 0, largest = 0;
            for (size_t i = 0; i < frames.size(); ++i)
            {
                if (!current() || !Context::Read(context.buttons[i], 80, frames[i])
                    || !Position(frames[i], original[i]) || !current()) return false;
                TextProbe probe;
                if (!probe.Capture(context.buttons[i], current)) return false;
                float top = 0, bottom = 0, box = 0;
                if (!Context::Read(probe.text, 12, top) || !Context::Read(probe.text, 20, bottom)
                    || !Context::Read(probe.text, 28, box) || !std::isfinite(top) || !std::isfinite(bottom)) return false;
                auto center = (top + bottom) * 0.5f;
                uint16_t height = 0;
                if (!Context::Read(probe.text, 6, height) || height < 240
                    || (i && height != screenHeight)) return false;
                screenHeight = height;
                centers[i] = center;
                if (i == 1) direction = center >= previous ? 1.0f : -1.0f;
                if (i && (center - previous) * direction < box + 4) return false;
                if (!i) first = center;
                previous = last = center;
                largest = (std::max)(largest, box);
            }
            // Keep all stock rows fixed. The native page has a large spacer
            // before Back; add the category there at the ordinary row spacing.
            const auto gap = std::abs(centers[4] - first) / 4.0f;
            return current() && gap >= largest + 4.0f
                && (last - centers[4]) * direction >= gap + largest + 4.0f;
        }

        ABI::Vector Location(size_t index) const
        {
            const auto source = index == 6 ? size_t(5) : index == 5 ? size_t(0) : index;
            auto result = original[source];
            const auto target = index == 5 ? centers[4] + (centers[4] - centers[0]) / 4.0f : centers[source];
            // Native text bounds project world Y by -screenHeight * 4/3.
            // The source meshes contain their row offsets; their frame origins
            // need not form a vertical line. Preserve each frame's X and Z.
            result.y -= (target - centers[source]) / (static_cast<float>(screenHeight) * (4.0f / 3.0f));
            return result;
        }

        bool Apply(const Context& context, void* entry, const std::function<bool()>& current)
        {
            changed = true;
            for (size_t i = 0; i < frames.size(); ++i)
            {
                if (!current()) return false;
                Move(frames[i], Location(i == 5 ? 6 : i));
                if (!current() || !Bounds(context.buttons[i]) || !current()) return false;
            }
            void* frame = nullptr;
            if (!Context::Read(entry, 80, frame) || !current()) return false;
            Move(frame, Location(5));
            return current() && Bounds(entry) && current();
        }

        void Restore(const Context& context)
        {
            if (!changed) return;
            for (size_t i = 0; i < frames.size(); ++i)
            {
                // The host retains the source asset/group until rollback.
                // Native controls may already be gone; their frames remain
                // retained by the source sector, so do not read dead widgets.
                Move(frames[i], original[i]);
                if (context.Alive()) Bounds(context.buttons[i]);
            }
            changed = false;
        }
    };
}

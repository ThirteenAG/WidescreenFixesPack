#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace DeerHud
{
    enum class Anchor { Left, Centre, Right };

    struct Matrix
    {
        float right[3];
        uint32_t flags;
        float up[3];
        uint32_t pad1;
        float at[3];
        uint32_t pad2;
        float position[3];
        uint32_t pad3;
    };
    static_assert(sizeof(Matrix) == 64);

    struct Transform
    {
        float scale = 1.0f;
        float shift = 0.0f;

        static bool Build(float aspect, float constraint, float span, Anchor anchor, Transform& result)
        {
            if (!std::isfinite(aspect) || !std::isfinite(constraint) || !std::isfinite(span) ||
                aspect <= 0.0f || constraint <= 0.0f || span <= 0.0f)
                return false;
            const float box = std::clamp(constraint, std::min(aspect, 4.0f / 3.0f), aspect);
            const float scale = (4.0f / 3.0f) / aspect;
            float origin = (1.0f - scale) * 0.5f;
            if (anchor == Anchor::Left)
                origin = (1.0f - box / aspect) * 0.5f;
            else if (anchor == Anchor::Right)
                origin = (1.0f + box / aspect) * 0.5f - scale;
            result = { scale, span * origin };
            return true;
        }
    };

    class MatrixScope
    {
        Matrix* matrix = nullptr;
        Matrix saved{};
        int* world = nullptr;
        int* derived = nullptr;

    public:
        MatrixScope(Matrix* target, const Transform& transform, int* worldCache, int* derivedCache)
        {
            if (!target || !worldCache || !derivedCache ||
                (transform.scale == 1.0f && transform.shift == 0.0f))
                return;
            matrix = target;
            world = worldCache;
            derived = derivedCache;
            std::memcpy(&saved, matrix, sizeof(saved));
            matrix->right[0] *= transform.scale;
            matrix->up[0] *= transform.scale;
            matrix->at[0] *= transform.scale;
            matrix->position[0] = matrix->position[0] * transform.scale + transform.shift;
            matrix->flags &= ~0x20003u;
            *world = 0;
            *derived = 0;
        }

        MatrixScope(const MatrixScope&) = delete;
        MatrixScope& operator=(const MatrixScope&) = delete;

        ~MatrixScope()
        {
            if (!matrix)
                return;
            std::memcpy(matrix, &saved, sizeof(saved));
            *world = 0;
            *derived = 0;
        }
    };
}

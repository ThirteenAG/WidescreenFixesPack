module;
#include <cstdint>
#include "common.h"

export module Placeable;

export class CPlaceable
{
protected:
    CMatrix m_matrix;

public:
    virtual ~CPlaceable() = default;

    const CVector& GetPosition() const { return m_matrix.GetPosition(); }
    void SetPosition(float x, float y, float z) { auto& p = m_matrix.GetPosition(); p.x = x; p.y = y; p.z = z; }
    void SetPosition(const CVector& pos) { m_matrix.GetPosition() = pos; }

    CVector& GetRight() { return m_matrix.GetRight(); }
    CVector& GetForward() { return m_matrix.GetForward(); }
    CVector& GetUp() { return m_matrix.GetUp(); }
    CMatrix& GetMatrix() { return m_matrix; }

    bool IsWithinArea(float x1, float y1, float x2, float y2)
    {
        const auto& p = GetPosition();
        return p.x >= x1 && p.x <= x2 && p.y >= y1 && p.y <= y2;
    }
    bool IsWithinArea(float x1, float y1, float z1, float x2, float y2, float z2)
    {
        const auto& p = GetPosition();
        return p.x >= x1 && p.x <= x2 && p.y >= y1 && p.y <= y2 && p.z >= z1 && p.z <= z2;
    }
};
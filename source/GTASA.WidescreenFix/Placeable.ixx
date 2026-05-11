module;
#include <cstdint>
#include "rw.h"

export module Placeable;

export class CSimpleTransform
{
public:
    CVector m_translate;
    float m_heading;

public:
    void UpdateRwMatrix(RwMatrix* pMatrix) const
    {
        pMatrix->right.x = cos(m_heading);
        pMatrix->right.y = sin(m_heading);
        pMatrix->right.z = 0.0f;

        pMatrix->at.x = 0.0f;
        pMatrix->at.y = 0.0f;
        pMatrix->at.z = 1.0f;

        pMatrix->up.x = -sin(m_heading);
        pMatrix->up.y = cos(m_heading);
        pMatrix->up.z = 0.0f;

        pMatrix->pos.x = m_translate.x;
        pMatrix->pos.y = m_translate.y;
        pMatrix->pos.z = m_translate.z;

        pMatrix->flags &= ~(0x00000003 | 0x00020000);
    }

    void Invert(const CSimpleTransform& src)
    {
        m_translate.x = -(cos(src.m_heading) * src.m_translate.x - sin(src.m_heading) * src.m_translate.y);;
        m_translate.y = sin(src.m_heading) * src.m_translate.x - cos(m_heading) * src.m_translate.y;
        m_translate.z = -src.m_translate.z;
        m_heading = -src.m_heading;
    }
};

export class CPlaceable
{
private:
    CSimpleTransform m_transform;
    CMatrix* m_pCoords;

public:
    // Line up the VMTs
    virtual ~CPlaceable() {}

    inline CPlaceable() {}

    inline CVector& GetCoords()
    {
        return m_pCoords ? reinterpret_cast<CVector&>(m_pCoords->GetPosition()) : m_transform.m_translate;
    }
    inline CMatrix* GetMatrix()
    {
        return m_pCoords;
    }
    inline CSimpleTransform& GetTransform()
    {
        return m_transform;
    }
    inline float GetHeading()
    {
        return m_pCoords ? atan2(-m_pCoords->GetUp().x, m_pCoords->GetUp().y) : m_transform.m_heading;
    }

    inline void SetCoords(const CVector& pos)
    {
        if (m_pCoords) { m_pCoords->GetPosition().x = pos.x; m_pCoords->GetPosition().y = pos.y; m_pCoords->GetPosition().z = pos.z; }
        else m_transform.m_translate = pos;
    }

    void AllocateMatrix();
};
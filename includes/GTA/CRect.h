#pragma once
#include "CVector2D.h"

/* CRect class describes a rectangle.

	A(left;top)_____________________
	|                               |
	|                               |
	|                               |
	|_________________B(right;bottom)
	
*/

#pragma pack(push, 4)
class CRect
{
public:
	float m_fLeft;          // x1
	float m_fBottom;        // y1
	float m_fRight;         // x2
	float m_fTop;           // y2

	inline CRect() {}
	inline CRect(float a, float b, float c, float d)
		: m_fLeft(a), m_fBottom(b), m_fRight(c), m_fTop(d)
	{}
};
#pragma pack(pop)
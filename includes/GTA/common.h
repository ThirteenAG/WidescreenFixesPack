#include "stdafx.h"
#include "CDraw.h"
#include "CCamera.h"

#pragma pack(push, 1)
class CRGBA
{
public:
	union {
		unsigned int colorInt;
		struct {
			unsigned char red, green, blue, alpha;
		};
	};

	inline CRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255)
		: red(r), green(g), blue(b), alpha(a)
	{}
	inline CRGBA() {}
};
#pragma pack(pop)

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

struct RsGlobalType
{
	char *AppName;
	int MaximumWidth;
	int MaximumHeight;
	int frameLimit;
	int quit;
	int ps;
	/**/
};

struct RwV3d
{
	float x;
	float y;
	float z;
};

class CSprite2d;

class CVector;
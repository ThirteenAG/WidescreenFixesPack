#pragma once

#pragma pack(push, 1)
class CRGBA
{
public:
	union{
		unsigned int colorInt;
		struct{
			unsigned char red, green, blue, alpha;
		};
	};

	inline CRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255)
		: red(r), green(g), blue(b), alpha(a)
	{}
	inline CRGBA() {}
};
#pragma pack(pop)
#pragma once

class CCamera
{
public:
	char align01[0x6C];
	bool m_bWideScreenOn;
	char align02[0x8EA];

	static void DrawBordersForWideScreen();
};

void Hide1pxAABug();
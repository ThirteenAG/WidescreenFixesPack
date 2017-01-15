#pragma once

class CDraw
{
public:
	static float* pfScreenAspectRatio;
	static float* pfScreenFieldOfView;

	static void CalculateAspectRatio();
	static void SetFOV(float);
};
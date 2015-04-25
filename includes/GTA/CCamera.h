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

template<uintptr_t addr>
void DrawBordersForWideScreenHook()
{
	using func_hook = injector::function_hooker_thiscall<addr, void(void*)>;
	injector::make_static_hook<func_hook>([](func_hook::func_type DrawBordersForWideScreen, void* camera)
	{
		if (!*(char*)bWideScreen == 0)
			DrawBordersForWideScreen(camera);
	});
}
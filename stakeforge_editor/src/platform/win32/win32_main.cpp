
#include <sfg/core/engine.hpp>
#include <sfg/platform/process.hpp>

#include <cstdio>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR pCmdLine, _In_ int nCmdShow)
{
	AllocConsole();

	sfg::process::init();

	sfg::engine_t engine;
	engine.init({
		.fixed_framerate_ns		   = 16'666'667.0,
		.fixed_framerate_max_ticks = 4,
	});

	while (true)
	{
		engine.tick();
	}

	engine.uninit();

	sfg::process::uninit();
	FreeConsole();

	return 0;
}

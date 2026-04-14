
#include <sfg/engine/engine.hpp>
#include <sfg/platform/process.hpp>

#include <cstdio>
#include <cstring>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR pCmdLine, _In_ int nCmdShow)
{
	AllocConsole();

	FILE* stream = nullptr;
	freopen_s(&stream, "CONIN$", "r", stdin);
	freopen_s(&stream, "CONOUT$", "w", stdout);
	freopen_s(&stream, "CONOUT$", "w", stderr);

	sfg::process::init();

	sfg::engine_t				 engine;
	const sfg::engine_error_code result = engine.init({
		.fixed_framerate_ns		   = 16'666'667.0,
		.fixed_framerate_max_ticks = 4,
	});

	if (result != sfg::engine_error_code::none)
	{
		MessageBoxA(nullptr, "Engine init failed. Check the console.", "stakeforge_editor", MB_OK | MB_ICONERROR);
		sfg::process::uninit();
		FreeConsole();
		return static_cast<int>(result);
	}

	sfg::window_t window_config = {};
	window_config.pos			   = sfg::vec2i16_t(100, 100);
	window_config.size			   = sfg::vec2u16_t(800, 600);
	window_config.style			   = sfg::window_style::app_window;
	window_config.high_frequency_input = true;
	strcpy_s(window_config.title, "sfg");

	const sfg::engine_id_t main_window = engine.create_window(window_config);
	sfg::renderer_t&	   renderer	   = engine.get_renderer();
	renderer.create_surface(engine.get_window(main_window).size, sfg::format_t::b8g8r8a8_srgb, &engine.get_window_runtime(main_window));

	engine.start_render();
	while (!engine.get_window_runtime(main_window).close_requested)
	{
		engine.tick();
	}
	engine.end_render();
	engine.uninit();

	sfg::process::uninit();
	FreeConsole();

	return 0;
}

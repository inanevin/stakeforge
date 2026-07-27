#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <sfg/stakeforge_api.hpp>
#include <sfg/runtime/engine/engine_runtime.hpp>
#include <sfg/runtime/ui/glyph_atlas.hpp>

#include <Windows.h>

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR pCmdLine, _In_ int nCmdShow)
{
	sfg::engine_runtime_t& runtime = sfg::engine_runtime_t::get();
	runtime.init_globals();

	if (!runtime.init_backend())
	{
		runtime.uninit_globals();
		return static_cast<int>(sfg_api_result_backend_failed);
	}

	const sfg_api_result_t result = sfg_engine_init();
	if (result != sfg_api_result_success)
	{
		runtime.uninit_globals();
		runtime.uninit_backend();
		return static_cast<int>(result);
	}

	sfg_engine_uninit();

	runtime.uninit_globals();
	runtime.uninit_backend();
	return 0;
}

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <sfg/stakeforge_api.hpp>
#include <sfg/runtime/engine/engine_runtime.hpp>
#include <sfg/runtime/ui/glyph_atlas.hpp>

#include <Windows.h>

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR pCmdLine, _In_ int nCmdShow)
{
	sfg::engine_runtime_t::init_globals();

	if (!sfg::engine_runtime_t::init_backend({}))
	{
		sfg::engine_runtime_t::uninit_globals();
		return static_cast<int>(sfg_api_result_backend_failed);
	}

	const sfg_api_result_t result = sfg_engine_init();
	if (result != sfg_api_result_success)
	{
		sfg::engine_runtime_t::uninit_globals();
		sfg::engine_runtime_t::uninit_backend();
		return static_cast<int>(result);
	}

	sfg_engine_uninit();

	sfg::engine_runtime_t::uninit_globals();
	sfg::engine_runtime_t::uninit_backend();
	return 0;
}

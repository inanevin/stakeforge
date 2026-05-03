#include <sfg/stakeforge_api.hpp>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR pCmdLine, _In_ int nCmdShow)
{
	const sfg::engine_config_t config = {};
	const sfg_api_result_t	   result = sfg_engine_init(config);
	if (result != sfg_api_result_success)
		return static_cast<int>(result);

	sfg::world_handle_t world = {};
	if (sfg_world_create(&world) != sfg_api_result_success)
	{
		sfg_engine_uninit();
		return static_cast<int>(sfg_api_result_engine_init_failed);
	}

	sfg_engine_frame();
	sfg_world_destroy(world);
	sfg_engine_uninit();
	return 0;
}

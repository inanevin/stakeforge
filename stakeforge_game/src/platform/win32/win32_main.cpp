#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "game.hpp"

#include <sfg/platform/process.hpp>

#include <Windows.h>

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR pCmdLine, _In_ int nCmdShow)
{
	sfg::game_t game = {};

	if (!game.init())
	{
		sfg::process::message_box("Stakeforge Game", game.get_init_failure_reason());
		return 1;
	}

	game.run();
	game.uninit();

	return 0;
}

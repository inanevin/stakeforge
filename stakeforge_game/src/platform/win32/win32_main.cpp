#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "game.hpp"

#include <sfg/core/platform/process.hpp>
#include <sfg/core/io/error_reporting.hpp>

#include <Windows.h>

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR pCmdLine, _In_ int nCmdShow)
{
	sfg::error_reporting_t& reporting = sfg::error_reporting_t::get();

	if (!reporting.init("Game"))
	{
		sfg::process::message_box("Stakeforge Game", "Failed to initialize error reporting.");
		return 1;
	}

	try
	{
		sfg::game_t game = {};

		if (!game.init())
		{
			sfg::process::message_box("Stakeforge Game", game.get_init_failure_reason());
			reporting.uninit();
			return 1;
		}

		game.run();
		game.uninit();
		reporting.uninit();
		return 0;
	}
	catch (const std::exception& error)
	{
		reporting.report_crash(error.what());
	}
	catch (...)
	{
		reporting.report_crash("Unhandled non-standard C++ exception.");
	}
}

/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.

*/
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <sfg/editor/editor_app.hpp>
#include <sfg/core/platform/process.hpp>
#include <sfg/core/io/error_reporting.hpp>
#include <sfg/core/io/log.hpp>

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR pCmdLine, _In_ int nCmdShow)
{
	sfg::error_reporting_t& reporting = sfg::error_reporting_t::get();

	if (!reporting.init("Editor"))
	{
		sfg::process::message_box("Stakeforge Editor", "Failed to initialize error reporting.");
		return 1;
	}

	try
	{
#ifdef SFG_DEBUG
		AllocConsole();
#endif

		sfg::editor_app_t& app = sfg::editor_app_t::get();

		if (!app.init())
		{
			SFG_ERR("failed initializing editor app");
			sfg::process::message_box("Stakeforge Editor", "Failed to initialize the editor. Check the error log.");
			reporting.uninit();
#ifdef SFG_DEBUG
			FreeConsole();
#endif
			return 1;
		}

		app.uninit();
		reporting.uninit();
#ifdef SFG_DEBUG
		FreeConsole();
#endif
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

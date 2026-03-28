#include "app_runner.hpp"

#include "app.hpp"
#include "memory/memory_tracer.hpp"
#include "platform/process.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace SFG
{
	int run_app()
	{
		if (AllocConsole() == FALSE)
		{
		}

		process::init();

		PUSH_MEMORY_CATEGORY("General");

		{
			app app;

			try
			{
			}
			catch (std::exception e)
			{
				MessageBox(nullptr, e.what(), "Error", MB_OK | MB_ICONERROR);
				FreeConsole();
				return 0;
			}

			const app::init_status status = app.init(vector2ui16(1920, 1080));
			if (status != app::init_status::ok)
			{
				if (status == app::init_status::working_directory_dont_exist)
					process::message_box("Toolmode requires a valid working directory!");
				else if (status == app::init_status::renderer_failed)
					process::message_box("Renderer failed initializing!");
				else if (status == app::init_status::backend_failed)
					process::message_box("Gfx backend failed initializing!");
				else if (status == app::init_status::window_failed)
					process::message_box("Main window failed initializing!");
				else if (status == app::init_status::engine_resources_failed)
					process::message_box("Failed loading engine shaders!");
				else if (status == app::init_status::packages_failed)
					process::message_box("Failed loading one of the packages: engine.stkpkg, res.sktpkg or world.stkpkg");

				process::uninit();
				POP_MEMORY_CATEGORY();
				FreeConsole();
				return 0;
			}

			app.tick();
			app.uninit();
		}

		POP_MEMORY_CATEGORY();

		FreeConsole();

		process::uninit();

		return 0;
	}
}

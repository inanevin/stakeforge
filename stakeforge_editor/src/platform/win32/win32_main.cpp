
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <editor_app.hpp>
#include <sfg/platform/process.hpp>

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR pCmdLine, _In_ int nCmdShow)
{
	AllocConsole();
	sfg::editor_app_t& app = sfg::editor_app_t::get();

	if (!app.init())
	{
		sfg::process::message_box("error!", "failed initializing editor app, check console");
		FreeConsole();
		return 0;
	}
	app.uninit();
	FreeConsole();
	return 0;
}

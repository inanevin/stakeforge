
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR pCmdLine, _In_ int nCmdShow)
{
	AllocConsole();

	while (true)
	{
		Sleep(1000);
	}

	FreeConsole();
	return 0;
}

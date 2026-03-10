#include "sfg_api.hpp"
#include "app/app.hpp"
#include "io/log.hpp"
#

extern "C"
{
	SFG::app* create_app(u32 width, u32 height)
	{
		SFG::app* app = new SFG::app();
		app->init({width, height});
		// app->tick();
		return app;
	}

	void destroy_app(SFG::app* app)
	{
		// app->uninit();
		delete app;
	}
}
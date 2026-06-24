// Copyright (c) 2025 Inan Evin

#include "engine_threads.hpp"
#include <sfg/io/assert.hpp>

namespace sfg
{
	engine_thread_ids_t g_engine_thread_ids;

	namespace
	{
		thread_local u32 g_render_access_depth = 0;
	}

	render_access_scope_t::render_access_scope_t()
	{
		g_render_access_depth++;
	}

	render_access_scope_t::~render_access_scope_t()
	{
		SFG_ASSERT(g_render_access_depth > 0);
		g_render_access_depth--;
	}

	bool is_render_access_thread()
	{
		return !SFG_IS_RENDER_RUNNING() || SFG_IS_RENDER_THREAD() || g_render_access_depth != 0;
	}
}

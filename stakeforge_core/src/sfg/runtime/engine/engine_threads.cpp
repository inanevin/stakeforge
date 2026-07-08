// Copyright (c) 2025 Inan Evin

#include "engine_threads.hpp"
#include <sfg/io/assert.hpp>

namespace sfg
{
	engine_thread_ids_t g_engine_thread_ids;

	namespace
	{
		thread_local u32 g_main_thread_depth   = 0;
		thread_local u32 g_render_access_depth = 0;
	}

	main_thread_scope_t::main_thread_scope_t()
	{
		g_main_thread_depth++;
	}

	main_thread_scope_t::~main_thread_scope_t()
	{
		SFG_ASSERT(g_main_thread_depth > 0);
		g_main_thread_depth--;
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

	bool is_main_thread()
	{
		return g_engine_thread_ids.main_thread_id.load() == SFG_THIS_THREAD_ID() || g_main_thread_depth != 0;
	}

	bool is_render_thread()
	{
		return g_engine_thread_ids.render_thread_id.load() == SFG_THIS_THREAD_ID();
	}

	bool is_render_access_thread()
	{
		return !SFG_IS_RENDER_RUNNING() || is_render_thread() || g_render_access_depth != 0;
	}
}

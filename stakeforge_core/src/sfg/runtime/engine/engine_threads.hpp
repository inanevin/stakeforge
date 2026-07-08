// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/atomic.hpp>

#include <functional>
#include <thread>

namespace sfg
{
	struct engine_thread_ids_t
	{
		atomic_t<u64> main_thread_id   = 0;
		atomic_t<u64> render_thread_id = 0;
	};

	extern engine_thread_ids_t g_engine_thread_ids;

	class render_access_scope_t final
	{
	public:
		render_access_scope_t();
		~render_access_scope_t();
		render_access_scope_t(const render_access_scope_t&)			   = delete;
		render_access_scope_t& operator=(const render_access_scope_t&) = delete;
	};

	class main_thread_scope_t final
	{
	public:
		main_thread_scope_t();
		~main_thread_scope_t();
		main_thread_scope_t(const main_thread_scope_t&)			   = delete;
		main_thread_scope_t& operator=(const main_thread_scope_t&) = delete;
	};

	bool is_main_thread();
	bool is_render_thread();
	bool is_render_access_thread();
}

#define SFG_THIS_THREAD_ID()	static_cast<u64>(std::hash<std::thread::id>{}(std::this_thread::get_id()))
#define SFG_IS_RENDER_RUNNING() (sfg::g_engine_thread_ids.render_thread_id.load() != 0)
#define SFG_IS_MAIN_THREAD()	(sfg::is_main_thread())
#define SFG_IS_RENDER_THREAD()	(sfg::is_render_thread())

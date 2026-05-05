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
}

#define SFG_THIS_THREAD_ID()	static_cast<u64>(std::hash<std::thread::id>{}(std::this_thread::get_id()))
#define SFG_IS_RENDER_RUNNING() (sfg::g_engine_thread_ids.render_thread_id.load() != 0)
#define SFG_IS_MAIN_THREAD()	(sfg::g_engine_thread_ids.main_thread_id.load() == SFG_THIS_THREAD_ID())
#define SFG_IS_RENDER_THREAD()	(sfg::g_engine_thread_ids.render_thread_id.load() == SFG_THIS_THREAD_ID())

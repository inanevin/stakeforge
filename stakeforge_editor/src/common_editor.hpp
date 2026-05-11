// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/common/size_definitions.hpp>
#include <sfg/memory/pool_handle.hpp>

namespace sfg
{
	struct editor_surface_tag_t;
	typedef pool_handle_t<u16, editor_surface_tag_t> surface_handle_t;

	enum class editor_panel_type_e : u8
	{
		invalid,
		entities,
		assets,
		inspector,
		profiling,
		logs,
		world,
	};
}

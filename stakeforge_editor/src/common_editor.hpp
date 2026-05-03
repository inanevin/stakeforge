// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/memory/pool_handle.hpp>

namespace sfg
{
	struct editor_surface_tag_t;
	typedef pool_handle_t<u16, editor_surface_tag_t> surface_handle_t;
}

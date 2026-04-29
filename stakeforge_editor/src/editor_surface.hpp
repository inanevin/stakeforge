// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_editor.hpp"
#include "data/unique.hpp"
#include "gfx/common/gfx_constants.hpp"
#include "math/vec2u16.hpp"
#include "platform/common_window.hpp"
#include "ui/ui_context.hpp"

namespace sfg
{
	struct editor_surface_t
	{
		window_runtime_t		   runtime		  = {};
		gfx_swapchain_handle	   swapchain	  = {};
		vec2u16_t				   swapchain_size = {};
		unique_t<ui::ui_context> ui;
	};
}

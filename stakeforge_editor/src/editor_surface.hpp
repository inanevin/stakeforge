// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_editor.hpp"
#include <sfg/data/unique.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/platform/common_window.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	struct editor_surface_t
	{
		window_runtime_t		 runtime		= {};
		gfx_swapchain_handle	 swapchain		= {};
		vec2u16_t				 swapchain_size = {};
		unique_t<ui::ui_context> ui;
		u16						 settings_idx = 0;
		bool					 is_minimized = false;
	};
}

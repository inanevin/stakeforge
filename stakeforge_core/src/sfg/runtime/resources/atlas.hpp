// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/common/size_definitions.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>

namespace sfg
{
	struct atlas_runtime_t
	{
		u8*	 data		  = nullptr;
		u32	 data_size	  = 0;
		u32	 width		  = 0;
		u32	 height		  = 0;
		u32	 id			  = 0xFFFFFFFFu;
		u32	 vertical_pos = 0;
		bool is_lcd		  = false;
		bool dirty		  = false;
	};

	struct atlas_internals_t
	{
		gfx_texture_handle texture = {};
	};

	void atlas_init(atlas_runtime_t& atlas, u32 width, u32 height, bool is_lcd);
	void atlas_uninit(atlas_runtime_t& atlas);
}

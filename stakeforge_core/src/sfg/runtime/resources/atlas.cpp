// Copyright (c) 2025 Inan Evin

#include "atlas.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/memory/memory.hpp>

namespace sfg
{
	void atlas_init(atlas_runtime_t& atlas, u32 width, u32 height, bool is_lcd)
	{
		SFG_ASSERT(width > 0 && height > 0);
		atlas.width		= width;
		atlas.height	= height;
		atlas.is_lcd	= is_lcd;
		atlas.data_size = width * height * (is_lcd ? 4u : 1u);
		atlas.data		= static_cast<u8*>(SFG_MALLOC(atlas.data_size));
		SFG_MEMSET(atlas.data, 0, atlas.data_size);
		atlas.vertical_pos = 0;
		atlas.dirty		   = true;
	}

	void atlas_uninit(atlas_runtime_t& atlas)
	{
		if (atlas.data)
			SFG_FREE(atlas.data);
		atlas.data		   = nullptr;
		atlas.data_size	   = 0;
		atlas.vertical_pos = 0;
		atlas.dirty		   = false;
	}
}

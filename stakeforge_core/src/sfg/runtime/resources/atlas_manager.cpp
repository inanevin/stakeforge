// Copyright (c) 2025 Inan Evin

#include "atlas_manager.hpp"
#include <sfg/io/assert.hpp>

namespace sfg
{
	void atlas_manager_t::init(u32 default_atlas_width, u32 default_atlas_height)
	{
		SFG_ASSERT(default_atlas_width > 0 && default_atlas_height > 0);
		_atlas_width  = default_atlas_width;
		_atlas_height = default_atlas_height;
	}

	void atlas_manager_t::uninit()
	{
		for (unique_t<atlas_runtime_t>& a : _atlases)
			atlas_uninit(*a);
		_atlases.clear();
		_atlas_width  = 0;
		_atlas_height = 0;
	}
}

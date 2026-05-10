// Copyright (c) 2025 Inan Evin
#pragma once

#include "atlas.hpp"
#include <sfg/data/unique.hpp>
#include <sfg/data/vector.hpp>

namespace sfg
{
	class atlas_manager_t
	{
	public:
		atlas_manager_t()								   = default;
		atlas_manager_t(const atlas_manager_t&)			   = delete;
		atlas_manager_t& operator=(const atlas_manager_t&) = delete;
		~atlas_manager_t()								   = default;

		void init(u32 default_atlas_width = 1024, u32 default_atlas_height = 1024);
		void uninit();

		inline const vector_t<unique_t<atlas_runtime_t>>& get_atlases() const
		{
			return _atlases;
		}

	private:
		vector_t<unique_t<atlas_runtime_t>> _atlases;
		u32									_atlas_width  = 0;
		u32									_atlas_height = 0;
	};
}

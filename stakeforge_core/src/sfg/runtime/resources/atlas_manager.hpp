// Copyright (c) 2025 Inan Evin
#pragma once

#include "atlas.hpp"
#include <sfg/data/unique.hpp>
#include <sfg/data/vector.hpp>

namespace sfg
{
	struct font_runtime_t;

	class atlas_manager_t
	{
	public:
		atlas_manager_t()								   = default;
		atlas_manager_t(const atlas_manager_t&)			   = delete;
		atlas_manager_t& operator=(const atlas_manager_t&) = delete;
		~atlas_manager_t()								   = default;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(u32 default_atlas_width = 1024, u32 default_atlas_height = 1024);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		bool add_font(font_runtime_t* font);
		void remove_font(font_runtime_t* font);

		inline const vector_t<unique_t<atlas_t>>& get_atlases() const
		{
			return _atlases;
		}

	private:
		vector_t<unique_t<atlas_t>> _atlases;
		u32							_atlas_width  = 0;
		u32							_atlas_height = 0;
	};
}

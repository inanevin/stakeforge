/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.

*/

#pragma once

#include <sfg/common/size_definitions.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/math/vec2u16.hpp>

namespace sfg
{
#define WORLD_RENDER_SHADOW_VIEW_CAPACITY		64
#define WORLD_RENDER_SHADOW_ALLOCATION_CAPACITY 32

	struct world_render_shadow_context_config_t
	{
		u16 view_max = 0;
	};

	struct world_render_shadow_allocation_t
	{
		gfx_handle_t texture	   = {};
		u64			 last_used_id  = 0;
		u64			 retire_id	   = 0;
		gpu_index_t	 texture_index = NULL_GPU_INDEX;
		u32			 stable_id	   = UINT32_MAX;
		vec2u16_t	 resolution	   = vec2u16_t::zero;
		u8			 type		   = 0;
		u8			 layer_count   = 0;
	};

	class world_render_shadow_context_t final
	{
	public:
		world_render_shadow_context_t()												   = default;
		~world_render_shadow_context_t()											   = default;
		world_render_shadow_context_t(const world_render_shadow_context_t&)			   = delete;
		world_render_shadow_context_t& operator=(const world_render_shadow_context_t&) = delete;
		world_render_shadow_context_t(world_render_shadow_context_t&& other) noexcept;
		world_render_shadow_context_t& operator=(world_render_shadow_context_t&& other) noexcept;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------
		void init(const world_render_shadow_context_config_t& config);
		void uninit();
		void begin_allocations();
		void end_allocations();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------
		world_render_shadow_allocation_t* get_or_create_allocation(u32 stable_id, u8 type, vec2u16_t resolution, u8 layer_count);

		// -----------------------------------------------------------------------------
		// queries
		// -----------------------------------------------------------------------------
		const world_render_shadow_allocation_t* find_allocation(u32 stable_id, u8 type) const;

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------
		inline gfx_handle_t get_command_buffer(u8 frame_index) const
		{
			return _pfd[frame_index].command_buffer;
		}

		inline u8* get_mapped_views(u8 frame_index) const
		{
			return _pfd[frame_index].mapped_views;
		}

		inline gpu_index_t get_view_buffer_index(u8 frame_index) const
		{
			return _pfd[frame_index].view_buffer_index;
		}

		inline gpu_index_t get_view_data_index(u8 frame_index, u16 view) const
		{
			return _pfd[frame_index].view_data_indices[view];
		}

		inline u8* get_mapped_view_data(u8 frame_index, u16 view) const
		{
			return _pfd[frame_index].mapped_view_data[view];
		}

		inline u16 get_view_max() const
		{
			return _config.view_max;
		}

	private:
		struct per_frame_data_t
		{
			u8*			 mapped_views										  = nullptr;
			u8*			 mapped_view_data[WORLD_RENDER_SHADOW_VIEW_CAPACITY]  = {};
			gfx_handle_t command_buffer										  = {};
			gfx_handle_t view_buffer										  = {};
			gfx_handle_t view_data[WORLD_RENDER_SHADOW_VIEW_CAPACITY]		  = {};
			gpu_index_t	 view_buffer_index									  = NULL_GPU_INDEX;
			gpu_index_t	 view_data_indices[WORLD_RENDER_SHADOW_VIEW_CAPACITY] = {};
		};

	private:
		per_frame_data_t					 _pfd[BACK_BUFFER_COUNT]							   = {};
		world_render_shadow_allocation_t	 _allocations[WORLD_RENDER_SHADOW_ALLOCATION_CAPACITY] = {};
		u64									 _id_counter										   = 0;
		world_render_shadow_context_config_t _config											   = {};
	};
}

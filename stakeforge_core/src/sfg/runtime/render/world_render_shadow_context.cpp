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

#include "world_render_shadow_context.hpp"
#include "world_render_context.hpp"
#include "world_render_light.hpp"
#include "world_render_shadow.hpp"
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>

namespace sfg
{
	world_render_shadow_context_t::world_render_shadow_context_t(world_render_shadow_context_t&& other) noexcept
	{
		*this = static_cast<world_render_shadow_context_t&&>(other);
	}

	world_render_shadow_context_t& world_render_shadow_context_t::operator=(world_render_shadow_context_t&& other) noexcept
	{
		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			_pfd[i]		  = other._pfd[i];
			other._pfd[i] = {};
		}

		for (u32 i = 0; i < WORLD_RENDER_SHADOW_ALLOCATION_CAPACITY; ++i)
		{
			_allocations[i]		  = other._allocations[i];
			other._allocations[i] = {};
		}

		_config			  = other._config;
		other._config	  = {};
		_id_counter		  = other._id_counter;
		other._id_counter = 0;

		return *this;
	}

	void world_render_shadow_context_t::init(const world_render_shadow_context_config_t& config)
	{
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());
		SFG_ASSERT(config.view_max <= WORLD_RENDER_SHADOW_VIEW_CAPACITY);

		_config = config;

		resource_desc_t view_buffer_desc = {};
		const u32		view_count		 = config.view_max == 0 ? 1 : config.view_max;
		view_buffer_desc.size			 = static_cast<u32>(sizeof(gpu_shadow_view_t) * view_count);
		view_buffer_desc.structure_size	 = static_cast<u32>(sizeof(gpu_shadow_view_t));
		view_buffer_desc.structure_count = view_count;
		view_buffer_desc.flags			 = resource_flags::rf_storage_buffer | resource_flags::rf_cpu_visible;
		view_buffer_desc.set_name("world_shadow_views");

		resource_desc_t view_data_desc = {};
		view_data_desc.size			   = static_cast<u32>(sizeof(render_pass_data_view_gpu_t));
		view_data_desc.flags		   = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible;
		view_data_desc.set_name("world_shadow_view_data");

		gfx_backend& backend = gfx_backend::get();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			SFG_ASSERT(_pfd[i].view_buffer.is_null());
			SFG_ASSERT(_pfd[i].command_buffer.is_null());

			_pfd[i].view_buffer = backend.create_resource(view_buffer_desc);
			backend.map_resource(_pfd[i].view_buffer, _pfd[i].mapped_views);
			_pfd[i].view_buffer_index = backend.get_resource_gpu_index(_pfd[i].view_buffer);

			if (config.view_max > 0)
			{
				_pfd[i].command_buffer = backend.create_command_buffer({
					.type		= command_type::graphics,
					.debug_name = "world_shadows",
				});

				for (u16 view = 0; view < config.view_max; ++view)
				{
					_pfd[i].view_data[view] = backend.create_resource(view_data_desc);
					backend.map_resource(_pfd[i].view_data[view], _pfd[i].mapped_view_data[view]);
					_pfd[i].view_data_indices[view] = backend.get_resource_gpu_index(_pfd[i].view_data[view]);
				}
			}
		}
	}

	void world_render_shadow_context_t::uninit()
	{
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());

		gfx_backend& backend = gfx_backend::get();

		for (world_render_shadow_allocation_t& allocation : _allocations)
		{
			if (!allocation.texture.is_null())
				backend.destroy_texture(allocation.texture);

			allocation = {};
		}

		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			SFG_ASSERT(!_pfd[i].view_buffer.is_null());

			backend.destroy_resource(_pfd[i].view_buffer);

			if (_config.view_max > 0)
			{
				for (u16 view = 0; view < _config.view_max; ++view)
					backend.destroy_resource(_pfd[i].view_data[view]);

				backend.destroy_command_buffer(_pfd[i].command_buffer);
			}

			_pfd[i] = {};
		}

		_config		= {};
		_id_counter = 0;
	}

	void world_render_shadow_context_t::begin_allocations()
	{
		++_id_counter;

		gfx_backend& backend = gfx_backend::get();

		for (world_render_shadow_allocation_t& allocation : _allocations)
		{
			if (!allocation.texture.is_null() && allocation.retire_id != 0 && _id_counter - allocation.retire_id >= BACK_BUFFER_COUNT)
			{
				backend.destroy_texture(allocation.texture);
				allocation = {};
			}
		}
	}

	void world_render_shadow_context_t::end_allocations()
	{
		for (world_render_shadow_allocation_t& allocation : _allocations)
		{
			if (!allocation.texture.is_null() && allocation.last_used_id != _id_counter && allocation.retire_id == 0)
				allocation.retire_id = _id_counter;
		}
	}

	world_render_shadow_allocation_t* world_render_shadow_context_t::get_or_create_allocation(u32 stable_id, u8 type, vec2u16_t resolution, u8 layer_count)
	{
		for (world_render_shadow_allocation_t& allocation : _allocations)
		{
			if (allocation.stable_id == stable_id && allocation.type == type && allocation.resolution == resolution && allocation.layer_count == layer_count)
			{
				allocation.last_used_id = _id_counter;
				allocation.retire_id	= 0;
				return &allocation;
			}
		}

		world_render_shadow_allocation_t* allocation = nullptr;

		for (world_render_shadow_allocation_t& candidate : _allocations)
		{
			if (candidate.texture.is_null())
			{
				allocation = &candidate;
				break;
			}
		}

		if (allocation == nullptr)
			return nullptr;

		texture_desc_t desc		  = {};
		desc.texture_format		  = format_e::r32_sfloat;
		desc.depth_stencil_format = format_e::d32_sfloat;
		desc.initial_states		  = resource_state_ps_resource;
		desc.size				  = resolution;
		desc.flags				  = texture_flags::tf_depth_texture | texture_flags::tf_typeless | texture_flags::tf_is_2d | texture_flags::tf_sampled;

		if (type == static_cast<u8>(world_render_light_type_e::point))
			desc.flags.set(texture_flags::tf_cubemap);

		desc.array_length = layer_count;
		desc.view_count	  = static_cast<u8>(layer_count + 1);

		for (u8 layer = 0; layer < layer_count; ++layer)
			desc.views[layer] = {.type = view_type::depth_stencil, .base_arr_level = layer, .level_count = 1};

		desc.views[layer_count] = {.type = view_type::sampled, .base_arr_level = 0, .level_count = layer_count, .is_cubemap = type == static_cast<u8>(world_render_light_type_e::point)};
		desc.clear_values[0]	= 1.0f;
		desc.set_name("world_shadow_map");

		gfx_backend& backend = gfx_backend::get();

		allocation->texture = backend.create_texture(desc);

		if (allocation->texture.is_null())
		{
			SFG_ERR("failed to create shadow map texture");
			*allocation = {};
			return nullptr;
		}

		allocation->texture_index = backend.get_texture_gpu_index(allocation->texture, layer_count);
		allocation->resolution	  = resolution;
		allocation->last_used_id  = _id_counter;
		allocation->stable_id	  = stable_id;
		allocation->type		  = type;
		allocation->layer_count	  = layer_count;

		return allocation;
	}

	const world_render_shadow_allocation_t* world_render_shadow_context_t::find_allocation(u32 stable_id, u8 type) const
	{
		for (const world_render_shadow_allocation_t& allocation : _allocations)
		{
			if (allocation.stable_id == stable_id && allocation.type == type && allocation.retire_id == 0)
				return &allocation;
		}

		return nullptr;
	}
}

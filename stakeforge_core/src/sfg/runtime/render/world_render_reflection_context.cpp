/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "world_render_reflection_context.hpp"
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/gfx/util/image_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>

namespace sfg
{
	world_render_reflection_context_t::world_render_reflection_context_t(world_render_reflection_context_t&& other) noexcept
	{
		*this = static_cast<world_render_reflection_context_t&&>(other);
	}

	world_render_reflection_context_t& world_render_reflection_context_t::operator=(world_render_reflection_context_t&& other) noexcept
	{
		for (u32 i = 0; i < WORLD_RENDER_REFLECTION_ALLOCATION_CAPACITY; ++i)
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

	void world_render_reflection_context_t::init(const world_render_reflection_context_config_t& config)
	{
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());
		SFG_ASSERT(config.allocation_max <= WORLD_RENDER_REFLECTION_ALLOCATION_CAPACITY);

		_config = config;
	}

	void world_render_reflection_context_t::uninit()
	{
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());

		gfx_backend& backend = gfx_backend::get();

		for (world_render_reflection_allocation_t& allocation : _allocations)
		{
			if (!allocation.radiance_texture.is_null())
			{
				SFG_ASSERT(!allocation.specular_texture.is_null());
				backend.destroy_texture(allocation.radiance_texture);
				backend.destroy_texture(allocation.specular_texture);
			}

			allocation = {};
		}

		_config		= {};
		_id_counter = 0;
	}

	void world_render_reflection_context_t::begin_allocations()
	{
		++_id_counter;

		gfx_backend& backend = gfx_backend::get();

		for (u16 i = 0; i < _config.allocation_max; ++i)
		{
			world_render_reflection_allocation_t& allocation = _allocations[i];

			if (!allocation.radiance_texture.is_null() && allocation.retire_id != 0 && _id_counter - allocation.retire_id >= BACK_BUFFER_COUNT)
			{
				backend.destroy_texture(allocation.radiance_texture);
				backend.destroy_texture(allocation.specular_texture);
				allocation = {};
			}
		}
	}

	void world_render_reflection_context_t::end_allocations()
	{
		for (u16 i = 0; i < _config.allocation_max; ++i)
		{
			world_render_reflection_allocation_t& allocation = _allocations[i];

			if (!allocation.radiance_texture.is_null() && allocation.last_used_id != _id_counter && allocation.retire_id == 0)
				allocation.retire_id = _id_counter;
		}
	}

	world_render_reflection_allocation_t* world_render_reflection_context_t::get_or_create_allocation(u32 stable_id, u16 resolution)
	{
		for (u16 i = 0; i < _config.allocation_max; ++i)
		{
			world_render_reflection_allocation_t& allocation = _allocations[i];

			if (allocation.stable_id == stable_id && allocation.resolution == resolution)
			{
				allocation.last_used_id = _id_counter;
				allocation.retire_id	= 0;
				return &allocation;
			}
		}

		world_render_reflection_allocation_t* allocation = nullptr;

		for (u16 i = 0; i < _config.allocation_max; ++i)
		{
			world_render_reflection_allocation_t& candidate = _allocations[i];

			if (candidate.radiance_texture.is_null())
			{
				allocation = &candidate;
				break;
			}
		}

		if (allocation == nullptr)
			return nullptr;

		const vec2u16_t texture_size = {resolution, resolution};

		texture_desc_t radiance_desc = {};
		radiance_desc.texture_format = format_e::r16g16b16a16_sfloat;
		radiance_desc.initial_states = resource_state_ps_resource;
		radiance_desc.size			 = texture_size;
		radiance_desc.flags			 = texture_flags::tf_render_target | texture_flags::tf_sampled | texture_flags::tf_is_2d | texture_flags::tf_cubemap;
		radiance_desc.view_count	 = 7;
		radiance_desc.array_length	 = 6;

		for (u8 face = 0; face < 6; ++face)
			radiance_desc.views[face] = {.type = view_type::render_target, .base_arr_level = face, .level_count = 1};

		radiance_desc.views[6] = {.type = view_type::sampled, .base_arr_level = 0, .level_count = 6, .mip_count = 0, .is_cubemap = 1};
		radiance_desc.set_name("world_reflection_probe_radiance");

		const u8 specular_mip_count = image_util_t::calculate_mip_levels(resolution, resolution);

		texture_desc_t specular_desc = {};
		specular_desc.texture_format = format_e::r16g16b16a16_sfloat;
		specular_desc.initial_states = resource_state_ps_resource;
		specular_desc.size			 = texture_size;
		specular_desc.flags			 = texture_flags::tf_sampled | texture_flags::tf_gpu_write | texture_flags::tf_is_2d | texture_flags::tf_cubemap;
		specular_desc.view_count	 = 1;
		specular_desc.mip_levels	 = specular_mip_count;
		specular_desc.array_length	 = 6;
		specular_desc.views[0]		 = {.type = view_type::sampled, .base_arr_level = 0, .level_count = 6, .mip_count = 0, .is_cubemap = 1};
		specular_desc.set_name("world_reflection_probe_specular");

		gfx_backend& backend = gfx_backend::get();

		allocation->radiance_texture = backend.create_texture(radiance_desc);

		if (allocation->radiance_texture.is_null())
		{
			SFG_ERR("failed to create reflection probe radiance texture");
			*allocation = {};
			return nullptr;
		}

		allocation->specular_texture = backend.create_texture(specular_desc);

		if (allocation->specular_texture.is_null())
		{
			SFG_ERR("failed to create reflection probe specular texture");
			backend.destroy_texture(allocation->radiance_texture);
			*allocation = {};
			return nullptr;
		}

		allocation->radiance_texture_index = backend.get_texture_gpu_index(allocation->radiance_texture, 6);
		allocation->specular_texture_index = backend.get_texture_gpu_index(allocation->specular_texture, 0);
		allocation->last_used_id		   = _id_counter;
		allocation->stable_id			   = stable_id;
		allocation->resolution			   = resolution;
		allocation->specular_mip_count	   = specular_mip_count;

		return allocation;
	}

	const world_render_reflection_allocation_t* world_render_reflection_context_t::find_allocation(u32 stable_id) const
	{
		for (u16 i = 0; i < _config.allocation_max; ++i)
		{
			const world_render_reflection_allocation_t& allocation = _allocations[i];

			if (allocation.stable_id == stable_id && allocation.retire_id == 0)
				return &allocation;
		}

		return nullptr;
	}
}

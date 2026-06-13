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

#include "world_render_context.hpp"
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/resources/shader.hpp>

namespace sfg
{
	world_render_context_t::world_render_context_t(world_render_context_t&& other) noexcept
	{
		*this = static_cast<world_render_context_t&&>(other);
	}

	world_render_context_t& world_render_context_t::operator=(world_render_context_t&& other) noexcept
	{
		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			_pfd[i]						  = other._pfd[i];
			other._pfd[i]				  = {};
			other._pfd[i].gfx0_done_value = 0;
		}
		_size		= other._size;
		other._size = vec2u16_t::zero;
		return *this;
	}

	void world_render_context_t::init(vec2u16_t size)
	{
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());
		SFG_ASSERT(size.x > 0 && size.y > 0);

		resource_desc_t opaque_render_pass_data_desc = {};
		opaque_render_pass_data_desc.size			 = static_cast<u32>(sizeof(render_pass_data_opaque_gpu_t));
		opaque_render_pass_data_desc.flags			 = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible;
		opaque_render_pass_data_desc.set_name("world_opaque_render_pass_data");

		resource_desc_t entity_buffer_desc = {};
		entity_buffer_desc.size			   = static_cast<u32>(sizeof(gpu_entity_t) * WORLD_RENDER_ENTITY_BUFFER_CAPACITY);
		entity_buffer_desc.structure_size  = static_cast<u32>(sizeof(gpu_entity_t));
		entity_buffer_desc.structure_count = WORLD_RENDER_ENTITY_BUFFER_CAPACITY;
		entity_buffer_desc.flags		   = resource_flags::rf_storage_buffer | resource_flags::rf_cpu_visible;
		entity_buffer_desc.set_name("world_entity_buffer");

		gfx_backend& backend = gfx_backend::get();
		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			SFG_ASSERT(_pfd[i].cmd_gfx0.is_null());
			SFG_ASSERT(_pfd[i].cmd_gfx1.is_null());
			SFG_ASSERT(_pfd[i].lighting_texture.is_null());
			SFG_ASSERT(_pfd[i].post_process_texture.is_null());
			SFG_ASSERT(_pfd[i].depth_texture.is_null());
			SFG_ASSERT(_pfd[i].gbuffer_albedo.is_null());
			SFG_ASSERT(_pfd[i].gbuffer_normal.is_null());
			SFG_ASSERT(_pfd[i].gbuffer_orm.is_null());
			SFG_ASSERT(_pfd[i].gbuffer_emissive.is_null());
			SFG_ASSERT(_pfd[i].gfx0_done_semaphore.is_null());
			SFG_ASSERT(_pfd[i].opaque_render_pass_data.is_null());
			SFG_ASSERT(_pfd[i].entity_buffer.is_null());

			_pfd[i].cmd_gfx0				= backend.create_command_buffer({
							   .type	   = command_type::graphics,
							   .debug_name = "world_gfx0",
			   });
			_pfd[i].cmd_gfx1				= backend.create_command_buffer({
							   .type	   = command_type::graphics,
							   .debug_name = "world_gfx1",
			   });
			_pfd[i].gfx0_done_semaphore		= backend.create_semaphore();
			_pfd[i].opaque_render_pass_data = backend.create_resource(opaque_render_pass_data_desc);
			_pfd[i].entity_buffer			= backend.create_resource(entity_buffer_desc);
			backend.map_resource(_pfd[i].opaque_render_pass_data, _pfd[i].mapped_opaque_render_pass_data);
			backend.map_resource(_pfd[i].entity_buffer, _pfd[i].mapped_entity_buffer);
			_pfd[i].opaque_render_pass_data_index = backend.get_resource_gpu_index(_pfd[i].opaque_render_pass_data);
			_pfd[i].entity_buffer_index			  = backend.get_resource_gpu_index(_pfd[i].entity_buffer);
		}
		const shader_internals_t* sh = resource_manager_t::get().find_internals<shader_internals_t>("engine/shaders/world/deferred_lighting.hlsl"_hs);
		_shaders.lighting			 = sh->psos[0];

		create_texture(size);
	}

	void world_render_context_t::uninit()
	{
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());

		destroy_texture();
		gfx_backend& backend = gfx_backend::get();
		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			SFG_ASSERT(!_pfd[i].cmd_gfx0.is_null());
			SFG_ASSERT(!_pfd[i].cmd_gfx1.is_null());
			SFG_ASSERT(!_pfd[i].gfx0_done_semaphore.is_null());
			SFG_ASSERT(!_pfd[i].opaque_render_pass_data.is_null());
			SFG_ASSERT(!_pfd[i].entity_buffer.is_null());
			backend.destroy_resource(_pfd[i].opaque_render_pass_data);
			backend.destroy_resource(_pfd[i].entity_buffer);
			backend.destroy_command_buffer(_pfd[i].cmd_gfx0);
			backend.destroy_command_buffer(_pfd[i].cmd_gfx1);
			backend.destroy_semaphore(_pfd[i].gfx0_done_semaphore);
			_pfd[i].cmd_gfx0					   = {};
			_pfd[i].cmd_gfx1					   = {};
			_pfd[i].gfx0_done_semaphore			   = {};
			_pfd[i].gfx0_done_value				   = 0;
			_pfd[i].opaque_render_pass_data		   = {};
			_pfd[i].entity_buffer				   = {};
			_pfd[i].mapped_opaque_render_pass_data = nullptr;
			_pfd[i].mapped_entity_buffer		   = nullptr;
			_pfd[i].opaque_render_pass_data_index  = NULL_GPU_INDEX;
			_pfd[i].entity_buffer_index			   = NULL_GPU_INDEX;
		}
	}

	void world_render_context_t::resize(vec2u16_t size)
	{
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());
		SFG_ASSERT(size.x > 0 && size.y > 0);

		destroy_texture();
		create_texture(size);
	}

	gfx_command_buffer_handle world_render_context_t::get_command_buffer(u8 frame_index) const
	{
		return get_command_buffer_gfx1(frame_index);
	}

	gfx_command_buffer_handle world_render_context_t::get_command_buffer_gfx0(u8 frame_index) const
	{
		SFG_ASSERT(frame_index < BACK_BUFFER_COUNT);
		return _pfd[frame_index].cmd_gfx0;
	}

	gfx_command_buffer_handle world_render_context_t::get_command_buffer_gfx1(u8 frame_index) const
	{
		SFG_ASSERT(frame_index < BACK_BUFFER_COUNT);
		return _pfd[frame_index].cmd_gfx1;
	}

	gfx_texture_handle world_render_context_t::get_world_texture(u8 frame_index) const
	{
		SFG_ASSERT(frame_index < BACK_BUFFER_COUNT);
		return _pfd[frame_index].post_process_texture;
	}

	gfx_texture_handle world_render_context_t::get_lighting_texture(u8 frame_index) const
	{
		SFG_ASSERT(frame_index < BACK_BUFFER_COUNT);
		return _pfd[frame_index].lighting_texture;
	}

	gfx_texture_handle world_render_context_t::get_depth_texture(u8 frame_index) const
	{
		SFG_ASSERT(frame_index < BACK_BUFFER_COUNT);
		return _pfd[frame_index].depth_texture;
	}

	gfx_texture_handle world_render_context_t::get_post_process_texture(u8 frame_index) const
	{
		SFG_ASSERT(frame_index < BACK_BUFFER_COUNT);
		return _pfd[frame_index].post_process_texture;
	}

	gfx_texture_handle world_render_context_t::get_gbuffer_albedo_texture(u8 frame_index) const
	{
		SFG_ASSERT(frame_index < BACK_BUFFER_COUNT);
		return _pfd[frame_index].gbuffer_albedo;
	}

	gfx_texture_handle world_render_context_t::get_gbuffer_normal_texture(u8 frame_index) const
	{
		SFG_ASSERT(frame_index < BACK_BUFFER_COUNT);
		return _pfd[frame_index].gbuffer_normal;
	}

	gfx_texture_handle world_render_context_t::get_gbuffer_orm_texture(u8 frame_index) const
	{
		SFG_ASSERT(frame_index < BACK_BUFFER_COUNT);
		return _pfd[frame_index].gbuffer_orm;
	}

	gfx_texture_handle world_render_context_t::get_gbuffer_emissive_texture(u8 frame_index) const
	{
		SFG_ASSERT(frame_index < BACK_BUFFER_COUNT);
		return _pfd[frame_index].gbuffer_emissive;
	}

	gfx_semaphore_handle world_render_context_t::get_gfx0_done_semaphore(u8 frame_index) const
	{
		SFG_ASSERT(frame_index < BACK_BUFFER_COUNT);
		return _pfd[frame_index].gfx0_done_semaphore;
	}

	u64 world_render_context_t::next_gfx0_done_semaphore_value(u8 frame_index) const
	{
		SFG_ASSERT(frame_index < BACK_BUFFER_COUNT);
		return ++_pfd[frame_index].gfx0_done_value;
	}

	gpu_index_t world_render_context_t::get_world_texture_index(u8 frame_index) const
	{
		SFG_ASSERT(frame_index < BACK_BUFFER_COUNT);
		return _pfd[frame_index].post_process_texture_index;
	}

	gpu_index_t world_render_context_t::get_opaque_render_pass_data_index(u8 frame_index) const
	{
		SFG_ASSERT(frame_index < BACK_BUFFER_COUNT);
		return _pfd[frame_index].opaque_render_pass_data_index;
	}

	gpu_index_t world_render_context_t::get_entity_buffer_index(u8 frame_index) const
	{
		SFG_ASSERT(frame_index < BACK_BUFFER_COUNT);
		return _pfd[frame_index].entity_buffer_index;
	}

	u8* world_render_context_t::get_mapped_opaque_render_pass_data(u8 frame_index) const
	{
		SFG_ASSERT(frame_index < BACK_BUFFER_COUNT);
		return _pfd[frame_index].mapped_opaque_render_pass_data;
	}

	u8* world_render_context_t::get_mapped_entity_buffer(u8 frame_index) const
	{
		SFG_ASSERT(frame_index < BACK_BUFFER_COUNT);
		return _pfd[frame_index].mapped_entity_buffer;
	}

	vec2u16_t world_render_context_t::get_size() const
	{
		return _size;
	}

	void world_render_context_t::create_texture(vec2u16_t size)
	{
		texture_desc_t lighting_desc = {};
		lighting_desc.texture_format = format_e::r8g8b8a8_unorm;
		lighting_desc.size			 = size;
		lighting_desc.flags			 = texture_flags::tf_render_target | texture_flags::tf_sampled | texture_flags::tf_is_2d;
		lighting_desc.view_count	 = 2;
		lighting_desc.views[0]		 = {.type = view_type::sampled};
		lighting_desc.views[1]		 = {.type = view_type::render_target};
		lighting_desc.set_name("world_lighting");

		texture_desc_t post_process_desc = lighting_desc;
		post_process_desc.set_name("world_post_process");

		texture_desc_t depth_desc		= {};
		depth_desc.texture_format		= format_e::r32_sfloat;
		depth_desc.depth_stencil_format = format_e::d32_sfloat;
		depth_desc.size					= size;
		depth_desc.flags				= texture_flags::tf_depth_texture | texture_flags::tf_typeless | texture_flags::tf_is_2d | texture_flags::tf_sampled;
		depth_desc.view_count			= 3;
		depth_desc.views[0]				= {.type = view_type::depth_stencil};
		depth_desc.views[1]				= {.type = view_type::depth_stencil, .read_only = 1};
		depth_desc.views[2]				= {.type = view_type::sampled};
		depth_desc.clear_values[0]		= 0.0f;
		depth_desc.set_name("world_depth");

		texture_desc_t gbuffer_albedo_desc = {};
		gbuffer_albedo_desc.texture_format = format_e::r8g8b8a8_srgb;
		gbuffer_albedo_desc.size		   = size;
		gbuffer_albedo_desc.flags		   = texture_flags::tf_render_target | texture_flags::tf_sampled | texture_flags::tf_is_2d;
		gbuffer_albedo_desc.view_count	   = 2;
		gbuffer_albedo_desc.views[0]	   = {.type = view_type::render_target};
		gbuffer_albedo_desc.views[1]	   = {.type = view_type::sampled};
		gbuffer_albedo_desc.set_name("gbuffer_albedo");

		texture_desc_t gbuffer_normal_desc = {};
		gbuffer_normal_desc.texture_format = format_e::r10g0b10a2_unorm;
		gbuffer_normal_desc.size		   = size;
		gbuffer_normal_desc.flags		   = texture_flags::tf_render_target | texture_flags::tf_sampled | texture_flags::tf_is_2d;
		gbuffer_normal_desc.view_count	   = 2;
		gbuffer_normal_desc.views[0]	   = {.type = view_type::render_target};
		gbuffer_normal_desc.views[1]	   = {.type = view_type::sampled};
		gbuffer_normal_desc.set_name("gbuffer_normal");

		texture_desc_t gbuffer_orm_desc = {};
		gbuffer_orm_desc.texture_format = format_e::r8g8b8a8_unorm;
		gbuffer_orm_desc.size			= size;
		gbuffer_orm_desc.flags			= texture_flags::tf_render_target | texture_flags::tf_sampled | texture_flags::tf_is_2d;
		gbuffer_orm_desc.view_count		= 2;
		gbuffer_orm_desc.views[0]		= {.type = view_type::render_target};
		gbuffer_orm_desc.views[1]		= {.type = view_type::sampled};
		gbuffer_orm_desc.set_name("gbuffer_orm");

		texture_desc_t gbuffer_emissive_desc = {};
		gbuffer_emissive_desc.texture_format = format_e::r16g16b16a16_sfloat;
		gbuffer_emissive_desc.size			 = size;
		gbuffer_emissive_desc.flags			 = texture_flags::tf_render_target | texture_flags::tf_sampled | texture_flags::tf_is_2d;
		gbuffer_emissive_desc.view_count	 = 2;
		gbuffer_emissive_desc.views[0]		 = {.type = view_type::render_target};
		gbuffer_emissive_desc.views[1]		 = {.type = view_type::sampled};
		gbuffer_emissive_desc.set_name("gbuffer_emissive");

		gfx_backend& backend = gfx_backend::get();
		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			SFG_ASSERT(_pfd[i].lighting_texture.is_null());
			SFG_ASSERT(_pfd[i].post_process_texture.is_null());
			SFG_ASSERT(_pfd[i].depth_texture.is_null());
			SFG_ASSERT(_pfd[i].gbuffer_albedo.is_null());
			SFG_ASSERT(_pfd[i].gbuffer_normal.is_null());
			SFG_ASSERT(_pfd[i].gbuffer_orm.is_null());
			SFG_ASSERT(_pfd[i].gbuffer_emissive.is_null());

			_pfd[i].lighting_texture		   = backend.create_texture(lighting_desc);
			_pfd[i].post_process_texture	   = backend.create_texture(post_process_desc);
			_pfd[i].depth_texture			   = backend.create_texture(depth_desc);
			_pfd[i].gbuffer_albedo			   = backend.create_texture(gbuffer_albedo_desc);
			_pfd[i].gbuffer_normal			   = backend.create_texture(gbuffer_normal_desc);
			_pfd[i].gbuffer_orm				   = backend.create_texture(gbuffer_orm_desc);
			_pfd[i].gbuffer_emissive		   = backend.create_texture(gbuffer_emissive_desc);
			_pfd[i].lighting_texture_index	   = backend.get_texture_gpu_index(_pfd[i].lighting_texture, 0);
			_pfd[i].post_process_texture_index = backend.get_texture_gpu_index(_pfd[i].post_process_texture, 0);
			_pfd[i].depth_texture_index		   = backend.get_texture_gpu_index(_pfd[i].depth_texture, 2);
			_pfd[i].gbuffer_albedo_index	   = backend.get_texture_gpu_index(_pfd[i].gbuffer_albedo, 1);
			_pfd[i].gbuffer_normal_index	   = backend.get_texture_gpu_index(_pfd[i].gbuffer_normal, 1);
			_pfd[i].gbuffer_orm_index		   = backend.get_texture_gpu_index(_pfd[i].gbuffer_orm, 1);
			_pfd[i].gbuffer_emissive_index	   = backend.get_texture_gpu_index(_pfd[i].gbuffer_emissive, 1);
		}
		_size = size;
	}

	void world_render_context_t::destroy_texture()
	{
		gfx_backend& backend = gfx_backend::get();
		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			SFG_ASSERT(!_pfd[i].lighting_texture.is_null());
			SFG_ASSERT(!_pfd[i].post_process_texture.is_null());
			SFG_ASSERT(!_pfd[i].depth_texture.is_null());
			SFG_ASSERT(!_pfd[i].gbuffer_albedo.is_null());
			SFG_ASSERT(!_pfd[i].gbuffer_normal.is_null());
			SFG_ASSERT(!_pfd[i].gbuffer_orm.is_null());
			SFG_ASSERT(!_pfd[i].gbuffer_emissive.is_null());

			backend.destroy_texture(_pfd[i].lighting_texture);
			backend.destroy_texture(_pfd[i].post_process_texture);
			backend.destroy_texture(_pfd[i].depth_texture);
			backend.destroy_texture(_pfd[i].gbuffer_albedo);
			backend.destroy_texture(_pfd[i].gbuffer_normal);
			backend.destroy_texture(_pfd[i].gbuffer_orm);
			backend.destroy_texture(_pfd[i].gbuffer_emissive);
			_pfd[i].lighting_texture		   = {};
			_pfd[i].post_process_texture	   = {};
			_pfd[i].depth_texture			   = {};
			_pfd[i].gbuffer_albedo			   = {};
			_pfd[i].gbuffer_normal			   = {};
			_pfd[i].gbuffer_orm				   = {};
			_pfd[i].gbuffer_emissive		   = {};
			_pfd[i].lighting_texture_index	   = NULL_GPU_INDEX;
			_pfd[i].post_process_texture_index = NULL_GPU_INDEX;
			_pfd[i].depth_texture_index		   = NULL_GPU_INDEX;
			_pfd[i].gbuffer_albedo_index	   = NULL_GPU_INDEX;
			_pfd[i].gbuffer_normal_index	   = NULL_GPU_INDEX;
			_pfd[i].gbuffer_orm_index		   = NULL_GPU_INDEX;
			_pfd[i].gbuffer_emissive_index	   = NULL_GPU_INDEX;
		}
		_size = vec2u16_t::zero;
	}
}

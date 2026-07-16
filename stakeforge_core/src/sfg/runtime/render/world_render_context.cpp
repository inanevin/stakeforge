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
#include <sfg/runtime/render/render_resources.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/resources/shader.hpp>
#include <sfg/runtime/resources/vertex.hpp>

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
		_shaders	   = other._shaders;
		other._shaders = {};
		_config		   = other._config;
		other._config  = {};
		return *this;
	}

	void world_render_context_t::init(const world_render_context_config_t& config)
	{
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());
		SFG_ASSERT(config.size.x > 0 && config.size.y > 0);
		SFG_ASSERT((config.line_vertex_max == 0) == (config.line_index_max == 0));
		SFG_ASSERT((config.text_vertex_max == 0) == (config.text_index_max == 0));
		_config = config;

		resource_desc_t opaque_render_pass_data_desc = {};
		opaque_render_pass_data_desc.size			 = static_cast<u32>(sizeof(render_pass_data_opaque_gpu_t));
		opaque_render_pass_data_desc.flags			 = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible;
		opaque_render_pass_data_desc.set_name("world_opaque_render_pass_data");

		resource_desc_t lighting_render_pass_data_desc = {};
		lighting_render_pass_data_desc.size			   = static_cast<u32>(sizeof(render_pass_data_lighting_gpu_t));
		lighting_render_pass_data_desc.flags		   = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible;
		lighting_render_pass_data_desc.set_name("world_lighting_render_pass_data");

		resource_desc_t post_process_render_pass_data_desc = {};
		post_process_render_pass_data_desc.size			   = static_cast<u32>(sizeof(render_pass_data_post_process_gpu_t));
		post_process_render_pass_data_desc.flags		   = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible;
		post_process_render_pass_data_desc.set_name("world_post_process_render_pass_data");

		resource_desc_t entity_buffer_desc = {};
		entity_buffer_desc.size			   = static_cast<u32>(sizeof(gpu_entity_t) * WORLD_RENDER_ENTITY_BUFFER_CAPACITY);
		entity_buffer_desc.structure_size  = static_cast<u32>(sizeof(gpu_entity_t));
		entity_buffer_desc.structure_count = WORLD_RENDER_ENTITY_BUFFER_CAPACITY;
		entity_buffer_desc.flags		   = resource_flags::rf_storage_buffer | resource_flags::rf_cpu_visible;
		entity_buffer_desc.set_name("world_entity_buffer");

		resource_desc_t debug_line_data_desc = {};
		debug_line_data_desc.size			 = static_cast<u32>(sizeof(world_debug_line_gpu_data_t));
		debug_line_data_desc.flags			 = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible;
		debug_line_data_desc.set_name("world_debug_line_data");

		resource_desc_t debug_line_vertex_desc = {};
		debug_line_vertex_desc.size			   = config.line_vertex_max * static_cast<u32>(sizeof(vertex_debug_line_t));
		debug_line_vertex_desc.flags		   = resource_flags::rf_vertex_buffer | resource_flags::rf_cpu_visible;
		debug_line_vertex_desc.set_name("world_debug_line_vertices");

		resource_desc_t debug_line_index_desc = {};
		debug_line_index_desc.size			  = config.line_index_max * static_cast<u32>(sizeof(primitive_index));
		debug_line_index_desc.flags			  = resource_flags::rf_index_buffer | resource_flags::rf_cpu_visible;
		debug_line_index_desc.set_name("world_debug_line_indices");

		resource_desc_t debug_text_data_desc = {};
		debug_text_data_desc.size			 = static_cast<u32>(sizeof(world_debug_text_gpu_data_t));
		debug_text_data_desc.flags			 = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible;
		debug_text_data_desc.set_name("world_debug_text_data");

		resource_desc_t debug_text_vertex_desc = {};
		debug_text_vertex_desc.size			   = config.text_vertex_max * static_cast<u32>(sizeof(vertex_debug_text_t));
		debug_text_vertex_desc.flags		   = resource_flags::rf_vertex_buffer | resource_flags::rf_cpu_visible;
		debug_text_vertex_desc.set_name("world_debug_text_vertices");

		resource_desc_t debug_text_index_desc = {};
		debug_text_index_desc.size			  = config.text_index_max * static_cast<u32>(sizeof(primitive_index));
		debug_text_index_desc.flags			  = resource_flags::rf_index_buffer | resource_flags::rf_cpu_visible;
		debug_text_index_desc.set_name("world_debug_text_indices");

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
			SFG_ASSERT(_pfd[i].ao_texture.is_null());
			SFG_ASSERT(_pfd[i].gfx0_done_semaphore.is_null());
			SFG_ASSERT(_pfd[i].opaque_render_pass_data.is_null());
			SFG_ASSERT(_pfd[i].lighting_render_pass_data.is_null());
			SFG_ASSERT(_pfd[i].post_process_render_pass_data.is_null());
			SFG_ASSERT(_pfd[i].entity_buffer.is_null());

			_pfd[i].cmd_gfx0					  = backend.create_command_buffer({
				.type		= command_type::graphics,
				.debug_name = "world_gfx0",
			});
			_pfd[i].cmd_gfx1					  = backend.create_command_buffer({
				.type		= command_type::graphics,
				.debug_name = "world_gfx1",
			});
			_pfd[i].gfx0_done_semaphore			  = backend.create_semaphore();
			_pfd[i].opaque_render_pass_data		  = backend.create_resource(opaque_render_pass_data_desc);
			_pfd[i].lighting_render_pass_data	  = backend.create_resource(lighting_render_pass_data_desc);
			_pfd[i].post_process_render_pass_data = backend.create_resource(post_process_render_pass_data_desc);
			_pfd[i].entity_buffer				  = backend.create_resource(entity_buffer_desc);

			backend.map_resource(_pfd[i].opaque_render_pass_data, _pfd[i].mapped_opaque_render_pass_data);
			backend.map_resource(_pfd[i].lighting_render_pass_data, _pfd[i].mapped_lighting_render_pass_data);
			backend.map_resource(_pfd[i].post_process_render_pass_data, _pfd[i].mapped_post_process_render_pass_data);
			backend.map_resource(_pfd[i].entity_buffer, _pfd[i].mapped_entity_buffer);

			_pfd[i].opaque_render_pass_data_index		= backend.get_resource_gpu_index(_pfd[i].opaque_render_pass_data);
			_pfd[i].lighting_render_pass_data_index		= backend.get_resource_gpu_index(_pfd[i].lighting_render_pass_data);
			_pfd[i].post_process_render_pass_data_index = backend.get_resource_gpu_index(_pfd[i].post_process_render_pass_data);
			_pfd[i].entity_buffer_index					= backend.get_resource_gpu_index(_pfd[i].entity_buffer);

			if (config.line_vertex_max > 0)
			{
				_pfd[i].debug_line_data			 = backend.create_resource(debug_line_data_desc);
				_pfd[i].debug_line_vertex_buffer = backend.create_resource(debug_line_vertex_desc);
				_pfd[i].debug_line_index_buffer	 = backend.create_resource(debug_line_index_desc);
				backend.map_resource(_pfd[i].debug_line_data, _pfd[i].mapped_debug_line_data);
				backend.map_resource(_pfd[i].debug_line_vertex_buffer, _pfd[i].mapped_debug_line_vertices);
				backend.map_resource(_pfd[i].debug_line_index_buffer, _pfd[i].mapped_debug_line_indices);
				_pfd[i].debug_line_data_index = backend.get_resource_gpu_index(_pfd[i].debug_line_data);
			}

			if (config.text_vertex_max > 0)
			{
				_pfd[i].debug_text_data			 = backend.create_resource(debug_text_data_desc);
				_pfd[i].debug_text_vertex_buffer = backend.create_resource(debug_text_vertex_desc);
				_pfd[i].debug_text_index_buffer	 = backend.create_resource(debug_text_index_desc);
				backend.map_resource(_pfd[i].debug_text_data, _pfd[i].mapped_debug_text_data);
				backend.map_resource(_pfd[i].debug_text_vertex_buffer, _pfd[i].mapped_debug_text_vertices);
				backend.map_resource(_pfd[i].debug_text_index_buffer, _pfd[i].mapped_debug_text_indices);
				_pfd[i].debug_text_data_index = backend.get_resource_gpu_index(_pfd[i].debug_text_data);
			}
		}

		const render_resources_t& render_resources = render_resources_t::get();
		const shader_internals_t* sh			   = resource_manager_t::get().find_internals<shader_internals_t>("common/shaders/world/deferred_lighting.hlsl"_hs);
		_shaders.lighting						   = render_resources.get_shader_hw(sh->psos[0]);
		sh										   = resource_manager_t::get().find_internals<shader_internals_t>("common/shaders/world/post_combiner.hlsl"_hs);
		_shaders.post_combiner					   = render_resources.get_shader_hw(sh->psos[0]);
		sh										   = resource_manager_t::get().find_internals<shader_internals_t>("common/shaders/world/debug_line.hlsl"_hs);
		_shaders.debug_line						   = render_resources.get_shader_hw(sh->psos[0]);
		sh										   = resource_manager_t::get().find_internals<shader_internals_t>("common/shaders/world/debug_text.hlsl"_hs);
		_shaders.debug_text						   = render_resources.get_shader_hw(sh->psos[0]);

		create_texture(config.size);
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
			SFG_ASSERT(!_pfd[i].lighting_render_pass_data.is_null());
			SFG_ASSERT(!_pfd[i].post_process_render_pass_data.is_null());
			SFG_ASSERT(!_pfd[i].entity_buffer.is_null());
			backend.destroy_resource(_pfd[i].opaque_render_pass_data);
			backend.destroy_resource(_pfd[i].lighting_render_pass_data);
			backend.destroy_resource(_pfd[i].post_process_render_pass_data);
			backend.destroy_resource(_pfd[i].entity_buffer);
			if (_config.line_vertex_max > 0)
			{
				backend.destroy_resource(_pfd[i].debug_line_data);
				backend.destroy_resource(_pfd[i].debug_line_vertex_buffer);
				backend.destroy_resource(_pfd[i].debug_line_index_buffer);
			}
			if (_config.text_vertex_max > 0)
			{
				backend.destroy_resource(_pfd[i].debug_text_data);
				backend.destroy_resource(_pfd[i].debug_text_vertex_buffer);
				backend.destroy_resource(_pfd[i].debug_text_index_buffer);
			}
			backend.destroy_command_buffer(_pfd[i].cmd_gfx0);
			backend.destroy_command_buffer(_pfd[i].cmd_gfx1);
			backend.destroy_semaphore(_pfd[i].gfx0_done_semaphore);
			_pfd[i].cmd_gfx0							 = {};
			_pfd[i].cmd_gfx1							 = {};
			_pfd[i].gfx0_done_semaphore					 = {};
			_pfd[i].gfx0_done_value						 = 0;
			_pfd[i].opaque_render_pass_data				 = {};
			_pfd[i].lighting_render_pass_data			 = {};
			_pfd[i].post_process_render_pass_data		 = {};
			_pfd[i].entity_buffer						 = {};
			_pfd[i].debug_line_data						 = {};
			_pfd[i].debug_line_vertex_buffer			 = {};
			_pfd[i].debug_line_index_buffer				 = {};
			_pfd[i].debug_text_data						 = {};
			_pfd[i].debug_text_vertex_buffer			 = {};
			_pfd[i].debug_text_index_buffer				 = {};
			_pfd[i].mapped_opaque_render_pass_data		 = nullptr;
			_pfd[i].mapped_lighting_render_pass_data	 = nullptr;
			_pfd[i].mapped_post_process_render_pass_data = nullptr;
			_pfd[i].mapped_entity_buffer				 = nullptr;
			_pfd[i].mapped_debug_line_data				 = nullptr;
			_pfd[i].mapped_debug_line_vertices			 = nullptr;
			_pfd[i].mapped_debug_line_indices			 = nullptr;
			_pfd[i].mapped_debug_text_data				 = nullptr;
			_pfd[i].mapped_debug_text_vertices			 = nullptr;
			_pfd[i].mapped_debug_text_indices			 = nullptr;
			_pfd[i].opaque_render_pass_data_index		 = NULL_GPU_INDEX;
			_pfd[i].lighting_render_pass_data_index		 = NULL_GPU_INDEX;
			_pfd[i].post_process_render_pass_data_index	 = NULL_GPU_INDEX;
			_pfd[i].entity_buffer_index					 = NULL_GPU_INDEX;
			_pfd[i].debug_line_data_index				 = NULL_GPU_INDEX;
			_pfd[i].debug_text_data_index				 = NULL_GPU_INDEX;
		}
		_shaders = {};
		_config	 = {};
	}

	void world_render_context_t::resize(vec2u16_t size)
	{
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());
		SFG_ASSERT(size.x > 0 && size.y > 0);

		destroy_texture();
		create_texture(size);
	}

	void world_render_context_t::create_texture(vec2u16_t size)
	{
		texture_desc_t lighting_desc = {};
		lighting_desc.texture_format = format_e::r16g16b16a16_sfloat;
		lighting_desc.size			 = size;
		lighting_desc.flags			 = texture_flags::tf_render_target | texture_flags::tf_sampled | texture_flags::tf_is_2d;
		lighting_desc.view_count	 = 2;
		lighting_desc.views[0]		 = {.type = view_type::sampled};
		lighting_desc.views[1]		 = {.type = view_type::render_target};
		lighting_desc.set_name("world_lighting");

		texture_desc_t post_process_desc = lighting_desc;
		post_process_desc.texture_format = format_e::r8g8b8a8_srgb;
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

		texture_desc_t ao_desc	= {};
		ao_desc.texture_format	= format_e::r8_unorm;
		ao_desc.size			= size;
		ao_desc.flags			= texture_flags::tf_sampled | texture_flags::tf_gpu_write | texture_flags::tf_is_2d;
		ao_desc.view_count		= 2;
		ao_desc.views[0]		= {.type = view_type::sampled};
		ao_desc.views[1]		= {.type = view_type::gpu_write};
		ao_desc.clear_values[0] = 1.0f;
		ao_desc.set_name("world_ao");

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
			SFG_ASSERT(_pfd[i].ao_texture.is_null());

			_pfd[i].lighting_texture		   = backend.create_texture(lighting_desc);
			_pfd[i].post_process_texture	   = backend.create_texture(post_process_desc);
			_pfd[i].depth_texture			   = backend.create_texture(depth_desc);
			_pfd[i].gbuffer_albedo			   = backend.create_texture(gbuffer_albedo_desc);
			_pfd[i].gbuffer_normal			   = backend.create_texture(gbuffer_normal_desc);
			_pfd[i].gbuffer_orm				   = backend.create_texture(gbuffer_orm_desc);
			_pfd[i].gbuffer_emissive		   = backend.create_texture(gbuffer_emissive_desc);
			_pfd[i].ao_texture				   = backend.create_texture(ao_desc);
			_pfd[i].lighting_texture_index	   = backend.get_texture_gpu_index(_pfd[i].lighting_texture, 0);
			_pfd[i].post_process_texture_index = backend.get_texture_gpu_index(_pfd[i].post_process_texture, 0);
			_pfd[i].depth_texture_index		   = backend.get_texture_gpu_index(_pfd[i].depth_texture, 2);
			_pfd[i].gbuffer_albedo_index	   = backend.get_texture_gpu_index(_pfd[i].gbuffer_albedo, 1);
			_pfd[i].gbuffer_normal_index	   = backend.get_texture_gpu_index(_pfd[i].gbuffer_normal, 1);
			_pfd[i].gbuffer_orm_index		   = backend.get_texture_gpu_index(_pfd[i].gbuffer_orm, 1);
			_pfd[i].gbuffer_emissive_index	   = backend.get_texture_gpu_index(_pfd[i].gbuffer_emissive, 1);
			_pfd[i].ao_texture_index		   = backend.get_texture_gpu_index(_pfd[i].ao_texture, 0);
		}
		_config.size = size;
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
			SFG_ASSERT(!_pfd[i].ao_texture.is_null());

			backend.destroy_texture(_pfd[i].lighting_texture);
			backend.destroy_texture(_pfd[i].post_process_texture);
			backend.destroy_texture(_pfd[i].depth_texture);
			backend.destroy_texture(_pfd[i].gbuffer_albedo);
			backend.destroy_texture(_pfd[i].gbuffer_normal);
			backend.destroy_texture(_pfd[i].gbuffer_orm);
			backend.destroy_texture(_pfd[i].gbuffer_emissive);
			backend.destroy_texture(_pfd[i].ao_texture);
			_pfd[i].lighting_texture		   = {};
			_pfd[i].post_process_texture	   = {};
			_pfd[i].depth_texture			   = {};
			_pfd[i].gbuffer_albedo			   = {};
			_pfd[i].gbuffer_normal			   = {};
			_pfd[i].gbuffer_orm				   = {};
			_pfd[i].gbuffer_emissive		   = {};
			_pfd[i].ao_texture				   = {};
			_pfd[i].lighting_texture_index	   = NULL_GPU_INDEX;
			_pfd[i].post_process_texture_index = NULL_GPU_INDEX;
			_pfd[i].depth_texture_index		   = NULL_GPU_INDEX;
			_pfd[i].gbuffer_albedo_index	   = NULL_GPU_INDEX;
			_pfd[i].gbuffer_normal_index	   = NULL_GPU_INDEX;
			_pfd[i].gbuffer_orm_index		   = NULL_GPU_INDEX;
			_pfd[i].gbuffer_emissive_index	   = NULL_GPU_INDEX;
			_pfd[i].ao_texture_index		   = NULL_GPU_INDEX;
		}
		_config.size = vec2u16_t::zero;
	}
}

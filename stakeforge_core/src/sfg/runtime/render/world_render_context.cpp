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
#include "world_gpu_bone.hpp"
#include "world_gpu_entity.hpp"
#include "world_gpu_light.hpp"
#include "world_draw.hpp"
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/gfx/util/render_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>
#include <sfg/runtime/render/render_resources.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/resources/shader.hpp>
#include <sfg/runtime/resources/vertex.hpp>
#include <sfg/runtime/ui/ui_renderer.hpp>
#include <random>

namespace sfg
{
	world_render_context_t::world_render_context_t() = default;

	world_render_context_t::~world_render_context_t() = default;

	world_render_context_t::world_render_context_t(world_render_context_t&& other) noexcept
	{
		*this = static_cast<world_render_context_t&&>(other);
	}

	world_render_context_t& world_render_context_t::operator=(world_render_context_t&& other) noexcept
	{
		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			_pfd[i]		  = other._pfd[i];
			other._pfd[i] = {};
		}

		_shaders						= other._shaders;
		other._shaders					= {};
		_config							= other._config;
		other._config					= {};
		_ssao_noise_texture				= other._ssao_noise_texture;
		other._ssao_noise_texture		= {};
		_ssao_noise_staging				= other._ssao_noise_staging;
		other._ssao_noise_staging		= {};
		_ssao_noise_texture_index		= other._ssao_noise_texture_index;
		other._ssao_noise_texture_index = NULL_GPU_INDEX;
		_post_process_hdr_scratch		= other._post_process_hdr_scratch;
		other._post_process_hdr_scratch = 0;
		_post_process_ldr_scratch		= other._post_process_ldr_scratch;
		other._post_process_ldr_scratch = 0;
		_canvas_before_renderer			= std::move(other._canvas_before_renderer);
		_canvas_after_renderer			= std::move(other._canvas_after_renderer);

		_shadow_context		= static_cast<world_render_shadow_context_t&&>(other._shadow_context);
		_reflection_context = static_cast<world_render_reflection_context_t&&>(other._reflection_context);

		return *this;
	}

	void world_render_context_t::init(const world_render_context_config_t& config)
	{
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());
		SFG_ASSERT((config.line_vertex_max == 0) == (config.line_index_max == 0));
		SFG_ASSERT((config.triangle_vertex_max == 0) == (config.triangle_index_max == 0));
		SFG_ASSERT((config.text_vertex_max == 0) == (config.text_index_max == 0));
		SFG_ASSERT(config.entity_max > 0);
		SFG_ASSERT(config.bone_max > 0);
		SFG_ASSERT(config.reflection_probe_max <= WORLD_RENDER_REFLECTION_ALLOCATION_CAPACITY);

		_config = config;
		render_util_t::ensure_world_resolution(_config.size);

		resource_desc_t view_render_pass_data_desc = {};
		view_render_pass_data_desc.size			   = static_cast<u32>(sizeof(render_pass_data_view_gpu_t));
		view_render_pass_data_desc.flags		   = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible;
		view_render_pass_data_desc.set_name("world_view_render_pass_data");

		resource_desc_t lighting_render_pass_data_desc = {};
		lighting_render_pass_data_desc.size			   = static_cast<u32>(sizeof(render_pass_data_lighting_gpu_t));
		lighting_render_pass_data_desc.flags		   = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible;
		lighting_render_pass_data_desc.set_name("world_lighting_render_pass_data");

		resource_desc_t fog_render_pass_data_desc = {};
		fog_render_pass_data_desc.size			  = static_cast<u32>(sizeof(render_pass_data_fog_gpu_t));
		fog_render_pass_data_desc.flags			  = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible;
		fog_render_pass_data_desc.set_name("world_fog_render_pass_data");

		resource_desc_t deferred_lighting_render_pass_data_desc = {};
		deferred_lighting_render_pass_data_desc.size			= static_cast<u32>(sizeof(render_pass_data_deferred_lighting_gpu_t));
		deferred_lighting_render_pass_data_desc.flags			= resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible;
		deferred_lighting_render_pass_data_desc.set_name("world_deferred_lighting_render_pass_data");

		resource_desc_t post_process_render_pass_data_desc = {};
		post_process_render_pass_data_desc.size			   = static_cast<u32>(sizeof(render_pass_data_post_process_gpu_t));
		post_process_render_pass_data_desc.flags		   = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible;
		post_process_render_pass_data_desc.set_name("world_post_process_render_pass_data");

		resource_desc_t ssao_render_pass_data_desc = {};
		ssao_render_pass_data_desc.size			   = static_cast<u32>(sizeof(render_pass_data_ssao_gpu_t));
		ssao_render_pass_data_desc.flags		   = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible;
		ssao_render_pass_data_desc.set_name("world_ssao_render_pass_data");

		resource_desc_t bloom_render_pass_data_desc = {};
		bloom_render_pass_data_desc.size			= static_cast<u32>(sizeof(render_pass_data_bloom_gpu_t));
		bloom_render_pass_data_desc.flags			= resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible;
		bloom_render_pass_data_desc.set_name("world_bloom_render_pass_data");

		resource_desc_t entity_buffer_desc = {};
		entity_buffer_desc.size			   = static_cast<u32>(sizeof(gpu_entity_t) * config.entity_max);
		entity_buffer_desc.structure_size  = static_cast<u32>(sizeof(gpu_entity_t));
		entity_buffer_desc.structure_count = config.entity_max;
		entity_buffer_desc.flags		   = resource_flags::rf_storage_buffer | resource_flags::rf_cpu_visible;
		entity_buffer_desc.set_name("world_entity_buffer");

		resource_desc_t sprite_instance_buffer_desc = {};
		const u32		sprite_instance_count		= get_sprite_instance_max();
		sprite_instance_buffer_desc.size			= static_cast<u32>(sizeof(world_draw_sprite_instance_gpu_t) * sprite_instance_count);
		sprite_instance_buffer_desc.structure_size	= static_cast<u32>(sizeof(world_draw_sprite_instance_gpu_t));
		sprite_instance_buffer_desc.structure_count = sprite_instance_count;
		sprite_instance_buffer_desc.flags			= resource_flags::rf_storage_buffer | resource_flags::rf_cpu_visible;
		sprite_instance_buffer_desc.set_name("world_sprite_instance_buffer");

		resource_desc_t particle_instance_buffer_desc = {};
		const u32		particle_instance_count		  = get_particle_instance_max();
		particle_instance_buffer_desc.size			  = static_cast<u32>(sizeof(world_draw_particle_instance_gpu_t) * particle_instance_count);
		particle_instance_buffer_desc.structure_size  = static_cast<u32>(sizeof(world_draw_particle_instance_gpu_t));
		particle_instance_buffer_desc.structure_count = particle_instance_count;
		particle_instance_buffer_desc.flags			  = resource_flags::rf_storage_buffer | resource_flags::rf_cpu_visible;
		particle_instance_buffer_desc.set_name("world_particle_instance_buffer");

		resource_desc_t debug_texture_buffer_desc = {};
		debug_texture_buffer_desc.size			  = static_cast<u32>(sizeof(world_debug_draw_texture_gpu_t) * config.debug_texture_max);
		debug_texture_buffer_desc.structure_size  = static_cast<u32>(sizeof(world_debug_draw_texture_gpu_t));
		debug_texture_buffer_desc.structure_count = config.debug_texture_max;
		debug_texture_buffer_desc.flags			  = resource_flags::rf_storage_buffer | resource_flags::rf_cpu_visible;
		debug_texture_buffer_desc.set_name("world_debug_texture_buffer");

		resource_desc_t bone_buffer_desc = {};
		bone_buffer_desc.size			 = static_cast<u32>(sizeof(gpu_bone_t) * config.bone_max);
		bone_buffer_desc.structure_size	 = static_cast<u32>(sizeof(gpu_bone_t));
		bone_buffer_desc.structure_count = config.bone_max;
		bone_buffer_desc.flags			 = resource_flags::rf_storage_buffer | resource_flags::rf_cpu_visible;
		bone_buffer_desc.set_name("world_bone_buffer");

		resource_desc_t light_buffer_desc  = {};
		const u32		light_buffer_count = config.light_max == 0 ? 1 : config.light_max;
		light_buffer_desc.size			   = static_cast<u32>(sizeof(gpu_light_t) * light_buffer_count);
		light_buffer_desc.structure_size   = static_cast<u32>(sizeof(gpu_light_t));
		light_buffer_desc.structure_count  = light_buffer_count;
		light_buffer_desc.flags			   = resource_flags::rf_storage_buffer | resource_flags::rf_cpu_visible;
		light_buffer_desc.set_name("world_light_buffer");

		resource_desc_t debug_line_data_desc = {};
		debug_line_data_desc.size			 = static_cast<u32>(sizeof(render_pass_data_debug_line_gpu_t));
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

		resource_desc_t debug_triangle_vertex_desc = {};
		debug_triangle_vertex_desc.size			   = config.triangle_vertex_max * static_cast<u32>(sizeof(vertex_debug_triangle_t));
		debug_triangle_vertex_desc.flags		   = resource_flags::rf_vertex_buffer | resource_flags::rf_cpu_visible;
		debug_triangle_vertex_desc.set_name("world_debug_triangle_vertices");

		resource_desc_t debug_triangle_index_desc = {};
		debug_triangle_index_desc.size			  = config.triangle_index_max * static_cast<u32>(sizeof(primitive_index));
		debug_triangle_index_desc.flags			  = resource_flags::rf_index_buffer | resource_flags::rf_cpu_visible;
		debug_triangle_index_desc.set_name("world_debug_triangle_indices");

		resource_desc_t debug_text_data_desc = {};
		debug_text_data_desc.size			 = static_cast<u32>(sizeof(render_pass_data_debug_text_gpu_t));
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
		if (config.enable_ssao != 0)
		{
			const vec2u16_t						noise_size		= {8, 8};
			const u32							noise_row_pitch = 16;
			const u32							noise_data_size = noise_row_pitch * noise_size.y;
			u8*									noise_data		= static_cast<u8*>(SFG_MALLOC(noise_data_size));
			std::mt19937						random(1337);
			std::uniform_real_distribution<f32> distribution(0.0f, 1.0f);
			for (u32 y = 0; y < noise_size.y; ++y)
			{
				for (u32 x = 0; x < noise_size.x; ++x)
				{
					const f32 theta		   = distribution(random) * 6.28318530718f;
					const u32 offset	   = (y * noise_size.x + x) * 2;
					noise_data[offset]	   = static_cast<u8>(std::round((std::cos(theta) * 0.5f + 0.5f) * 255.0f));
					noise_data[offset + 1] = static_cast<u8>(std::round((std::sin(theta) * 0.5f + 0.5f) * 255.0f));
				}
			}

			texture_desc_t noise_texture_desc = {};
			noise_texture_desc.texture_format = format_e::r8g8_unorm;
			noise_texture_desc.size			  = noise_size;
			noise_texture_desc.flags		  = texture_flags::tf_is_2d | texture_flags::tf_sampled | texture_flags::tf_transfer_dest;
			noise_texture_desc.view_count	  = 1;
			noise_texture_desc.views[0]		  = {.type = view_type::sampled};
			noise_texture_desc.set_name("world_ssao_noise");

			resource_desc_t noise_staging_desc = {};
			noise_staging_desc.size			   = gfx_backend::align_texture_size(gfx_backend::align_texture_size_pitch(noise_row_pitch) * noise_size.y);
			noise_staging_desc.flags		   = resource_flags::rf_cpu_visible;
			noise_staging_desc.set_name("world_ssao_noise_staging");

			_ssao_noise_texture		  = backend.create_texture(noise_texture_desc);
			_ssao_noise_staging		  = backend.create_resource(noise_staging_desc);
			_ssao_noise_texture_index = backend.get_texture_gpu_index(_ssao_noise_texture, 0);

			const texture_buffer_t noise_buffer = {
				.pixels	   = noise_data,
				.data_size = noise_data_size,
				.row_pitch = noise_row_pitch,
				.size	   = noise_size,
				.bpp	   = 2,
			};
			render_resources_t::get().get_texture_upload_queue().add({
				.texture		   = _ssao_noise_texture,
				.staging		   = _ssao_noise_staging,
				.mips			   = {.data = &noise_buffer, .size = 1},
				.target_states	   = resource_state_non_ps_resource,
				.destination_slice = 0,
				.ownership		   = texture_data_ownership_e::c_free,
			});
		}

		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			SFG_ASSERT(_pfd[i].cmd_depth.is_null());
			SFG_ASSERT(_pfd[i].cmd_gbuffer.is_null());
			SFG_ASSERT(_pfd[i].cmd_lighting.is_null());
			SFG_ASSERT(_pfd[i].cmd_forward.is_null());
			SFG_ASSERT(_pfd[i].cmd_post.is_null());
			SFG_ASSERT(_pfd[i].cmd_clustered_lighting.is_null());
			SFG_ASSERT(_pfd[i].lighting_texture.is_null());
			SFG_ASSERT(_pfd[i].post_process_texture.is_null());
			SFG_ASSERT(_pfd[i].post_process_hdr_scratch.is_null());
			SFG_ASSERT(_pfd[i].post_process_ldr_scratch.is_null());
			SFG_ASSERT(_pfd[i].depth_texture.is_null());
			SFG_ASSERT(_pfd[i].gbuffer_albedo.is_null());
			SFG_ASSERT(_pfd[i].gbuffer_normal.is_null());
			SFG_ASSERT(_pfd[i].gbuffer_orm.is_null());
			SFG_ASSERT(_pfd[i].gbuffer_emissive.is_null());
			SFG_ASSERT(_pfd[i].ao_texture.is_null());
			SFG_ASSERT(_pfd[i].ssao_semaphore.is_null());
			SFG_ASSERT(_pfd[i].bloom_semaphore.is_null());
			SFG_ASSERT(_pfd[i].clustered_lighting_semaphore.is_null());
			SFG_ASSERT(_pfd[i].view_render_pass_data.is_null());
			SFG_ASSERT(_pfd[i].lighting_render_pass_data.is_null());
			SFG_ASSERT(_pfd[i].fog_render_pass_data.is_null());
			SFG_ASSERT(_pfd[i].deferred_lighting_render_pass_data.is_null());
			SFG_ASSERT(_pfd[i].post_process_render_pass_data.is_null());
			SFG_ASSERT(_pfd[i].entity_buffer.is_null());
			SFG_ASSERT(_pfd[i].sprite_instance_buffer.is_null());
			SFG_ASSERT(_pfd[i].debug_texture_buffer.is_null());
			SFG_ASSERT(_pfd[i].bone_buffer.is_null());
			SFG_ASSERT(_pfd[i].light_buffer.is_null());

			_pfd[i].cmd_depth					 = backend.create_command_buffer({
				.type		= command_type::graphics,
				.debug_name = "world_depth",
			});
			_pfd[i].cmd_gbuffer					 = backend.create_command_buffer({
				.type		= command_type::graphics,
				.debug_name = "world_gbuffer",
			});
			_pfd[i].cmd_lighting				 = backend.create_command_buffer({
				.type		= command_type::graphics,
				.debug_name = "world_lighting",
			});
			_pfd[i].cmd_forward					 = backend.create_command_buffer({
				.type		= command_type::graphics,
				.debug_name = "world_forward",
			});
			_pfd[i].cmd_post					 = backend.create_command_buffer({
				.type		= command_type::graphics,
				.debug_name = "world_post",
			});
			_pfd[i].cmd_clustered_lighting		 = backend.create_command_buffer({
				.type		= command_type::compute,
				.debug_name = "world_clusters",
			});
			_pfd[i].clustered_lighting_semaphore = backend.create_semaphore();

			_pfd[i].view_render_pass_data			   = backend.create_resource(view_render_pass_data_desc);
			_pfd[i].lighting_render_pass_data		   = backend.create_resource(lighting_render_pass_data_desc);
			_pfd[i].fog_render_pass_data			   = backend.create_resource(fog_render_pass_data_desc);
			_pfd[i].deferred_lighting_render_pass_data = backend.create_resource(deferred_lighting_render_pass_data_desc);
			_pfd[i].post_process_render_pass_data	   = backend.create_resource(post_process_render_pass_data_desc);
			_pfd[i].entity_buffer					   = backend.create_resource(entity_buffer_desc);
			_pfd[i].sprite_instance_buffer			   = backend.create_resource(sprite_instance_buffer_desc);
			_pfd[i].particle_instance_buffer		   = backend.create_resource(particle_instance_buffer_desc);
			_pfd[i].bone_buffer						   = backend.create_resource(bone_buffer_desc);
			_pfd[i].light_buffer					   = backend.create_resource(light_buffer_desc);

			if (config.debug_texture_max > 0)
			{
				_pfd[i].debug_texture_buffer = backend.create_resource(debug_texture_buffer_desc);
				backend.map_resource(_pfd[i].debug_texture_buffer, _pfd[i].mapped_debug_texture_buffer);
				_pfd[i].debug_texture_buffer_index = backend.get_resource_gpu_index(_pfd[i].debug_texture_buffer);
			}

			if (config.enable_ssao != 0)
			{
				_pfd[i].cmd_ssao			  = backend.create_command_buffer({.type = command_type::compute, .debug_name = "world_ssao"});
				_pfd[i].ssao_semaphore		  = backend.create_semaphore();
				_pfd[i].ssao_render_pass_data = backend.create_resource(ssao_render_pass_data_desc);
				backend.map_resource(_pfd[i].ssao_render_pass_data, _pfd[i].mapped_ssao_render_pass_data);
				_pfd[i].ssao_render_pass_data_index = backend.get_resource_gpu_index(_pfd[i].ssao_render_pass_data);
			}

			if (config.enable_bloom != 0)
			{
				_pfd[i].cmd_bloom			   = backend.create_command_buffer({.type = command_type::compute, .debug_name = "world_bloom"});
				_pfd[i].bloom_semaphore		   = backend.create_semaphore();
				_pfd[i].bloom_render_pass_data = backend.create_resource(bloom_render_pass_data_desc);
				backend.map_resource(_pfd[i].bloom_render_pass_data, _pfd[i].mapped_bloom_render_pass_data);
				_pfd[i].bloom_render_pass_data_index = backend.get_resource_gpu_index(_pfd[i].bloom_render_pass_data);
			}

			backend.map_resource(_pfd[i].view_render_pass_data, _pfd[i].mapped_view_render_pass_data);
			backend.map_resource(_pfd[i].lighting_render_pass_data, _pfd[i].mapped_lighting_render_pass_data);
			backend.map_resource(_pfd[i].fog_render_pass_data, _pfd[i].mapped_fog_render_pass_data);
			backend.map_resource(_pfd[i].deferred_lighting_render_pass_data, _pfd[i].mapped_deferred_lighting_render_pass_data);
			backend.map_resource(_pfd[i].post_process_render_pass_data, _pfd[i].mapped_post_process_render_pass_data);
			backend.map_resource(_pfd[i].entity_buffer, _pfd[i].mapped_entity_buffer);
			backend.map_resource(_pfd[i].sprite_instance_buffer, _pfd[i].mapped_sprite_instance_buffer);
			backend.map_resource(_pfd[i].particle_instance_buffer, _pfd[i].mapped_particle_instance_buffer);
			backend.map_resource(_pfd[i].bone_buffer, _pfd[i].mapped_bone_buffer);
			backend.map_resource(_pfd[i].light_buffer, _pfd[i].mapped_light_buffer);

			_pfd[i].view_render_pass_data_index				 = backend.get_resource_gpu_index(_pfd[i].view_render_pass_data);
			_pfd[i].lighting_render_pass_data_index			 = backend.get_resource_gpu_index(_pfd[i].lighting_render_pass_data);
			_pfd[i].fog_render_pass_data_index				 = backend.get_resource_gpu_index(_pfd[i].fog_render_pass_data);
			_pfd[i].deferred_lighting_render_pass_data_index = backend.get_resource_gpu_index(_pfd[i].deferred_lighting_render_pass_data);
			_pfd[i].post_process_render_pass_data_index		 = backend.get_resource_gpu_index(_pfd[i].post_process_render_pass_data);
			_pfd[i].entity_buffer_index						 = backend.get_resource_gpu_index(_pfd[i].entity_buffer);
			_pfd[i].sprite_instance_buffer_index			 = backend.get_resource_gpu_index(_pfd[i].sprite_instance_buffer);
			_pfd[i].particle_instance_buffer_index			 = backend.get_resource_gpu_index(_pfd[i].particle_instance_buffer);
			_pfd[i].bone_buffer_index						 = backend.get_resource_gpu_index(_pfd[i].bone_buffer);
			_pfd[i].light_buffer_index						 = backend.get_resource_gpu_index(_pfd[i].light_buffer);

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

			if (config.triangle_vertex_max > 0)
			{
				_pfd[i].debug_triangle_vertex_buffer = backend.create_resource(debug_triangle_vertex_desc);
				_pfd[i].debug_triangle_index_buffer	 = backend.create_resource(debug_triangle_index_desc);
				backend.map_resource(_pfd[i].debug_triangle_vertex_buffer, _pfd[i].mapped_debug_triangle_vertices);
				backend.map_resource(_pfd[i].debug_triangle_index_buffer, _pfd[i].mapped_debug_triangle_indices);
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

		_shadow_context.init({.view_max = config.shadow_view_max});
		_reflection_context.init({.allocation_max = static_cast<u16>(config.reflection_probe_max)});

		const render_resources_t& render_resources = render_resources_t::get();
		const shader_internals_t* sh			   = resource_manager_t::get().find_internals<shader_internals_t>("common/shaders/world/deferred_lighting.hlsl"_hs);
		_shaders.lighting						   = render_resources.get_shader_hw(sh->psos[0]);
		sh										   = resource_manager_t::get().find_internals<shader_internals_t>("common/shaders/world/post_combiner.hlsl"_hs);
		_shaders.post_combiner					   = render_resources.get_shader_hw(sh->psos[0]);
		sh										   = resource_manager_t::get().find_internals<shader_internals_t>("common/shaders/world/fxaa.hlsl"_hs);
		_shaders.fxaa							   = render_resources.get_shader_hw(sh->psos[0]);
		sh										   = resource_manager_t::get().find_internals<shader_internals_t>("common/shaders/world/debug_line.hlsl"_hs);
		_shaders.debug_line						   = render_resources.get_shader_hw(sh->psos[0]);
		sh										   = resource_manager_t::get().find_internals<shader_internals_t>("common/shaders/world/debug_text.hlsl"_hs);
		_shaders.debug_text						   = render_resources.get_shader_hw(sh->psos[0]);
		sh										   = resource_manager_t::get().find_internals<shader_internals_t>("common/shaders/world/debug_texture.hlsl"_hs);
		_shaders.debug_texture					   = render_resources.get_shader_hw(sh->psos[0]);
		_shaders.debug_texture_id				   = render_resources.get_shader_hw(sh->psos[1]);
		sh										   = resource_manager_t::get().find_internals<shader_internals_t>("common/shaders/world/clustered_light_culling.hlsl"_hs);
		_shaders.clustered_light_culling		   = render_resources.get_shader_hw(sh->psos[0]);

		if (config.enable_ssao != 0)
		{
			sh					   = resource_manager_t::get().find_internals<shader_internals_t>("common/shaders/world/ssao.hlsl"_hs);
			_shaders.ssao		   = render_resources.get_shader_hw(sh->psos[0]);
			sh					   = resource_manager_t::get().find_internals<shader_internals_t>("common/shaders/world/ssao_upsample.hlsl"_hs);
			_shaders.ssao_upsample = render_resources.get_shader_hw(sh->psos[0]);
		}

		if (config.enable_bloom != 0)
		{
			sh						  = resource_manager_t::get().find_internals<shader_internals_t>("common/shaders/world/bloom_downsample.hlsl"_hs);
			_shaders.bloom_downsample = render_resources.get_shader_hw(sh->psos[0]);
			sh						  = resource_manager_t::get().find_internals<shader_internals_t>("common/shaders/world/bloom_upsample.hlsl"_hs);
			_shaders.bloom_upsample	  = render_resources.get_shader_hw(sh->psos[0]);
		}

		_canvas_before_renderer = make_unique<ui::ui_renderer_t>();
		_canvas_before_renderer->init({
			.vertex_buffer_max_bytes = config.canvas_vertex_max_bytes,
			.index_buffer_max_bytes	 = config.canvas_index_max_bytes,
		});
		_canvas_after_renderer = make_unique<ui::ui_renderer_t>();
		_canvas_after_renderer->init({
			.vertex_buffer_max_bytes = config.canvas_vertex_max_bytes,
			.index_buffer_max_bytes	 = config.canvas_index_max_bytes,
		});

		create_texture(_config.size);
	}

	void world_render_context_t::uninit()
	{
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());

		_canvas_before_renderer->uninit();
		_canvas_before_renderer.reset();
		_canvas_after_renderer->uninit();
		_canvas_after_renderer.reset();

		destroy_texture();
		_shadow_context.uninit();
		_reflection_context.uninit();

		gfx_backend& backend = gfx_backend::get();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			SFG_ASSERT(!_pfd[i].view_render_pass_data.is_null());
			SFG_ASSERT(!_pfd[i].lighting_render_pass_data.is_null());
			SFG_ASSERT(!_pfd[i].fog_render_pass_data.is_null());
			SFG_ASSERT(!_pfd[i].deferred_lighting_render_pass_data.is_null());
			SFG_ASSERT(!_pfd[i].post_process_render_pass_data.is_null());
			SFG_ASSERT(!_pfd[i].entity_buffer.is_null());
			SFG_ASSERT(!_pfd[i].sprite_instance_buffer.is_null());
			SFG_ASSERT(!_pfd[i].particle_instance_buffer.is_null());
			SFG_ASSERT(!_pfd[i].bone_buffer.is_null());
			SFG_ASSERT(!_pfd[i].light_buffer.is_null());
			SFG_ASSERT(!_pfd[i].cmd_clustered_lighting.is_null());
			SFG_ASSERT(!_pfd[i].clustered_lighting_semaphore.is_null());

			backend.destroy_resource(_pfd[i].view_render_pass_data);
			backend.destroy_resource(_pfd[i].lighting_render_pass_data);
			backend.destroy_resource(_pfd[i].fog_render_pass_data);
			backend.destroy_resource(_pfd[i].deferred_lighting_render_pass_data);
			backend.destroy_resource(_pfd[i].post_process_render_pass_data);
			backend.destroy_resource(_pfd[i].entity_buffer);
			backend.destroy_resource(_pfd[i].sprite_instance_buffer);
			backend.destroy_resource(_pfd[i].particle_instance_buffer);
			backend.destroy_resource(_pfd[i].bone_buffer);
			backend.destroy_resource(_pfd[i].light_buffer);

			if (_config.enable_ssao != 0)
			{
				backend.destroy_resource(_pfd[i].ssao_render_pass_data);
				backend.destroy_command_buffer(_pfd[i].cmd_ssao);
				backend.destroy_semaphore(_pfd[i].ssao_semaphore);
			}

			if (_config.enable_bloom != 0)
			{
				backend.destroy_resource(_pfd[i].bloom_render_pass_data);
				backend.destroy_command_buffer(_pfd[i].cmd_bloom);
				backend.destroy_semaphore(_pfd[i].bloom_semaphore);
			}

			if (_config.line_vertex_max > 0)
			{
				backend.destroy_resource(_pfd[i].debug_line_data);
				backend.destroy_resource(_pfd[i].debug_line_vertex_buffer);
				backend.destroy_resource(_pfd[i].debug_line_index_buffer);
			}

			if (_config.triangle_vertex_max > 0)
			{
				backend.destroy_resource(_pfd[i].debug_triangle_vertex_buffer);
				backend.destroy_resource(_pfd[i].debug_triangle_index_buffer);
			}

			if (_config.text_vertex_max > 0)
			{
				backend.destroy_resource(_pfd[i].debug_text_data);
				backend.destroy_resource(_pfd[i].debug_text_vertex_buffer);
				backend.destroy_resource(_pfd[i].debug_text_index_buffer);
			}

			if (_config.debug_texture_max > 0)
				backend.destroy_resource(_pfd[i].debug_texture_buffer);

			backend.destroy_command_buffer(_pfd[i].cmd_depth);
			backend.destroy_command_buffer(_pfd[i].cmd_gbuffer);
			backend.destroy_command_buffer(_pfd[i].cmd_lighting);
			backend.destroy_command_buffer(_pfd[i].cmd_forward);
			backend.destroy_command_buffer(_pfd[i].cmd_post);
			backend.destroy_command_buffer(_pfd[i].cmd_clustered_lighting);
			backend.destroy_semaphore(_pfd[i].clustered_lighting_semaphore);

			_pfd[i].view_render_pass_data					  = {};
			_pfd[i].lighting_render_pass_data				  = {};
			_pfd[i].fog_render_pass_data					  = {};
			_pfd[i].deferred_lighting_render_pass_data		  = {};
			_pfd[i].post_process_render_pass_data			  = {};
			_pfd[i].entity_buffer							  = {};
			_pfd[i].sprite_instance_buffer					  = {};
			_pfd[i].particle_instance_buffer				  = {};
			_pfd[i].debug_texture_buffer					  = {};
			_pfd[i].bone_buffer								  = {};
			_pfd[i].light_buffer							  = {};
			_pfd[i].debug_line_data							  = {};
			_pfd[i].debug_line_vertex_buffer				  = {};
			_pfd[i].debug_line_index_buffer					  = {};
			_pfd[i].debug_text_data							  = {};
			_pfd[i].debug_text_vertex_buffer				  = {};
			_pfd[i].debug_text_index_buffer					  = {};
			_pfd[i].mapped_view_render_pass_data			  = nullptr;
			_pfd[i].mapped_lighting_render_pass_data		  = nullptr;
			_pfd[i].mapped_fog_render_pass_data				  = nullptr;
			_pfd[i].mapped_deferred_lighting_render_pass_data = nullptr;
			_pfd[i].mapped_post_process_render_pass_data	  = nullptr;
			_pfd[i].mapped_entity_buffer					  = nullptr;
			_pfd[i].mapped_sprite_instance_buffer			  = nullptr;
			_pfd[i].mapped_particle_instance_buffer			  = nullptr;
			_pfd[i].mapped_debug_texture_buffer				  = nullptr;
			_pfd[i].mapped_bone_buffer						  = nullptr;
			_pfd[i].mapped_light_buffer						  = nullptr;
			_pfd[i].mapped_debug_line_data					  = nullptr;
			_pfd[i].mapped_debug_line_vertices				  = nullptr;
			_pfd[i].mapped_debug_line_indices				  = nullptr;
			_pfd[i].mapped_debug_text_data					  = nullptr;
			_pfd[i].mapped_debug_text_vertices				  = nullptr;
			_pfd[i].mapped_debug_text_indices				  = nullptr;
			_pfd[i].view_render_pass_data_index				  = NULL_GPU_INDEX;
			_pfd[i].lighting_render_pass_data_index			  = NULL_GPU_INDEX;
			_pfd[i].fog_render_pass_data_index				  = NULL_GPU_INDEX;
			_pfd[i].deferred_lighting_render_pass_data_index  = NULL_GPU_INDEX;
			_pfd[i].post_process_render_pass_data_index		  = NULL_GPU_INDEX;
			_pfd[i].entity_buffer_index						  = NULL_GPU_INDEX;
			_pfd[i].sprite_instance_buffer_index			  = NULL_GPU_INDEX;
			_pfd[i].particle_instance_buffer_index			  = NULL_GPU_INDEX;
			_pfd[i].debug_texture_buffer_index				  = NULL_GPU_INDEX;
			_pfd[i].bone_buffer_index						  = NULL_GPU_INDEX;
			_pfd[i].light_buffer_index						  = NULL_GPU_INDEX;
			_pfd[i].debug_line_data_index					  = NULL_GPU_INDEX;
			_pfd[i].debug_text_data_index					  = NULL_GPU_INDEX;
			_pfd[i]											  = {};
		}

		if (_config.enable_ssao != 0)
		{
			backend.destroy_texture(_ssao_noise_texture);
			backend.destroy_resource(_ssao_noise_staging);
			_ssao_noise_texture		  = {};
			_ssao_noise_staging		  = {};
			_ssao_noise_texture_index = NULL_GPU_INDEX;
		}

		_shaders				  = {};
		_config					  = {};
		_post_process_hdr_scratch = 0;
		_post_process_ldr_scratch = 0;
	}

	void world_render_context_t::resize(vec2u16_t size)
	{
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());
		render_util_t::ensure_world_resolution(size);

		destroy_texture();
		create_texture(size);
	}

	void world_render_context_t::create_light_cluster_buffers(u8 frame_index, u32 cluster_count)
	{
		per_frame_data_t& frame_data = _pfd[frame_index];

		SFG_ASSERT(frame_data.light_cluster_buffer.is_null());
		SFG_ASSERT(frame_data.light_cluster_indices_buffer.is_null());

		resource_desc_t light_cluster_buffer_desc = {};
		light_cluster_buffer_desc.size			  = static_cast<u32>(sizeof(gpu_light_cluster_t) * cluster_count);
		light_cluster_buffer_desc.structure_size  = static_cast<u32>(sizeof(gpu_light_cluster_t));
		light_cluster_buffer_desc.structure_count = cluster_count;
		light_cluster_buffer_desc.flags			  = resource_flags::rf_storage_buffer | resource_flags::rf_gpu_only | resource_flags::rf_gpu_write;
		light_cluster_buffer_desc.set_name("world_light_clusters");

		resource_desc_t light_cluster_indices_buffer_desc = {};
		light_cluster_indices_buffer_desc.size			  = static_cast<u32>(sizeof(u32) * cluster_count * WORLD_RENDER_CLUSTER_LIGHT_CAPACITY);
		light_cluster_indices_buffer_desc.structure_size  = static_cast<u32>(sizeof(u32));
		light_cluster_indices_buffer_desc.structure_count = cluster_count * WORLD_RENDER_CLUSTER_LIGHT_CAPACITY;
		light_cluster_indices_buffer_desc.flags			  = resource_flags::rf_storage_buffer | resource_flags::rf_gpu_only | resource_flags::rf_gpu_write;
		light_cluster_indices_buffer_desc.set_name("world_light_cluster_indices");

		gfx_backend& backend = gfx_backend::get();

		frame_data.light_cluster_buffer					  = backend.create_resource(light_cluster_buffer_desc);
		frame_data.light_cluster_indices_buffer			  = backend.create_resource(light_cluster_indices_buffer_desc);
		frame_data.light_cluster_buffer_index			  = backend.get_resource_gpu_index(frame_data.light_cluster_buffer);
		frame_data.light_cluster_buffer_uav_index		  = backend.get_resource_gpu_index(frame_data.light_cluster_buffer, true);
		frame_data.light_cluster_indices_buffer_index	  = backend.get_resource_gpu_index(frame_data.light_cluster_indices_buffer);
		frame_data.light_cluster_indices_buffer_uav_index = backend.get_resource_gpu_index(frame_data.light_cluster_indices_buffer, true);
		frame_data.light_cluster_capacity				  = cluster_count;
	}

	void world_render_context_t::destroy_light_cluster_buffers(u8 frame_index)
	{
		per_frame_data_t& frame_data = _pfd[frame_index];

		SFG_ASSERT(!frame_data.light_cluster_buffer.is_null());
		SFG_ASSERT(!frame_data.light_cluster_indices_buffer.is_null());

		gfx_backend& backend = gfx_backend::get();

		backend.destroy_resource(frame_data.light_cluster_buffer);
		backend.destroy_resource(frame_data.light_cluster_indices_buffer);

		frame_data.light_cluster_buffer					  = {};
		frame_data.light_cluster_indices_buffer			  = {};
		frame_data.light_cluster_buffer_index			  = NULL_GPU_INDEX;
		frame_data.light_cluster_buffer_uav_index		  = NULL_GPU_INDEX;
		frame_data.light_cluster_indices_buffer_index	  = NULL_GPU_INDEX;
		frame_data.light_cluster_indices_buffer_uav_index = NULL_GPU_INDEX;
		frame_data.light_cluster_capacity				  = 0;
	}

	void world_render_context_t::ensure_light_cluster_capacity(u8 frame_index, u32 cluster_count)
	{
		per_frame_data_t& frame_data = _pfd[frame_index];

		if (frame_data.light_cluster_capacity >= cluster_count)
			return;

		const u32 grown_capacity = std::max(cluster_count, frame_data.light_cluster_capacity * 2);

		destroy_light_cluster_buffers(frame_index);
		create_light_cluster_buffers(frame_index, grown_capacity);
	}

	void world_render_context_t::ensure_post_process_scratch(bool hdr, bool ldr)
	{
		const bool create_hdr = hdr && _post_process_hdr_scratch == 0;
		const bool create_ldr = ldr && _post_process_ldr_scratch == 0;

		if (!create_hdr && !create_ldr)
			return;

		create_post_process_scratch(_config.size, create_hdr, create_ldr);
		_post_process_hdr_scratch |= create_hdr ? 1 : 0;
		_post_process_ldr_scratch |= create_ldr ? 1 : 0;
	}

	void world_render_context_t::create_post_process_scratch(vec2u16_t size, bool hdr, bool ldr)
	{
		texture_desc_t hdr_desc = {};
		hdr_desc.texture_format = format_e::r16g16b16a16_sfloat;
		hdr_desc.initial_states = resource_state_ps_resource;
		hdr_desc.size			= size;
		hdr_desc.flags			= texture_flags::tf_render_target | texture_flags::tf_sampled | texture_flags::tf_is_2d;
		hdr_desc.view_count		= 2;
		hdr_desc.views[0]		= {.type = view_type::sampled};
		hdr_desc.views[1]		= {.type = view_type::render_target};
		hdr_desc.set_name("world_post_process_hdr_scratch");

		texture_desc_t ldr_desc = hdr_desc;
		ldr_desc.texture_format = format_e::r8g8b8a8_srgb;
		ldr_desc.set_name("world_post_process_ldr_scratch");

		gfx_backend& backend = gfx_backend::get();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			if (hdr)
			{
				SFG_ASSERT(_pfd[i].post_process_hdr_scratch.is_null());
				_pfd[i].post_process_hdr_scratch	   = backend.create_texture(hdr_desc);
				_pfd[i].post_process_hdr_scratch_index = backend.get_texture_gpu_index(_pfd[i].post_process_hdr_scratch, 0);
			}

			if (ldr)
			{
				SFG_ASSERT(_pfd[i].post_process_ldr_scratch.is_null());
				_pfd[i].post_process_ldr_scratch	   = backend.create_texture(ldr_desc);
				_pfd[i].post_process_ldr_scratch_index = backend.get_texture_gpu_index(_pfd[i].post_process_ldr_scratch, 0);
			}
		}
	}

	void world_render_context_t::create_texture(vec2u16_t size)
	{
		texture_desc_t lighting_desc = {};
		lighting_desc.texture_format = format_e::r16g16b16a16_sfloat;
		lighting_desc.initial_states = resource_state_ps_resource;
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
		depth_desc.initial_states		= resource_state_depth_read | resource_state_non_ps_resource | resource_state_ps_resource;
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
		gbuffer_albedo_desc.initial_states = resource_state_ps_resource;
		gbuffer_albedo_desc.size		   = size;
		gbuffer_albedo_desc.flags		   = texture_flags::tf_render_target | texture_flags::tf_sampled | texture_flags::tf_is_2d;
		gbuffer_albedo_desc.view_count	   = 2;
		gbuffer_albedo_desc.views[0]	   = {.type = view_type::render_target};
		gbuffer_albedo_desc.views[1]	   = {.type = view_type::sampled};
		gbuffer_albedo_desc.set_name("gbuffer_albedo");

		texture_desc_t gbuffer_normal_desc = {};
		gbuffer_normal_desc.texture_format = format_e::r10g0b10a2_unorm;
		gbuffer_normal_desc.initial_states = resource_state_non_ps_resource | resource_state_ps_resource;
		gbuffer_normal_desc.size		   = size;
		gbuffer_normal_desc.flags		   = texture_flags::tf_render_target | texture_flags::tf_sampled | texture_flags::tf_is_2d;
		gbuffer_normal_desc.view_count	   = 2;
		gbuffer_normal_desc.views[0]	   = {.type = view_type::render_target};
		gbuffer_normal_desc.views[1]	   = {.type = view_type::sampled};
		gbuffer_normal_desc.set_name("gbuffer_normal");

		texture_desc_t gbuffer_orm_desc = {};
		gbuffer_orm_desc.texture_format = format_e::r8g8b8a8_unorm;
		gbuffer_orm_desc.initial_states = resource_state_ps_resource;
		gbuffer_orm_desc.size			= size;
		gbuffer_orm_desc.flags			= texture_flags::tf_render_target | texture_flags::tf_sampled | texture_flags::tf_is_2d;
		gbuffer_orm_desc.view_count		= 2;
		gbuffer_orm_desc.views[0]		= {.type = view_type::render_target};
		gbuffer_orm_desc.views[1]		= {.type = view_type::sampled};
		gbuffer_orm_desc.set_name("gbuffer_orm");

		texture_desc_t gbuffer_emissive_desc = {};
		gbuffer_emissive_desc.texture_format = format_e::r16g16b16a16_sfloat;
		gbuffer_emissive_desc.initial_states = resource_state_ps_resource;
		gbuffer_emissive_desc.size			 = size;
		gbuffer_emissive_desc.flags			 = texture_flags::tf_render_target | texture_flags::tf_sampled | texture_flags::tf_is_2d;
		gbuffer_emissive_desc.view_count	 = 2;
		gbuffer_emissive_desc.views[0]		 = {.type = view_type::render_target};
		gbuffer_emissive_desc.views[1]		 = {.type = view_type::sampled};
		gbuffer_emissive_desc.set_name("gbuffer_emissive");

		texture_desc_t ao_desc	= {};
		ao_desc.texture_format	= format_e::r8_unorm;
		ao_desc.initial_states	= resource_state_non_ps_resource;
		ao_desc.size			= size;
		ao_desc.flags			= texture_flags::tf_sampled | texture_flags::tf_gpu_write | texture_flags::tf_is_2d;
		ao_desc.view_count		= 2;
		ao_desc.views[0]		= {.type = view_type::sampled};
		ao_desc.views[1]		= {.type = view_type::gpu_write};
		ao_desc.clear_values[0] = 1.0f;
		ao_desc.set_name("world_ao");

		texture_desc_t ao_half_desc = ao_desc;
		ao_half_desc.size			= {static_cast<u16>(size.x / 2), static_cast<u16>(size.y / 2)};
		ao_half_desc.set_name("world_ao_half");

		texture_desc_t bloom_downsample_desc = {};
		bloom_downsample_desc.texture_format = format_e::r16g16b16a16_sfloat;
		bloom_downsample_desc.initial_states = resource_state_non_ps_resource;
		bloom_downsample_desc.size			 = {
			static_cast<u16>(std::max<u32>(1, size.x / 2)),
			static_cast<u16>(std::max<u32>(1, size.y / 2)),
		};
		bloom_downsample_desc.flags		 = texture_flags::tf_sampled | texture_flags::tf_gpu_write | texture_flags::tf_is_2d;
		bloom_downsample_desc.view_count = WORLD_RENDER_BLOOM_LEVEL_COUNT * 2;
		bloom_downsample_desc.mip_levels = WORLD_RENDER_BLOOM_LEVEL_COUNT;

		for (u8 level = 0; level < WORLD_RENDER_BLOOM_LEVEL_COUNT; ++level)
		{
			bloom_downsample_desc.views[level] = {
				.type			= view_type::sampled,
				.base_mip_level = level,
				.mip_count		= 1,
			};

			bloom_downsample_desc.views[WORLD_RENDER_BLOOM_LEVEL_COUNT + level] = {
				.type			= view_type::gpu_write,
				.base_mip_level = level,
				.mip_count		= 1,
			};
		}

		bloom_downsample_desc.set_name("world_bloom_downsample");

		texture_desc_t bloom_upsample_desc = bloom_downsample_desc;
		bloom_upsample_desc.size		   = size;
		bloom_upsample_desc.set_name("world_bloom_upsample");

		const u32 light_cluster_count_x = (static_cast<u32>(size.x) + WORLD_RENDER_CLUSTER_TILE_SIZE - 1) / WORLD_RENDER_CLUSTER_TILE_SIZE;
		const u32 light_cluster_count_y = (static_cast<u32>(size.y) + WORLD_RENDER_CLUSTER_TILE_SIZE - 1) / WORLD_RENDER_CLUSTER_TILE_SIZE;
		const u32 light_cluster_count	= light_cluster_count_x * light_cluster_count_y * WORLD_RENDER_CLUSTER_DEPTH_SLICE_COUNT;

		gfx_backend& backend = gfx_backend::get();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			SFG_ASSERT(_pfd[i].lighting_texture.is_null());
			SFG_ASSERT(_pfd[i].post_process_texture.is_null());
			SFG_ASSERT(_pfd[i].post_process_hdr_scratch.is_null());
			SFG_ASSERT(_pfd[i].post_process_ldr_scratch.is_null());
			SFG_ASSERT(_pfd[i].depth_texture.is_null());
			SFG_ASSERT(_pfd[i].gbuffer_albedo.is_null());
			SFG_ASSERT(_pfd[i].gbuffer_normal.is_null());
			SFG_ASSERT(_pfd[i].gbuffer_orm.is_null());
			SFG_ASSERT(_pfd[i].gbuffer_emissive.is_null());
			SFG_ASSERT(_pfd[i].ao_texture.is_null());
			SFG_ASSERT(_pfd[i].ao_half_texture.is_null());
			SFG_ASSERT(_pfd[i].light_cluster_buffer.is_null());
			SFG_ASSERT(_pfd[i].light_cluster_indices_buffer.is_null());

			_pfd[i].lighting_texture	 = backend.create_texture(lighting_desc);
			_pfd[i].post_process_texture = backend.create_texture(post_process_desc);
			_pfd[i].depth_texture		 = backend.create_texture(depth_desc);
			_pfd[i].gbuffer_albedo		 = backend.create_texture(gbuffer_albedo_desc);
			_pfd[i].gbuffer_normal		 = backend.create_texture(gbuffer_normal_desc);
			_pfd[i].gbuffer_orm			 = backend.create_texture(gbuffer_orm_desc);
			_pfd[i].gbuffer_emissive	 = backend.create_texture(gbuffer_emissive_desc);
			create_light_cluster_buffers(static_cast<u8>(i), light_cluster_count);

			if (_config.enable_ssao != 0)
			{
				_pfd[i].ao_texture		= backend.create_texture(ao_desc);
				_pfd[i].ao_half_texture = backend.create_texture(ao_half_desc);
			}

			_pfd[i].lighting_texture_index	   = backend.get_texture_gpu_index(_pfd[i].lighting_texture, 0);
			_pfd[i].post_process_texture_index = backend.get_texture_gpu_index(_pfd[i].post_process_texture, 0);
			_pfd[i].depth_texture_index		   = backend.get_texture_gpu_index(_pfd[i].depth_texture, 2);
			_pfd[i].gbuffer_albedo_index	   = backend.get_texture_gpu_index(_pfd[i].gbuffer_albedo, 1);
			_pfd[i].gbuffer_normal_index	   = backend.get_texture_gpu_index(_pfd[i].gbuffer_normal, 1);
			_pfd[i].gbuffer_orm_index		   = backend.get_texture_gpu_index(_pfd[i].gbuffer_orm, 1);
			_pfd[i].gbuffer_emissive_index	   = backend.get_texture_gpu_index(_pfd[i].gbuffer_emissive, 1);

			if (_config.enable_ssao != 0)
			{
				_pfd[i].ao_texture_index		  = backend.get_texture_gpu_index(_pfd[i].ao_texture, 0);
				_pfd[i].ao_texture_uav_index	  = backend.get_texture_gpu_index(_pfd[i].ao_texture, 1);
				_pfd[i].ao_half_texture_index	  = backend.get_texture_gpu_index(_pfd[i].ao_half_texture, 0);
				_pfd[i].ao_half_texture_uav_index = backend.get_texture_gpu_index(_pfd[i].ao_half_texture, 1);
			}

			if (_config.enable_bloom != 0)
			{
				_pfd[i].bloom_downsample = backend.create_texture(bloom_downsample_desc);
				_pfd[i].bloom_upsample	 = backend.create_texture(bloom_upsample_desc);

				for (u8 level = 0; level < WORLD_RENDER_BLOOM_LEVEL_COUNT; ++level)
				{
					_pfd[i].bloom_downsample_index[level]	  = backend.get_texture_gpu_index(_pfd[i].bloom_downsample, level);
					_pfd[i].bloom_downsample_uav_index[level] = backend.get_texture_gpu_index(_pfd[i].bloom_downsample, WORLD_RENDER_BLOOM_LEVEL_COUNT + level);
					_pfd[i].bloom_upsample_index[level]		  = backend.get_texture_gpu_index(_pfd[i].bloom_upsample, level);
					_pfd[i].bloom_upsample_uav_index[level]	  = backend.get_texture_gpu_index(_pfd[i].bloom_upsample, WORLD_RENDER_BLOOM_LEVEL_COUNT + level);
				}
			}
		}

		create_post_process_scratch(size, _post_process_hdr_scratch != 0, _post_process_ldr_scratch != 0);
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
			SFG_ASSERT(!_pfd[i].light_cluster_buffer.is_null());
			SFG_ASSERT(!_pfd[i].light_cluster_indices_buffer.is_null());

			backend.destroy_texture(_pfd[i].lighting_texture);
			backend.destroy_texture(_pfd[i].post_process_texture);
			backend.destroy_texture(_pfd[i].depth_texture);
			backend.destroy_texture(_pfd[i].gbuffer_albedo);
			backend.destroy_texture(_pfd[i].gbuffer_normal);
			backend.destroy_texture(_pfd[i].gbuffer_orm);
			backend.destroy_texture(_pfd[i].gbuffer_emissive);
			destroy_light_cluster_buffers(static_cast<u8>(i));
			if (_config.enable_ssao != 0)
			{
				backend.destroy_texture(_pfd[i].ao_texture);
				backend.destroy_texture(_pfd[i].ao_half_texture);
			}
			if (_config.enable_bloom != 0)
			{
				backend.destroy_texture(_pfd[i].bloom_downsample);
				backend.destroy_texture(_pfd[i].bloom_upsample);
			}

			if (_post_process_hdr_scratch != 0)
			{
				SFG_ASSERT(!_pfd[i].post_process_hdr_scratch.is_null());
				backend.destroy_texture(_pfd[i].post_process_hdr_scratch);
			}

			if (_post_process_ldr_scratch != 0)
			{
				SFG_ASSERT(!_pfd[i].post_process_ldr_scratch.is_null());
				backend.destroy_texture(_pfd[i].post_process_ldr_scratch);
			}

			_pfd[i].lighting_texture		 = {};
			_pfd[i].post_process_texture	 = {};
			_pfd[i].post_process_hdr_scratch = {};
			_pfd[i].post_process_ldr_scratch = {};
			_pfd[i].depth_texture			 = {};
			_pfd[i].gbuffer_albedo			 = {};
			_pfd[i].gbuffer_normal			 = {};
			_pfd[i].gbuffer_orm				 = {};
			_pfd[i].gbuffer_emissive		 = {};
			_pfd[i].ao_texture				 = {};
			_pfd[i].ao_half_texture			 = {};
			_pfd[i].bloom_downsample		 = {};
			_pfd[i].bloom_upsample			 = {};
			for (u8 level = 0; level < WORLD_RENDER_BLOOM_LEVEL_COUNT; ++level)
			{
				_pfd[i].bloom_downsample_index[level]	  = NULL_GPU_INDEX;
				_pfd[i].bloom_downsample_uav_index[level] = NULL_GPU_INDEX;
				_pfd[i].bloom_upsample_index[level]		  = NULL_GPU_INDEX;
				_pfd[i].bloom_upsample_uav_index[level]	  = NULL_GPU_INDEX;
			}
			_pfd[i].lighting_texture_index		   = NULL_GPU_INDEX;
			_pfd[i].post_process_texture_index	   = NULL_GPU_INDEX;
			_pfd[i].post_process_hdr_scratch_index = NULL_GPU_INDEX;
			_pfd[i].post_process_ldr_scratch_index = NULL_GPU_INDEX;
			_pfd[i].depth_texture_index			   = NULL_GPU_INDEX;
			_pfd[i].gbuffer_albedo_index		   = NULL_GPU_INDEX;
			_pfd[i].gbuffer_normal_index		   = NULL_GPU_INDEX;
			_pfd[i].gbuffer_orm_index			   = NULL_GPU_INDEX;
			_pfd[i].gbuffer_emissive_index		   = NULL_GPU_INDEX;
			_pfd[i].ao_texture_index			   = NULL_GPU_INDEX;
			_pfd[i].ao_texture_uav_index		   = NULL_GPU_INDEX;
			_pfd[i].ao_half_texture_index		   = NULL_GPU_INDEX;
			_pfd[i].ao_half_texture_uav_index	   = NULL_GPU_INDEX;
		}
		_config.size = vec2u16_t::zero;
	}
}

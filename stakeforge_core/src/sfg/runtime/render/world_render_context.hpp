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

#pragma once

#include <sfg/common/size_definitions.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/math/mat4x4.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/math/vec4f.hpp>

namespace sfg
{
#define WORLD_RENDER_ENTITY_BUFFER_CAPACITY 8000

	struct render_pass_data_opaque_gpu_t
	{
		mat4x4_t view_proj = mat4x4_t::identity;
	};

	struct gpu_entity_t
	{
		mat4x4_t model		   = mat4x4_t::identity;
		mat4x4_t normal_matrix = mat4x4_t::identity;
		vec4f_t	 position	   = vec4f_t::zero;
		vec4f_t	 forward	   = vec4f_t::zero;
	};

	class world_render_context_t final
	{
	public:
		world_render_context_t()										 = default;
		~world_render_context_t()										 = default;
		world_render_context_t(const world_render_context_t&)			 = delete;
		world_render_context_t& operator=(const world_render_context_t&) = delete;
		world_render_context_t(world_render_context_t&& other) noexcept;
		world_render_context_t& operator=(world_render_context_t&& other) noexcept;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------
		void init(vec2u16_t size);
		void uninit();
		void resize(vec2u16_t size);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------
		gfx_command_buffer_handle get_command_buffer(u8 frame_index) const;
		gfx_command_buffer_handle get_command_buffer_gfx0(u8 frame_index) const;
		gfx_command_buffer_handle get_command_buffer_gfx1(u8 frame_index) const;
		gfx_texture_handle		  get_world_texture(u8 frame_index) const;
		gfx_texture_handle		  get_lighting_texture(u8 frame_index) const;
		gfx_texture_handle		  get_depth_texture(u8 frame_index) const;
		gfx_texture_handle		  get_post_process_texture(u8 frame_index) const;
		gfx_texture_handle		  get_gbuffer_albedo_texture(u8 frame_index) const;
		gfx_texture_handle		  get_gbuffer_normal_texture(u8 frame_index) const;
		gfx_texture_handle		  get_gbuffer_orm_texture(u8 frame_index) const;
		gfx_texture_handle		  get_gbuffer_emissive_texture(u8 frame_index) const;
		gfx_semaphore_handle	  get_gfx0_done_semaphore(u8 frame_index) const;
		u64						  next_gfx0_done_semaphore_value(u8 frame_index) const;
		gpu_index_t				  get_world_texture_index(u8 frame_index) const;
		gpu_index_t				  get_opaque_render_pass_data_index(u8 frame_index) const;
		gpu_index_t				  get_entity_buffer_index(u8 frame_index) const;
		u8*						  get_mapped_opaque_render_pass_data(u8 frame_index) const;
		u8*						  get_mapped_entity_buffer(u8 frame_index) const;
		vec2u16_t				  get_size() const;

	private:
		void create_texture(vec2u16_t size);
		void destroy_texture();

		struct per_frame_data_t
		{
			u8*						  mapped_opaque_render_pass_data = nullptr;
			u8*						  mapped_entity_buffer			 = nullptr;
			gfx_command_buffer_handle cmd_gfx0						 = {};
			gfx_command_buffer_handle cmd_gfx1						 = {};
			gfx_resource_handle		  opaque_render_pass_data		 = {};
			gfx_resource_handle		  entity_buffer					 = {};
			gfx_texture_handle		  lighting_texture				 = {};
			gfx_texture_handle		  post_process_texture			 = {};
			gfx_texture_handle		  depth_texture					 = {};
			gfx_texture_handle		  gbuffer_albedo				 = {};
			gfx_texture_handle		  gbuffer_normal				 = {};
			gfx_texture_handle		  gbuffer_orm					 = {};
			gfx_texture_handle		  gbuffer_emissive				 = {};
			gfx_semaphore_handle	  gfx0_done_semaphore			 = {};
			mutable u64				  gfx0_done_value				 = 0;
			gpu_index_t				  lighting_texture_index		 = NULL_GPU_INDEX;
			gpu_index_t				  post_process_texture_index	 = NULL_GPU_INDEX;
			gpu_index_t				  depth_texture_index			 = NULL_GPU_INDEX;
			gpu_index_t				  gbuffer_albedo_index			 = NULL_GPU_INDEX;
			gpu_index_t				  gbuffer_normal_index			 = NULL_GPU_INDEX;
			gpu_index_t				  gbuffer_orm_index				 = NULL_GPU_INDEX;
			gpu_index_t				  gbuffer_emissive_index		 = NULL_GPU_INDEX;
			gpu_index_t				  opaque_render_pass_data_index	 = NULL_GPU_INDEX;
			gpu_index_t				  entity_buffer_index			 = NULL_GPU_INDEX;
		};

		struct shaders_t
		{
			gfx_shader_handle lighting = {};
		};

	private:
		per_frame_data_t _pfd[BACK_BUFFER_COUNT] = {};
		shaders_t		 _shaders				 = {};
		vec2u16_t		 _size					 = vec2u16_t::zero;
	};
}

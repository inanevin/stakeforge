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

	struct render_pass_data_lighting_gpu_t
	{
		mat4x4_t inv_view_proj = mat4x4_t::identity;
		mat4x4_t inv_view	   = mat4x4_t::identity;
		vec4f_t	 camera_pos	   = vec4f_t::zero;
		vec4f_t	 skybox_params = vec4f_t::zero;
	};

	struct render_pass_data_post_process_gpu_t
	{
		vec4f_t params = vec4f_t::zero;
	};

	struct world_debug_line_gpu_data_t
	{
		mat4x4_t view	= mat4x4_t::identity;
		mat4x4_t proj	= mat4x4_t::identity;
		vec4f_t	 params = vec4f_t::zero;
	};

	struct world_debug_text_gpu_data_t
	{
		mat4x4_t view_proj = mat4x4_t::identity;
		vec4f_t	 params	   = vec4f_t::zero;
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
		inline gfx_handle_t get_command_buffer(u8 frame_index) const
		{
			return _pfd[frame_index].cmd_gfx1;
		}

		inline gfx_handle_t get_command_buffer_gfx0(u8 frame_index) const
		{
			return _pfd[frame_index].cmd_gfx0;
		}

		inline gfx_handle_t get_command_buffer_gfx1(u8 frame_index) const
		{
			return _pfd[frame_index].cmd_gfx1;
		}

		inline gfx_handle_t get_world_texture(u8 frame_index) const
		{
			return _pfd[frame_index].post_process_texture;
		}

		inline gfx_handle_t get_lighting_texture(u8 frame_index) const
		{
			return _pfd[frame_index].lighting_texture;
		}

		inline gfx_handle_t get_depth_texture(u8 frame_index) const
		{
			return _pfd[frame_index].depth_texture;
		}

		inline gfx_handle_t get_post_process_texture(u8 frame_index) const
		{
			return _pfd[frame_index].post_process_texture;
		}

		inline gfx_handle_t get_gbuffer_albedo_texture(u8 frame_index) const
		{
			return _pfd[frame_index].gbuffer_albedo;
		}

		inline gfx_handle_t get_gbuffer_normal_texture(u8 frame_index) const
		{
			return _pfd[frame_index].gbuffer_normal;
		}

		inline gfx_handle_t get_gbuffer_orm_texture(u8 frame_index) const
		{
			return _pfd[frame_index].gbuffer_orm;
		}

		inline gfx_handle_t get_gbuffer_emissive_texture(u8 frame_index) const
		{
			return _pfd[frame_index].gbuffer_emissive;
		}

		inline gfx_handle_t get_ao_texture(u8 frame_index) const
		{
			return _pfd[frame_index].ao_texture;
		}

		inline gfx_handle_t get_gfx0_done_semaphore(u8 frame_index) const
		{
			return _pfd[frame_index].gfx0_done_semaphore;
		}

		inline u64 next_gfx0_done_semaphore_value(u8 frame_index) const
		{
			return ++_pfd[frame_index].gfx0_done_value;
		}

		inline gpu_index_t get_world_texture_index(u8 frame_index) const
		{
			return _pfd[frame_index].post_process_texture_index;
		}

		inline gpu_index_t get_lighting_texture_index(u8 frame_index) const
		{
			return _pfd[frame_index].lighting_texture_index;
		}

		inline gpu_index_t get_post_process_texture_index(u8 frame_index) const
		{
			return _pfd[frame_index].post_process_texture_index;
		}

		inline gpu_index_t get_opaque_render_pass_data_index(u8 frame_index) const
		{
			return _pfd[frame_index].opaque_render_pass_data_index;
		}

		inline gpu_index_t get_lighting_render_pass_data_index(u8 frame_index) const
		{
			return _pfd[frame_index].lighting_render_pass_data_index;
		}

		inline gpu_index_t get_post_process_render_pass_data_index(u8 frame_index) const
		{
			return _pfd[frame_index].post_process_render_pass_data_index;
		}

		inline gpu_index_t get_debug_line_data_index(u8 frame_index) const
		{
			return _pfd[frame_index].debug_line_data_index;
		}

		inline gpu_index_t get_debug_text_data_index(u8 frame_index) const
		{
			return _pfd[frame_index].debug_text_data_index;
		}

		inline gpu_index_t get_entity_buffer_index(u8 frame_index) const
		{
			return _pfd[frame_index].entity_buffer_index;
		}

		inline gpu_index_t get_gbuffer_albedo_index(u8 frame_index) const
		{
			return _pfd[frame_index].gbuffer_albedo_index;
		}

		inline gpu_index_t get_gbuffer_normal_index(u8 frame_index) const
		{
			return _pfd[frame_index].gbuffer_normal_index;
		}

		inline gpu_index_t get_gbuffer_orm_index(u8 frame_index) const
		{
			return _pfd[frame_index].gbuffer_orm_index;
		}

		inline gpu_index_t get_gbuffer_emissive_index(u8 frame_index) const
		{
			return _pfd[frame_index].gbuffer_emissive_index;
		}

		inline gpu_index_t get_depth_texture_index(u8 frame_index) const
		{
			return _pfd[frame_index].depth_texture_index;
		}

		inline gpu_index_t get_ao_texture_index(u8 frame_index) const
		{
			return _pfd[frame_index].ao_texture_index;
		}

		inline gfx_handle_t get_lighting_shader() const
		{
			return _shaders.lighting;
		}

		inline gfx_handle_t get_post_combiner_shader() const
		{
			return _shaders.post_combiner;
		}

		inline gfx_handle_t get_debug_line_shader() const
		{
			return _shaders.debug_line;
		}

		inline gfx_handle_t get_debug_text_shader() const
		{
			return _shaders.debug_text;
		}

		inline u8* get_mapped_opaque_render_pass_data(u8 frame_index) const
		{
			return _pfd[frame_index].mapped_opaque_render_pass_data;
		}

		inline u8* get_mapped_lighting_render_pass_data(u8 frame_index) const
		{
			return _pfd[frame_index].mapped_lighting_render_pass_data;
		}

		inline u8* get_mapped_post_process_render_pass_data(u8 frame_index) const
		{
			return _pfd[frame_index].mapped_post_process_render_pass_data;
		}

		inline u8* get_mapped_entity_buffer(u8 frame_index) const
		{
			return _pfd[frame_index].mapped_entity_buffer;
		}

		inline u8* get_mapped_debug_line_data(u8 frame_index) const
		{
			return _pfd[frame_index].mapped_debug_line_data;
		}

		inline u8* get_mapped_debug_line_vertices(u8 frame_index) const
		{
			return _pfd[frame_index].mapped_debug_line_vertices;
		}

		inline u8* get_mapped_debug_line_indices(u8 frame_index) const
		{
			return _pfd[frame_index].mapped_debug_line_indices;
		}

		inline u8* get_mapped_debug_text_data(u8 frame_index) const
		{
			return _pfd[frame_index].mapped_debug_text_data;
		}

		inline u8* get_mapped_debug_text_vertices(u8 frame_index) const
		{
			return _pfd[frame_index].mapped_debug_text_vertices;
		}

		inline u8* get_mapped_debug_text_indices(u8 frame_index) const
		{
			return _pfd[frame_index].mapped_debug_text_indices;
		}

		inline gfx_handle_t get_debug_line_vertex_buffer(u8 frame_index) const
		{
			return _pfd[frame_index].debug_line_vertex_buffer;
		}

		inline gfx_handle_t get_debug_line_index_buffer(u8 frame_index) const
		{
			return _pfd[frame_index].debug_line_index_buffer;
		}

		inline gfx_handle_t get_debug_text_vertex_buffer(u8 frame_index) const
		{
			return _pfd[frame_index].debug_text_vertex_buffer;
		}

		inline gfx_handle_t get_debug_text_index_buffer(u8 frame_index) const
		{
			return _pfd[frame_index].debug_text_index_buffer;
		}

		inline vec2u16_t get_size() const
		{
			return _size;
		}

	private:
		void create_texture(vec2u16_t size);
		void destroy_texture();

		struct per_frame_data_t
		{
			u8*			 mapped_debug_line_vertices			  = nullptr;
			u8*			 mapped_debug_line_indices			  = nullptr;
			u8*			 mapped_debug_line_data				  = nullptr;
			u8*			 mapped_debug_text_vertices			  = nullptr;
			u8*			 mapped_debug_text_indices			  = nullptr;
			u8*			 mapped_debug_text_data				  = nullptr;
			u8*			 mapped_opaque_render_pass_data		  = nullptr;
			u8*			 mapped_lighting_render_pass_data	  = nullptr;
			u8*			 mapped_post_process_render_pass_data = nullptr;
			u8*			 mapped_entity_buffer				  = nullptr;
			gfx_handle_t cmd_gfx0							  = {};
			gfx_handle_t cmd_gfx1							  = {};
			gfx_handle_t opaque_render_pass_data			  = {};
			gfx_handle_t lighting_render_pass_data			  = {};
			gfx_handle_t post_process_render_pass_data		  = {};
			gfx_handle_t entity_buffer						  = {};
			gfx_handle_t debug_line_data					  = {};
			gfx_handle_t debug_line_vertex_buffer			  = {};
			gfx_handle_t debug_line_index_buffer			  = {};
			gfx_handle_t debug_text_data					  = {};
			gfx_handle_t debug_text_vertex_buffer			  = {};
			gfx_handle_t debug_text_index_buffer			  = {};
			gfx_handle_t lighting_texture					  = {};
			gfx_handle_t post_process_texture				  = {};
			gfx_handle_t depth_texture						  = {};
			gfx_handle_t gbuffer_albedo						  = {};
			gfx_handle_t gbuffer_normal						  = {};
			gfx_handle_t gbuffer_orm						  = {};
			gfx_handle_t gbuffer_emissive					  = {};
			gfx_handle_t ao_texture							  = {};
			gfx_handle_t gfx0_done_semaphore				  = {};
			mutable u64	 gfx0_done_value					  = 0;
			gpu_index_t	 lighting_texture_index				  = NULL_GPU_INDEX;
			gpu_index_t	 post_process_texture_index			  = NULL_GPU_INDEX;
			gpu_index_t	 depth_texture_index				  = NULL_GPU_INDEX;
			gpu_index_t	 gbuffer_albedo_index				  = NULL_GPU_INDEX;
			gpu_index_t	 gbuffer_normal_index				  = NULL_GPU_INDEX;
			gpu_index_t	 gbuffer_orm_index					  = NULL_GPU_INDEX;
			gpu_index_t	 gbuffer_emissive_index				  = NULL_GPU_INDEX;
			gpu_index_t	 ao_texture_index					  = NULL_GPU_INDEX;
			gpu_index_t	 opaque_render_pass_data_index		  = NULL_GPU_INDEX;
			gpu_index_t	 lighting_render_pass_data_index	  = NULL_GPU_INDEX;
			gpu_index_t	 post_process_render_pass_data_index  = NULL_GPU_INDEX;
			gpu_index_t	 entity_buffer_index				  = NULL_GPU_INDEX;
			gpu_index_t	 debug_line_data_index				  = NULL_GPU_INDEX;
			gpu_index_t	 debug_text_data_index				  = NULL_GPU_INDEX;
		};

		struct shaders_t
		{
			gfx_handle_t lighting	   = {};
			gfx_handle_t post_combiner = {};
			gfx_handle_t debug_line	   = {};
			gfx_handle_t debug_text	   = {};
		};

	private:
		per_frame_data_t _pfd[BACK_BUFFER_COUNT] = {};
		shaders_t		 _shaders				 = {};
		vec2u16_t		 _size					 = vec2u16_t::zero;
	};
}

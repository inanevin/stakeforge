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
#include "world_render_shadow.hpp"

namespace sfg
{
#define WORLD_RENDER_ENTITY_BUFFER_CAPACITY		8000
#define WORLD_RENDER_BLOOM_LEVEL_COUNT			5
#define WORLD_RENDER_SHADOW_VIEW_CAPACITY		64
#define WORLD_RENDER_SHADOW_ALLOCATION_CAPACITY 32

	struct render_pass_data_opaque_gpu_t
	{
		mat4x4_t view_proj = mat4x4_t::identity;
	};

	struct render_pass_data_lighting_gpu_t
	{
		mat4x4_t inv_view_proj	 = mat4x4_t::identity;
		mat4x4_t inv_view		 = mat4x4_t::identity;
		mat4x4_t view			 = mat4x4_t::identity;
		vec4f_t	 camera_pos		 = vec4f_t::zero;
		vec4f_t	 skybox_params	 = vec4f_t::zero;
		u32		 light_counts[4] = {};
	};

	struct render_pass_data_post_process_gpu_t
	{
		vec4f_t params0 = vec4f_t::zero;
		vec4f_t params1 = vec4f_t::zero;
	};

	struct render_pass_data_ssao_gpu_t
	{
		mat4x4_t proj				 = mat4x4_t::identity;
		mat4x4_t inv_proj			 = mat4x4_t::identity;
		mat4x4_t view_matrix		 = mat4x4_t::identity;
		u32		 full_size[2]		 = {};
		u32		 half_size[2]		 = {};
		f32		 inv_full[2]		 = {};
		f32		 inv_half[2]		 = {};
		f32		 z_near				 = 0.0f;
		f32		 z_far				 = 0.0f;
		f32		 radius_world		 = 0.0f;
		f32		 bias				 = 0.0f;
		f32		 intensity			 = 0.0f;
		f32		 power				 = 0.0f;
		u32		 num_dirs			 = 0;
		u32		 num_steps			 = 0;
		f32		 random_rot_strength = 0.0f;
	};

	struct render_pass_data_bloom_gpu_t
	{
		f32 filter_radius = 0.01f;
		f32 pad[3]		  = {};
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

	struct gpu_light_t
	{
		vec4f_t position_range	 = vec4f_t::zero;
		vec4f_t direction_param0 = vec4f_t::zero;
		vec4f_t right_param1	 = vec4f_t::zero;
		vec4f_t color_intensity	 = vec4f_t::zero;
		u32		shadow[4]		 = {UINT32_MAX, 0, 0, 0};
	};

	static_assert(sizeof(gpu_light_t) == 80);

	struct world_render_context_config_t
	{
		vec2u16_t size			  = vec2u16_t::zero;
		u32		  light_max		  = 0;
		u32		  line_vertex_max = 0;
		u32		  line_index_max  = 0;
		u32		  text_vertex_max = 0;
		u32		  text_index_max  = 0;
		u16		  shadow_view_max = 0;
		u8		  enable_ssao	  = 1;
		u8		  enable_bloom	  = 1;
	};

	struct world_render_shadow_allocation_t
	{
		gfx_handle_t texture		  = {};
		gpu_index_t	 texture_index	  = NULL_GPU_INDEX;
		vec2u16_t	 resolution		  = vec2u16_t::zero;
		u32			 stable_id		  = UINT32_MAX;
		u8			 type			  = 0;
		u8			 layer_count	  = 0;
		u64			 last_used_serial = 0;
		u64			 retire_serial	  = 0;
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
		void									init(const world_render_context_config_t& config);
		void									uninit();
		void									resize(vec2u16_t size);
		world_render_shadow_allocation_t*		get_or_create_shadow_allocation(u32 stable_id, u8 type, vec2u16_t resolution, u8 layer_count);
		const world_render_shadow_allocation_t* find_shadow_allocation(u32 stable_id, u8 type) const;
		void									begin_shadow_allocations();
		void									end_shadow_allocations();

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------
		inline gfx_handle_t get_command_buffer(u8 frame_index) const
		{
			return _pfd[frame_index].cmd_post;
		}

		inline gfx_handle_t get_command_buffer_depth(u8 frame_index) const
		{
			return _pfd[frame_index].cmd_depth;
		}

		inline gfx_handle_t get_command_buffer_gbuffer(u8 frame_index) const
		{
			return _pfd[frame_index].cmd_gbuffer;
		}

		inline gfx_handle_t get_command_buffer_lighting(u8 frame_index) const
		{
			return _pfd[frame_index].cmd_lighting;
		}

		inline gfx_handle_t get_command_buffer_forward(u8 frame_index) const
		{
			return _pfd[frame_index].cmd_forward;
		}

		inline gfx_handle_t get_command_buffer_post(u8 frame_index) const
		{
			return _pfd[frame_index].cmd_post;
		}

		inline gfx_handle_t get_command_buffer_ssao(u8 frame_index) const
		{
			return _pfd[frame_index].cmd_ssao;
		}

		inline gfx_handle_t get_command_buffer_bloom(u8 frame_index) const
		{
			return _pfd[frame_index].cmd_bloom;
		}

		inline gfx_handle_t get_command_buffer_shadows(u8 frame_index) const
		{
			return _pfd[frame_index].cmd_shadows;
		}

		inline u8* get_mapped_shadow_views(u8 frame_index) const
		{
			return _pfd[frame_index].mapped_shadow_views;
		}

		inline gpu_index_t get_shadow_view_buffer_index(u8 frame_index) const
		{
			return _pfd[frame_index].shadow_view_buffer_index;
		}

		inline gpu_index_t get_shadow_view_data_index(u8 frame_index, u16 view) const
		{
			return _pfd[frame_index].shadow_view_data_indices[view];
		}

		inline u8* get_mapped_shadow_view_data(u8 frame_index, u16 view) const
		{
			return _pfd[frame_index].mapped_shadow_view_data[view];
		}

		inline u16 get_shadow_view_max() const
		{
			return _config.shadow_view_max;
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

		inline gfx_handle_t get_ao_half_texture(u8 frame_index) const
		{
			return _pfd[frame_index].ao_half_texture;
		}

		inline gfx_handle_t get_ssao_semaphore(u8 frame_index) const
		{
			return _pfd[frame_index].ssao_semaphore;
		}

		inline u64 next_ssao_semaphore_value(u8 frame_index) const
		{
			return ++_pfd[frame_index].ssao_semaphore_value;
		}

		inline gfx_handle_t get_bloom_semaphore(u8 frame_index) const
		{
			return _pfd[frame_index].bloom_semaphore;
		}

		inline u64 next_bloom_semaphore_value(u8 frame_index) const
		{
			return ++_pfd[frame_index].bloom_semaphore_value;
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

		inline gpu_index_t get_light_buffer_index(u8 frame_index) const
		{
			return _pfd[frame_index].light_buffer_index;
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

		inline gpu_index_t get_ao_texture_uav_index(u8 frame_index) const
		{
			return _pfd[frame_index].ao_texture_uav_index;
		}

		inline gpu_index_t get_ao_half_texture_index(u8 frame_index) const
		{
			return _pfd[frame_index].ao_half_texture_index;
		}

		inline gpu_index_t get_ao_half_texture_uav_index(u8 frame_index) const
		{
			return _pfd[frame_index].ao_half_texture_uav_index;
		}

		inline gpu_index_t get_ssao_noise_texture_index() const
		{
			return _ssao_noise_texture_index;
		}

		inline gfx_handle_t get_bloom_downsample_texture(u8 frame_index) const
		{
			return _pfd[frame_index].bloom_downsample;
		}

		inline gfx_handle_t get_bloom_upsample_texture(u8 frame_index) const
		{
			return _pfd[frame_index].bloom_upsample;
		}

		inline gpu_index_t get_bloom_downsample_index(u8 frame_index, u8 level) const
		{
			return _pfd[frame_index].bloom_downsample_index[level];
		}

		inline gpu_index_t get_bloom_downsample_uav_index(u8 frame_index, u8 level) const
		{
			return _pfd[frame_index].bloom_downsample_uav_index[level];
		}

		inline gpu_index_t get_bloom_upsample_index(u8 frame_index, u8 level) const
		{
			return _pfd[frame_index].bloom_upsample_index[level];
		}

		inline gpu_index_t get_bloom_upsample_uav_index(u8 frame_index, u8 level) const
		{
			return _pfd[frame_index].bloom_upsample_uav_index[level];
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

		inline gfx_handle_t get_ssao_shader() const
		{
			return _shaders.ssao;
		}

		inline gfx_handle_t get_ssao_upsample_shader() const
		{
			return _shaders.ssao_upsample;
		}

		inline gfx_handle_t get_bloom_downsample_shader() const
		{
			return _shaders.bloom_downsample;
		}

		inline gfx_handle_t get_bloom_upsample_shader() const
		{
			return _shaders.bloom_upsample;
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

		inline u8* get_mapped_ssao_render_pass_data(u8 frame_index) const
		{
			return _pfd[frame_index].mapped_ssao_render_pass_data;
		}

		inline u8* get_mapped_bloom_render_pass_data(u8 frame_index) const
		{
			return _pfd[frame_index].mapped_bloom_render_pass_data;
		}

		inline gpu_index_t get_ssao_render_pass_data_index(u8 frame_index) const
		{
			return _pfd[frame_index].ssao_render_pass_data_index;
		}

		inline gpu_index_t get_bloom_render_pass_data_index(u8 frame_index) const
		{
			return _pfd[frame_index].bloom_render_pass_data_index;
		}

		inline u8* get_mapped_entity_buffer(u8 frame_index) const
		{
			return _pfd[frame_index].mapped_entity_buffer;
		}

		inline u8* get_mapped_light_buffer(u8 frame_index) const
		{
			return _pfd[frame_index].mapped_light_buffer;
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
			return _config.size;
		}

		inline bool is_ssao_enabled() const
		{
			return _config.enable_ssao != 0;
		}

		inline bool is_bloom_enabled() const
		{
			return _config.enable_bloom != 0;
		}

		inline u32 get_light_max() const
		{
			return _config.light_max;
		}

	private:
		void create_texture(vec2u16_t size);
		void destroy_texture();

		struct per_frame_data_t
		{
			u8*			 mapped_shadow_views										 = nullptr;
			u8*			 mapped_shadow_view_data[WORLD_RENDER_SHADOW_VIEW_CAPACITY]	 = {};
			u8*			 mapped_debug_line_vertices									 = nullptr;
			u8*			 mapped_debug_line_indices									 = nullptr;
			u8*			 mapped_debug_line_data										 = nullptr;
			u8*			 mapped_debug_text_vertices									 = nullptr;
			u8*			 mapped_debug_text_indices									 = nullptr;
			u8*			 mapped_debug_text_data										 = nullptr;
			u8*			 mapped_opaque_render_pass_data								 = nullptr;
			u8*			 mapped_lighting_render_pass_data							 = nullptr;
			u8*			 mapped_post_process_render_pass_data						 = nullptr;
			u8*			 mapped_entity_buffer										 = nullptr;
			u8*			 mapped_light_buffer										 = nullptr;
			u8*			 mapped_ssao_render_pass_data								 = nullptr;
			u8*			 mapped_bloom_render_pass_data								 = nullptr;
			gfx_handle_t cmd_depth													 = {};
			gfx_handle_t cmd_gbuffer												 = {};
			gfx_handle_t cmd_lighting												 = {};
			gfx_handle_t cmd_forward												 = {};
			gfx_handle_t cmd_post													 = {};
			gfx_handle_t cmd_ssao													 = {};
			gfx_handle_t cmd_bloom													 = {};
			gfx_handle_t cmd_shadows												 = {};
			gfx_handle_t shadow_view_buffer											 = {};
			gfx_handle_t shadow_view_data[WORLD_RENDER_SHADOW_VIEW_CAPACITY]		 = {};
			gpu_index_t	 shadow_view_buffer_index									 = NULL_GPU_INDEX;
			gpu_index_t	 shadow_view_data_indices[WORLD_RENDER_SHADOW_VIEW_CAPACITY] = {};
			gfx_handle_t opaque_render_pass_data									 = {};
			gfx_handle_t lighting_render_pass_data									 = {};
			gfx_handle_t post_process_render_pass_data								 = {};
			gfx_handle_t ssao_render_pass_data										 = {};
			gfx_handle_t bloom_render_pass_data										 = {};
			gfx_handle_t entity_buffer												 = {};
			gfx_handle_t light_buffer												 = {};
			gfx_handle_t debug_line_data											 = {};
			gfx_handle_t debug_line_vertex_buffer									 = {};
			gfx_handle_t debug_line_index_buffer									 = {};
			gfx_handle_t debug_text_data											 = {};
			gfx_handle_t debug_text_vertex_buffer									 = {};
			gfx_handle_t debug_text_index_buffer									 = {};
			gfx_handle_t lighting_texture											 = {};
			gfx_handle_t post_process_texture										 = {};
			gfx_handle_t depth_texture												 = {};
			gfx_handle_t gbuffer_albedo												 = {};
			gfx_handle_t gbuffer_normal												 = {};
			gfx_handle_t gbuffer_orm												 = {};
			gfx_handle_t gbuffer_emissive											 = {};
			gfx_handle_t ao_texture													 = {};
			gfx_handle_t ao_half_texture											 = {};
			gfx_handle_t bloom_downsample											 = {};
			gfx_handle_t bloom_upsample												 = {};
			gfx_handle_t ssao_semaphore												 = {};
			gfx_handle_t bloom_semaphore											 = {};
			mutable u64	 ssao_semaphore_value										 = 0;
			mutable u64	 bloom_semaphore_value										 = 0;
			gpu_index_t	 lighting_texture_index										 = NULL_GPU_INDEX;
			gpu_index_t	 post_process_texture_index									 = NULL_GPU_INDEX;
			gpu_index_t	 depth_texture_index										 = NULL_GPU_INDEX;
			gpu_index_t	 gbuffer_albedo_index										 = NULL_GPU_INDEX;
			gpu_index_t	 gbuffer_normal_index										 = NULL_GPU_INDEX;
			gpu_index_t	 gbuffer_orm_index											 = NULL_GPU_INDEX;
			gpu_index_t	 gbuffer_emissive_index										 = NULL_GPU_INDEX;
			gpu_index_t	 ao_texture_index											 = NULL_GPU_INDEX;
			gpu_index_t	 ao_texture_uav_index										 = NULL_GPU_INDEX;
			gpu_index_t	 ao_half_texture_index										 = NULL_GPU_INDEX;
			gpu_index_t	 ao_half_texture_uav_index									 = NULL_GPU_INDEX;
			gpu_index_t	 bloom_downsample_index[WORLD_RENDER_BLOOM_LEVEL_COUNT]		 = {};
			gpu_index_t	 bloom_downsample_uav_index[WORLD_RENDER_BLOOM_LEVEL_COUNT]	 = {};
			gpu_index_t	 bloom_upsample_index[WORLD_RENDER_BLOOM_LEVEL_COUNT]		 = {};
			gpu_index_t	 bloom_upsample_uav_index[WORLD_RENDER_BLOOM_LEVEL_COUNT]	 = {};
			gpu_index_t	 opaque_render_pass_data_index								 = NULL_GPU_INDEX;
			gpu_index_t	 lighting_render_pass_data_index							 = NULL_GPU_INDEX;
			gpu_index_t	 post_process_render_pass_data_index						 = NULL_GPU_INDEX;
			gpu_index_t	 ssao_render_pass_data_index								 = NULL_GPU_INDEX;
			gpu_index_t	 bloom_render_pass_data_index								 = NULL_GPU_INDEX;
			gpu_index_t	 entity_buffer_index										 = NULL_GPU_INDEX;
			gpu_index_t	 light_buffer_index											 = NULL_GPU_INDEX;
			gpu_index_t	 debug_line_data_index										 = NULL_GPU_INDEX;
			gpu_index_t	 debug_text_data_index										 = NULL_GPU_INDEX;
		};

		struct shaders_t
		{
			gfx_handle_t lighting		  = {};
			gfx_handle_t post_combiner	  = {};
			gfx_handle_t debug_line		  = {};
			gfx_handle_t debug_text		  = {};
			gfx_handle_t ssao			  = {};
			gfx_handle_t ssao_upsample	  = {};
			gfx_handle_t bloom_downsample = {};
			gfx_handle_t bloom_upsample	  = {};
		};

	private:
		per_frame_data_t				 _pfd[BACK_BUFFER_COUNT]									  = {};
		shaders_t						 _shaders													  = {};
		world_render_context_config_t	 _config													  = {};
		gfx_handle_t					 _ssao_noise_texture										  = {};
		gfx_handle_t					 _ssao_noise_staging										  = {};
		gpu_index_t						 _ssao_noise_texture_index									  = NULL_GPU_INDEX;
		world_render_shadow_allocation_t _shadow_allocations[WORLD_RENDER_SHADOW_ALLOCATION_CAPACITY] = {};
		u64								 _shadow_serial												  = 0;
	};
}

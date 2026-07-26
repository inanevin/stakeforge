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

#include "world_draw.hpp"
#include "world_debug_draw_snapshot.hpp"
#include "world_render_bone.hpp"
#include "world_render_entity.hpp"
#include "world_render_light.hpp"
#include "world_render_reflection_probe.hpp"
#include "world_render_shadow.hpp"
#include "world_render_material.hpp"
#include "world_render_view.hpp"
#include <sfg/data/frame_vector.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/math/frustum.hpp>
#include <sfg/math/mat4x4.hpp>
#include <sfg/math/vec2f.hpp>
#include <sfg/math/vec4f.hpp>
#include <sfg/runtime/render/render_resource_handle.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/runtime/project/project_settings.hpp>

namespace sfg
{
#define WORLD_RENDER_PREP_INITIAL_VIEW_CAPACITY 65

	struct world_render_environment_t
	{
		vec4f_t ambient_color		  = vec4f_t::zero;
		u32		material_index		  = UINT32_MAX;
		f32		intensity			  = 1.0f;
		u8		debug_cluster_heatmap = 0;
	};

	struct world_render_ssao_t
	{
		f32 radius_world			 = 0.75f;
		f32 bias					 = 0.04f;
		f32 intensity				 = 1.25f;
		f32 power					 = 1.25f;
		f32 random_rotation_strength = 1.5f;
		u32 direction_count			 = 8;
		u32 step_count				 = 6;
		u8	enabled					 = 0;
	};

	struct world_render_bloom_t
	{
		f32 strength	  = 0.04f;
		f32 filter_radius = 0.01f;
		u8	enabled		  = 0;
	};

	struct world_render_post_process_t
	{
		world_render_ssao_t	 ssao				  = {};
		world_render_bloom_t bloom				  = {};
		f32					 exposure_ev		  = 0.0f;
		f32					 saturation			  = 1.0f;
		f32					 temperature		  = 0.0f;
		f32					 tint				  = 0.0f;
		f32					 reinhard_white_point = 6.0f;
		u32					 tonemap_mode		  = 1;
	};

	struct world_render_prep_view_t
	{
		mat4x4_t	view				= mat4x4_t::identity;
		mat4x4_t	view_proj			= mat4x4_t::identity;
		mat4x4_t	inv_view			= mat4x4_t::identity;
		mat4x4_t	inv_view_proj		= mat4x4_t::identity;
		frustum_t	frustum				= {};
		vec4f_t		camera_pos			= vec4f_t::zero;
		vec2f_t		viewport_size		= vec2f_t::zero;
		vec2f_t		inv_viewport_size	= vec2f_t::zero;
		f32			near_plane			= 0.0f;
		f32			far_plane			= 0.0f;
		gpu_index_t depth_texture_index = NULL_GPU_INDEX;
	};

	struct world_render_prep_data_t
	{
		vector_t<world_render_prep_view_t>	 views					= {};
		vector_t<u64>						 draw_cull_masks		= {};
		vector_t<world_render_shadow_view_t> shadow_views			= {};
		u32									 cull_word_count		= 0;
		u32									 reflection_probe_count = 0;

		inline void reserve(size_t draws)
		{
			const size_t word_count = (draws + 63) / 64;

			views.reserve(WORLD_RENDER_PREP_INITIAL_VIEW_CAPACITY);
			draw_cull_masks.reserve(word_count * WORLD_RENDER_PREP_INITIAL_VIEW_CAPACITY);
			shadow_views.reserve(64);
		}

		inline void reset()
		{
			views.resize(0);
			draw_cull_masks.resize(0);
			shadow_views.resize(0);
			cull_word_count		   = 0;
			reflection_probe_count = 0;
		}

		inline u16 add_view(const world_render_prep_view_t& view)
		{
			const u16 index = static_cast<u16>(views.size());
			views.push_back(view);
			return index;
		}

		inline bool is_draw_culled(u16 view_index, u32 draw_index) const
		{
			const u64 word = draw_cull_masks[static_cast<size_t>(view_index) * cull_word_count + draw_index / 64];
			return (word & (1ull << (draw_index % 64))) != 0;
		}
	};

	struct world_render_snapshot_reserve_config_t
	{
		size_t entity_count			  = 0;
		size_t bone_count			  = 0;
		size_t light_count			  = 0;
		size_t reflection_probe_count = 0;
		size_t line_vertex_count	  = 0;
		size_t line_index_count		  = 0;
		size_t triangle_vertex_count  = 0;
		size_t triangle_index_count	  = 0;
		size_t text_vertex_count	  = 0;
		size_t text_index_count		  = 0;
	};

	struct world_render_snapshot_t
	{
		void*									  user_data			= nullptr;
		engine_shadow_settings_t				  shadows			= {};
		world_render_view_t						  main_view			= {};
		world_render_environment_t				  environment		= {};
		world_render_post_process_t				  post_process		= {};
		world_debug_draw_snapshot_t				  debug_draw		= {};
		vector_t<world_render_material_t>		  materials			= {};
		vector_t<world_render_entity_t>			  entities			= {};
		vector_t<world_render_bone_t>			  bones				= {};
		vector_t<world_render_light_t>			  lights			= {};
		vector_t<world_render_reflection_probe_t> reflection_probes = {};
		vector_t<world_draw_t>					  draws				= {};
		engine_quality_level_e					  quality_level		= engine_quality_level_e::high;

		inline void reserve(const world_render_snapshot_reserve_config_t& config)
		{
			entities.reserve(config.entity_count);
			bones.reserve(config.bone_count);
			lights.reserve(config.light_count);
			reflection_probes.reserve(config.reflection_probe_count);
			debug_draw.line_vertices.reserve(config.line_vertex_count);
			debug_draw.line_indices.reserve(config.line_index_count);
			debug_draw.triangle_vertices.reserve(config.triangle_vertex_count);
			debug_draw.triangle_indices.reserve(config.triangle_index_count);
			debug_draw.text_vertices.reserve(config.text_vertex_count);
			debug_draw.text_indices.reserve(config.text_index_count);
		}
	};
}

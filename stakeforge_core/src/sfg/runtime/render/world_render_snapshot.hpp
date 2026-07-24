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
#include "world_render_shadow.hpp"
#include "world_render_material.hpp"
#include "world_render_view.hpp"
#include <sfg/data/frame_vector.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/math/vec4f.hpp>
#include <sfg/runtime/render/render_resource_handle.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/runtime/project/project_settings.hpp>

namespace sfg
{
	struct world_render_environment_t
	{
		vec4f_t ambient_color  = vec4f_t::zero;
		u32		material_index = UINT32_MAX;
		f32		intensity	   = 1.0f;
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

	struct world_render_prep_draw_cull_t
	{
		u64 cull_mask = 0;
	};

	struct world_render_prep_data_t
	{
		vector_t<world_render_prep_draw_cull_t> draw_culls			= {};
		vector_t<world_render_shadow_view_t>	shadow_views		= {};
		vector_t<u32>							shadow_draw_indices = {};

		inline void reserve(size_t culls)
		{
			draw_culls.reserve(culls);
			shadow_views.reserve(64);
			shadow_draw_indices.reserve(culls * 4);
		}

		inline void reset()
		{
			draw_culls.resize(0);
			shadow_views.resize(0);
			shadow_draw_indices.resize(0);
		}
	};

	struct world_render_snapshot_reserve_config_t
	{
		size_t entity_count			 = 0;
		size_t bone_count			 = 0;
		size_t light_count			 = 0;
		size_t line_vertex_count	 = 0;
		size_t line_index_count		 = 0;
		size_t triangle_vertex_count = 0;
		size_t triangle_index_count	 = 0;
		size_t text_vertex_count	 = 0;
		size_t text_index_count		 = 0;
	};

	struct world_render_snapshot_t
	{
		void*							  user_data		= nullptr;
		engine_shadow_settings_t		  shadows		= {};
		world_render_view_t				  main_view		= {};
		world_render_environment_t		  environment	= {};
		world_render_post_process_t		  post_process	= {};
		world_debug_draw_snapshot_t		  debug_draw	= {};
		vector_t<world_render_material_t> materials		= {};
		vector_t<world_render_entity_t>	  entities		= {};
		vector_t<world_render_bone_t>	  bones			= {};
		vector_t<world_render_light_t>	  lights		= {};
		vector_t<world_draw_t>			  draws			= {};
		engine_quality_level_e			  quality_level = engine_quality_level_e::high;

		inline void reserve(const world_render_snapshot_reserve_config_t& config)
		{
			entities.reserve(config.entity_count);
			bones.reserve(config.bone_count);
			lights.reserve(config.light_count);
			debug_draw.line_vertices.reserve(config.line_vertex_count);
			debug_draw.line_indices.reserve(config.line_index_count);
			debug_draw.triangle_vertices.reserve(config.triangle_vertex_count);
			debug_draw.triangle_indices.reserve(config.triangle_index_count);
			debug_draw.text_vertices.reserve(config.text_vertex_count);
			debug_draw.text_indices.reserve(config.text_index_count);
		}
	};
}

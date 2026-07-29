/*
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
#include "world_canvas_render_snapshot.hpp"
#include "world_render_bone.hpp"
#include "world_render_entity.hpp"
#include "world_render_light.hpp"
#include "world_render_reflection_probe.hpp"
#include "world_render_shadow.hpp"
#include "world_render_material.hpp"
#include "world_render_view.hpp"
#include <sfg/data/frame_vector.hpp>
#include <sfg/data/inplace_vector.hpp>
#include <sfg/data/span.hpp>
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
	enum class world_render_fog_type_e : u32
	{
		linear,
		exponential,
		exponential_squared,
		exponential_height,
	};

	struct world_render_fog_t
	{
		vec4f_t					color		   = vec4f_t::zero;
		f32						intensity	   = 0.0f;
		f32						density		   = 0.01f;
		f32						start_distance = 0.0f;
		f32						end_distance   = 100.0f;
		f32						height		   = 0.0f;
		f32						height_falloff = 0.1f;
		f32						max_opacity	   = 1.0f;
		world_render_fog_type_e type		   = world_render_fog_type_e::exponential;
	};

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

	struct world_render_fxaa_t
	{
		f32 fixed_threshold	   = 0.0833f;
		f32 relative_threshold = 0.166f;
		f32 subpixel_blending  = 0.75f;
		u8	enabled			   = 0;
	};

	struct world_render_post_process_effect_t
	{
		u32 material_index = UINT32_MAX;
	};

	struct world_render_post_process_t
	{
		inplace_vector_t<world_render_post_process_effect_t, 8> before_tonemap		 = {};
		inplace_vector_t<world_render_post_process_effect_t, 8> after_tonemap		 = {};
		world_render_ssao_t										ssao				 = {};
		world_render_fxaa_t										fxaa				 = {};
		world_render_bloom_t									bloom				 = {};
		f32														exposure_ev			 = 0.0f;
		f32														saturation			 = 1.0f;
		f32														temperature			 = 0.0f;
		f32														tint				 = 0.0f;
		f32														reinhard_white_point = 6.0f;
		u32														tonemap_mode		 = 1;
	};

	struct world_render_queue_range_t
	{
		u32 offset = 0;
		u32 count  = 0;
	};

	struct world_render_prep_view_t
	{
		mat4x4_t				   view								   = mat4x4_t::identity;
		mat4x4_t				   view_proj						   = mat4x4_t::identity;
		mat4x4_t				   inv_view							   = mat4x4_t::identity;
		mat4x4_t				   inv_view_proj					   = mat4x4_t::identity;
		frustum_t				   frustum							   = {};
		vec4f_t					   camera_pos						   = vec4f_t::zero;
		vec4f_t					   cluster_depth					   = vec4f_t::zero;
		u32						   cluster_dims[4]					   = {};
		vec2f_t					   viewport_size					   = vec2f_t::zero;
		vec2f_t					   inv_viewport_size				   = vec2f_t::zero;
		f32						   near_plane						   = 0.0f;
		f32						   far_plane						   = 0.0f;
		gpu_index_t				   depth_texture_index				   = NULL_GPU_INDEX;
		u32						   cluster_buffer_offset			   = 0;
		u32						   cluster_light_indices_buffer_offset = 0;
		u32						   cluster_light_capacity			   = 0;
		u32						   queue_flags						   = 0;
		world_render_queue_range_t depth_range						   = {};
		world_render_queue_range_t opaque_range						   = {};
		world_render_queue_range_t transparent_range				   = {};
		world_render_queue_range_t shadow_range						   = {};
		world_render_queue_range_t visible_range					   = {};
	};

	enum world_render_queue_flags_e : u32
	{
		world_render_queue_flag_depth		= 1 << 0,
		world_render_queue_flag_opaque		= 1 << 1,
		world_render_queue_flag_transparent = 1 << 2,
		world_render_queue_flag_shadow		= 1 << 3,
		world_render_queue_flag_visible		= 1 << 4,
	};

	struct world_render_queue_item_t
	{
		f32 depth					= 0.0f;
		u32 renderable_index		= UINT32_MAX;
		u32 sprite_instance_index	= UINT32_MAX;
		u32 particle_instance_index = UINT32_MAX;
	};

	struct world_render_prep_initial_capacity_config_t
	{
		size_t view_initial_capacity			  = 65;
		size_t depth_queue_initial_capacity		  = 0;
		size_t opaque_queue_initial_capacity	  = 0;
		size_t transparent_queue_initial_capacity = 0;
		size_t shadow_queue_initial_capacity	  = 0;
		size_t visible_queue_initial_capacity	  = 0;
		size_t shadow_view_initial_capacity		  = 0;
	};

	struct world_render_prep_data_t
	{
		vector_t<world_render_prep_view_t>	 views					= {};
		vector_t<world_render_queue_item_t>	 depth_queue			= {};
		vector_t<world_render_queue_item_t>	 opaque_queue			= {};
		vector_t<world_render_queue_item_t>	 transparent_queue		= {};
		vector_t<world_render_queue_item_t>	 shadow_queue			= {};
		vector_t<world_render_queue_item_t>	 visible_queue			= {};
		vector_t<world_render_shadow_view_t> shadow_views			= {};
		u32									 reflection_probe_count = 0;

		inline void reserve(const world_render_prep_initial_capacity_config_t& config)
		{
			views.reserve(config.view_initial_capacity);
			depth_queue.reserve(config.depth_queue_initial_capacity);
			opaque_queue.reserve(config.opaque_queue_initial_capacity);
			transparent_queue.reserve(config.transparent_queue_initial_capacity);
			shadow_queue.reserve(config.shadow_queue_initial_capacity);
			visible_queue.reserve(config.visible_queue_initial_capacity);
			shadow_views.reserve(config.shadow_view_initial_capacity);
		}

		inline void reset()
		{
			views.resize(0);
			depth_queue.resize(0);
			opaque_queue.resize(0);
			transparent_queue.resize(0);
			shadow_queue.resize(0);
			visible_queue.resize(0);
			shadow_views.resize(0);
			reflection_probe_count = 0;
		}

		inline u16 add_view(const world_render_prep_view_t& view)
		{
			const u16 index = static_cast<u16>(views.size());
			views.push_back(view);
			return index;
		}

		inline void begin_view_queues(world_render_prep_view_t& view) const
		{
			view.depth_range	   = {.offset = static_cast<u32>(depth_queue.size())};
			view.opaque_range	   = {.offset = static_cast<u32>(opaque_queue.size())};
			view.transparent_range = {.offset = static_cast<u32>(transparent_queue.size())};
			view.shadow_range	   = {.offset = static_cast<u32>(shadow_queue.size())};
			view.visible_range	   = {.offset = static_cast<u32>(visible_queue.size())};
		}

		inline void end_view_queues(world_render_prep_view_t& view) const
		{
			view.depth_range.count		 = static_cast<u32>(depth_queue.size()) - view.depth_range.offset;
			view.opaque_range.count		 = static_cast<u32>(opaque_queue.size()) - view.opaque_range.offset;
			view.transparent_range.count = static_cast<u32>(transparent_queue.size()) - view.transparent_range.offset;
			view.shadow_range.count		 = static_cast<u32>(shadow_queue.size()) - view.shadow_range.offset;
			view.visible_range.count	 = static_cast<u32>(visible_queue.size()) - view.visible_range.offset;
		}

		inline span_t<const world_render_queue_item_t> get_depth_queue(const world_render_prep_view_t& view) const
		{
			return {.data = depth_queue.data() + view.depth_range.offset, .size = view.depth_range.count};
		}

		inline span_t<const world_render_queue_item_t> get_opaque_queue(const world_render_prep_view_t& view) const
		{
			return {.data = opaque_queue.data() + view.opaque_range.offset, .size = view.opaque_range.count};
		}

		inline span_t<const world_render_queue_item_t> get_transparent_queue(const world_render_prep_view_t& view) const
		{
			return {.data = transparent_queue.data() + view.transparent_range.offset, .size = view.transparent_range.count};
		}

		inline span_t<const world_render_queue_item_t> get_shadow_queue(const world_render_prep_view_t& view) const
		{
			return {.data = shadow_queue.data() + view.shadow_range.offset, .size = view.shadow_range.count};
		}

		inline span_t<const world_render_queue_item_t> get_visible_queue(const world_render_prep_view_t& view) const
		{
			return {.data = visible_queue.data() + view.visible_range.offset, .size = view.visible_range.count};
		}

		inline span_t<world_render_queue_item_t> get_mutable_depth_queue(const world_render_prep_view_t& view)
		{
			return {.data = depth_queue.data() + view.depth_range.offset, .size = view.depth_range.count};
		}

		inline span_t<world_render_queue_item_t> get_mutable_opaque_queue(const world_render_prep_view_t& view)
		{
			return {.data = opaque_queue.data() + view.opaque_range.offset, .size = view.opaque_range.count};
		}

		inline span_t<world_render_queue_item_t> get_mutable_transparent_queue(const world_render_prep_view_t& view)
		{
			return {.data = transparent_queue.data() + view.transparent_range.offset, .size = view.transparent_range.count};
		}

		inline span_t<world_render_queue_item_t> get_mutable_shadow_queue(const world_render_prep_view_t& view)
		{
			return {.data = shadow_queue.data() + view.shadow_range.offset, .size = view.shadow_range.count};
		}

		inline span_t<world_render_queue_item_t> get_mutable_visible_queue(const world_render_prep_view_t& view)
		{
			return {.data = visible_queue.data() + view.visible_range.offset, .size = view.visible_range.count};
		}
	};

	struct world_render_snapshot_initial_capacity_config_t
	{
		size_t material_initial_capacity		   = 0;
		size_t entity_initial_capacity			   = 0;
		size_t renderable_initial_capacity		   = 0;
		size_t draw_initial_capacity			   = 0;
		size_t sprite_initial_capacity			   = 0;
		size_t particle_draw_initial_capacity	   = 0;
		size_t particle_initial_capacity		   = 0;
		size_t bone_initial_capacity			   = 0;
		size_t light_initial_capacity			   = 0;
		size_t reflection_probe_initial_capacity   = 0;
		size_t line_vertex_initial_capacity		   = 0;
		size_t line_index_initial_capacity		   = 0;
		size_t triangle_vertex_initial_capacity	   = 0;
		size_t triangle_index_initial_capacity	   = 0;
		size_t text_vertex_initial_capacity		   = 0;
		size_t text_index_initial_capacity		   = 0;
		size_t debug_texture_initial_capacity	   = 0;
		size_t canvas_draw_buffer_initial_capacity = 0;
		size_t canvas_vertex_initial_capacity	   = 0;
		size_t canvas_index_initial_capacity	   = 0;
	};

	struct world_render_snapshot_t
	{
		void*									  user_data			= nullptr;
		engine_shadow_settings_t				  shadows			= {};
		world_render_view_t						  main_view			= {};
		world_render_environment_t				  environment		= {};
		world_render_fog_t						  fog				= {};
		world_render_post_process_t				  post_process		= {};
		world_debug_draw_snapshot_t				  debug_draw		= {};
		world_canvas_render_snapshot_t			  canvas			= {};
		vector_t<world_render_material_t>		  materials			= {};
		vector_t<world_render_entity_t>			  entities			= {};
		vector_t<world_render_bone_t>			  bones				= {};
		vector_t<world_render_light_t>			  lights			= {};
		vector_t<world_render_reflection_probe_t> reflection_probes = {};
		vector_t<world_renderable_t>			  renderables		= {};
		vector_t<world_mesh_draw_t>				  mesh_draws		= {};
		vector_t<world_sprite_draw_t>			  sprite_draws		= {};
		vector_t<world_particle_draw_t>			  particle_draws	= {};
		vector_t<world_particle_t>				  particles			= {};
		engine_quality_level_e					  quality_level		= engine_quality_level_e::high;

		inline void reserve(const world_render_snapshot_initial_capacity_config_t& config)
		{
			materials.reserve(config.material_initial_capacity);
			entities.reserve(config.entity_initial_capacity);
			bones.reserve(config.bone_initial_capacity);
			lights.reserve(config.light_initial_capacity);
			reflection_probes.reserve(config.reflection_probe_initial_capacity);
			renderables.reserve(config.renderable_initial_capacity);
			mesh_draws.reserve(config.draw_initial_capacity);
			sprite_draws.reserve(config.sprite_initial_capacity);
			particle_draws.reserve(config.particle_draw_initial_capacity);
			particles.reserve(config.particle_initial_capacity);
			debug_draw.line_vertices.reserve(config.line_vertex_initial_capacity);
			debug_draw.line_indices.reserve(config.line_index_initial_capacity);
			debug_draw.triangle_vertices.reserve(config.triangle_vertex_initial_capacity);
			debug_draw.triangle_indices.reserve(config.triangle_index_initial_capacity);
			debug_draw.text_vertices.reserve(config.text_vertex_initial_capacity);
			debug_draw.text_indices.reserve(config.text_index_initial_capacity);
			debug_draw.textures.reserve(config.debug_texture_initial_capacity);
			canvas.before_post_process.draw_buffers.reserve(config.canvas_draw_buffer_initial_capacity);
			canvas.before_post_process.vertices.reserve(config.canvas_vertex_initial_capacity);
			canvas.before_post_process.indices.reserve(config.canvas_index_initial_capacity);
			canvas.after_post_process.draw_buffers.reserve(config.canvas_draw_buffer_initial_capacity);
			canvas.after_post_process.vertices.reserve(config.canvas_vertex_initial_capacity);
			canvas.after_post_process.indices.reserve(config.canvas_index_initial_capacity);
		}
	};
}
